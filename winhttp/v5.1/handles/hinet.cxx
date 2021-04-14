/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    hinet.cxx

Abstract:

    contains methods for INTERNET_HANDLE_BASE class

    Contents:
        HANDLE_OBJECT::HANDLE_OBJECT()
        HANDLE_OBJECT::HANDLE_OBJECT()
        HANDLE_OBJECT::Reference()
        HANDLE_OBJECT::Dereference()
        HANDLE_OBJECT::IsValid()
        INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE(LPCSTR, ...)
        INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE(INTERNET_HANDLE_BASE*)
        INTERNET_HANDLE_BASE::~INTERNET_HANDLE_BASE()
        INTERNET_HANDLE_BASE::SetAbortHandle(ICSocket)
        INTERNET_HANDLE_BASE::ResetAbortHandle()
        INTERNET_HANDLE_BASE::AbortSocket()
        INTERNET_HANDLE_BASE::SetProxyInfo()
        INTERNET_HANDLE_BASE::GetProxyInfo(LPVOID, LPDWORD)
        INTERNET_HANDLE_BASE::GetProxyInfo(INTERNET_SCHEME, LPINTERNET_SCHEME, LPSTR *, LPDWORD, LPINTERNET_PORT)

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "../http/cookiejar.h"
//
// private manifests
//

#define PROXY_REGISTRY_STRING_LENGTH    (4 K)

//
// methods
//


HANDLE_OBJECT::HANDLE_OBJECT(
    IN HANDLE_OBJECT * Parent
    )

/*++

Routine Description:

    HANDLE_OBJECT constructor

Arguments:

    Parent  - pointer to parent HANDLE_OBJECT

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "HANDLE_OBJECT",
                "%#x",
                this
                ));

    //InitializeListHead(&_List);
    _Parent = Parent;
    if (_Parent)
        _Parent->Reference();
    _Status = AllocateHandle(this, &_Handle);
    _ObjectType = TypeGenericHandle;
    _ReferenceCount = 1;
    _Invalid = FALSE;
    _Error = ERROR_SUCCESS;
    _Signature = OBJECT_SIGNATURE;
    _Context = NULL;

#if INET_DEBUG
    if (!InsertAtTailOfSerializedList(&GlobalObjectList, &_List))
        _Status = ERROR_NOT_ENOUGH_MEMORY;
#endif

    //
    // if AllocateHandle() failed then we cannot create this handle object.
    // Invalidate it ready for the destructor
    //

    if (_Status != ERROR_SUCCESS) {
        _Invalid = TRUE;
        _ReferenceCount = 0;
    }

    DEBUG_PRINT(OBJECTS,
                INFO,
                ("handle %#x created; address %#x; %d objects\n",
                _Handle,
                this,
                ElementsOnSerializedList(&GlobalObjectList)
                ));

    DEBUG_LEAVE(0);
}


HANDLE_OBJECT::~HANDLE_OBJECT(VOID)

/*++

Routine Description:

    HANDLE_OBJECT destructor. Virtual function

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~HANDLE_OBJECT",
                "%#x",
                this
                ));

#if INET_DEBUG
    //
    // remove this object from global object list
    //

    if (LockSerializedList(&GlobalObjectList))
    {
        // should always succeed since we already have the lock
        RemoveFromSerializedList(&GlobalObjectList, &_List);
        UnlockSerializedList(&GlobalObjectList);
    }
#endif

    INET_DEBUG_ASSERT((_List.Flink == NULL) && (_List.Blink == NULL));

    //
    // inform the app that this handle is completely closed
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo != NULL)
    {
        HINTERNET hCurrent = _InternetGetObjectHandle(lpThreadInfo);
        HINTERNET hCurrentMapped = _InternetGetMappedObjectHandle(lpThreadInfo);

        _InternetSetObjectHandle(lpThreadInfo, _Handle, (HINTERNET)this);

        HINTERNET hTemp = _Handle;
        InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING,
                               (LPVOID)&hTemp,
                               sizeof(hTemp)
                               );

        _InternetSetObjectHandle(lpThreadInfo, hCurrent, hCurrentMapped);
    }

    if (_Parent != NULL)
        _Parent->Dereference();
        
    //
    // now we can free up the API handle value
    //

    if (_Handle != NULL) {
        _Status = FreeHandle(_Handle);

        INET_ASSERT(_Status == ERROR_SUCCESS);

    }

    //
    // set the signature to a value that indicates the handle has been
    // destroyed (not useful in debug builds)
    //

    _Signature = DESTROYED_OBJECT_SIGNATURE;

    INET_ASSERT((_ReferenceCount == 0) && _Invalid);

    DEBUG_PRINT(OBJECTS,
                INFO,
                ("handle %#x destroyed; type %s; address %#x; %d objects\n",
                _Handle,
                InternetMapHandleType(_ObjectType),
                this,
                ElementsOnSerializedList(&GlobalObjectList)
                ));

    DEBUG_LEAVE(0);
}


DWORD
HANDLE_OBJECT::Reference(
    VOID
    )

/*++

Routine Description:

    Increases the reference count on the HANDLE_OBJECT

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    Handle has already been invalidated
                  ERROR_ACCESS_DENIED
                    Handle object is being destroyed, cannot use it

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Dword,
                 "HANDLE_OBJECT::Reference",
                 "{%#x}",
                 _Handle
                 ));

    DWORD error;

    if (_Invalid) {

        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("handle object %#x [%#x] is invalid\n",
                    _Handle,
                    this
                    ));

        error = ERROR_INVALID_HANDLE;
    } else {
        error = ERROR_SUCCESS;
    }

    //
    // even if the handle has been invalidated (i.e. closed), we allow it
    // to continue to be referenced. The caller should return the fact
    // that the handle has been invalidated, but may require information
    // from the object in order to do so (e.g. in async thread)
    //

    do
    {
        LONG lRefCountBeforeIncrement = _ReferenceCount;

        //
        // refcount is > 0 means that the object's destructor has not been called yet
        //
        if (lRefCountBeforeIncrement > 0)
        {
            //
            // try to increment the refcount using compare-exchange
            //
#ifndef _WIN64
            LONG lRefCountCurrent = (LONG)SHInterlockedCompareExchange((LPVOID*)&_ReferenceCount,
                                                                       (LPVOID)(lRefCountBeforeIncrement + 1),
                                                                       (LPVOID)lRefCountBeforeIncrement);
#else
            //
            // can't use SHInterlockedCompareExchange on win64 because the values are really LONG's (32-bits) but they
            // are treated as pointers (64-bits) because SHInterlockedCompareExchange should really be called 
            // SHInterlockedCompareExchangePointer (sigh...).
            //
            LONG lRefCountCurrent = InterlockedCompareExchange(&_ReferenceCount,
                                                               lRefCountBeforeIncrement + 1,
                                                               lRefCountBeforeIncrement);
#endif        
            if (lRefCountCurrent == lRefCountBeforeIncrement)
            {
                //
                // since SHInterlockedCompareExchange returns the value in _ReferenceCount 
                // before the exchange, we know the exchange sucessfully took place (i.e. we 
                // sucessfully incremented the refrence count of the object by one)
                //
                INET_ASSERT(lRefCountCurrent > 0);
                break;
            }
        }
        else
        {
            //
            // the refcount dropped to zero before we could increment it,
            // so the object is being destroyed. 
            //
            error = ERROR_ACCESS_DENIED;
            break;
        }

    } while (TRUE);

    DEBUG_PRINT(REFCOUNT,
                INFO,
                ("handle object %#x [%#x] ReferenceCount = %d\n",
                _Handle,
                this,
                _ReferenceCount
                ));

    DEBUG_LEAVE(error);

    return error;
}


BOOL
HANDLE_OBJECT::Dereference(
    VOID
    )

/*++

Routine Description:

    Reduces the reference count on the HANDLE_OBJECT, and if it goes to zero,
    the object is deleted

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - this object was deleted

        FALSE   - this object is still valid

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 Bool,
                 "HANDLE_OBJECT::Dereference",
                 "{%#x}",
                 _Handle
                 ));

    //
    // by the time we get here, the reference count should not be 0. There
    // should be 1 call to Dereference() for each call to Reference()
    //

    INET_ASSERT(_ReferenceCount != 0);

    BOOL deleted = FALSE;

    if (InterlockedDecrement(&_ReferenceCount) == 0)
    {
        deleted = TRUE;
    }


    if (deleted)
    {
        //
        // if we are calling the destructor, the handle had better be invalid!
        //
        INET_ASSERT(_Invalid);
        
        //
        // this handle has now been closed. If there is no activity on it
        // then it will be destroyed
        //

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("handle object %#x [%#x] ReferenceCount = %d\n",
                    _Handle,
                    this,
                    _ReferenceCount
                    ));

        delete this;
    } else {

        DEBUG_PRINT(REFCOUNT,
                    INFO,
                    ("handle object %#x [%#x] ReferenceCount = %d\n",
                    _Handle,
                    this,
                    _ReferenceCount
                    ));
    }

    DEBUG_LEAVE(deleted);

    return deleted;
}


DWORD
HANDLE_OBJECT::IsValid(
    IN HINTERNET_HANDLE_TYPE ExpectedHandleType
    )

/*++

Routine Description:

    Checks a HANDLE_OBJECT for validity

Arguments:

    ExpectedHandleType  - type of object we are testing for. Can be
                          TypeWildHandle which matches any valid handle

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    The handle object is invalid

                  ERROR_WINHTTP_INCORRECT_HANDLE_TYPE
                    The handle object is valid, but not the type we want

--*/

{
    DWORD error;

    //
    // test handle object within try..except in case we are given a bad address
    //

    __try {
        if (_Signature == OBJECT_SIGNATURE) {

            error = ERROR_SUCCESS;

            //
            // check handle type if we are asked to do so.
            //

            if (ExpectedHandleType != TypeWildHandle) {
                if (ExpectedHandleType != this->GetHandleType()) {
                    error = ERROR_WINHTTP_INCORRECT_HANDLE_TYPE;
                }
            }
        } else {
            error = ERROR_INVALID_HANDLE;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_HANDLE;
    }
    ENDEXCEPT
    return error;
}


INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE(
    LPCSTR UserAgent,
    DWORD AccessMethod,
    LPSTR ProxyServerList,
    LPSTR ProxyBypassList,
    DWORD Flags
    ) : HANDLE_OBJECT(NULL)

/*++

Routine Description:

    Creates the handle object for InternetOpen()

Arguments:

    UserAgent       - name of agent (user-agent string for HTTP)

    AccessMethod    - DIRECT, PROXY or PRECONFIG

    ProxyServerList - one or more proxy servers. The string has the form:

                        [<scheme>=][<scheme>"://"]<server>[":"<port>][";"*]

    ProxyBypassList - zero or more addresses which if matched will result in
                      requests NOT going via the proxy (only if PROXY access).
                      The string has the form:

                        bp_entry ::= [<scheme>"://"]<server>[":"<port>]
                        bp_macro ::= "<local>"
                        bp_list ::= [<> | bp_entry bp_macro][";"*]

    Flags           - various open flags:

                        WINHTTP_FLAG_ASYNC - not support in WinHttpX

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE",
                 NULL
                 ));


    //
    // if the HANDLE_OBJECT constructor failed then bail out now
    //

    if (_Status != ERROR_SUCCESS) {

        DEBUG_PRINT(OBJECTS,
                    ERROR,
                    ("early-exit: _Status = %d\n",
                    _Status
                    ));

        DEBUG_LEAVE(0);

        return;
    }

    _PPContext = 0;

    _fDisableTweener = TRUE;
    _fDisableKeyring = TRUE;

    _ThreadToken = 0;
    
    _IsCopy = FALSE;
    _UserAgent = (LPSTR)UserAgent;
    _ProxyInfo = NULL;
    _dwInternetOpenFlags = Flags;
    _WinsockLoaded = FALSE;

    _Context = NULL;

    _MaxConnectionsPerServer    = WINHTTP_CONNS_PER_SERVER_UNLIMITED;
    _MaxConnectionsPer1_0Server = WINHTTP_CONNS_PER_SERVER_UNLIMITED;
    
    //
    // set _Async based on the WINHTTP_FLAG_ASYNC supplied to InternetOpen()
    //

    _dwCodePage                 = CP_UTF8;
    _Async = (Flags & WINHTTP_FLAG_ASYNC) ? TRUE : FALSE;

    //
    // no data available yet
    //

    SetAvailableDataLength(0);

    //
    // not yet end of file
    //

    ResetEndOfFile();

    //
    // no status callback by default
    //

    _StatusCallback = NULL;
    _StatusCallbackType = FALSE;
    _dwStatusCallbackFlags = 0;

    SetObjectType(TypeInternetHandle);

    _ProxyInfoResourceLock.Initialize();

    _Status = SetProxyInfo(AccessMethod, ProxyServerList, ProxyBypassList);

    //
    // if _pICSocket is not NULL then this is the socket that this object handle
    // is currently working on. We close it to cancel the operation
    //

    _pICSocket = NULL;

    
    if (::OpenThreadToken(
                GetCurrentThread(),
                TOKEN_READ | TOKEN_IMPERSONATE,
                FALSE,
                &_ThreadToken
                ) == FALSE)
    {
        _ThreadToken = 0;
    }
    //
    // load winsock now.
    //

    if (_Status == ERROR_SUCCESS) {
    
        _Status = LoadWinsock();
        _WinsockLoaded = (_Status == ERROR_SUCCESS);

        if ( _Status == ERROR_SUCCESS )
        {
             LONG lOpenHandleCnt;

             LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

             if ( lpThreadInfo )
             {
                 lOpenHandleCnt = InterlockedIncrement((LPLONG)&GlobalInternetOpenHandleCount);

                 if ( lOpenHandleCnt == 0 )
                 {
                    DWORD fAlreadyInInit = (DWORD) InterlockedExchange((LPLONG) &GlobalAutoProxyInInit, TRUE);

                    UNREFERENCED_PARAMETER(fAlreadyInInit);
                    INET_ASSERT (! fAlreadyInInit );

                    g_pGlobalProxyInfo->ReleaseQueuedRefresh();

                    InterlockedExchange((LPLONG)&GlobalAutoProxyInInit, FALSE);
                 }
             }
        }
    }

    DEBUG_LEAVE(0);
}


INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE(
    INTERNET_HANDLE_BASE *INetObj
    ) : HANDLE_OBJECT((HANDLE_OBJECT*)INetObj)

/*++

Routine Description:

    Constructor for derived handle object. We are creating this handle as part
    of an INTERNET_CONNECT_HANDLE_OBJECT

Arguments:

    INetObj - pointer to INTERNET_HANDLE_BASE to copy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_BASE::INTERNET_HANDLE_BASE",
                 "{IsCopy}"
                 ));

    _PPContext = INetObj->_PPContext;
    _fDisableTweener = INetObj->_fDisableTweener;
    _fDisableKeyring = INetObj->_fDisableKeyring;
    _ThreadToken = INetObj->_ThreadToken;
    
    _IsCopy = TRUE;

    //
    // copy user agent string
    //

    //
    // BUGBUG - compiler generated copy constructor (no new string)
    //

    _UserAgent = INetObj->_UserAgent;

    //
    // do not inherit the proxy info - code must go to parent handle
    //

    _ProxyInfo = NULL;

    _ProxyInfoResourceLock.Initialize();

    _dwInternetOpenFlags = INetObj->_dwInternetOpenFlags;

    //
    // creating this handle didn't load winsock
    //

    _WinsockLoaded = FALSE;

    //
    // inherit the context, async flag and status callback from
    // the parent object handle
    //

    _Context = INetObj->_Context;
 
    _MaxConnectionsPerServer    = INetObj->_MaxConnectionsPerServer;
    _MaxConnectionsPer1_0Server = INetObj->_MaxConnectionsPer1_0Server;

    _dwCodePage = INetObj->_dwCodePage;

    _Async = INetObj->_Async;

    //
    // inherit callback function
    //

    SetAvailableDataLength(0);
    ResetEndOfFile();
    _StatusCallback = INetObj->_StatusCallback;
    _StatusCallbackType = INetObj->_StatusCallbackType;
    _dwStatusCallbackFlags = INetObj->_dwStatusCallbackFlags;


    //
    // no socket operation to abort yet
    //

    _pICSocket = NULL;

    //
    // BUGBUG - this overwrites status set above?
    //

    _Status = INetObj->_Status;


    DEBUG_LEAVE(0);
}


INTERNET_HANDLE_BASE::~INTERNET_HANDLE_BASE(
    VOID
    )

/*++

Routine Description:

    INTERNET_HANDLE_BASE destructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_BASE::~INTERNET_HANDLE_BASE",
                 ""
                 ));


    if (_ProxyInfo && !IsProxyGlobal() && (_ProxyInfo != PROXY_INFO_DIRECT))
    {
        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("Free-ing ProxyInfo\n"
                    ));

        delete _ProxyInfo;
        _ProxyInfo = NULL;
    }


    //
    // if this handle is not a copy (i.e., it is a Session handle), then delete
    // the Passport context
    //

    if (!IsCopy()) {

        DEBUG_PRINT(OBJECTS,
                    INFO,
                    ("Not a Copy...\n"
                    ));

        if (_PPContext)
        {
            ::PP_FreeContext(_PPContext);
        }

        //
        // don't unload winsock. There really is no need to unload separately
        // from process detach and if we do unload, we first have to terminate
        // async support. Dynaloading and unloading winsock is vestigial
        //

        //if (_WinsockLoaded) {
        //    UnloadWinsock();
        //}

        if (_ThreadToken)
        {
            ::CloseHandle(_ThreadToken);
            _ThreadToken = NULL;
        }
    }

    DEBUG_LEAVE(0);
}

DWORD
INTERNET_HANDLE_BASE::ExchangeStatusCallback(
    LPWINHTTP_STATUS_CALLBACK lpStatusCallback,
    BOOL fType,
    DWORD dwFlags
    )
{
    DWORD error;


    WINHTTP_STATUS_CALLBACK callback;

    // exchange new and current callbacks
    callback = _StatusCallback;
    _StatusCallback = *lpStatusCallback;
    *lpStatusCallback = callback;
    _StatusCallbackType = fType;
    _dwStatusCallbackFlags = dwFlags;
    error = ERROR_SUCCESS;

    return error;
}


VOID
INTERNET_HANDLE_BASE::SetAbortHandle(
    IN ICSocket * Socket
    )

/*++

Routine Description:

    Associates with this request handle the ICSocket object currently being used
    for network I/O

Arguments:

    Socket  - pointer to ICSocket

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_BASE::SetAbortHandle",
                 "{%#x} %#x [sock=%#x ref=%d]",
                 GetPseudoHandle(),
                 Socket,
                 Socket ? Socket->GetSocket() : 0,
                 Socket ? Socket->ReferenceCount() : 0
                 ));

    INET_ASSERT(Socket != NULL);

    //
    // first off, increase the socket reference count to stop any other threads
    // killing it whilst we are performing the socket operation. The only way
    // another thread can dereference the socket is by calling our AbortSocket()
    // method
    //

    Socket->Reference();

    //
    // now associate the socket object with this handle object. We should not
    // have a current association
    //

    ICSocket * pSocket;

    pSocket = (ICSocket *) InterlockedExchangePointer((PVOID*)&_pICSocket, Socket);

    //
    // because ConnectSocket() can call this method multiple times without
    // intervening calls to ResetAbortHandle(), pSocket can legitimately be
    // non-NULL at this point
    //

    //INET_ASSERT(pSocket == NULL);

    //
    // if the handle was invalidated on another thread before we got
    // chance to set the socket to close, then abort the request now
    //

    //
    // BUGBUG - screws up normal FTP close handle processing - we
    //          have to communicate with the server in order to
    //          drop the connection
    //

    //if (IsInvalidated()) {
    //    AbortSocket();
    //}

    DEBUG_LEAVE(0);
}


VOID
INTERNET_HANDLE_BASE::ResetAbortHandle(
    VOID
    )

/*++

Routine Description:

    Disassociates this request handle and the ICSocket object when the network
    operation has completed

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_BASE::ResetAbortHandle",
                 "{%#x}",
                 GetPseudoHandle()
                 ));

    //
    // there really should be a ICSocket associated with this object, otherwise
    // our handle close/invalidation logic is broken
    //

    //
    // however, we can call ResetAbortHandle() from paths where we completed
    // early, not having called SetAbortHandle()
    //

    //INET_ASSERT(pSocket != NULL);

    //
    // so if there was a ICSocket associated with this object then remove the
    // reference added in SetAbortHandle()
    //


    ICSocket * pICSocket;

    pICSocket = (ICSocket *)InterlockedExchangePointer((PVOID*)&_pICSocket, NULL);
    if (pICSocket != NULL) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("socket=%#x ref=%d\n",
                    pICSocket->GetSocket(),
                    pICSocket->ReferenceCount()
                    ));

        pICSocket->Dereference();
    }

    DEBUG_LEAVE(0);
}


VOID
INTERNET_HANDLE_BASE::AbortSocket(
    VOID
    )

/*++

Routine Description:

    If there is a ICSocket associated with this handle object then abort it. This
    forces the current network operation aborted and the request to complete
    with ERROR_WINHTTP_OPERATION_CANCELLED

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS | DBG_SOCKETS,
                 None,
                 "INTERNET_HANDLE_BASE::AbortSocket",
                 "{%#x, %#x [sock=%#x, ref=%d]}",
                 GetPseudoHandle(),
                 (_pICSocket != NULL)
                    ? (LPVOID)_pICSocket
                    : (LPVOID)_pICSocket,
                 _pICSocket
                    ? _pICSocket->GetSocket()
                    : (_pICSocket
                        ? _pICSocket->GetSocket()
                        : 0),
                 _pICSocket
                    ? _pICSocket->ReferenceCount()
                    : (_pICSocket
                        ? _pICSocket->ReferenceCount()
                        : 0)
                 ));

    //
    // get the associated ICSocket. It may have already been removed by a call
    // to ResetAbortHandle()
    //

    //
    // if there is an associated ICSocket then abort it (close the socket handle)
    // which will complete the current network I/O (if active) with an error.
    // Once the ICSocket is aborted, we reduce the reference count that was added
    // in SetAbortHandle(). This may cause the ICSocket to be deleted
    //

    LPVOID pAddr;

    pAddr = (LPVOID)InterlockedExchangePointer((PVOID*)&_pICSocket, NULL);
    if (pAddr != NULL) {

        ICSocket * pSocket = (ICSocket *)pAddr;
//dprintf(">>>>>>>> %#x AbortSocket %#x [%#x]\n", GetCurrentThreadId(), pSocket, pSocket->GetSocket());
        pSocket->Abort();
        pSocket->Dereference();
    }

    DEBUG_LEAVE(0);
}


DWORD
INTERNET_HANDLE_BASE::Refresh()
/*++

Routine Description:

    Refreshes the proxy info on an InternetOpen() HINTERNET based on the parameters

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Return Value:

    DWORD
        Success - ERROR_SUCCESS

--*/
{

    //
    // Reload the proxy info from registry into the GlobalProxyInfo object,
    // unless it was changed in-process to something else.
    //

    if (!g_pGlobalProxyInfo->IsModifiedInProcess()) {

        return LoadProxySettings();

    } else {

        //
        // not using global proxy or it has been set to something other
        // than the registry contents. Just return success
        //

        return ERROR_SUCCESS;
    }
}



DWORD
INTERNET_HANDLE_BASE::SetProxyInfo(
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL
    )

/*++

Routine Description:

    Sets the proxy info on an InternetOpen() HINTERNET based on the parameters

    Assumes: 1. The parameters have already been validated in the API that calls
                this method (i.e. InternetOpen(), InternetSetOption())

Arguments:

    dwAccessType    - type of proxy access required

    lpszProxy       - pointer to proxy server list

    lpszProxyBypass - pointer to proxy bypass list

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER
                    The lpszProxy or lpszProxyBypass list was bad

                  ERROR_NOT_ENOUGH_MEMORY
                    Failed to create an object or allocate space for a list,
                    etc.

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "INTERNET_HANDLE_BASE::SetProxyInfo",
                "%s (%d), %#x (%q), %#x (%q)",
                InternetMapOpenType(dwAccessType),
                dwAccessType,
                lpszProxy,
                lpszProxy,
                lpszProxyBypass,
                lpszProxyBypass
                ));

    //
    // Session and HTTP Request objects can have proxy information
    //

    INET_ASSERT((GetHandleType()==TypeInternetHandle) || (GetHandleType()==TypeHttpRequestHandle));

/*

    We are setting the proxy information for an InternetOpen() handle. Based on
    the current and new settings we do the following (Note: the handle is
    initialized to DIRECT operation):

                                        current access
                +---------------------------------------------------------------
        new     |      DIRECT        |       PROXY        |      PRECONFIG
       access   |                    |                    |
    +-----------+--------------------+--------------------+---------------------
    | DIRECT    | No action          | Delete proxy info  | Remove reference to
    |           |                    |                    | global proxy info
    +-----------+--------------------+--------------------+---------------------
    | PROXY     | Set new proxy info | Delete proxy info. | Remove reference to
    |           |                    | Set new proxy info | global proxy info.
    |           |                    |                    | Set new proxy info
    +-----------+--------------------+--------------------+---------------------
    | PRECONFIG | Set proxy info to  | Delete proxy info. | No action
    |           | global proxy info  | Set proxy info to  |
    |           |                    | global proxy info  |
    +-----------+--------------------+--------------------+---------------------
*/

    DWORD error = ERROR_SUCCESS;
    PROXY_INFO * proxyInfo = NULL;

    //
    // acquire proxy info for exclusive access
    //

    if (!AcquireProxyInfo(TRUE))
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if (IsProxy()) {

        //
        // delete private proxy info, or unlink from global proxy info
        //

        SafeDeleteProxyInfo();
    }


    switch (dwAccessType)
    {
        case WINHTTP_ACCESS_TYPE_NO_PROXY:
            proxyInfo = PROXY_INFO_DIRECT;
            break;

        case WINHTTP_ACCESS_TYPE_NAMED_PROXY:
            {
                INET_ASSERT(!IsProxy());

                INTERNET_HANDLE_OBJECT * pSession;

                if (IsCopy())
                {
                    pSession = GetRootHandle(this);
                }
                else
                {
                    pSession = static_cast<INTERNET_HANDLE_OBJECT *>(this);
                    INET_ASSERT(pSession->IsValid(TypeInternetHandle) == ERROR_SUCCESS);
                }

                proxyInfo = New PROXY_INFO;
                if (proxyInfo != NULL) {
                    proxyInfo->InitializeProxySettings();
                    proxyInfo->SetSessionObject(pSession);
                    error = proxyInfo->GetError();
                    if (error == ERROR_SUCCESS &&
                        lpszProxy ) 
                    {

                        INTERNET_PROXY_INFO_EX info;
                        memset(&info, 0, sizeof(info));
                        info.dwStructSize = sizeof(info);
                        info.dwFlags = (PROXY_TYPE_DIRECT | PROXY_TYPE_PROXY);

                        info.lpszProxy = lpszProxy;
                        info.lpszProxyBypass = lpszProxyBypass;

                        error = proxyInfo->SetProxySettings(&info, TRUE /*modified*/);

                    }
                    if (error != ERROR_SUCCESS) {
                        delete proxyInfo;
                        proxyInfo = NULL;
                    }
                } else {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }

                break;
            }

        case WINHTTP_ACCESS_TYPE_DEFAULT_PROXY:
            {
                INTERNET_HANDLE_OBJECT * pSession;

                if (IsCopy())
                {
                    pSession = GetRootHandle(this);
                }
                else
                {
                    pSession = static_cast<INTERNET_HANDLE_OBJECT *>(this);
                    INET_ASSERT(pSession->IsValid(TypeInternetHandle) == ERROR_SUCCESS);
                }

                // Refresh global proxy info.
                Refresh();

                proxyInfo = New PROXY_INFO;
                if (proxyInfo != NULL) {
                    proxyInfo->InitializeProxySettings();
                    proxyInfo->SetSessionObject(pSession);
                    error = proxyInfo->GetError();
                    if (error == ERROR_SUCCESS)
                    {

                        INTERNET_PROXY_INFO_EX info;
                        memset(&info, 0, sizeof(info));
                        info.dwStructSize = sizeof(info);

                        INET_ASSERT(g_pGlobalProxyInfo != NULL);

                        error = g_pGlobalProxyInfo->GetProxySettings(&info, FALSE);

                        if (error == ERROR_SUCCESS)
                        {
                            error = proxyInfo->SetProxySettings(&info, TRUE /*modified*/);

                            if (info.lpszProxy)
                            {
                                FREE_MEMORY(info.lpszProxy);
                            }
                            if (info.lpszProxyBypass)
                            {
                                FREE_MEMORY(info.lpszProxyBypass);
                            }
                        }
                    }
                    if (error != ERROR_SUCCESS)
                    {
                        delete proxyInfo;
                        proxyInfo = NULL;
                    }
                } else {
                    error = ERROR_NOT_ENOUGH_MEMORY;
                }

                break;
            }

        default:
            proxyInfo = NULL;
            break;
    }

    SetProxyInfo(proxyInfo);

    ReleaseProxyInfo();

quit:
    DEBUG_LEAVE(error);

    return error;
}


DWORD
INTERNET_HANDLE_BASE::GetProxyStringInfo(
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the current proxy information for this INTERNET_HANDLE_BASE

Arguments:

    lpBuffer            - pointer to buffer where WINHTTP_PROXY_INFOA will be
                          written, and any proxy strings (if sufficient space)

    lpdwBufferLength    - IN: number of bytes in lpBuffer
                          OUT: number of bytes returned in lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer doesn't have enough space to hold the proxy
                    information. *lpdwBufferLength has the required size

--*/

{
    DEBUG_ENTER((DBG_INET,
                Dword,
                "INTERNET_HANDLE_BASE::GetProxyStringInfo",
                "%#x, %#x [%d]",
                lpBuffer,
                lpdwBufferLength,
                lpdwBufferLength ? *lpdwBufferLength : 0
                ));

    INET_ASSERT(!IsCopy());

    AcquireProxyInfo(FALSE);

    DWORD error;

    if (IsProxy()) {
        error = _ProxyInfo->GetProxyStringInfo(lpBuffer, lpdwBufferLength);
    } else {
        if (*lpdwBufferLength >= sizeof(WINHTTP_PROXY_INFOA)) {

            WINHTTP_PROXY_INFOA * lpInfo = (WINHTTP_PROXY_INFOA *)lpBuffer;

            lpInfo->dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
            lpInfo->lpszProxy = NULL;
            lpInfo->lpszProxyBypass = NULL;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        *lpdwBufferLength = sizeof(WINHTTP_PROXY_INFOA);
    }

    ReleaseProxyInfo();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
INTERNET_HANDLE_BASE::GetProxyInfo(
    IN AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
    )

/*++

Routine Description:

    Returns all proxy information based on a protocol scheme

Arguments:

    tProtocol           - protocol to get proxy info for

    lptScheme           - returned scheme

    lplpszHostName      - returned proxy name

    lpdwHostNameLength  - returned length of proxy name

    lpHostPort          - returned proxy port

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "INTERNET_HANDLE_BASE::GetProxyInfo",
                 "%#x",
                 ppQueryForProxyInfo
                 ));

    INET_ASSERT((GetHandleType() == TypeInternetHandle) ||
                (GetHandleType() == TypeHttpRequestHandle));

    DWORD error;

    AcquireProxyInfo(FALSE);

    if ( _ProxyInfo && _ProxyInfo != PROXY_INFO_DIRECT )
    {
        error = _ProxyInfo->QueryProxySettings(ppQueryForProxyInfo);
    }
    else
    {
        error = ERROR_SUCCESS;
        (*ppQueryForProxyInfo)->SetUseProxy(FALSE);
    }

    ReleaseProxyInfo();

    DEBUG_LEAVE(error);

    return error;
}


BOOL
INTERNET_HANDLE_BASE::RedoSendRequest(
    IN OUT LPDWORD lpdwError,
    IN DWORD dwSecureStatus,
    IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
    IN CServerInfo *pOriginServer,
    IN CServerInfo *pProxyServer
    )
{

    INET_ASSERT(!IsCopy());

    BOOL rc;

   AcquireProxyInfo(FALSE);

    if ( _ProxyInfo )
    {
        rc = _ProxyInfo->RedoSendRequest(
                                         lpdwError,
                                         dwSecureStatus,
                                         pQueryForProxyInfo,
                                         pOriginServer,
                                         pProxyServer
                                         );
    }
    else
    {
        rc = FALSE;
    }

    ReleaseProxyInfo();

    return rc;
}

INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT
    (LPCSTR ua, DWORD access, LPSTR proxy, LPSTR bypass, DWORD flags)
        : INTERNET_HANDLE_BASE (ua, access, proxy, bypass, flags)
{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_OBJECT::INTERNET_HANDLE_OBJECT",
                 NULL
                 ));
                 
    InterlockedIncrement(&g_cSessionCount);
    if (g_pAsyncCount)
    {
        if (flags & WINHTTP_FLAG_ASYNC)
        {
            _Status = g_pAsyncCount->AddRef();
        }
    }
    else
    {
        RIP(FALSE);
    }

    InitializeSerializedList(&_ServerInfoList);
    //
    // WinHttpX supports session cookies. Each session has it's own
    // cookie jar, instead of a shared global cookie jar as in WinInet.
    //
    _CookieJar = NULL;
    _pOptionalParams = NULL;
    _pSessionCertCache = NULL;
    _pAutoProxy = NULL;
    _hAPBinding = NULL;

    _pResolverCache = New CResolverCache(&_Status);
    if (!_pResolverCache)
    {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
    }
    
    if (_Status == ERROR_SUCCESS)
    {
        _CookieJar = CreateCookieJar();
        if (_CookieJar == NULL)
            _Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (_Status == ERROR_SUCCESS)
    {
        _pSessionCertCache = New SECURITY_CACHE_LIST;
        if (!_pSessionCertCache || !_pSessionCertCache->Initialize())
        {
            _Status = ERROR_NOT_ENOUGH_MEMORY;

            if (_pSessionCertCache)
            {
                delete _pSessionCertCache;
                _pSessionCertCache = NULL;
            }
        }
    }

    if (_Status == ERROR_SUCCESS)
    {
        _pAutoProxy = New CAutoProxy(this);

        if (!_pAutoProxy || !_pAutoProxy->Initialize())
        {
            _Status = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DEBUG_LEAVE(0);
};

INTERNET_HANDLE_OBJECT::~INTERNET_HANDLE_OBJECT ( )
{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "INTERNET_HANDLE_OBJECT::~INTERNET_HANDLE_OBJECT",
                 NULL
                 ));
                 
    PurgeServerInfoList(TRUE);

    TerminateSerializedList(&_ServerInfoList);

    if (_pResolverCache)
    {
        _pResolverCache->EmptyHandlesList();
        delete _pResolverCache;
    }
    
    delete _pOptionalParams;
    _pOptionalParams = NULL;
    // Delete Cookie Jar
    CloseCookieJar(_CookieJar);
    _CookieJar = NULL;

    if (_hAPBinding)
    {
        ::RpcBindingFree(&_hAPBinding);
        _hAPBinding = NULL;
    }

    if (_pSessionCertCache)
        _pSessionCertCache->Release();

    if (_pAutoProxy)
        delete _pAutoProxy;

    if (g_pAsyncCount)
    {
        if (_Async)
            g_pAsyncCount->Release();
    }
    else
    {
        RIP(FALSE);
    }
        
    DEBUG_LEAVE(0);
}

//
// This function walks up to the InternetOpen handle from either
// a connect handle (child) or a request handle (grandchild).
// We only go one or two hops rather than recurse.
//

INTERNET_HANDLE_OBJECT* GetRootHandle (HANDLE_OBJECT* pHandle)
{
    pHandle = (HANDLE_OBJECT*) pHandle->GetParent();
    INET_ASSERT (pHandle);
    if (pHandle->GetHandleType() == TypeInternetHandle)
        return (INTERNET_HANDLE_OBJECT*) pHandle;

    pHandle =  (HANDLE_OBJECT*) pHandle->GetParent();
    INET_ASSERT (pHandle);
    INET_ASSERT (pHandle->GetHandleType() == TypeInternetHandle);
    return (INTERNET_HANDLE_OBJECT*) pHandle;
}

BOOL INTERNET_HANDLE_OBJECT::ClearPassportCookies(PSTR pszUrl)
{
    BOOL fRetVal = FALSE;

    char *  pchRDomain = NULL;
    char *  pchPath = NULL;
    BOOL    fSecure;

    if (!PathAndRDomainFromURL(pszUrl, &pchRDomain, &pchPath, &fSecure, FALSE))
    {
        return fRetVal;
    }

    CCookieJar* pcj = _CookieJar;
    if (pcj->_csCookieJar.Lock())
    {
        FILETIME ftNow;
        GetSystemTimeAsFileTime(&ftNow);

        CCookieLocation *pLocation = pcj->GetCookies(pchRDomain, pchPath, NULL, &ftNow);

        while (pLocation) 
        {
            CCookie *pCookie = NULL;
            
            if (NULL != (pCookie = pLocation->GetCookie("MSPAuth", FALSE)))
            {
                pCookie->SetValue(NULL);
            }

            if (NULL != (pCookie = pLocation->GetCookie("MSPProf", FALSE)))
            {
                pCookie->SetValue(NULL);
            }

            if (NULL != (pCookie = pLocation->GetCookie("MSPProfC", FALSE)))
            {
                pCookie->SetValue(NULL);
            }

            if (NULL != (pCookie = pLocation->GetCookie("MSPSec", FALSE)))
            {
                pCookie->SetValue(NULL);
            }

            pLocation = pcj->GetCookies(pchRDomain, pchPath, pLocation, &ftNow);
        } // while (pLocation)
        pcj->_csCookieJar.Unlock();
    } // if pcj->_csCookieJar.Lock()

    fRetVal = TRUE;

    if (pchRDomain)
    {
        FREE_MEMORY(pchRDomain);
    }

    if (pchPath)
    {
        FREE_MEMORY(pchPath);
    }

    return fRetVal;
}

//
//
//INTERNET_HANDLE_BASE::SetDwordOption() and HTTP_REQUEST_HANDLE_OBJECT::SetDwordOption()
//maintain DWORD options with the following semantics:
//  1)  Settings affect behavior of request and are per-request configureable.
//  2)  Defaults for new requests are stored in the session and those defaults are per-session configureable.
//
//

/* 
 * When called from API functions,
 * caller should SetLastError() in case of failure
 */
BOOL
INTERNET_HANDLE_OBJECT::SetDwordOption(
    IN DWORD dwDwordOption,
    IN DWORD dwDwordValue
    )
{
    BOOL bRetval = TRUE;
    
    if (!_pOptionalParams)
    {
        _pOptionalParams = New OPTIONAL_SESSION_PARAMS();

        if (!_pOptionalParams)
        {
            bRetval = FALSE;
            goto quit;
        }
    }

    switch (dwDwordOption) 
    {
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        _pOptionalParams->dwResolveTimeout = dwDwordValue;
        break;

    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        _pOptionalParams->dwConnectTimeout = dwDwordValue;
        break;

    case WINHTTP_OPTION_CONNECT_RETRIES:
        _pOptionalParams->dwConnectRetries = dwDwordValue;
        break;

    case WINHTTP_OPTION_SEND_TIMEOUT:
        _pOptionalParams->dwSendTimeout = dwDwordValue;
        break;

    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        _pOptionalParams->dwReceiveTimeout = dwDwordValue;

        // Ensure that the ReceiveResponse timeout is not less than the Receive timeout
        _pOptionalParams->dwReceiveResponseTimeout = max(dwDwordValue, _pOptionalParams->dwReceiveResponseTimeout);
        break;

    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        // Ensure that the ReceiveResponse timeout is not less than the Receive timeout
        _pOptionalParams->dwReceiveResponseTimeout = max(dwDwordValue, _pOptionalParams->dwReceiveTimeout);
        break;

    case WINHTTP_OPTION_REDIRECT_POLICY:
        _pOptionalParams->dwRedirectPolicy = dwDwordValue;
        break;

    case WINHTTP_OPTION_AUTOLOGON_POLICY:
        _pOptionalParams->dwAutoLogonPolicy = dwDwordValue;
        break;

    case WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS:
        _pOptionalParams->dwMaxHttpAutomaticRedirects = dwDwordValue;
        break;
        
    case WINHTTP_OPTION_MAX_HTTP_STATUS_CONTINUE:
        _pOptionalParams->dwMaxHttpStatusContinues = dwDwordValue;
        break;

    case WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE:
        _pOptionalParams->dwMaxResponseHeaderSize = dwDwordValue;
        break;
        
    case WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE:
        _pOptionalParams->dwMaxResponseDrainSize = dwDwordValue;
        break;

    default:
        INET_ASSERT(0);
        bRetval = FALSE;
    }

quit:
    return bRetval;
}


/* 
 * When called from API functions,
 * caller should SetLastError() in case of failure
 */
BOOL
INTERNET_HANDLE_OBJECT::SetTimeout(
    IN DWORD dwTimeoutOption,
    IN DWORD dwTimeoutValue
    )
{
    switch (dwTimeoutOption) 
    {
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_RETRIES:
    case WINHTTP_OPTION_SEND_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        return SetDwordOption( dwTimeoutOption, dwTimeoutValue);
    default:
        INET_ASSERT(0);
        return FALSE;
    }
}

/* 
 * When called from API functions,
 * caller should SetLastError() in case of failure
 */
BOOL
INTERNET_HANDLE_OBJECT::SetTimeouts(
    IN DWORD        dwResolveTimeout,
    IN DWORD        dwConnectTimeout,
    IN DWORD        dwSendTimeout,
    IN DWORD        dwReceiveTimeout
    )
{
    BOOL bRetval = TRUE;
    
    if (!_pOptionalParams)
    {
        _pOptionalParams = New OPTIONAL_SESSION_PARAMS();

        if (!_pOptionalParams)
        {
            bRetval = FALSE;
            goto quit;
        }
    }

    _pOptionalParams->dwResolveTimeout = dwResolveTimeout;
    _pOptionalParams->dwConnectTimeout = dwConnectTimeout;
    _pOptionalParams->dwSendTimeout = dwSendTimeout;
    _pOptionalParams->dwReceiveTimeout = dwReceiveTimeout;
    
    // Ensure that the ReceiveResponse timeout is not less than the Receive timeout
    _pOptionalParams->dwReceiveResponseTimeout = max(dwReceiveTimeout, _pOptionalParams->dwReceiveResponseTimeout);
            
quit:
    return bRetval;
}

/* 
 * When called from API functions,
 * caller should SetLastError() in case of failure
 */
BOOL
INTERNET_HANDLE_OBJECT::GetDwordOption(
    IN DWORD dwDwordOption,
    OUT DWORD* pdwDwordValue
    )
{
    BOOL bRetval = TRUE;
    if (!_pOptionalParams)
    {
        _pOptionalParams = New OPTIONAL_SESSION_PARAMS();

        if (!_pOptionalParams)
        {
            bRetval = FALSE;
            goto quit;
        }
    }
    
    switch (dwDwordOption) 
    {
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
        *pdwDwordValue = _pOptionalParams->dwResolveTimeout;
        break;
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
        *pdwDwordValue = _pOptionalParams->dwConnectTimeout;
        break;
    case WINHTTP_OPTION_CONNECT_RETRIES:
        *pdwDwordValue = _pOptionalParams->dwConnectRetries;
        break;
    case WINHTTP_OPTION_SEND_TIMEOUT:
        *pdwDwordValue = _pOptionalParams->dwSendTimeout;
        break;
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
        *pdwDwordValue = _pOptionalParams->dwReceiveTimeout;
        break;
    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        *pdwDwordValue = _pOptionalParams->dwReceiveResponseTimeout;
        break;
    case WINHTTP_OPTION_REDIRECT_POLICY:
        *pdwDwordValue = _pOptionalParams->dwRedirectPolicy;
        break;
    case WINHTTP_OPTION_AUTOLOGON_POLICY:
        *pdwDwordValue = _pOptionalParams->dwAutoLogonPolicy;
        break;
    case WINHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS:
        *pdwDwordValue = _pOptionalParams->dwMaxHttpAutomaticRedirects;
        break;
    case WINHTTP_OPTION_MAX_HTTP_STATUS_CONTINUE:
        *pdwDwordValue = _pOptionalParams->dwMaxHttpStatusContinues;
        break;
    case WINHTTP_OPTION_MAX_RESPONSE_HEADER_SIZE:
        *pdwDwordValue = _pOptionalParams->dwMaxResponseHeaderSize;
        break;
    case WINHTTP_OPTION_MAX_RESPONSE_DRAIN_SIZE:
        *pdwDwordValue = _pOptionalParams->dwMaxResponseDrainSize;
        break;

    default:
        INET_ASSERT(0);
        bRetval = FALSE;
        break;
    }

quit:    
    return bRetval;
}

/* 
 * When called from API functions,
 * caller should SetLastError() in case of failure
 */
BOOL
INTERNET_HANDLE_OBJECT::GetTimeout(
    IN DWORD dwTimeoutOption,
    OUT DWORD* pdwTimeoutValue
    )
{
    switch (dwTimeoutOption) 
    {
    case WINHTTP_OPTION_RESOLVE_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_TIMEOUT:
    case WINHTTP_OPTION_CONNECT_RETRIES:
    case WINHTTP_OPTION_SEND_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_TIMEOUT:
    case WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT:
        return GetDwordOption( dwTimeoutOption, pdwTimeoutValue);
        
    default:
        INET_ASSERT(0);
        return FALSE;
    }
}
    
