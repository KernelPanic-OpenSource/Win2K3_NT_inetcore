#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include "olesite.hxx"
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

ExternTag(tagOleSiteClient);

MtDefine(CProgressBindStatusCallback, Dwn, "CProgressBindStatusCallback")

//+---------------------------------------------------------------------------
//
//  Member:     CLock::CLock
//
//  Synopsis:   ctor/dtor
//
//----------------------------------------------------------------------------

CProgressBindStatusCallback::CLock::CLock(CProgressBindStatusCallback *pBSC)
{
    _pBSC = pBSC;
    _pBSC->AddRef();
}


CProgressBindStatusCallback::CLock::~CLock()
{
    _pBSC->Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::CProgressBindStatusCallback
//
//  Synopsis:   ctor
//
//----------------------------------------------------------------------------

CProgressBindStatusCallback::CProgressBindStatusCallback()
{
    _ulRefs = 1;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::~CProgressBindStatusCallback
//
//  Synopsis:   dtor
//
//----------------------------------------------------------------------------

CProgressBindStatusCallback::~CProgressBindStatusCallback()
{
    if (_dwProgCookie)
    {
        Assert( _pProgSink );
        _pProgSink->DelProgress(_dwProgCookie);
    }

    if (_pMarkup)
    {
        _pMarkup->SubRelease();
    }
    ReleaseInterface(_pProgSink);
    ReleaseInterface(_pBSCChain);
    ReleaseInterface(_pBCtx);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::Init
//
//  Synopsis:   initializer
//
//----------------------------------------------------------------------------

HRESULT
CProgressBindStatusCallback::Init(
    CMarkup *               pMarkup, 
    DWORD                   dwCompatFlags,
    IBindStatusCallback *   pBSCChain,
    IBindCtx *              pBCtx)
{
    HRESULT hr = S_OK;

    Assert( pMarkup );
    if (!pMarkup->GetDwnDoc())
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    _pMarkup = pMarkup;
    _pMarkup->SubAddRef();
    
    _dwCompatFlags = dwCompatFlags;
    _dwBindf = pMarkup->GetDwnDoc()->GetBindf();
    
    ReplaceInterface(&_pBCtx, pBCtx);
    ReplaceInterface(&_pBSCChain, pBSCChain);
    
    _pProgSink = CMarkup::GetProgSinkHelper(pMarkup);
    if( _pProgSink )
    {
        _pProgSink->AddRef();
    
        //
        // If we failed to add this to the progress loader, it's ok to
        // continue loading.
        //
    
        IGNORE_HR(_pProgSink->AddProgress(PROGSINK_CLASS_OTHER, &_dwProgCookie));
    }

Cleanup:
    RRETURN( hr );
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::Terminate
//
//  Synopsis:   Kills the download.
//
//----------------------------------------------------------------------------

void
CProgressBindStatusCallback::Terminate()
{
    CLock   Lock(this);

    if (_pBinding)
    {
        _pBinding->Abort();
    }
    ClearInterface(&_pBinding);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::QueryInterface
//
//  Synopsis:   per IUnknown
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;
    
    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
    {
        *ppv = (IBindStatusCallback *)this;
    }
    else if (riid == IID_IServiceProvider)
    {
        *ppv = (IServiceProvider *)this;        
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    
    RRETURN(E_NOINTERFACE);
}


STDMETHODIMP
CProgressBindStatusCallback::QueryService(REFGUID guidService, REFIID iid, void ** ppv)
{
    HRESULT hr = E_NOINTERFACE;

    *ppv = NULL;
    IServiceProvider *pServiceProvider = NULL;

    // The CProgressBindStatusCallback doesn't have any interesting service to
    // offer here (yet), but our client might.
    if (_pBSCChain)
    {
        hr = _pBSCChain->QueryInterface(IID_IServiceProvider,
                                        (void **)&pServiceProvider);
        if (S_OK == hr)
        {
            hr = pServiceProvider->QueryService(guidService, iid, ppv);
        }
        ReleaseInterface(pServiceProvider);
    }

    if( FAILED( hr ) && _pMarkup)
    {
        CDocument * pDocument;

        // Try the CDocument and the CDoc.  
        // This is here specifically so that javascript:
        // URLs will work from plugins.  This also provides handy
        // connectivity that may be useful elsewhere.
        // Since CDocument delegates to CDoc if it does not handle the
        // guidService, we don't have to have additional logic here.

        hr = _pMarkup->EnsureDocument(&pDocument);
        if (hr)
            goto Cleanup;

        Assert(pDocument);

        hr = pDocument->QueryService(guidService, iid, ppv);
    }

Cleanup:
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnStartBinding
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnStartBinding(
    DWORD dwReserved, 
    IBinding *pBinding)
{
    ReplaceInterface(&_pBinding, pBinding);

    IGNORE_HR(pBinding->SetPriority(THREAD_PRIORITY_BELOW_NORMAL));

    RRETURN(_pBSCChain ? 
        THR(_pBSCChain->OnStartBinding(dwReserved, pBinding)) :
        S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::GetPriority
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP        
CProgressBindStatusCallback::GetPriority(LONG *pnPriority)
{
    RRETURN(_pBSCChain ? 
        THR(_pBSCChain->GetPriority(pnPriority)) :
        S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnLowResource
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    RRETURN(_pBSCChain ? 
        THR(_pBSCChain->OnLowResource(dwReserved)) :
        S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnProgress
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnProgress(
    ULONG   ulProgress,
    ULONG   ulProgressMax,
    ULONG   ulStatusCode,
    LPCWSTR szStatusText)
{
    HRESULT hr = S_OK;
    
    if ((ulStatusCode == BINDSTATUS_REDIRECTING) &&  
        (_dwCompatFlags & COMPAT_SECURITYCHECKONREDIRECT) &&
        _pMarkup && !_pMarkup->AccessAllowed(szStatusText))
    {
        static const IID IID_IAmTheTDC = {0x3050f6c2,0x98b5,0x11cf,{0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
        IUnknown *pTDC;
        if (_pBSCChain && S_OK == _pBSCChain->QueryInterface(IID_IAmTheTDC, (void**)&pTDC))
        {
            // bug 85290 - TDC control handles its own cross-domain security,
            // so downloads on its behalf should not be aborted
            ReleaseInterface(pTDC);
        }
        else
        {
            if (_pBinding)
                _pBinding->Abort();
            _fAbort = TRUE;
        }
    }

    if (_dwProgCookie && ulStatusCode == BINDSTATUS_DOWNLOADINGDATA)
    {
        Assert( _pProgSink );
        hr = THR(_pProgSink->SetProgress( 
                _dwProgCookie, 
                PROGSINK_SET_STATE | PROGSINK_SET_POS | PROGSINK_SET_MAX | PROGSINK_SET_TEXT,
                PROGSINK_STATE_LOADING, 
                szStatusText,
                0,
                ulProgress, 
                ulProgressMax));
        if (hr)
            goto Cleanup;
    }

    if (_pBSCChain)
    {
        hr = THR(_pBSCChain->OnProgress(
                ulProgress, 
                ulProgressMax, 
                ulStatusCode, 
                szStatusText));
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnStopBinding
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    HRESULT hr;
    
    CLock   Lock(this);

    ClearInterface(&_pBinding);
    if (_pMarkup && _pMarkup->Doc())
    {
        _pMarkup->Doc()->_aryChildDownloads.DeleteByValue(this);
    }
    
    hr = _pBSCChain ? 
        THR(_pBSCChain->OnStopBinding(hresult, szError)) :
        S_OK;
    if (hr)
        goto Cleanup;

    hr = THR(RevokeBindStatusCallback(_pBCtx, this));
    if (hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::GetBindInfo
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::GetBindInfo(DWORD *grfBINDF, BINDINFO *pbindinfo)
{
    HRESULT     hr = S_OK;
    
    if (!pbindinfo)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    // Prime the BINDF flags from our Doc's bind ctx flags.  This informs the 
    // object what BINDF we are using.
    *grfBINDF |= _dwBindf; 

    if (_pBSCChain)
    {
        hr = THR(_pBSCChain->GetBindInfo(grfBINDF,pbindinfo));
        if (hr)
            goto Cleanup;
            
        if (_dwCompatFlags & COMPAT_NO_BINDF_OFFLINEOPERATION)
        {
            //
            // The powerpoint animation viewer always passes in offline 
            // operation.  Obviously this will fail if the requested file
            // is not available in the cache.
            //
            
            *grfBINDF &= ~BINDF_OFFLINEOPERATION;
        }

        // Reapply our BINDF flags just in case they cleared any out.
        *grfBINDF |= _dwBindf; 
    
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnDataAvailable
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnDataAvailable(
    DWORD grfBSCF,
    DWORD dwSize,
    FORMATETC *pformatetc,
    STGMEDIUM *pstgmed)
{
    RRETURN1(_pBSCChain && !_fAbort ? 
        THR(_pBSCChain->OnDataAvailable(grfBSCF, dwSize, pformatetc, pstgmed)) :
        S_OK, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressBindStatusCallback::OnObjectAvailable
//
//  Synopsis:   per IBindStatusCallback
//
//----------------------------------------------------------------------------

STDMETHODIMP
CProgressBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    RRETURN(_pBSCChain ? 
        THR(_pBSCChain->OnObjectAvailable(riid, punk)) :
        S_OK);
}



//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::CreateMoniker, IBindHost
//
//  Synopsis:   Parses display name and returns a URL moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::CreateMoniker(LPOLESTR szName, IBindCtx * pbc, IMoniker ** ppmk, DWORD dwReserved)
{
    if (MyOleSite()->IllegalSiteCall(VALIDATE_ATTACHED|VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);

    TCHAR       cBuf[pdlUrlLen];
    TCHAR *     pchUrl = cBuf;
    HRESULT     hr;

    hr = THR(CMarkup::ExpandUrl(
            MyOleSite()->IsInMarkup() ? MyOleSite()->GetMarkup()->GetFrameOrPrimaryMarkup() : NULL,
                szName, ARRAY_SIZE(cBuf), pchUrl, MyOleSite()));
    if (hr)
        goto Cleanup;

    TraceTag((tagOleSiteClient, 
        "COleSite::CClient::CreateMoniker SSN=0x%x url=%ls", 
        MyOleSite()->_ulSSN,
        pchUrl));

    hr = THR(CreateURLMoniker(NULL, pchUrl, ppmk));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   MonikerBind
//
//  Synopsis:   Helper routine contains the common code for BindToObject() and
//              BindToStorage().
//
//----------------------------------------------------------------------------

HRESULT
MonikerBind(
    CMarkup *               pMarkup,
    IMoniker *              pmk, 
    IBindCtx *              pbc,
    IBindStatusCallback *   pbsc, 
    REFIID                  riid, 
    void **                 ppv,
    BOOL                    fObject,
    DWORD                   dwCompatFlags)
{
    IBindCtx *                      pbcNew = NULL;
    CProgressBindStatusCallback *   pBSCWrap = NULL;
    HRESULT                         hr = S_OK;

    Assert( pMarkup );

    if (!pbc)
    {
        hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pbcNew, 0));
        if (hr)
            goto Cleanup;

        pbc = pbcNew;
    }

    //
    // Create a wrapper bsc which will update our status bar appropriately.
    //
    
    pBSCWrap = new CProgressBindStatusCallback;
    if (!pBSCWrap)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pBSCWrap->Init(
            pMarkup, 
            dwCompatFlags,
            pbsc,
            pbc));
    if (hr)
        goto Cleanup;

    //
    // Register our wrapper bsc into the bind ctx.  This addrefs
    // the bscwrap.  
    //

    hr = THR(RegisterBindStatusCallback(pbc, pBSCWrap, NULL, NULL));
    if (!OK(hr))
        goto Cleanup;
    
    // Add pBSCWrap to the _aryChildDownloads.  If the bind
    // ends up happening synchronous, then it will be removed
    // further down the stack.
    IGNORE_HR(pMarkup->Doc()->_aryChildDownloads.Append(pBSCWrap));

    hr = fObject ? 
        THR(pmk->BindToObject(pbc, NULL, riid, ppv)) :
        THR(pmk->BindToStorage(pbc, NULL, riid, ppv));
    if (!OK(hr))
        goto Cleanup;
        
Cleanup:
    ReleaseInterface(pbcNew);

    if (hr != S_ASYNCHRONOUS)
    {
        pMarkup->Doc()->_aryChildDownloads.DeleteByValue(pBSCWrap);
    }
    
    if (pBSCWrap)
    {
        pBSCWrap->Release();
    }
    RRETURN1(hr, S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::MonikerBindToStorage, IBindHost
//
//  Synopsis:   Calls BindToStorage on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::MonikerBindToStorage(
    IMoniker * pmk, 
    IBindCtx * pbc,
    IBindStatusCallback * pbsc, 
    REFIID riid, 
    void ** ppvObj)
{
    INSTANTCLASSINFO * pici;
    
    TraceTag((tagOleSiteClient, "COleSite::CClient::MonikerBindToStorage SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(
            COleSite::VALIDATE_ATTACHED|COleSite::VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);

    pici = MyOleSite()->GetInstantClassInfo();
    
    RRETURN1(MonikerBind(
        MyOleSite()->GetWindowedMarkupContext(), 
        pmk, 
        pbc, 
        pbsc, 
        riid, 
        ppvObj, 
        FALSE,
        pici ? pici->dwCompatFlags : 0), S_ASYNCHRONOUS);
}


//+---------------------------------------------------------------------------
//
//  Member:     COleSite::CClient::MonikerBindToObject, IBindHost
//
//  Synopsis:   Calls BindToObject on the moniker
//
//----------------------------------------------------------------------------

STDMETHODIMP
COleSite::CClient::MonikerBindToObject(
    IMoniker * pmk, 
    IBindCtx * pbc,
    IBindStatusCallback * pbsc, 
    REFIID riid, 
    void ** ppvObj)
{
    INSTANTCLASSINFO * pici;
    
    TraceTag((tagOleSiteClient, "COleSite::CClient::MonikerBindToObject SSN=0x%x", MyOleSite()->_ulSSN));

    if (MyOleSite()->IllegalSiteCall(
            COleSite::VALIDATE_ATTACHED|COleSite::VALIDATE_DOC_ALIVE))
        RRETURN(E_UNEXPECTED);

    pici = MyOleSite()->GetInstantClassInfo();
    
    RRETURN1(MonikerBind(
        MyOleSite()->GetWindowedMarkupContext(), 
        pmk, 
        pbc, 
        pbsc, 
        riid, 
        ppvObj, 
        TRUE,
        pici ? pici->dwCompatFlags : 0), S_ASYNCHRONOUS);
}

