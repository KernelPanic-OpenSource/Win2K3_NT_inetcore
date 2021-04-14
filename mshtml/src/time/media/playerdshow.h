/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: player.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _PLAYERDSHOW_H
#define _PLAYERDSHOW_H

#include "playerdshowbase.h"
#include "mixerocx.h"
#include <strmif.h>
#include <control.h>
#include <inc\qnetwork.h>
#include "dshowproxy.h"

#include "importman.h"

#define MP_INFINITY -1

/////////////////////////////////////////////////////////////////////////////
// CTTIMEPlayer

class CTIMEDshowPlayer :
    public CTIMEDshowBasePlayer,
    public ITIMEImportMedia,
    public ITIMEInternalEventSink,
    public IMixerOCXNotify
{
  public:
    CTIMEDshowPlayer(CTIMEDshowPlayerProxy * pProxy);
    virtual ~CTIMEDshowPlayer();

    HRESULT Init(CTIMEMediaElement *pelem,
                 LPOLESTR base,
                 LPOLESTR src,
                 LPOLESTR lpMimeType = NULL,
                 double dblClipBegin = -1.0,
                 double dblClipEnd = -1.0);//lint !e1735

  protected:
    HRESULT InitDshow();
    HRESULT InitElementSize();
    void DeinitDshow();
    HRESULT BuildGraph();
    HRESULT GetSpecificInterfaces();
    void ReleaseSpecificInterfaces();
    void FreeSpecificData();

    HRESULT BeginDownload();
    HRESULT GraphFinish();

    HRESULT ReadContentProperty(IGraphBuilder *pGraph, LPCWSTR lpcwstrTag, BSTR *pbstr);

    virtual void Block();
    virtual void UnBlock();
    virtual bool CanCallThrough();
    virtual bool FireProxyEvent(PLAYER_EVENT plEvent);

  public:
    HRESULT DetachFromHostElement (void);

    // IUnknown Methods
    STDMETHOD (QueryInterface)(REFIID refiid, void** ppunk)
        {   return _InternalQueryInterface(refiid, ppunk); };
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    virtual void OnTick(double dblSegmentTime,
                        LONG lCurrRepeatCount);

    HRESULT Reset();
    HRESULT Render(HDC hdc, LPRECT prc);

    HRESULT SetSrc(LPOLESTR base, LPOLESTR src);

    HRESULT SetSize(RECT *prect);

    HRESULT GetMediaLength(double &dblLength);
    HRESULT CanSeek(bool &fcanSeek);
    virtual PlayerState GetState();
    virtual HRESULT CanSeekToMarkers(bool &bacnSeekToM);
    virtual HRESULT IsBroadcast(bool &bisBroad);
    virtual HRESULT SetRate(double dblRate);
    virtual HRESULT GetRate(double &dblRate);
    virtual HRESULT HasMedia(bool &fHasMedia);
    virtual HRESULT HasVisual(bool &fHasVideo);
    virtual HRESULT HasAudio(bool &fHasAudio);

    HRESULT GetNaturalHeight(long *height);
    HRESULT GetNaturalWidth(long *width);

    virtual HRESULT GetAuthor(BSTR *pAuthor);
    virtual HRESULT GetTitle(BSTR *pTitle);
    virtual HRESULT GetCopyright(BSTR *pCopyright);
    virtual HRESULT GetAbstract(BSTR *pAbstract);
    virtual HRESULT GetRating(BSTR *pAbstract);

    virtual HRESULT GetIsStreamed(bool &fIsStreamed);
    virtual HRESULT GetBufferingProgress(double &dblBufferingProgress);
    virtual HRESULT GetHasDownloadProgress(bool &fHasDownloadProgress);

    BEGIN_COM_MAP(CTIMEDshowPlayer)
        COM_INTERFACE_ENTRY(ITIMEImportMedia)
        COM_INTERFACE_ENTRY_CHAIN(CTIMEDshowBasePlayer)
    END_COM_MAP_X();

    //
    // ITIMEImportMedia methods
    //
    STDMETHOD(CueMedia)();
    STDMETHOD(GetPriority)(double *);
    STDMETHOD(GetUniqueID)(long *);
    STDMETHOD(InitializeElementAfterDownload)();
    STDMETHOD(GetMediaDownloader)(ITIMEMediaDownloader ** ppImportMedia);
    STDMETHOD(PutMediaDownloader)(ITIMEMediaDownloader * pImportMedia);
    STDMETHOD(CanBeCued)(VARIANT_BOOL * pVB_CanCue);
    STDMETHOD(MediaDownloadError)();

    //
    // ITIMEInternalEventSink
    //
    STDMETHOD(InternalEvent)();
    
  protected:
    long    m_lSrc;
    bool m_fHasVideo;
    bool m_fDoneSetup;
    double m_dblSeekAtStart;
    bool m_fLoadError;
    bool m_fHasMedia;
    bool m_fRemoved;

  private:

    // IMixerOCXNotify methods
    STDMETHOD(OnInvalidateRect)(LPCRECT lpcRect);
    STDMETHOD(OnStatusChange)(ULONG ulStatusFlags);
    STDMETHOD(OnDataChange)(ULONG ulDataFlags);

    HRESULT SetMixerSize(RECT *prect);
    HRESULT SetUpHdc();

    void GraphStart(void);

    bool IsOvMConnected(IBaseFilter *pOvM);

    void SetStreamFlags(LPOLESTR src);
    HRESULT DisableAudioVideo();

    IMixerOCX *m_pIMixerOCX;
    DWORD m_nativeVideoWidth;
    DWORD m_nativeVideoHeight;
    DWORD m_displayVideoWidth;
    DWORD m_displayVideoHeight;

    LPSTREAM                    m_pTIMEMediaPlayerStream;

    bool m_fUsingInterfaces;
    bool m_fNeedToDeleteInterfaces;

    bool m_fCanCueNow;
    
    bool m_fHavePriority;
    double m_dblPriority;
    HRESULT m_hrRenderFileReturn;

    CTIMEDshowPlayerProxy * m_pProxy;
    CritSect                m_CriticalSection;
    
  private:
    CTIMEDshowPlayer();
};

#endif /* _PLAYERDSHOW_H */

