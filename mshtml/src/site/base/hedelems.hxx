//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       hedelems.hxx
//
//  Contents:   CTitleElement, CMetaElement, CBaseElement, CIsIndexElement, CLinkElement
//
//----------------------------------------------------------------------------

#ifndef I_HEDELEMS_HXX_
#define I_HEDELEMS_HXX_
#pragma INCMSG("--- Beg 'hedelems.hxx'")

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#define _hxx_
#include "hedelems.hdl"

MtExtern(CTitleElement)
MtExtern(CMetaElement)
MtExtern(CBaseElement)
MtExtern(CIsIndexElement)
MtExtern(CNextIdElement)
MtExtern(CHtmlElement)
MtExtern(CHeadElement)

#define PERSISTENCE_META  _T("save")


//+--------------------------------------------------------------------------- 
//
//  Head Element classes
//
//----------------------------------------------------------------------------

class CHtmlElement : public CElement
{
    DECLARE_CLASS_TYPES(CHtmlElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHtmlElement))

    CHtmlElement(CDoc *pDoc)
      : CElement(ETAG_HTML, pDoc) {}

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);
    virtual void    Notify(CNotification *pNF);

    virtual CBase *GetBaseObjectFor(DISPID dispID, CMarkup * pMarkup = NULL);

    virtual DWORD   GetBorderInfo(
                        CDocInfo * pdci,
                        CBorderInfo * pborderinfo,
                        BOOL fAll = FALSE,
                        BOOL fAllPhysical = FALSE
                        FCCOMMA FORMAT_CONTEXT FCPARAM FCDEFAULT);

    virtual HRESULT Save ( CStreamWriteBuff * pStreamWrBuff, BOOL fEnd );

    #define _CHtmlElement_
    #include "hedelems.hdl"

    BOOL     ShouldStealBackground( FORMAT_CONTEXT FCPARAM );

    DECLARE_CLASSDESC_MEMBERS;

    static CElement::ACCELS             s_AccelsHtmlRun;
};

class CHeadElement : public CElement
{
    DECLARE_CLASS_TYPES(CHeadElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CHeadElement))

    CHeadElement(CDoc *pDoc)
      : CElement(ETAG_HEAD, pDoc) {}

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);
    
    #define _CHeadElement_
    #include "hedelems.hdl"


    DECLARE_CLASSDESC_MEMBERS;
};

class CTitleElement : public CElement
{
    DECLARE_CLASS_TYPES(CTitleElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CTitleElement))

    CTitleElement(CDoc *pDoc) : CElement(ETAG_TITLE_ELEMENT, pDoc) {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

#if DBG == 1
    ~ CTitleElement ( );
#endif

    HRESULT         Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    // internal helper/access methods
    HRESULT  SetTitle(TCHAR *pchTitle);

    inline LPTSTR   GetTitle() const  
        {  return _cstrTitle; };

    inline long     Length() 
        { return _cstrTitle.Length(); };


    // interface methods and properties
    #define _CTitleElement_
    #include "hedelems.hdl"


    // Members variables
    CStr  _cstrTitle;


protected:

    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CTitleElement);
};


class CMetaElement : public CElement
{
    DECLARE_CLASS_TYPES(CMetaElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CMetaElement))

    CMetaElement(CDoc *pDoc) : CElement(ETAG_META, pDoc) {}
    ~CMetaElement() {}

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    HRESULT Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd);

    virtual HRESULT Init2(CInit2Context * pContext);

    // Helper functions for special meta tags relating to codepages
    BOOL     IsCodePageMeta();
    CODEPAGE GetCodePageFromMeta();
    
    // Helper functions for the persistence Meta Tags
    BOOL IsPersistMeta(long eState);

    // Helper functions for the Theme Meta Tags
    int  TestThemeMeta();

    // Helper functions for the GalleryMode meta tag
    BOOL IsGalleryMeta();

    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    #define _CMetaElement_
    #include "hedelems.hdl"

protected:

    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CMetaElement);
};




class CBaseElement : public CElement
{
    DECLARE_CLASS_TYPES(CBaseElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CBaseElement))

    CBaseElement(ELEMENT_TAG etag, CDoc *pDoc) : CElement(etag, pDoc) {}
    ~CBaseElement() {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    virtual HRESULT Init2(CInit2Context * pContext);
    virtual HRESULT OnPropertyChange(DISPID dispid, DWORD dwFlags, const PROPERTYDESC *ppropdesc = NULL);

    virtual void Notify( CNotification *nf );

    TCHAR * GetHref() { return _cstrBase; }

    void BroadcastBaseUrlChange( );

    #define _CBaseElement_
    #include "hedelems.hdl"


protected:

    DECLARE_CLASSDESC_MEMBERS;

private:

    CStr _cstrBase;

    NO_COPY(CBaseElement);
};



class CIsIndexElement : public CElement
{
    DECLARE_CLASS_TYPES(CIsIndexElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CIsIndexElement))

    CIsIndexElement(CDoc *pDoc) : CElement(ETAG_ISINDEX, pDoc) {}
    ~CIsIndexElement() {}

    STDMETHOD(PrivateQueryInterface) (REFIID iid, void ** ppv);

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);

    #define _CIsIndexElement_
    #include "hedelems.hdl"

protected:

    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CIsIndexElement);
};


class CNextIdElement : public CElement
{
    DECLARE_CLASS_TYPES(CNextIdElement, CElement)

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CNextIdElement))

    CNextIdElement(CDoc *pDoc) : CElement(ETAG_NEXTID, pDoc) {}
    ~CNextIdElement() {}

    static HRESULT CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult);


    #define _CNextIdElement_
    #include "hedelems.hdl"

protected:

    DECLARE_CLASSDESC_MEMBERS;

private:

    NO_COPY(CNextIdElement);
};

inline BOOL
CHtmlElement::ShouldStealBackground( FORMAT_CONTEXT FCPARAM )
{
    // We should steal background off the element child, if we don't have our own.
    return !GetFirstBranch()->GetFancyFormat(FCPARAM)->HasBackgrounds(FALSE);
}

#pragma INCMSG("--- End 'hedelems.hxx'")
#else
#pragma INCMSG("*** Dup 'hedelems.hxx'")
#endif
