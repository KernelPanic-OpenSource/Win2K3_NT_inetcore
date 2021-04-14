//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tdraw.cxx
//
//  Contents:   CTable and related classes.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include <download.hxx>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include <othrguid.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_DISPDEFS_HXX_
#define X_DISPDEFS_HXX_
#include "dispdefs.hxx"
#endif

MtDefine(CTableDrawSiteList_aryElements_pv, Locals, "CTable::DrawSiteList aryElements::_pv")
MtDefine(CTable_pBorderInfoCellDefault, CTable, "CTable::_pBorderInfoCellDefault")

ExternTag(tagTableRecalc);

extern const WORD s_awEdgesFromTableFrame[];
const WORD s_awEdgesFromTableFrame[htmlFrameborder+1] =
{
    BF_RECT,        // not set case, use all edges
    0,
    BF_TOP,
    BF_BOTTOM,
    BF_TOP | BF_BOTTOM,
    BF_LEFT,
    BF_RIGHT,
    BF_LEFT | BF_RIGHT,
    BF_RECT,
    BF_RECT
};


extern const WORD s_awEdgesFromTableRules[];
const WORD s_awEdgesFromTableRules[htmlRulesall+1] =
{
    BF_RECT,    // not set case, use all edges
    0,
    0,
    BF_TOP | BF_BOTTOM,
    BF_LEFT | BF_RIGHT,
    BF_RECT,
};


//+---------------------------------------------------------------------------
//
//  Member:     CTableCell::GetBorderInfo
//
//  Synopsis:   fill out border information
//
//----------------------------------------------------------------------------

DWORD
CTableCell::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    // (t-michda) I added four default parameters so a format context could be included.
    return Layout( pdci->GetLayoutContext() )->GetCellBorderInfo(pdci, pborderinfo, fAll, fAllPhysical, 0, NULL, NULL, NULL FCCOMMA FCPARAM);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTable::GetBorderInfo
//
//  Synopsis:   fill out border information
//
//----------------------------------------------------------------------------

DWORD
CTable::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll, BOOL fAllPhysical FCCOMMA FORMAT_CONTEXT FCPARAM)
{
    return TableLayoutCache()->GetTableBorderInfo(pdci, pborderinfo, fAll, fAllPhysical FCCOMMA FCPARAM);
}

