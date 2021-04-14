//+------------------------------------------------------------------------
//
//  File:       memutil.cxx
//
//  Contents:   Memory utilities
//
//  History:    30-Oct-94   GaryBu  Consolidated from places far & wide.
//              06-Jul-95   PaulG   Macintosh compiles now use Global instead
//                                  of Heap memory functions
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_VMEM_HXX_
#define X_VMEM_HXX_
#include "vmem.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#if TIMEHEAP
#define HeapBegTimer(x) SwitchesBegTimer(x)
#define HeapEndTimer(x) SwitchesEndTimer(x)
#else
#define HeapBegTimer(x)
#define HeapEndTimer(x)
#endif

MtDefine(PerfPigs, NULL, "MSHTML Performance Pigs")
MtDefine(Metrics, NULL, "MSHTML Metrics")
MtDefine(WorkingSet, NULL, "MSHTML Working Set")
MtDefine(Mem, WorkingSet, "MemAlloc")
MtDefine(OpNew, Mem, "operator new")
MtDefine(Locals, Mem, "Per Function Local")
MtDefine(PerThread, Mem, "Per Thread Global")
MtDefine(PerProcess, Mem, "Per Process Global")
MtDefine(Layout, Mem, "Layout")
MtDefine(ObjectModel, Mem, "Object Model")
MtDefine(Utilities, Mem, "Utilities")
MtDefine(Printing, Mem, "Printing")
MtDefine(Tree, Mem, "Tree")
MtDefine(Edit, Mem, "Edit")
MtDefine(Behaviors, Mem, "Behaviors")

EXTERN_C HANDLE g_hProcessHeap;

#if !defined(UNIX) && !defined(_MAC) && !defined(_M_AMD64) && !defined(_M_IA64) && !defined(SLOWALLOC)
#define SMALLBLOCKHEAP 1
#endif

// DbgExCheckHeap -------------------------------------------------------------

#if DBG==1

DeclareTag(tagNoCheckHeap, "!Memory", "Disable CHECK_HEAP() macro");

void WINAPI
DbgExCheckHeap()
{
    if (!DbgExIsTagEnabled(tagNoCheckHeap))
    {
        AssertSz(DbgExValidateInternalHeap(), "Internal heap is corrupt!");
        AssertSz(CheckSmallBlockHeap(), "Small block heap is corrupt!");
    }
}

#endif

//+------------------------------------------------------------------------
// Allocation functions not implemented in this file:
//
//      CDUTIL.HXX
//          operator new
//          operator delete
//
//      OLE's OBJBASE.H
//          CoTaskMemAlloc, CoTaskMemFree
//
//-------------------------------------------------------------------------

#if SMALLBLOCKHEAP

DeclareTag(tagSmallBlockHeap, "!Memory", "Check small block heap every time")
DeclareTag(tagSmallBlockHeapDisable, "!Memory", "Disable small block heap");

#define _CRTBLD 1
#include "winheap.h"
EXTERN_C CRITICAL_SECTION g_csHeap;

#if DBG == 1
#define CHECKSBH if (IsTagEnabled(tagSmallBlockHeap)) {Assert(CheckSmallBlockHeap() && "Small block heap corrupt");};
BOOL IsSmallBlockHeapEnabled()
{
    static int g_fSmallBlockHeap = -1;
    if (g_fSmallBlockHeap == -1)
        g_fSmallBlockHeap = IsTagEnabled(tagSmallBlockHeapDisable) ? 0 : 1;
    return(g_fSmallBlockHeap == 1);
}
BOOL CheckSmallBlockHeap()
{
    if (IsSmallBlockHeapEnabled())
    {
        EnterCriticalSection(&g_csHeap);
        BOOL f = __sbh_heap_check() >= 0;
        LeaveCriticalSection(&g_csHeap);
        return f;
    }
    return TRUE;
}
#else
#define CHECKSBH
#endif

#else

#if DBG == 1
BOOL CheckSmallBlockHeap()
{
    return TRUE;
}
#endif

#endif SMALLBLOCKHEAP

#if DBG==1

static BOOL  g_fVMemInit    = FALSE;
static BOOL  g_fVMemEnable  = FALSE;
static DWORD g_dwFlagsVMem  = 0;

BOOL
IsVMemEnabled()
{
    if (!g_fVMemInit)
    {
        g_fVMemEnable = IsTagEnabled(tagMemoryStrict);

        if (IsTagEnabled(tagMemoryStrictTail))
        {
            g_dwFlagsVMem |= VMEM_BACKSIDESTRICT;

            if (IsTagEnabled(tagMemoryStrictAlign))
            {
                g_dwFlagsVMem |= VMEM_BACKSIDEALIGN8;
            }
        }

        g_fVMemInit = TRUE;
    }

    return(g_fVMemEnable);
}

#define VMEM

#elif defined(PERFTAGS) && !defined(PERFMETER)

PerfTag(tgMemoryStrict,        "!Memory", "Use VMem for MemAlloc")
PerfTag(tgMemoryStrictHead,    "!Memory", "VMem strict at beginning (vs end)")
PerfTag(tgMemoryStrictAlign,   "!Memory", "VMem pad to quadword at end")

static BOOL  g_fVMemInit   = FALSE;
static BOOL  g_fVMemEnable = FALSE;
static DWORD g_dwFlagsVMem = 0;

BOOL
IsVMemEnabled()
{
    if (!g_fVMemInit)
    {
        g_fVMemEnable = IsPerfEnabled(tgMemoryStrict);

        if (!IsPerfEnabled(tgMemoryStrictHead))
        {
            g_dwFlagsVMem |= VMEM_BACKSIDESTRICT;

            if (IsPerfEnabled(tgMemoryStrictAlign))
            {
                g_dwFlagsVMem |= VMEM_BACKSIDEALIGN8;
            }
        }

        g_fVMemInit = TRUE;
    }

    return(g_fVMemEnable);
}

#define VMEM

#elif defined(USE_VMEM_FOR_MEMALLOC)

#ifdef VMEM_FOR_MEMALLOC_FLAGS
#define g_dwFlagsVMem   VMEM_FOR_MEMALLOC_FLAGS
#else
#define g_dwFlagsVMem   VMEM_BACKSIDESTRICT
#endif

BOOL
IsVMemEnabled()
{
    return(TRUE);
}

#define VMEM

#endif

//+------------------------------------------------------------------------
//
//  Function:   _MemGetSize
//
//  Synopsis:   Get size of block allocated with MemAlloc/MemRealloc.
//
//              Note that MemAlloc/MemRealloc can allocate more than
//              the requested number of bytes. Therefore the size returned
//              from this function is possibly greater than the size
//              passed to MemAlloc/Realloc.
//
//  Arguments:  [pv] - Return size of this block.
//
//  Returns:    The size of the block, or zero of pv == NULL.
//
//-------------------------------------------------------------------------
ULONG
_MemGetSize(void *pv)
{
    if (pv == NULL)
        return 0;

#ifdef VMEM
    if (IsVMemEnabled())
    {
        return(VMemGetSize(pv));
    }
#endif

    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        __sbh_region_t *    preg;
        __sbh_page_t *      ppage;
        __map_t *           pmap;

        EnterCriticalSection(&g_csHeap);

        HeapBegTimer(SWITCHES_TIMER_SBHEAP_GETSIZE);

        if ((pmap = __sbh_find_block(DbgPreGetSize(pv), &preg, &ppage)) != NULL )
        {
            Assert(*pmap != 0);
            size_t s = DbgPostGetSize(((size_t)(*pmap)) << _PARASHIFT);

            HeapEndTimer(SWITCHES_TIMER_SBHEAP_GETSIZE);

            LeaveCriticalSection(&g_csHeap);

            return s;
        }

        HeapEndTimer(SWITCHES_TIMER_SBHEAP_GETSIZE);

        LeaveCriticalSection(&g_csHeap);
    }
#endif

    HeapBegTimer(SWITCHES_TIMER_PROCHEAP_GETSIZE);

    ULONG cbRet = DbgPostGetSize(HeapSize(g_hProcessHeap, 0, DbgPreGetSize(pv)));

    HeapEndTimer(SWITCHES_TIMER_PROCHEAP_GETSIZE);

    return cbRet;
}

//+------------------------------------------------------------------------
//
//  Function:   _MemAlloc
//
//  Synopsis:   Allocate block of memory.
//
//              The contents of the block are undefined.  If the requested size
//              is zero, this function returns a valid pointer.  The returned
//              pointer is guaranteed to be suitably aligned for storage of any
//              object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
_MemAlloc(ULONG cb)
{
    AssertSz (cb, "Requesting zero sized block.");
    void * pvRet;

    // The small-block heap will lose its mind if this ever happens, so we
    // protect against the possibility.

    if (cb == 0)
        cb = 1;

#ifdef VMEM
    if (IsVMemEnabled())
    {
        return(VMemAlloc(cb, g_dwFlagsVMem));
    }
#endif


    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        /* round up to the nearest paragraph */
        size_t cbr = (DbgPreAlloc(cb) + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < __sbh_threshold)
        {
            CHECKSBH;

            EnterCriticalSection(&g_csHeap);

            HeapBegTimer(SWITCHES_TIMER_SBHEAP_ALLOC);

            pvRet = DbgPostAlloc(__sbh_alloc_block(cbr >> _PARASHIFT));

            HeapEndTimer(SWITCHES_TIMER_SBHEAP_ALLOC);

            LeaveCriticalSection(&g_csHeap);
            Assert(!pvRet || _MemGetSize(pvRet) >= cb);
            if (pvRet)
            {
                return pvRet;
            }

        }
    }
#endif

    HeapBegTimer(SWITCHES_TIMER_PROCHEAP_ALLOC);

    pvRet = DbgPostAlloc(HeapAlloc(g_hProcessHeap, 0, DbgPreAlloc(cb)));

    HeapEndTimer(SWITCHES_TIMER_PROCHEAP_ALLOC);

    return pvRet;
}


//+------------------------------------------------------------------------
//  Function:   _MemAllocClear
//
//  Synopsis:   Allocate a zero filled block of memory.
//
//              If the requested size is zero, this function returns a valid
//              pointer. The returned pointer is guaranteed to be suitably
//              aligned for storage of any object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void *
_MemAllocClear(ULONG cb)
{
    AssertSz (cb, "Allocating zero sized block.");

    void * pvRet;

    // The small-block heap will lose its mind if this ever happens, so we
    // protect against the possibility.

    if (cb == 0)
        cb = 1;

#ifdef VMEM
    if (IsVMemEnabled())
    {
        return(VMemAllocClear(cb, g_dwFlagsVMem));
    }
#endif

    Assert(g_hProcessHeap);

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        /* round up to the nearest paragraph */
        size_t cbr = (DbgPreAlloc(cb) + _PARASIZE - 1) & ~(_PARASIZE - 1);

        if (cbr < __sbh_threshold)
        {
            CHECKSBH;


            EnterCriticalSection(&g_csHeap);

            HeapBegTimer(SWITCHES_TIMER_SBHEAP_ALLOCCLEAR);

            pvRet = DbgPostAlloc(__sbh_alloc_block(cbr >> _PARASHIFT));

            HeapEndTimer(SWITCHES_TIMER_SBHEAP_ALLOCCLEAR);

            LeaveCriticalSection(&g_csHeap);
            Assert(!pvRet || _MemGetSize(pvRet) >= cb);
            if (pvRet)
            {
                HeapBegTimer(SWITCHES_TIMER_SBHEAP_ALLOCCLEAR);
                memset(pvRet, 0, cb);
                HeapEndTimer(SWITCHES_TIMER_SBHEAP_ALLOCCLEAR);

                return pvRet;
            }

        }
    }
#endif

    HeapBegTimer(SWITCHES_TIMER_PROCHEAP_ALLOCCLEAR);

    pvRet = DbgPostAlloc(HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY,
                         DbgPreAlloc(cb)));

    // In debug, DbgPostAlloc set the memory so we need to clear it again.
    // On the Mac, HpAlloc doesn't support HEAP_ZERO_MEMORY.

#if DBG==1 || defined(_MAC)
    if (pvRet)
    {
        memset(pvRet, 0, cb);
    }
#endif

    HeapEndTimer(SWITCHES_TIMER_PROCHEAP_ALLOCCLEAR);

    return pvRet;
}

//+------------------------------------------------------------------------
//
//  Function:   _MemFree
//
//  Synopsis:   Free a block of memory allocated with MemAlloc,
//              MemAllocFree or MemRealloc.
//
//  Arguments:  [pv] - Pointer to block to free.  A value of zero is
//              is ignored.
//
//-------------------------------------------------------------------------

void
_MemFree(void *pv)
{
    // The null check is required for HeapFree.
    if (pv == NULL)
        return;

#ifdef VMEM
    if (IsVMemEnabled())
    {
        VMemFree(pv);
        return;
    }
#endif

    Assert(g_hProcessHeap);

#if DBG == 1
    pv = DbgPreFree(pv);
#endif

#if SMALLBLOCKHEAP
#if DBG==1
    if (IsSmallBlockHeapEnabled())
#endif
    {
        __sbh_region_t *preg;
        __sbh_page_t *  ppage;
        __map_t *       pmap;

        CHECKSBH;

        EnterCriticalSection(&g_csHeap);

        HeapBegTimer(SWITCHES_TIMER_SBHEAP_FREE);

        if ( (pmap = __sbh_find_block(pv, &preg, &ppage)) != NULL ) {
            Assert(*pmap != 0);
            __sbh_free_block(preg, ppage, pmap);

            HeapEndTimer(SWITCHES_TIMER_SBHEAP_FREE);

            LeaveCriticalSection(&g_csHeap);
            DbgPostFree();

            return;
        }

        HeapEndTimer(SWITCHES_TIMER_SBHEAP_FREE);

        LeaveCriticalSection(&g_csHeap);

    }
#endif

    HeapBegTimer(SWITCHES_TIMER_PROCHEAP_FREE);

    HeapFree(g_hProcessHeap, 0, pv);

    HeapEndTimer(SWITCHES_TIMER_PROCHEAP_FREE);

    DbgPostFree();
}

//+------------------------------------------------------------------------
//  Function:   _MemRealloc
//
//  Synopsis:   Change the size of an existing block of memory, allocate a
//              block of memory, or free a block of memory depending on the
//              arguments.
//
//              If cb is zero, this function always frees the block of memory
//              and *ppv is set to zero.
//
//              If cb is not zero and *ppv is zero, then this function allocates
//              cb bytes.
//
//              If cb is not zero and *ppv is non-zero, then this function
//              changes the size of the block, possibly by moving it.
//
//              On error, *ppv is left unchanged.  The block contents remains
//              unchanged up to the smaller of the new and old sizes.  The
//              contents of the block beyond the old size is undefined.
//              The returned pointer is guaranteed to be suitably aligned for
//              storage of any object type.
//
//              The signature of this function is different than thy typical
//              realloc-like function to avoid the following common error:
//                  pv = realloc(pv, cb);
//              If realloc fails, then null is returned and the pointer to the
//              original block of memory is leaked.
//
//  Arguments:  [cb] - Requested size in bytes. A value of zero always frees
//                  the block.
//              [ppv] - On input, pointer to existing block pointer or null.
//                  On output, pointer to new block pointer.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
_MemRealloc(void **ppv, ULONG cb)
{
    void *  pv;

#ifdef VMEM
    if (IsVMemEnabled())
    {
        return(VMemRealloc(ppv, cb, g_dwFlagsVMem));
    }
#endif

    Assert(g_hProcessHeap);

    if (cb == 0)
    {
        _MemFree(*ppv);
        *ppv = 0;
    }
    else if (*ppv == NULL)
    {
        *ppv = _MemAlloc(cb);
        if (*ppv == NULL)
            return E_OUTOFMEMORY;
    }
    else
    {
    #if DBG == 1
    	#if SMALLBLOCKHEAP
    	    ULONG cbReq = cb;
    	#endif
        cb = DbgPreRealloc(*ppv, cb, &pv);
    #else
        pv = *ppv;
    #endif

    #if SMALLBLOCKHEAP
    #if DBG==1
        if (IsSmallBlockHeapEnabled())
    #endif
        {
            __sbh_region_t *preg;
            __sbh_page_t *  ppage;
            __map_t *       pmap;
            ULONG           cbr;
            void *          pvNew;

            cbr = (cb + _PARASIZE - 1) & ~(_PARASIZE - 1);

            CHECKSBH;

            EnterCriticalSection(&g_csHeap);

            HeapBegTimer(SWITCHES_TIMER_SBHEAP_REALLOC);

            if ( (pmap = __sbh_find_block(pv, &preg, &ppage)) != NULL )
            {
                Assert(*pmap != 0);

                pvNew = NULL;
                /*
                 * If the new size falls below __sbh_threshold, try to
                 * carry out the reallocation within the small-block
                 * heap.
                 */
                if ( cbr < __sbh_threshold ) {
                    if ( __sbh_resize_block(preg, ppage, pmap, cbr >> _PARASHIFT))
                    {
                        pvNew = pv;
                    }
                    else if ((pvNew = __sbh_alloc_block(cbr >> _PARASHIFT)) != NULL)
                    {
                        ULONG cbOld = ((size_t)(*pmap)) << _PARASHIFT;
                        memcpy(pvNew, pv, min(cbOld, cb));
                        __sbh_free_block(preg, ppage, pmap);
                    }
                }

                HeapEndTimer(SWITCHES_TIMER_SBHEAP_REALLOC);

                /*
                 * If the reallocation has not been (successfully)
                 * performed in the small-block heap, try to allocate a
                 * new block with HeapAlloc.
                 */
                HeapBegTimer(SWITCHES_TIMER_PROCHEAP_REALLOC);

                if ((pvNew == NULL) && ((pvNew = HeapAlloc(g_hProcessHeap, 0, cb)) != NULL))
                {
                    ULONG cbOld = ((size_t)(*pmap)) << _PARASHIFT;
                    memcpy(pvNew, pv, min(cbOld, cb));

                    HeapEndTimer(SWITCHES_TIMER_PROCHEAP_REALLOC);

                    __sbh_free_block(preg, ppage, pmap);
                }

                HeapEndTimer(SWITCHES_TIMER_PROCHEAP_REALLOC);

                LeaveCriticalSection(&g_csHeap);
                pv = DbgPostRealloc(pvNew);
                if (pv == NULL)
                {
                    return E_OUTOFMEMORY;
                }
                else
                {
#if DBG ==1
                    Assert(_MemGetSize(pv) >= cbReq);
#endif
                    *ppv = pv;
                    return S_OK;
                }
            }
            else
            {
                HeapEndTimer(SWITCHES_TIMER_SBHEAP_REALLOC);

                LeaveCriticalSection(&g_csHeap);
            }
        }

    #endif

        HeapBegTimer(SWITCHES_TIMER_PROCHEAP_REALLOC);

        void *pvTemp = DbgPostRealloc(HeapReAlloc(g_hProcessHeap, 0, pv, cb));

        HeapEndTimer(SWITCHES_TIMER_PROCHEAP_REALLOC);

        if (pvTemp == NULL)
            return E_OUTOFMEMORY;

        // if HeapReAlloc fails and returns NULL, *ppv is supposed to remain unchanged,
        // so only set it after checking if the realloc succeeds
        *ppv = pvTemp;

    }

    return S_OK;
}

#if DBG==1

char *
_MemGetName(void * pv)
{
    return(DbgExMemGetName(pv));
}

void __cdecl
_MemSetName(void * pv, char * szFmt, ...)
{
    if (!IsVMemEnabled())
    {
        char szBuf[1024];
        va_list va;

        va_start(va, szFmt);
        wvsprintfA(szBuf, szFmt, va);
        va_end(va);

        DbgExMemSetName(pv, "%s", szBuf);
    }
}

void
_MemSetHeader(void * pv, ULONG cb, PERFMETERTAG mt)
{
#ifdef PERFMETER
    if (!IsVMemEnabled())
    {
        DbgExMemSetHeader(pv, cb, mt);
    }
#endif
}

#else

#define _MemSetHeader(pv, cb, mt)

#endif

// Metered Memory -------------------------------------------------------------

#ifdef PERFMETER

struct MTMEMBLK
{
    PERFMETERTAG    mt;
    ULONG           cb;
};

void *
_MtMemAlloc(PERFMETERTAG mt, ULONG cb)
{
    if (MtSimulateOutOfMemory(mt, -1))
    {
        return NULL;
    }

    MTMEMBLK * pmb = (MTMEMBLK *)_MemAlloc(sizeof(MTMEMBLK) + cb);

    if (pmb)
    {
        _MemSetHeader(pmb, sizeof(MTMEMBLK), mt);
        pmb->mt = mt;
        pmb->cb = cb;
        MtAdd(mt, 1, (LONG)cb);
        return(pmb + 1);
    }
    else
    {
        return(NULL);
    }
}

void *
_MtMemAllocClear(PERFMETERTAG mt, ULONG cb)
{
    if (MtSimulateOutOfMemory(mt, -1))
    {
        return NULL;
    }

    MTMEMBLK * pmb = (MTMEMBLK *)_MemAllocClear(sizeof(MTMEMBLK) + cb);

    if (pmb)
    {
        _MemSetHeader(pmb, sizeof(MTMEMBLK), mt);
        pmb->mt = mt;
        pmb->cb = cb;
        MtAdd(mt, 1, (LONG)cb);
        return(pmb + 1);
    }
    else
    {
        return(NULL);
    }
}

HRESULT
_MtMemRealloc(PERFMETERTAG mt, void ** ppv, ULONG cb)
{
    if (cb == 0)
    {
        _MtMemFree(*ppv);
        *ppv = 0;
        return(S_OK);
    }

    if (*ppv == NULL)
    {
        *ppv = _MtMemAlloc(mt, cb);
        return(*ppv ? S_OK : E_OUTOFMEMORY);
    }

    MTMEMBLK *  pmb = (MTMEMBLK *)*ppv - 1;
    HRESULT     hr = _MemRealloc((void **)&pmb, sizeof(MTMEMBLK) + cb);

    if (hr == S_OK)
    {
        _MemSetHeader(pmb, sizeof(MTMEMBLK), mt);

        if (pmb->mt == mt)
        {
            MtAdd(mt, 0, (LONG)cb - (LONG)pmb->cb);
        }
        else
        {
            MtAdd(pmb->mt, -1, -(LONG)pmb->cb);
            pmb->mt = mt;
            MtAdd(pmb->mt, +1, +(LONG)cb);
        }

        pmb->cb = cb;

        *ppv = pmb + 1;
    }

    return(hr);
}

ULONG
_MtMemGetSize(void * pv)
{
    if (pv == NULL)
        return(0);
    else
        return(_MemGetSize((MTMEMBLK *)pv - 1) - sizeof(MTMEMBLK));
}

void
_MtMemFree(void * pv)
{
    if (pv)
    {
        MTMEMBLK * pmb = (MTMEMBLK *)pv - 1;
        MtAdd(pmb->mt, -1, -(LONG)pmb->cb);
        _MemFree(pmb);
    }
}

HRESULT
_MtMemAllocString(PERFMETERTAG mt, LPCTSTR pchSrc, LPTSTR * ppchDst)
{
    TCHAR *pch;
    size_t cb;

    cb = (_tcsclen(pchSrc) + 1) * sizeof(TCHAR);
    *ppchDst = pch = (TCHAR *)_MtMemAlloc(mt, cb);
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        return S_OK;
    }
}

HRESULT
_MtMemAllocString(PERFMETERTAG mt, ULONG cch, const TCHAR *pchSrc, TCHAR **ppchDest)
{
    TCHAR *pch;
    size_t cb = cch * sizeof(TCHAR);

    *ppchDest = pch = (TCHAR *)MemAlloc(mt, cb + sizeof(TCHAR));
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        pch[cch] = 0;
        return S_OK;
    }
}

HRESULT
_MtMemReplaceString(PERFMETERTAG mt, const TCHAR *pchSrc, TCHAR **ppchDest)
{
    HRESULT hr;
    TCHAR *pch;

    if (pchSrc)
    {
        hr = THR(_MtMemAllocString(mt, pchSrc, &pch));
        if (hr)
            RRETURN(hr);
    }
    else
    {
        pch = NULL;
    }

    _MtMemFreeString(*ppchDest);
    *ppchDest = pch;

    return S_OK;
}

#if DBG==1

char *
_MtMemGetName(void * pv)
{
    return(DbgExMemGetName((MTMEMBLK *)pv - 1));
}

void __cdecl
_MtMemSetName(void * pv, char * szFmt, ...)
{
    if (pv && !IsVMemEnabled())
    {
        char szBuf[1024];
        va_list va;

        va_start(va, szFmt);
        wvsprintfA(szBuf, szFmt, va);
        va_end(va);

        DbgExMemSetName((MTMEMBLK *)pv - 1, "%s", szBuf);
    }
}

#endif

int
_MtMemGetMeter(void * pv)
{
    return(((MTMEMBLK *)pv - 1)->mt);
}

void
_MtMemSetMeter(void * pv, PERFMETERTAG mt)
{
    if (pv)
    {
        MTMEMBLK * pmb = (MTMEMBLK *)pv - 1;

        if (pmb->mt != mt)
        {
            MtAdd(pmb->mt, -1, -(LONG)pmb->cb);
            pmb->mt = mt;
            MtAdd(pmb->mt, +1, +(LONG)pmb->cb);
            _MemSetHeader(pmb, sizeof(MTMEMBLK), mt);
        }
    }
}

#endif

#ifdef PERFMETER

DeclareTag(tagUseMeterOpNew, "!Memory", "Trap invalid operator new calls")

void * __cdecl UseOperatorNewWithMemoryMeterInstead(size_t cb)
{
    AssertSz(!IsTagEnabled(tagUseMeterOpNew), "Invalid use of global operator new.  "
        "Use the version which requires a meter tag instead.");

    return(MemAlloc(Mt(OpNew), cb));
}

#endif

// MEMGUARD -------------------------------------------------------------------

#if defined(MEMGUARD)

#define MGGUARDDATA 0xF0F0BAAD

struct MGGUARD
{
    MGGUARD *pNext;
    DWORD    dw;
};

MGGUARD * g_pMemList = NULL;

void
_MgMemValidate()
{
    EnterCriticalSection(&g_csHeap);

    MGGUARD *pg = g_pMemList;

    while (pg)
    {
        if (pg->dw != MGGUARDDATA)
        {
            DebugBreak();
        }

        pg = pg->pNext;
    }

    LeaveCriticalSection(&g_csHeap);
}

void
_MgRemove(MGGUARD *pmg)
{
    if (!pmg)
        return;

    EnterCriticalSection(&g_csHeap);

    MGGUARD *pg = g_pMemList;

    if (pmg == pg)
    {
        g_pMemList = pg->pNext;
        goto Cleanup;
    }

    while (pg)
    {
        if (pg->pNext == pmg)
        {
            pg->pNext = pg->pNext->pNext;
            break;
        }

        pg = pg->pNext;
    }

Cleanup:
    LeaveCriticalSection(&g_csHeap);

}

void
_MgAdd(MGGUARD *pmg)
{
    EnterCriticalSection(&g_csHeap);

    pmg->pNext = g_pMemList;
    g_pMemList = pmg;

    LeaveCriticalSection(&g_csHeap);
}

void *
_MgMemAlloc(ULONG cb)
{
    _MgMemValidate();

    MGGUARD * pmg = (MGGUARD *)_MemAlloc(sizeof(MGGUARD) + cb);

    if (pmg)
    {
        pmg->dw = MGGUARDDATA;

        _MgAdd(pmg);

        return(pmg + 1);
    }
    else
    {
        return(NULL);
    }

}

void *
_MgMemAllocClear(ULONG cb)
{
    _MgMemValidate();

    MGGUARD * pmg = (MGGUARD *)_MemAllocClear(sizeof(MGGUARD) + cb);

    if (pmg)
    {
        pmg->dw = MGGUARDDATA;

        _MgAdd(pmg);

        return(pmg + 1);
    }
    else
    {
        return(NULL);
    }
}

HRESULT
_MgMemRealloc(void ** ppv, ULONG cb)
{
    _MgMemValidate();

    if (cb == 0)
    {
        _MgMemFree(*ppv);
        *ppv = 0;
        return(S_OK);
    }

    if (*ppv == NULL)
    {
        *ppv = _MgMemAlloc(cb);
        return(*ppv ? S_OK : E_OUTOFMEMORY);
    }

    MGGUARD *  pmg = (MGGUARD *)*ppv - 1;

    _MgRemove(pmg);

    HRESULT    hr = _MemRealloc((void **)&pmg, sizeof(MGGUARD) + cb);

    if (hr == S_OK)
    {
        pmg->dw = MGGUARDDATA;

        _MgAdd(pmg);

        *ppv = pmg + 1;
    }

    return(hr);
}

ULONG
_MgMemGetSize(void * pv)
{
    _MgMemValidate();

    if (pv == NULL)
        return(0);
    else
        return(_MemGetSize((MGGUARD *)pv - 1) - sizeof(MGGUARD));
}

void
_MgMemFree(void * pv)
{
    _MgMemValidate();

    if (pv)
    {
        MGGUARD * pmg = (MGGUARD *)pv - 1;
        if (pmg->dw != MGGUARDDATA)
        {
            // The memory guard DWORD was overwritten! Bogus!
#ifdef _M_IX86
            _asm int 3  // To get a proper stacktrace.
#else
            DebugBreak();
#endif
        }
        _MgRemove(pmg);

        _MemFree(pmg);
    }
}

HRESULT
_MgMemAllocString(LPCTSTR pchSrc, LPTSTR * ppchDst)
{
    TCHAR *pch;
    size_t cb;

    cb = (_tcsclen(pchSrc) + 1) * sizeof(TCHAR);
    *ppchDst = pch = (TCHAR *)_MgMemAlloc(cb);
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        return S_OK;
    }
}

HRESULT
_MgMemAllocString(ULONG cch, const TCHAR *pchSrc, TCHAR **ppchDest)
{
    TCHAR *pch;
    size_t cb = cch * sizeof(TCHAR);

    *ppchDest = pch = (TCHAR *)_MgMemAlloc(cb + sizeof(TCHAR));
    if (!pch)
        return E_OUTOFMEMORY;
    else
    {
        memcpy(pch, pchSrc, cb);
        pch[cch] = 0;
        return S_OK;
    }
}

HRESULT
_MgMemReplaceString(const TCHAR *pchSrc, TCHAR **ppchDest)
{
    HRESULT hr;
    TCHAR *pch;

    if (pchSrc)
    {
        hr = THR(_MgMemAllocString(pchSrc, &pch));
        if (hr)
            RRETURN(hr);
    }
    else
    {
        pch = NULL;
    }

    _MgMemFreeString(*ppchDest);
    *ppchDest = pch;

    return S_OK;
}

#endif // MEMGUARD
