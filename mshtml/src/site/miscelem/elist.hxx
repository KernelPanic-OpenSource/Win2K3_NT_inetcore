//+---------------------------------------------------------------------------
//
//  Microsoft IEv5
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       elist.hxx
//
//  Contents:   CListElement class
//
//----------------------------------------------------------------------------

#ifndef I_ELIST_HXX_
#define I_ELIST_HXX_
#pragma INCMSG("--- Beg 'elist.hxx'")

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#define _hxx_
#include "list.hdl"

MtExtern(CListElement)

class CListElement : public CElement
{
    DECLARE_CLASS_TYPES(CListElement, CElement)
    
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CListElement))

    CListElement(ELEMENT_TAG etag, CDoc *pDoc)
        : CElement(etag, pDoc) {}

    // IUnknown methods
    DECLARE_PRIVATE_QI_FUNCS(CBase)

    virtual HRESULT ApplyDefaultFormat(CFormatInfo *pCFI);

    virtual styleListStyleType FilterHtmlListType(styleListStyleType type, WORD wLevel);

    void UpdateVersion()
    {
        if (!_fExittreePending)
            _dwVersion++;
    }
    void Notify(CNotification *pNF);

#define _CListElement_
#include "list.hdl"

    DWORD _dwVersion;
    
protected:
    DECLARE_CLASSDESC_MEMBERS;
};

class CListItemIterator : public CChildIterator
{
public:    
    CListItemIterator(CListElement *pElementContainer, CElement *pElementStart);
};

#pragma INCMSG("--- End 'elist.hxx'")
#else
#pragma INCMSG("*** Dup 'elist.hxx'")
#endif
