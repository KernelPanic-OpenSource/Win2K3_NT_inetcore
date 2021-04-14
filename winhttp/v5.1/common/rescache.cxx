/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    rescache.cxx

Abstract:

    Contains functions which manipulate resolver cache for winsock
    name resolution calls

    Contents:
        QueryResolverCache
        AddResolverCacheEntry
        FlushResolverCache
        ReleaseResolverCacheEntry
        (RemoveCacheEntry)
        (ResolverCacheHit)
        (AddrInfoMatch)
        (CreateCacheEntry)

Author:

    Richard L Firth (rfirth) 10-Jul-1994

Environment:

    Win-16/32 user level

Revision History:

    rfirth 10-Jul-1994
        Created

--*/

//
// includes
//

#include "wininetp.h"

//
// private manifests
//

//
// private macros
//

#define SET_EXPIRATION_TIME(cacheEntry)

//
// private data
//

PRIVATE BOOL ResolverCacheInitialized = FALSE;

//
// DnsCachingEnabled - caching is enabled by default
//

PRIVATE BOOL DnsCachingEnabled = TRUE;

//
// DnsCacheTimeout - number of seconds before a cache entry expires. This value
// is added to the current time (in seconds) to get the expiry time
//

PRIVATE DWORD DnsCacheTimeout = DEFAULT_DNS_CACHE_TIMEOUT;

//
// MaximumDnsCacheEntries - the maximum number of RESOLVER_CACHE_ENTRYs in the
// cache before we start throwing out the LRU
//

PRIVATE LONG MaximumDnsCacheEntries = DEFAULT_DNS_CACHE_ENTRIES;

//
// ResolverCache - serialized list of RESOLVER_CACHE_ENTRYs, kept in MRU order.
// We only need to remove the tail of the list to remove the LRU entry
//

//
// private prototypes
//

PRIVATE
VOID
RemoveCacheEntry(
    IN SERIALIZED_LIST* pResolverCache,
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry
    );

PRIVATE
BOOL
ResolverCacheHit(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL
    );

PRIVATE
BOOL
AddrInfoMatch(
    IN LPADDRINFO AddrInfo,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL
    );

PRIVATE
LPRESOLVER_CACHE_ENTRY
CreateCacheEntry(
    IN LPSTR lpszHostName,
    IN LPADDRINFO AddrInfo,
    IN DWORD TimeToLive,
    IN VOID** pAlloc=NULL,
    IN DWORD dwAllocSize=0
    );

#if INET_DEBUG

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheTimestr(
    IN DWORD Time
    );

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheAddrInfoStr(
    IN LPADDRINFO AddrInfo
    );

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheMapSockAddress(
    IN LPSOCKADDR Address
    );

#endif

//
// functions
//


LPRESOLVER_CACHE_ENTRY
QueryResolverCache(
    SERIALIZED_LIST* pResolverCache,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL,
    OUT LPADDRINFO * AddrInfo,
    OUT LPDWORD TimeToLive
    )

/*++

Routine Description:

    Checks if Name is stored in the last resolved name cache. If the entry is
    found, but has expired then it is removed from the cache

Arguments:

    Name        - pointer to name string

    Address     - pointer to IP address (in sockaddr format)

    AddrInfo    - pointer to returned pointer to addrinfo

    TimeToLive  - pointer to returned time to live

Return Value:

    BOOL

--*/

{
    UNREFERENCED_PARAMETER(TimeToLive);
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "QueryResolverCache",
                 "%q, %A, %#x, %#x",
                 Name,
                 Address,
                 AddrInfo,
                 TimeToLive
                 ));

    LPRESOLVER_CACHE_ENTRY lpEntry = NULL;

    if (!DnsCachingEnabled || !LockSerializedList(pResolverCache))
    {
        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled or unable to serialize access\n"
                    ));

        *AddrInfo = NULL;
        lpEntry = NULL;
        goto quit;
    }

    LPRESOLVER_CACHE_ENTRY cacheEntry;
    LPRESOLVER_CACHE_ENTRY previousEntry;
    DWORD timeNow;

    cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(pResolverCache);
    timeNow = (DWORD)time(NULL);

    while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(pResolverCache)) {

        //
        // on every cache lookup, purge any stale entries. LIVE_FOREVER means
        // that we don't expect the entry's net address to expire, but it
        // DOESN'T mean that we can't throw out the entry if its the LRU and
        // we're at maximum cache capacity. We can't do this if the item is
        // still in-use. In this case, we mark it stale
        //

        if ((cacheEntry->ExpirationTime != LIVE_FOREVER)
        && (cacheEntry->ExpirationTime <= timeNow)) {

            //
            // if reference count not zero then another thread is using
            // this entry - mark as stale else delete it
            //

            if (cacheEntry->ReferenceCount != 0) {

                INET_ASSERT(cacheEntry->State == ENTRY_IN_USE);

                cacheEntry->State = ENTRY_DELETE;
            } else {

                //
                // this entry is stale; throw it out
                // "my hovercraft is full of eels"
                //

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("throwing out stale DNS entry %q, expiry = %s\n",
                            cacheEntry->AddrInfo->ai_canonname,
                            CacheTimestr(cacheEntry->ExpirationTime)
                            ));

                //
                // BUGBUG - what happens if ExpirationTime == timeNow?
                //

                previousEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Blink;
                RemoveCacheEntry(pResolverCache, cacheEntry);
                cacheEntry = previousEntry;
            }
        } 
        else if (ResolverCacheHit(cacheEntry, Name, Address)
        && ((cacheEntry->State == ENTRY_UNUSED)
        || (cacheEntry->State == ENTRY_IN_USE))) {

            //
            // we found the entry, and it still has time to live. Make it the
            // head of the list (MRU first), set the state to in-use and increase
            // the reference count
            //

            if (RemoveFromSerializedList(pResolverCache, &cacheEntry->ListEntry))
            {
                if (InsertAtHeadOfSerializedList(pResolverCache, &cacheEntry->ListEntry))
                {
                    cacheEntry->State = ENTRY_IN_USE;
                    ++cacheEntry->ReferenceCount;
                    *AddrInfo = cacheEntry->AddrInfo;
                    lpEntry = cacheEntry;


                    DEBUG_PRINT(SOCKETS,
                                INFO,
                                ("entry found in DNS cache\n"
                                ));
                }
                else
                {
                    DEBUG_PRINT(SOCKETS,
                                INFO,
                                ("entry found in DNS cache, but removed due to not enough memory\n"
                                ));
                }
            }

            goto done;
        }
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
    }

    *AddrInfo = NULL;
    lpEntry = NULL;

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("didn't find entry in DNS cache\n"
                ));

done:

    UnlockSerializedList(pResolverCache);

quit:

    DEBUG_LEAVE(lpEntry);
    return lpEntry;
}


VOID
AddResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPSTR lpszHostName,
    IN LPADDRINFO AddrInfo,
    IN DWORD TimeToLive,
    IN VOID** pAlloc,
    IN DWORD dwAllocSize
    )

/*++

Routine Description:

    Adds an addrinfo pointer to the cache. Creates a new entry to hold it
    and links it into the cache list, displacing the LRU entry if required.
    If we cannot create the entry, the addrinfo is freed, no errors returned

    N.B.: Calling this routine gives the resolver cache "ownership" of the
    addrinfo chain. Caller should not use AddrInfo pointer afterwards.

Arguments:

    lpszHostName    - the name we originally requested be resolved. May be
                      different than the names returned by the resolver, e.g.
                      "proxy" => "proxy1.microsoft.com, proxy2.microsoft.com"

    AddrInfo        - pointer to addrinfo chain to add to the cache

    TimeToLive      - amount of time this information has to live. Can be:

                        LIVE_FOREVER    - don't timeout (but can be discarded)

                        LIVE_DEFAULT    - use the default value

                        anything else   - number of seconds to live

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "AddResolverCacheEntry",
                 "%q, %#x, %d",
                 lpszHostName,
                 AddrInfo,
                 TimeToLive
                 ));

    BOOL bAdded = FALSE;

    if (!DnsCachingEnabled || !LockSerializedList(pResolverCache))
    {
        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled or unable to serialize access\n"
                    ));

        goto quit;
    }

    //
    // check that the entry is not already in the cache - 2 or more threads may
    // have been simultaneously resolving the same name
    //

    LPADDRINFO lpAddrInfo;
    DWORD ttl;
    LPRESOLVER_CACHE_ENTRY lpResolverCacheEntry;

    INET_ASSERT(lpszHostName != NULL);

    if (NULL == (lpResolverCacheEntry=QueryResolverCache(pResolverCache, lpszHostName, NULL, &lpAddrInfo, &ttl))) {

        LPRESOLVER_CACHE_ENTRY cacheEntry;

        //
        // remove as many entries as we can beginning at the tail of the list.
        // We try to remove enough to get the cache size back below the limit.
        // This may consist of removing expired entries or entries marked as
        // DELETE. If there are expired, in-use entries then we mark them as
        // DELETE. This may result in the cache list growing until those threads
        // which have referenced cache entries release them
        //

        cacheEntry = (LPRESOLVER_CACHE_ENTRY)TailOfSerializedList(pResolverCache);

        while ((pResolverCache->ElementCount >= MaximumDnsCacheEntries)
        && (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(pResolverCache))) {

            //
            // cache has maximum entries: throw out the Least Recently Used (its
            // the one at the back of the queue, ma'am) but only if no-one else
            // is currently accessing it
            //

            if ((cacheEntry->State != ENTRY_IN_USE)
            && (cacheEntry->ReferenceCount == 0)) {

                INET_ASSERT((cacheEntry->State == ENTRY_UNUSED)
                            || (cacheEntry->State == ENTRY_DELETE));

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("throwing out LRU %q\n",
                            cacheEntry->AddrInfo->ai_canonname
                            ));

                LPRESOLVER_CACHE_ENTRY nextEntry;

                nextEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Flink;
                RemoveCacheEntry(pResolverCache, cacheEntry);
                cacheEntry = nextEntry;
            } else if (cacheEntry->State == ENTRY_IN_USE) {

                //
                // this entry needs to be freed when it is released
                //

                cacheEntry->State = ENTRY_DELETE;
            }
            cacheEntry = (LPRESOLVER_CACHE_ENTRY)cacheEntry->ListEntry.Blink;
        }

        //
        // add the entry at the head of the queue - it is the Most Recently Used
        // after all. If we fail to allocate memory, its no problem: it'll just
        // take a little longer if this entry would have been hit before we needed
        // to throw out another entry
        //

        if (NULL != (cacheEntry = CreateCacheEntry(lpszHostName, AddrInfo, TimeToLive, pAlloc, dwAllocSize))) {

#if INET_DEBUG
            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("caching %s, expiry = %s\n",
                        cacheEntry->HostName,
                        CacheTimestr(cacheEntry->ExpirationTime)
                        ));
            LPADDRINFO TmpAddrInfo = AddrInfo;
            while (TmpAddrInfo) {
                DEBUG_PRINT(SOCKETS,
                            INFO,
                           ("  address %A\n", 
                           TmpAddrInfo->ai_addr
                           ));
                TmpAddrInfo = TmpAddrInfo->ai_next;
            }
#endif

            if (!InsertAtHeadOfSerializedList(pResolverCache, &cacheEntry->ListEntry))
            {
                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("Unable to cache new entry due to not enough memory\n"
                            ));
                cacheEntry = (LPRESOLVER_CACHE_ENTRY)FREE_MEMORY((HLOCAL)cacheEntry);

                INET_ASSERT(cacheEntry == NULL);

            }
            else
            {
                bAdded = TRUE;
            }
        }
    } else {

        //
        // this entry is already in the cache. 2 or more threads must have been
        // resolving the same name simultaneously. We just bump the expiration
        // time to the more recent value
        //

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("found %q already in the cache!?\n",
                    lpszHostName
                    ));

        ReleaseResolverCacheEntry(pResolverCache, lpResolverCacheEntry);

    }

    UnlockSerializedList(pResolverCache);

quit:

    if (!bAdded) {
        //
        // failed to add this entry to the cache, so free it
        //
        _I_freeaddrinfo(AddrInfo);
    }

    DEBUG_LEAVE(0);
}


VOID
FlushResolverCache(
    SERIALIZED_LIST* pResolverCache
    )

/*++

Routine Description:

    Removes all entries in DNS resolver cache

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "FlushResolverCache",
                 NULL
                 ));

    LPRESOLVER_CACHE_ENTRY cacheEntry;
    LPRESOLVER_CACHE_ENTRY previousEntry;

    if (LockSerializedList(pResolverCache))
    {
        previousEntry = (LPRESOLVER_CACHE_ENTRY)SlSelf(pResolverCache);
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(pResolverCache);
        while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(pResolverCache)) {
            if (cacheEntry->State == ENTRY_UNUSED) {
                RemoveCacheEntry(pResolverCache, cacheEntry);
            } else {

                DEBUG_PRINT(SOCKETS,
                            WARNING,
                            ("cache entry %#x (%q) still in-use\n",
                            cacheEntry,
                            cacheEntry->HostName
                            ));

                cacheEntry->State = ENTRY_DELETE;
                previousEntry = cacheEntry;
            }
            cacheEntry = (LPRESOLVER_CACHE_ENTRY)previousEntry->ListEntry.Flink;
        }

        UnlockSerializedList(pResolverCache);
    }

    DEBUG_LEAVE(0);
}


VOID
ReleaseResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPRESOLVER_CACHE_ENTRY cacheEntry
    )

/*++

Routine Description:

    Either mark a entry unused or if it is stale, delete it

Arguments:

    lpAddrInfo   - pointer to AddrInfo field of entry to free

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "ReleaseResolverCacheEntry",
                 "%#x, %#x",
                 cacheEntry, (cacheEntry ? cacheEntry->AddrInfo : NULL)
                 ));

    if (LockSerializedList(pResolverCache))
    {
        //
        // reference count should never go below zero!
        //

        INET_ASSERT(cacheEntry->ReferenceCount > 0);

        if (--cacheEntry->ReferenceCount <= 0) {

            //
            // last releaser gets to decide what to do - mark unused or delete
            //

            if (cacheEntry->State == ENTRY_IN_USE) {
                cacheEntry->State = ENTRY_UNUSED;
            } else if (cacheEntry->State == ENTRY_DELETE) {

                //
                // entry is already stale - throw it out
                //

                RemoveCacheEntry(pResolverCache, cacheEntry);
            } else {

                //
                // unused? or bogus value? Someone changed state while refcount
                // not zero?
                //

                INET_ASSERT((cacheEntry->State == ENTRY_IN_USE)
                            || (cacheEntry->State == ENTRY_DELETE));

            }
        }

        UnlockSerializedList(pResolverCache);
    }

    DEBUG_LEAVE(0);
}

#ifdef NOT_USED

VOID
ThrowOutResolverCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPADDRINFO lpAddrInfo
    )

/*++

Routine Description:

    Removes this entry from the DNS cache, based on the host name. We assume
    that the entry came from the cache, so unless it has been already purged,
    we should be able to throw it out

Arguments:

    lpAddrInfo  - pointer to addrinfo field with name of entry to throw out

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "ThrowOutResolverCacheEntry",
                 "%#x [%q]",
                 lpAddrInfo,
                 lpAddrInfo->ai_canonname
                 ));

    if (DnsCachingEnabled && LockSerializedList(pResolverCache)) {

        LPRESOLVER_CACHE_ENTRY cacheEntry;

        cacheEntry = (LPRESOLVER_CACHE_ENTRY)HeadOfSerializedList(pResolverCache);
        while (cacheEntry != (LPRESOLVER_CACHE_ENTRY)SlSelf(pResolverCache)) {
            if (AddrInfoMatch(cacheEntry->AddrInfo, lpAddrInfo->ai_canonname, NULL)) {

                //
                // if the entry is unused then we can delete it, else we have
                // to leave it to the thread with the last reference
                //

                if (cacheEntry->State == ENTRY_UNUSED) {
                    RemoveCacheEntry(pResolverCache, cacheEntry);
                } else {
                    cacheEntry->State = ENTRY_DELETE;
                }
                break;
            }
        }

        UnlockSerializedList(pResolverCache);
    } else {

        DEBUG_PRINT(SOCKETS,
                    WARNING,
                    ("DNS caching disabled or unable to serialize access\n"
                    ));

    }

    DEBUG_LEAVE(0);
}
#endif //NOT_USED


PRIVATE
VOID
RemoveCacheEntry(
    SERIALIZED_LIST* pResolverCache,
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry
    )

/*++

Routine Description:

    Takes a cache entry off the list and frees it

    N.B.: This function must be called with the resolver cache serialized list
    already locked

Arguments:

    lpCacheEntry    - currently queued entry to remove

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 None,
                 "RemoveCacheEntry",
                 "%#x",
                 lpCacheEntry
                 ));

    if (RemoveFromSerializedList(pResolverCache, &lpCacheEntry->ListEntry))
    {

        INET_ASSERT(lpCacheEntry->ReferenceCount == 0);
        INET_ASSERT((lpCacheEntry->State == ENTRY_UNUSED)
                    || (lpCacheEntry->State == ENTRY_DELETE));

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("throwing out %q, expiry = %s\n",
                    lpCacheEntry->HostName,
                    CacheTimestr(lpCacheEntry->ExpirationTime)
                    ));

        _I_freeaddrinfo(lpCacheEntry->AddrInfo);
        lpCacheEntry = (LPRESOLVER_CACHE_ENTRY)FREE_MEMORY((HLOCAL)lpCacheEntry);

        INET_ASSERT(lpCacheEntry == NULL);

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("CurrentDnsCacheEntries = %d\n",
                    pResolverCache->ElementCount
                    ));

        INET_ASSERT((pResolverCache->ElementCount >= 0)
                    && (pResolverCache->ElementCount <= MaximumDnsCacheEntries));
    }
    else
    {
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("unable to throw out %q due to insufficient resources, expiry = %s\n",
                    lpCacheEntry->HostName,
                    CacheTimestr(lpCacheEntry->ExpirationTime)
                    ));
    }

    DEBUG_LEAVE(0);
}


PRIVATE
BOOL
ResolverCacheHit(
    IN LPRESOLVER_CACHE_ENTRY lpCacheEntry,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL
    )

/*++

Routine Description:

    Checks this RESOLVER_CACHE_ENTRY for a match with Name or Address. If Name,
    can match with ai_canonname in addrinfo, or with originally resolved name

Arguments:

    lpCacheEntry    - pointer to RESOLVER_CACHE_ENTRY to check

    Name            - optional name to check

    Address         - optional server address to check

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "ResolverCacheHit",
                 "%#x, %q, %A",
                 lpCacheEntry,
                 Name,
                 Address
                 ));

    BOOL found;

    if ((Name != NULL)
    && (lpCacheEntry->HostName != NULL)
    && (lstrcmpi(lpCacheEntry->HostName, Name) == 0)) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("matched name %q\n",
                    lpCacheEntry->HostName
                    ));

        found = TRUE;
    } else {
        found = FALSE;
    }
    if (!found) {
        found = AddrInfoMatch(lpCacheEntry->AddrInfo, Name, Address);
    }

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
BOOL
AddrInfoMatch(
    IN LPADDRINFO AddrInfo,
    IN LPSTR Name OPTIONAL,
    IN LPSOCKADDR Address OPTIONAL
    )

/*++

Routine Description:

    Compares a getaddrinfo result for a match with a host name or address

Arguments:

    AddrInfo - pointer to addrinfo chain to compare

    Name     - pointer to name string

    Address  - pointer to IP address (as a sockaddr)

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "AddrInfoMatch",
                 "%#x, %q, %A",
                 AddrInfo,
                 Name,
                 Address
                 ));

    BOOL found = FALSE;

    if (Name) {
        if ((AddrInfo->ai_canonname != NULL) &&
        (lstrcmpi(AddrInfo->ai_canonname, Name) == 0))
            found = TRUE;
    } else {

        INET_ASSERT(Address != NULL);

        do {
            if ((AddrInfo->ai_addr->sa_family == Address->sa_family)
            && (memcmp(AddrInfo->ai_addr, Address, AddrInfo->ai_addrlen) == 0)) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("matched %A\n",
                            Address
                            ));

                found = TRUE;
                break;
            }

        } while ((AddrInfo = AddrInfo->ai_next) != NULL);
    }

    if (found) {
        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("addrinfo = %A\n",
                    AddrInfo->ai_addr
                    ));
    }

    DEBUG_LEAVE(found);

    return found;
}


PRIVATE
LPRESOLVER_CACHE_ENTRY
CreateCacheEntry(
    IN LPSTR lpszHostName,
    IN LPADDRINFO AddrInfo,
    IN DWORD TimeToLive,
    IN VOID** pAlloc,
    IN DWORD dwAllocSize
    )

/*++

Routine Description:

    Allocates a RESOLVER_CACHE_ENTRY and packs it with the addrinfo information
    and sets the ExpirationTime

Arguments:

    lpszHostName    - name we resolved

    AddrInfo        - pointer to addrinfo chain to store in new entry

    TimeToLive      - amount of time before this entry expires

Return Value:

    LPRESOLVER_CACHE_ENTRY

--*/

{
    LPRESOLVER_CACHE_ENTRY cacheEntry;

    INET_ASSERT(lpszHostName != NULL);

    //
    // only copy lpszHostName if it is different from the name in addrinfo
    //

    UINT hostNameSize;

    if ((AddrInfo->ai_canonname != NULL)
    && lstrcmpi(AddrInfo->ai_canonname, lpszHostName) != 0) {
        hostNameSize = lstrlen(lpszHostName) + 1;
    }
    else if (AddrInfo->ai_canonname == NULL)
    {
        // if ap_canonname is null, we have to save the lpszHoatName
        hostNameSize = lstrlen(lpszHostName) + 1;
    }
    else 
    {
        hostNameSize = 0;
    }

    //
    // allocate space for the cache entry
    //

    DWORD dwSize = sizeof(RESOLVER_CACHE_ENTRY)
                 + hostNameSize;

    if (dwSize <= dwAllocSize)
    {
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)(*pAlloc);
        *pAlloc = NULL;
    }
    else
    {
        cacheEntry = (LPRESOLVER_CACHE_ENTRY)ALLOCATE_MEMORY(dwSize);
    }
    
    if (cacheEntry != NULL) {

        //
        // cache the getaddrinfo result
        //

        cacheEntry->AddrInfo = AddrInfo;

        //
        // copy the host name to the end of the buffer if required
        //

        if (hostNameSize != 0) {
            cacheEntry->HostName = (LPSTR)(cacheEntry + 1);
            RtlCopyMemory(cacheEntry->HostName, lpszHostName, hostNameSize);
        } else {
            cacheEntry->HostName = NULL;
        }

        //
        // calculate the expiration time as the current time (in seconds since
        // 1/1/70) + number of seconds to live OR indefinite if TimeToLive is
        // specified as LIVE_FOREVER, which is what we use if the host
        // information didn't originate from DNS
        //

        cacheEntry->ExpirationTime = (DWORD)((TimeToLive == LIVE_FOREVER)
                                        ? LIVE_FOREVER
                                        : time(NULL)
                                            + ((TimeToLive == LIVE_DEFAULT)
                                                ? DnsCacheTimeout
                                                : TimeToLive));

        //
        // the entry state is initially unused
        //

        cacheEntry->State = ENTRY_UNUSED;

        //
        // and reference is zero
        //

        cacheEntry->ReferenceCount = 0;
    }

    return cacheEntry;
}

#if INET_DEBUG

//
// CAVEAT - can only call these functions once per printf() etc. because of
//          static buffers (but still thread-safe)
//

PRIVATE
DEBUG_FUNCTION
LPSTR
CacheTimestr(IN DWORD Time) {

    //
    // previous code - writes formatted human-sensible date/time to buffer
    //

    //LPSTR p;
    //
    ////
    //// remove the LF from the time string returned by ctime()
    ////
    //
    //p = ctime((const time_t *)&Time);
    //p[strlen(p) - 1] = '\0';
    //return p;

    //
    // abbreviated CRT version - just write # seconds since 1970 to buffer
    //

    static char buf[16];

    wsprintf(buf, "%d", Time);
    return (LPSTR)buf;
}
#endif

#if defined(RNR_SUPPORTED)

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rescache.c

Abstract:

    Contains name resolution cache

    Contents:

Author:

    Shishir Pardikar    2-14-96

Environment:

    Win32 user mode

Revision History:

        2-14-96 shishirp
        Created

--*/

//
//BUGBUG: This include should be removed, duplicate of above
//
#ifndef SPX_SUPPORT
#include <wininetp.h>
#endif


//
// private manifests
//

#define NAMERES_CACHE_USED            0x00000001
#define NAMERES_CACHE_USES_GUID       0x00000002

#define ENTERCRIT_NAMERESCACHE()  (vcritNameresCache.Lock())
#define LEAVECRIT_NAMERESCACHE()  (vcritNameresCache.Unlock())
#define IS_EMPTY(indx)            ((vlpNameresCache[(indx)].dwFlags & NAMERES_CACHE_USED) == 0)
#define USES_GUID(indx)           ((vlpNameresCache[(indx)].dwFlags & NAMERES_CACHE_USES_GUID))

// number of cache entries
#define DEFAULT_NAMERES_CACHE_ENTRIES   10

// expiry time for an addresslist
#define DEFAULT_EXPIRY_DELTA            (24 * 60 * 60 * (LONGLONG)10000000)


//
//  structure definition
//

typedef struct tagNAMERES_CACHE {
    DWORD               dwFlags;       // general flags to be used as needed
    DWORD               dwNameSpace;   // namespace ??
    GUID                sGuid;         // GUID describing service type
    LPSTR               lpszName;      // ptr to name that needs resolution
    FILETIME            ftLastUsedTime;    // last accesstime, mainly for purging
    FILETIME            ftCreationTime;// When it was created
    ADDRESS_INFO_LIST   sAddrList;     // List of address (defined in ixport.h)
} NAMERES_CACHE, far *LPNAMERES_CACHE;





//
// private variables for name resolution cache
//


// Name cache size allocated in init
LPNAMERES_CACHE vlpNameresCache = NULL;

// Number of elements allowed in the nameres cache
int vcntNameresCacheEntries = DEFAULT_NAMERES_CACHE_ENTRIES;


// time in 100ns after which an address is expired
LONGLONG vftExpiryDelta = DEFAULT_EXPIRY_DELTA;

BOOL vfNameresCacheInited = FALSE;

// serialization
CCritSec vcritNameresCache;

//
// private function prototypes
//


PRIVATE
DWORD
CreateNameresCacheEntry(
    int     indx,
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName,
    INT     cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
);


PRIVATE
DWORD
DeleteNameresCacheEntry(
    int indx
);


PRIVATE
int
FindNameresCacheEntry(
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName
);


PRIVATE
int
FindNameresCacheEntryByAddr(
    int cntAddr,
    LPCSADDR_INFO lpCsaddrInfo
);

PRIVATE
int
PurgeEntries(
    BOOL    fForce  // purge atleast one entry
);


PRIVATE
DWORD
CopyCsaddr(
    LPCSADDR_INFO   lpSrc,
    int             cntAddr,
    LPCSADDR_INFO   *lplpDst
);

//
// functions
//


DWORD
InitNameresCache(
    VOID
)
/*++

Routine Description:

    Init name resolution cache. This routine a) allocates a table of
    name cache entries b)

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.

--*/
{


    if (vfNameresCacheInited)
    {
        return (ERROR_SUCCESS);
    }

    // first try to alloc the memory, if it fails just quit
    vlpNameresCache = (LPNAMERES_CACHE)ALLOCATE_ZERO_MEMORY(vcntNameresCacheEntries * sizeof(NAMERES_CACHE));

    if (!vlpNameresCache)
    {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    if (!vcritNameresCache.Init() || !ENTERCRIT_NAMERESCACHE())
    {
        FREE_MEMORY(vlpNameresCache);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    vfNameresCacheInited = TRUE;

    LEAVECRIT_NAMERESCACHE();

    return (ERROR_SUCCESS);

}


DWORD
AddNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    int      cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_SUCCESS;

    if (!vfNameresCacheInited) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (!ENTERCRIT_NAMERESCACHE())
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpName);

    // if indx is valid, delete the entry, do some purging too
    if (indx != -1) {
        DeleteNameresCacheEntry(indx);
        PurgeEntries(FALSE);
    }
    else {
        // create atleast one hole
        indx = PurgeEntries(TRUE);
    }

    INET_ASSERT((indx >=0 && (indx < vcntNameresCacheEntries)));

    dwError = CreateNameresCacheEntry(indx,
                            dwNameSpace,
                            lpGuid,
                            lpName,
                            cntAddresses,
                            lpCsaddrInfo);

    LEAVECRIT_NAMERESCACHE();

    return (dwError);
}




DWORD
RemoveNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpszName
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (vfNameresCacheInited) {

        if (!ENTERCRIT_NAMERESCACHE())
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpszName);

        if (indx != -1) {

            DeleteNameresCacheEntry(indx);

            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_FILE_NOT_FOUND; //yuk
        }

        LEAVECRIT_NAMERESCACHE();
    }
quit:
    return (dwError);
}


#ifdef MAYBE

DWORD
RemoveNameresCacheEntryByAddr(
    int cntAddresses,
    LPCSADDR_INFO lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD dwError = ERROR_INVALID_PARAMETER;

    if (vfNameresCacheInited) {
        if (!ENTERCRIT_NAMERESCACHE())
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }


        indx = FindNameresCacheEntryByAddr(cntAddresses, lpCsaddrInfo);

        if (indx != -1) {

            DeleteNameresCacheEntry(indx);

            dwError = ERROR_SUCCESS;
        }
        else {
            dwError = ERROR_FILE_NOT_FOUND;
        }

        LEAVECRIT_NAMERESCACHE();
    }
quit:
    return (dwError);

}
#endif //MAYBE

DWORD
GetNameresCacheEntry(
    DWORD    dwNameSpace,
    LPGUID   lpGuid,
    LPSTR    lpName,
    INT      *lpcntAddresses,
    LPCSADDR_INFO  *lplpCsaddrInfo
)
/*++

Routine Description:

    This routine looks up the cache and returns the list of addresses
    corresponding to lpGuid/lpName.

Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int indx;
    DWORD   dwError = ERROR_FILE_NOT_FOUND; // poor error

    if (!vfNameresCacheInited) {
        return (ERROR_INVALID_PARAMETER);
    }

    if (!ENTERCRIT_NAMERESCACHE())
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    // is this entry already cached?
    indx = FindNameresCacheEntry(dwNameSpace, lpGuid, lpName);


    if (indx != -1) {
        // yes, let use give back the info

        *lpcntAddresses = vlpNameresCache[indx].sAddrList.AddressCount;

        if ((dwError = CopyCsaddr(vlpNameresCache[indx].sAddrList.Addresses, *lpcntAddresses, lplpCsaddrInfo))
            != ERROR_SUCCESS) {

            goto bailout;
        }
        // update the last used time, we will use this to
        // age out the entries

        GetCurrentGmtTime(&(vlpNameresCache[indx].ftLastUsedTime));
        dwError = ERROR_SUCCESS;
    }

bailout:

    LEAVECRIT_NAMERESCACHE();

quit:
    return (dwError);
}


DWORD
DeinitNameresCache(
    VOID
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    if (vfNameresCacheInited) {
        if (!ENTERCRIT_NAMERESCACHE())
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        for (i = 0; i < vcntNameresCacheEntries; ++i) {
            if (!IS_EMPTY(i)) {
                DeleteNameresCacheEntry(i);
            }
        }

        FREE_MEMORY(vlpNameresCache);

        vlpNameresCache = NULL;

        vfNameresCacheInited = FALSE;

        LEAVECRIT_NAMERESCACHE();
        vcritNameresCache.FreeLock();
    }
    return (ERROR_SUCCESS);
}


PRIVATE
DWORD
CreateNameresCacheEntry(
    int     indx,
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName,
    int     cntAddresses,
    LPCSADDR_INFO  lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    DWORD dwError = ERROR_NOT_ENOUGH_MEMORY;

    INET_ASSERT((indx >=0 && (indx < vcntNameresCacheEntries)));

    INET_ASSERT(IS_EMPTY(indx));


    memset(&vlpNameresCache[indx], 0, sizeof(vlpNameresCache[indx]));

    // we could get a name or a guid
    // do it for name first before doing it for GUID

    // BUGBUG in future we should consider name+GUID+port
    if (lpszName) {
       vlpNameresCache[indx].lpszName = (LPSTR)ALLOCATE_ZERO_MEMORY(lstrlen(lpszName)+1);
       if (!vlpNameresCache[indx].lpszName) {
           goto bailout;
       }
       strcpy(vlpNameresCache[indx].lpszName, lpszName);
    }
    else if (lpGuid) {
        INET_ASSERT(FALSE); // rigth now. In future this should go away
        memcpy(&(vlpNameresCache[indx].sGuid), lpGuid, sizeof(GUID));
        vlpNameresCache[indx].dwFlags |= NAMERES_CACHE_USES_GUID;
    }
    else {
        dwError = ERROR_INVALID_PARAMETER;
        goto bailout;
    }

    INET_ASSERT(cntAddresses > 0);

    if (CopyCsaddr(lpCsaddrInfo, cntAddresses, &(vlpNameresCache[indx].sAddrList.Addresses))
        != ERROR_SUCCESS) {
        goto bailout;
    }

    vlpNameresCache[indx].sAddrList.AddressCount = cntAddresses;

    // mark this as being non-empty
    vlpNameresCache[indx].dwFlags |= NAMERES_CACHE_USED;

    // set the creation and last-used times as now

    GetCurrentGmtTime(&(vlpNameresCache[indx].ftCreationTime));
    vlpNameresCache[indx].ftLastUsedTime = vlpNameresCache[indx].ftCreationTime ;

    dwError = ERROR_SUCCESS;

bailout:

    if (dwError != ERROR_SUCCESS) {
        if (vlpNameresCache[indx].sAddrList.Addresses) {
            FREE_MEMORY(vlpNameresCache[indx].sAddrList.Addresses);
            vlpNameresCache[indx].sAddrList.Addresses = NULL;
        }
        if (vlpNameresCache[indx].lpszName) {
            FREE_MEMORY(vlpNameresCache[indx].lpszName);
            vlpNameresCache[indx].lpszName = NULL;
        }
        memset(&vlpNameresCache[indx], 0, sizeof(vlpNameresCache[indx]));
    }

    return (dwError);
}


PRIVATE
DWORD
DeleteNameresCacheEntry(
    int indx
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    INET_ASSERT((indx >=0) && (indx < vcntNameresCacheEntries));

    if (vlpNameresCache[indx].lpszName) {
        FREE_MEMORY(vlpNameresCache[indx].lpszName);
    }

    INET_ASSERT(vlpNameresCache[indx].sAddrList.Addresses);

    FREE_MEMORY(vlpNameresCache[indx].sAddrList.Addresses);

    memset(&vlpNameresCache[indx], 0, sizeof(NAMERES_CACHE));

    return (ERROR_SUCCESS);
}

#ifdef MAYBE

PRIVATE
int
FindNameresCacheEntryByAddr(
    int cntAddr,
    LPCSADDR_INFO lpCsaddrInfo
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i) && // not empty
            (vlpNameresCache[i].sAddrList.AddressCount == cntAddr) && // count is the same
            (!memcmp(vlpNameresCache[i].sAddrList.Addresses,    // list matches
                     lpCsaddrInfo,
                     cntAddr * sizeof(CSADDR_INFO)))) {
            return (i);
        }
    }
    return (-1);
}
#endif //MAYBE


PRIVATE
int
FindNameresCacheEntry(
    DWORD   dwNameSpace,
    LPGUID  lpGuid,
    LPSTR   lpszName
)
/*++

Routine Description:


Arguments:


Return Value:

    ERROR_SUCCESS if successful.

--*/
{
    int i;

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i)) {
            if (vlpNameresCache[i].dwNameSpace == dwNameSpace) {
                if (!USES_GUID(i)) {

                    INET_ASSERT(vlpNameresCache[i].lpszName);

                    if (lpszName &&
                        !lstrcmpi(lpszName, vlpNameresCache[i].lpszName)) {
                        return (i);
                    }
                }
                else{

                    if (lpGuid && !memcmp(lpGuid, &vlpNameresCache[i].sGuid, sizeof(GUID))) {
                        return (i);
                    }
                }
            }
        }
    }
    return (-1);
}


PRIVATE
int
PurgeEntries(
    BOOL    fForce  // purge atleast one entry
)
/*++

Routine Description:


Arguments:


Return Value:

    index of a free entry

--*/
{
    int i, indxlru = -1, indxHole=-1;
    FILETIME ft;
    BOOL fFoundHole = FALSE;

    GetCurrentGmtTime(&ft);

    for (i = 0; i < vcntNameresCacheEntries; ++i) {
        if (!IS_EMPTY(i)) {

            // purge stale entries
            if ( (FT2LL(ft) - FT2LL(vlpNameresCache[i].ftCreationTime))
                    > FT2LL(vftExpiryDelta)) {
                DeleteNameresCacheEntry(i);
                indxHole = i;
            }
            else if (FT2LL(vlpNameresCache[i].ftLastUsedTime) <= FT2LL(ft)) {
                ft = vlpNameresCache[i].ftLastUsedTime;
                indxlru = i; // LRU entry if we need to purge it
            }
        }
        else {
            indxHole = i;
        }
    }

    // if there is no hole, purge the LRU entry if forced
    if (indxHole == -1) {

        INET_ASSERT(indxlru != -1);

        if (fForce) {
            DeleteNameresCacheEntry(indxlru);
            indxHole = indxlru;
        }
    }
    return (indxHole);
}

PRIVATE
DWORD
CopyCsaddr(
    LPCSADDR_INFO   lpSrc,
    int             cntAddr,
    LPCSADDR_INFO   *lplpDst
)
{
    int i;
    LPCSADDR_INFO lpDst;
    UINT uSize;


    // BUGBUG assumes the way Compressaddress (ixport.cxx) allocates memory
    uSize = LocalSize(lpSrc);
    if (!uSize) {
        return (GetLastError());
    }

    *lplpDst = (LPCSADDR_INFO)ALLOCATE_ZERO_MEMORY(uSize);

    if (!*lplpDst) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    lpDst = *lplpDst;


    memcpy(lpDst, lpSrc, uSize);

    // now start doing fixups
    for (i=0; i<cntAddr; ++i) {
        lpDst[i].LocalAddr.lpSockaddr = (LPSOCKADDR)((LPBYTE)lpDst+((DWORD)(lpSrc[i].LocalAddr.lpSockaddr) - (DWORD)lpSrc));
        lpDst[i].RemoteAddr.lpSockaddr = (LPSOCKADDR)((LPBYTE)lpDst+((DWORD)(lpSrc[i].RemoteAddr.lpSockaddr) - (DWORD)lpSrc));
    }
    return (ERROR_SUCCESS);
}

#endif // defined(RNR_SUPPORTED)
