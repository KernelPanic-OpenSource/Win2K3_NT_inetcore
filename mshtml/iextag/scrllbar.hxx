//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\scrllbar.hxx
//
// Contents:    The UtilityButton control.
//
// Classes:     CScrollBar
//
// Interfaces:  IScrollBar
//   
//-------------------------------------------------------------------


#ifndef __SCROLLBAR_HXX_
#define __SCROLLBAR_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols
#include "math.h"

/////////////////////////////////////////////////////////////////////////////
//
// CScrollBar
//
/////////////////////////////////////////////////////////////////////////////

#define SCROLL_MIN_THUMBSIZE 8

enum Orientation
{
    Vertical = 0,
    Horizontal,
};

enum 
{
    SCROLL_PAGEINC      = 1,
    SCROLL_PAGEDEC      = 2,
    SCROLL_SLIDER       = 4,
    SCROLL_DELAY_TIMER  = 8,
    SCROLL_REPEAT_TIMER = 16,
};

class ATL_NO_VTABLE CScrollBar :
    public CBaseCtl, 
    public CComCoClass<CScrollBar, &CLSID_CScrollBar>,
    public IDispatchImpl<IScrollBar, &IID_IScrollBar, &LIBID_IEXTagLib>
{
public:

    CScrollBar ();
    ~CScrollBar ();

    //
    // CBaseCtl overrides
    //

    virtual HRESULT Init();

    //
    // Events
    //

    virtual HRESULT OnContentReady();
    virtual HRESULT OnMouseDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseUp(CEventObjectAccess *pEvent);
    virtual HRESULT OnKeyDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnSelectStart(CEventObjectAccess *pEvent);
    virtual HRESULT OnPropertyChange(CEventObjectAccess *pEvent, BSTR propertyName);

    //
    // Sink Callbacks
    //

    HRESULT BlockMoveStartDec(CEventObjectAccess *pEvent);
    HRESULT BlockMoveStartInc(CEventObjectAccess *pEvent);
    HRESULT BlockMoveSuspend(CEventObjectAccess *pEvent, IHTMLElement *pTrack);
    HRESULT BlockMoveResume(CEventObjectAccess *pEvent);
    HRESULT BlockMoveEnd(CEventObjectAccess *pEvent);
    HRESULT BlockMoveCheck(CEventObjectAccess *pEvent);

    HRESULT OnDelay();
    HRESULT OnRepeat();

    //
    // Utiltities
    //

    IHTMLElement * GetPart(unsigned id);


DECLARE_PROPDESC_MEMBERS(13);
DECLARE_REGISTRY_RESOURCEID(IDR_SCROLLBAR)
DECLARE_NOT_AGGREGATABLE(CScrollBar)

BEGIN_COM_MAP(CScrollBar)
    COM_INTERFACE_ENTRY2(IDispatch,IScrollBar)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IScrollBar)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    //
    // IElementBehavior
    //

    STDMETHOD(Detach)();

    // IScrollBar

    STDMETHOD(get_min)(long * pv);
    STDMETHOD(put_min)(long v);
    STDMETHOD(get_max)(long * pv);
    STDMETHOD(put_max)(long v);
    STDMETHOD(get_position)(long * pv);
    STDMETHOD(put_position)(long pv);
    STDMETHOD(get_unit)(long * pv);
    STDMETHOD(put_unit)(long v);
    STDMETHOD(get_block)(long * pv);
    STDMETHOD(put_block)(long v);
    STDMETHOD(get_visibleUnits)(long * pv);
    STDMETHOD(put_visibleUnits)(long v);
    STDMETHOD(get_orientation)(BSTR * pv);
    STDMETHOD(put_orientation)(BSTR v);

private:

    //
    //  Builder Assistance
    //

    HRESULT BuildContainer();
    HRESULT BuildIncrementer();
    HRESULT BuildDecrementer();
    HRESULT BuildBlockIncrementer();
    HRESULT BuildBlockDecrementer();
    HRESULT BuildSliderBar();
    HRESULT BuildSliderThumb();
    
    //
    //  Layout Utilities
    //

    HRESULT ReadProperties();
    HRESULT Layout();
    HRESULT CalcConstituentDimensions();
    HRESULT SetConstituentDimensions();
    HRESULT AdjustHeightAndWidth();

    HRESULT SetThumbPosition(long unit);
    HRESULT SyncThumbPosition(long pixels);

    HRESULT SetBlockMoverPositions();
    HRESULT BlockIncrement();
    HRESULT BlockDecrement();
    HRESULT BlockDisplay(IHTMLElement *pBlock, bool flag);
    HRESULT BlockHighlight(IHTMLElement *pBlock, bool flag);

    HRESULT DoIncrement();

    inline long   TotalUnits()    { return _lMaxPosition - _lMinPosition /*+ _lVisibleUnits*/; }
    inline long   TotalLength()   { return _lLength; }
    inline long   TotalWidth()    { return _lWidth;  }
    inline long   ButtonSize()    { return min( _lWidth, _lLength / 2); }
    inline long   TrackLength()   { return TotalLength() - ButtonSize() * 2; }
    inline long   ThumbLength()   { return _lThumbLength; }
    inline double PixelsPerUnit() { return (double) (TrackLength() - ThumbLength()) / (double) TotalUnits(); }
    
    inline long   NearestLong(double x) { return (long) floor ( x + 0.5 ); }

    //
    // Style Utilities
    //

    STDMETHOD(GetScrollbarColor)(VARIANT * pv);
    STDMETHOD(GetFaceColor)(VARIANT * pv);
    STDMETHOD(GetArrowColor)(VARIANT * pv);
    STDMETHOD(Get3DLightColor)(VARIANT * pv);
    STDMETHOD(GetShadowColor)(VARIANT * pv);
    STDMETHOD(GetHighlightColor)(VARIANT * pv);
    STDMETHOD(GetDarkShadowColor)(VARIANT * pv);

    //
    //  Timer Utilities
    //
    
    HRESULT StartDelayTimer();
    HRESULT StartRepeatTimer();

    HRESULT ClearDelayTimer();
    HRESULT ClearRepeatTimer();
    HRESULT ClearTimers();

    inline long GetDelayRate()  { return 250; }
    inline long GetRepeatRate() { return 50; }

    //
    //  Callbacks
    //

    HRESULT IncrementCallback(CUtilityButton *pButton);
    HRESULT DecrementCallback(CUtilityButton *pButton);

    HRESULT MouseMoveCallback(CUtilityButton *pButton, long &x, long &y);

    //
    //  View Link Event Sink Utility
    //

class CEventSink;

    HRESULT AttachEventToSink(IHTMLDocument3 *pDoc,   CComBSTR& bstr, CEventSink* pSink);
    HRESULT AttachEventToSink(IHTMLElement *pElement, CComBSTR& bstr, CEventSink* pSink);

private:


    class CEventSink : public IDispatch
    {
    public:

        CEventSink (CScrollBar *pParent, IHTMLElement *pElement, DWORD dwFlags);

        //
        // IUnknown
        //

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //
        // IDispatch
        //

        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames( REFIID riid,
                                    LPOLESTR *rgszNames,
                                    UINT cNames,
                                    LCID lcid,
                                    DISPID *rgDispId);
        STDMETHODIMP Invoke(DISPID dispIdMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS  *pDispParams,
                            VARIANT  *pVarResult,
                            EXCEPINFO *pExcepInfo,
                            UINT *puArgErr);
    private:

        CScrollBar         * _pParent;
        IHTMLElement       * _pElement;

        DWORD                _dwFlags;
    };

private:

    long         _lMaxPosition;          // maximum position
    long         _lMinPosition;          // minimum position
    long         _lCurrentPosition;      // current position

    long         _lUnitIncrement;        // unit (or line) increment
    long         _lBlockIncrement;       // block (or page) increment
    long         _lVisibleUnits;         // number of units visible

    long         _width;                 // scrollbar width
    long         _height;                // scrollbar height

    long         _lLength;               // pixels along orientation dimension
    long         _lWidth;                // pixels along opposite dimension
    long         _lThumbLength;          // pixels along orientation dimention of thumb

    Orientation  _eoOrientation;		 // horizontal, vertical, etc.

    long         _lCurIncrement;         // current increment size
    long         _sliderOffset;

    bool         _moving;  
    bool         _suspended;

    IHTMLElement *_pCurBlock;

    long         _lOnScrollCookie;
    long         _lOnChangeCookie;

    CEventSink          *_pSinkBlockInc;
    CEventSink          *_pSinkBlockDec;
    CEventSink          *_pSinkSlider;
    CEventSink          *_pSinkRepeatTimer;
    CEventSink          *_pSinkDelayTimer;

    IHTMLElement        *_pContainer;
    IHTMLElement        *_pSliderBar;
    IHTMLElement        *_pBlockInc;
    IHTMLElement        *_pBlockDec;
    long                 _lRepeatTimerId;
    long                 _lDelayTimerId;

    CComObject<CUtilityButton> *_pIncrementer;
    CComObject<CUtilityButton> *_pDecrementer;
    CComObject<CUtilityButton> *_pSliderThumb;
};


#endif // __SCROLLBAR_HXX_
