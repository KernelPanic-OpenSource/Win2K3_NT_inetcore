//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       getid.cxx
//
//  Contents:   GetUniqueID service and usage
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_URLHIST_H_
#define X_URLHIST_H_
#include "urlhist.h"
#endif

#ifndef X_SHLGUID_H_
#define X_SHLGUID_H_
#include "shlguid.h"
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif

#ifndef X_OPTSHOLD_HXX_
#define X_OPTSHOLD_HXX_
#include "optshold.hxx"
#endif

#ifndef X_TIMER_HXX_
#define X_TIMER_HXX_
#include "timer.hxx"
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

#ifndef X_ACTIVDBG_H_
#define X_ACTIVDBG_H_
#include <activdbg.h>
#endif

#ifndef X_PEERXTAG_HXX_
#define X_PEERXTAG_HXX_
#include "peerxtag.hxx"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_PRIVACY_HXX_
#define X_PRIVACY_HXX_
#include "privacy.hxx"
#endif

#ifndef NO_DDRAW
#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include <ddraw.h>
#endif

#ifndef X_DDRAWEX_H_
#define X_DDRAWEX_H_
#include <ddrawex.h>
#endif
#endif // NO_DDRAW

#ifndef NO_SCRIPT_DEBUGGER
extern interface IDebugApplication *g_pDebugApp;
#endif // NO_SCRIPT_DEBUGGER

extern CGlobalCriticalSection    g_csOscCache;

extern HRESULT EnsureAccWindow( CWindow * pWindow );

extern "C" const GUID SID_SHTMLEditServices;

//
//  CDoc methods
//


DeclareTag(tagNoQS, "Form", "Obstruct all QueryService to container")
MtExtern(CEnumPrivacyRecords)


//+------------------------------------------------------------------------
//
//  Member:     CDoc::QueryService
//
//  Synopsis:   QueryService for the form.  Delegates to the form's
//              own site.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDoc::QueryService(
        REFGUID guidService,
        REFIID iid,
        void ** ppv)
{
    HRESULT             hr;

#if DBG == 1
    if (IsTagEnabled(tagNoQS))
        hr = E_NOINTERFACE;
    else
#endif
    //
    // Certain services should never be bubbled up through 
    // the client site
    // SID_SContainerDispatch   -   Always provide container's IDispatch
    // SID_SLocalRegistry       -   Never bubble due to security concerns.
    //                              License manager is per document.
    // SID_SBindHost            -   We provide the service but might also
    //                              forward some calls to an outer bindhost,
    //                              if any.
    //

    // TODO (alexz) these "ifs" are redundant to "ifs" inside CreateService
    if (SID_SContainerDispatch == guidService ||
        SID_SLocalRegistry == guidService ||
        SID_SBindHost == guidService ||
        CLSID_HTMLDocument == guidService ||
        IID_IDebugApplication == guidService ||
        IID_IInternetHostSecurityManager == guidService ||
        IID_IDocHostUIHandler == guidService ||
        SID_SHTMLProperyPageArg == guidService ||
        IID_IAccessible == guidService ||
        IID_ISelectionServices == guidService ||
        SID_SHTMLEditServices == guidService || 
        IID_IElementNamespaceTable== guidService ||
        IID_IEnumPrivacyRecords == guidService ||
        IID_IPrivacyServices == guidService)
    {
        hr = THR_NOTRACE(CreateService(guidService, iid, ppv));
    }
    else
    {
        hr = THR_NOTRACE(CServer::QueryService(guidService, iid, ppv));
        if (hr)
        {
            hr = THR_NOTRACE(CreateService(guidService, iid, ppv));
        }
    }
    
    RRETURN_NOTRACE(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CDoc::CreateService, public
//
//  Synopsis:   Creates the requested service. Only called if our container
//              does not provide the service.
//
//  Arguments:  [guidService] -- Service being asked for.
//              [iid]         -- IID needed on that service.
//              [ppv]         -- Place to put pointer.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CDoc::CreateService(REFGUID guidService, REFIID iid, LPVOID * ppv)
{
    HRESULT hr;

    *ppv = NULL;

#ifndef NO_EDIT
    if (guidService == SID_SOleUndoManager)
    {
        hr = CreateUndoManager();
        if (hr)
            goto Cleanup;

        hr = THR_NOTRACE(_pUndoMgr->QueryInterface(iid, ppv));
    }
    else 
#endif // NO_EDIT
#ifndef NO_DDRAW
    if (guidService == SID_SDirectDraw3)
    {
        extern HRESULT InitSurface();
        extern IDirectDraw* g_pDirectDraw;
        LOCK_SECTION(g_csOscCache);
        hr = InitSurface();
        if (SUCCEEDED(hr))
            hr = THR_NOTRACE(g_pDirectDraw->QueryInterface(iid, ppv));
    } 
    else 
#endif //ndef NO_DDRAW
    if (guidService == SID_STimerService)
    {
        CTimerMan *pTimerMan;
        hr = THR(GetTimerManager(&pTimerMan));
        if (SUCCEEDED(hr))
        {
            hr = THR_NOTRACE(pTimerMan->QueryInterface(iid, ppv));

            if (SUCCEEDED(hr))
            {
                if (!_pTimerDraw)
                {
                    // Init NAMEDTIMER_DRAW which gets frozen for controls during paints.
                    // Not a problem if this fails as the results are not catastrophic.

                    IGNORE_HR(pTimerMan->GetNamedTimer(NAMEDTIMER_DRAW, &_pTimerDraw));
                }
                
                pTimerMan->Release();
            }
        }
    }
    else if (guidService == SID_SContainerDispatch)
    {
        hr = THR_NOTRACE(QueryInterface(iid, ppv));
    }
    else if (guidService == SID_SLocalRegistry)
    {
        //
        // Respond with license manager if one is available.
        //
        
        if (_pWindowPrimary->Window()->_pLicenseMgr)
        {
            hr = THR(_pWindowPrimary->Window()->_pLicenseMgr->QueryInterface(iid, ppv));
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }
    else if (guidService == SID_SUrlHistory) 
    {
        // Our container is not shdocvw --- we need to handle geting the
        // history container ourself. It would be nice to be able to
        // assert that shdocvw is not the host though to make sure we 
        // are not in the situation where we have a problem and don't
        // detect it.

        hr = THR(CoCreateInstance(CLSID_CUrlHistory, NULL,  CLSCTX_INPROC_SERVER, iid, ppv));
    }
    else if (   guidService == CLSID_HTMLDocument )
    {
        hr = THR(PrivateQueryInterface(iid, ppv));
    }
    else if ( guidService == IID_IInternetHostSecurityManager )
    {
        // delegate to the top level document object.

        Assert(_pWindowPrimary->Document());

        hr = THR(_pWindowPrimary->Document()->QueryInterface(iid, ppv));

    }
#ifndef NO_SCRIPT_DEBUGGER
    else if ((IID_IDebugApplication == guidService) && g_pDebugApp)
    {
        hr = THR_NOTRACE(g_pDebugApp->QueryInterface (iid, ppv));
    }
#endif
#ifndef NO_PROPERTY_PAGE
    else if (guidService == SID_SHTMLProperyPageArg)
    {
        COptionsHolder *    pcoh = new COptionsHolder(GetCurrentWindow()->Window());

        hr = 
            THR(pcoh->put_anythingAfterFrameset( 
            (VARIANT_BOOL) _fTagsInFrameset));

        hr = THR_NOTRACE(pcoh->QueryInterface(iid,ppv));
        ReleaseInterface(pcoh);
    }
#endif
    else if (guidService == IID_IAuthenticate && iid == IID_IAuthenticate)
    {
        hr = THR(CreateTearOffThunk(this, s_apfnIAuthenticate, NULL, ppv));

        if (hr == S_OK)
        {
            ((IUnknown *)(*ppv))->AddRef();
        }
    }
    else if (guidService == IID_IWindowForBindingUI && iid == IID_IWindowForBindingUI)
    {
        hr = THR(CreateTearOffThunk(this, s_apfnIWindowForBindingUI, NULL, ppv));

        if (hr == S_OK)
        {
            ((IUnknown *)(*ppv))->AddRef();
        }
    }
    else if ( guidService == IID_IAccessible )
    {
        // delegate to the top level document's QueryService handling code.
        
        // Should we protect against a passivation situation ?? (FerhanE)
        Assert(_pWindowPrimary);

        hr = THR(_pWindowPrimary->Document()->QueryService(guidService, iid, ppv));
    }
    else if ( guidService == IID_IDocHostUIHandler )
    {
        if (!_pHostUIHandler)
        {
            hr = E_NOINTERFACE;
            goto Cleanup;
        }

        *ppv = _pHostUIHandler;
        _pHostUIHandler->AddRef();
        hr = S_OK;
    }    
    else if (guidService == IID_ISelectionServices && iid == IID_ISelectionServices )
    {
        AssertSz(FALSE, "QueryServices for IID_ISelectionServices no longer support");
        hr = E_INVALIDARG;
    }
    else if (guidService == SID_SHTMLEditServices && iid == IID_IHTMLEditServices )
    {
        IHTMLEditor *pEd = GetHTMLEditor(TRUE);

        if (pEd != NULL)
            hr = THR( pEd->QueryInterface( IID_IHTMLEditServices, (void**) ppv ));
        else
        {
            *ppv = NULL ;
            hr = E_FAIL;
            goto Cleanup;
        }    
    }    
    else if (IID_IElementNamespaceTable == guidService)
    {
        hr = THR(EnsureExtendedTagTableHost());
        if (hr)
            goto Cleanup;

        hr = THR(_pExtendedTagTableHost->QueryInterface(iid, ppv));
    }
    else if (IID_IEnumPrivacyRecords == guidService)
    {
        *ppv = (void*)new(Mt(CEnumPrivacyRecords)) CEnumPrivacyRecords(_pPrivacyList);
        if (!*ppv)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = S_OK;
        goto Cleanup;
    }
    else if (   guidService == IID_IPrivacyServices )
    {
        hr = THR(PrivateQueryInterface(iid, ppv));
    }
    else
    {
        hr = E_NOINTERFACE;
    }

#if DBG==1
    if (guidService != CLSID_HTMLDocument)
    {
        DbgTrackItf(iid, "CDoc::QS", FALSE, ppv);
    }
#endif

Cleanup:
    RRETURN_NOTRACE(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CDoc::GetUniqueIdentifier
//
//  Synopsis:   Gets a unique ID for the control.  If possible, the form
//              coordinates with its container through the IGetUniqueID
//              service to get an ID unique within the container.
//
//  Arguments:  [pstr]      The string to set into
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDoc::GetUniqueIdentifier(CStr *pstr)
{
    TCHAR   ach[64];
    HRESULT hr;

    memset(ach, 0, sizeof(ach));

    // Prefix with id_ because scriptlet code
    // doesn't currently like ID's that are all digits
    hr = THR(Format(0, ach, ARRAY_SIZE(ach), UNIQUE_NAME_PREFIX _T("<0d>"), (long)++_ID));
    if (hr)
        goto Cleanup;

    hr = THR(pstr->Set(ach));
    if (hr)
        goto Cleanup;
        
Cleanup:
    RRETURN(hr);
}


