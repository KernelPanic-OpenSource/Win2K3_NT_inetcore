//+---------------------------------------------------------------------------
//
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       AccUtil.Cxx
//
//  Contents:   Accessibility util functions
//
//----------------------------------------------------------------------------


#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_ACCBASE_HXX_
#define X_ACCBASE_HXX_
#include "accbase.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_ACCUTIL_HXX_
#define X_ACCUTIL_HXX_
#include "accutil.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"    
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_ACCELEM_HXX_
#define X_ACCELEM_HXX_
#include "accelem.hxx"
#endif

#ifndef X_ACCWIND_HXX_
#define X_ACCWIND_HXX_
#include "accwind.hxx"
#endif

#ifndef X_ACCFRAME_HXX_
#define X_ACCFRAME_HXX_
#include "accframe.hxx"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

//-----------------------------------------------------------------------
//  IsSupportedElement()
//
//  DESCRIPTION:
//
//          Tells if the element is a valid accessibility element or not.
//
//  PARAMETERS:
//
//      pElem            pointer to the CElement
//
//  RETURNS:
//
//      TRUE | FALSE
//-----------------------------------------------------------------------
BOOL 
IsSupportedElement(CElement *pElem )
{
    BOOL fRet = FALSE;

    if (!pElem)
        goto Cleanup;

    // An element that has an attached behavior that implements IAccessible 
    // takes precedence over a supported tag.  However, given that this function
    // does not return why a tag is supported, it is more efficient to check if the
    // TAG is in the supported list, and only do the QI if we are still unsure if
    // the tag is supported.
      
    switch ( pElem->Tag() ) 
    {
    case ETAG_INPUT:        //both input and input text are used for input elements
        if (DYNCAST(CInput, pElem)->GetAAtype() == htmlInputHidden)
        {
            fRet = FALSE;
            break;
        }
    case ETAG_BODY:
    case ETAG_FRAME:
    case ETAG_IFRAME:
    case ETAG_A:
    case ETAG_BUTTON:
    case ETAG_IMG:
    case ETAG_TEXTAREA:
    case ETAG_MARQUEE:
    case ETAG_SELECT:
    case ETAG_OBJECT:
    case ETAG_APPLET:
    case ETAG_EMBED:
    case ETAG_TABLE:
    case ETAG_TD:
    case ETAG_TH:
    case ETAG_LABEL:
        fRet = TRUE;
        break;

    default:
        //
        // if we are not in the "official list" above, we still may be supported:
        //
        // if there is a tabstop defined on this element see CDoc::OnElementEnter for comments about Tabstops
        // if we have a behavior that implements IAccessible or a viewlink 
        //

        if (pElem->HasPeerHolder())
        {
            if (pElem->HasSlavePtr())
            {
                fRet = TRUE;
            }
            else
            {
                fRet = IsIAccessiblePeer(pElem);
            }
        }

        fRet  = fRet | (pElem->GetAAtabIndex() != htmlTabIndexNotSet);
        break;
    }

Cleanup:
    return fRet;
}

//-----------------------------------------------------------------------
//
//  IsIAccessiblePeer()
//
//  DESCRIPTION:
//
//          Tells if the peer implements IAccessible
//
//          In order to conclude that we have an accessible peer,
//          We need to find an attached behavior that implements IAccessible OR 
//          Element behavior that implements IAccessible.
//
//  PARAMETERS:
//
//      pElem            pointer to the CElement
//
//  RETURNS:
//
//      TRUE | FALSE
//-----------------------------------------------------------------------

BOOL 
IsIAccessiblePeer(CElement *pElem)
{
    HRESULT hr;
    BOOL fRet  = FALSE;

    CPeerHolder * pPH;
    IAccessible * pAcc = NULL;

    Assert (pElem->HasPeerHolder());

    pPH = pElem->GetPeerHolder();

    // Element behaviors are always first in the peer chain.

    hr = THR(pPH->QueryPeerInterfaceMulti(IID_IAccessible, (void **)&pAcc, FALSE));

    if (!hr)
    {
        fRet = TRUE;
        Assert(pAcc);                 // Catch bad behavior
    }

    ReleaseInterface(pAcc);
    return fRet;
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

HRESULT 
ScrollIn_Focus( CElement* pElem )
{
    HRESULT hr;
    
    if ( !pElem )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    
    //scrollintoview.
    hr = THR( pElem->ScrollIntoView());
    if ( hr )
    {
        goto Cleanup;
    }

    //focus
    hr = THR( pElem->focus() );
        
Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
HRESULT 
ScrollIn_Focus_Click( CElement* pElem )
{
    HRESULT hr;

    if ( !pElem )
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = THR( ScrollIn_Focus(pElem) );
    if ( hr )
    {
        goto Cleanup;
    }

    hr = THR( pElem->DoClick( NULL, NULL, FALSE, NULL, TRUE) );
    
Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  IsFocusable
//  
//  DESCRIPTION:
//      If the document that contains the elenent has focus, the function
//      returns TRUE, since the element has the ability to receive focus.
//
//  PARAMETERS:
//      pElem   :   Pointer to the CElement 
//  
//  RETURNS :
//      TRUE if the element is focusable, FALSE otherwise
//-----------------------------------------------------------------------
BOOL
IsFocusable( CElement* pElem )
{
    CDoc* pDoc= pElem->Doc();

    // does this element have a document at all?
    if ( pDoc )
    {
        return (pDoc->HasFocus());
    }
    else
        return FALSE;
}

//-----------------------------------------------------------------------
//  GetMarkupLimits
//  
//  DESCRIPTION:
//      Receives a CElement pointer, and returns two markup pointer that
//      are places after the beginning and end tags of this element.
//      This method is counted on to be called elements with scope only. 
//      
//  PARAMETERS:
//      pElem   :   CElement * to the element 
//      ppBegin :   Address of the variable that receives the begin tag markup
//      ppEnd   :   Address of the variable that receives the end tag markup
//-----------------------------------------------------------------------
HRESULT
GetMarkupLimits( CElement* pElem, CMarkupPointer* pBegin, CMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;

    if ( !pElem || !pBegin || !pEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pBegin->MoveAdjacentToElement( pElem, ELEM_ADJ_AfterBegin));
    if ( hr )
        goto Cleanup;

    hr = THR( pEnd->MoveAdjacentToElement( pElem, ELEM_ADJ_AfterEnd));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  GetSubMarkupLimits
//
//  DESCRIPTION:
//      Very similar to the GetMarkupLimits. However, this one is used for
//      viewlinked markups, so if the markup is created through DOM, it 
//      is not possible to place a markup pointer after the ROOT closes.
//      The end pointer is placed before the root tag in this case.
//-----------------------------------------------------------------------
HRESULT
GetSubMarkupLimits(CElement* pElem, CMarkupPointer* pBegin, CMarkupPointer* pEnd )
{
    HRESULT hr = S_OK;

    if ( !pElem || !pBegin || !pEnd )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    hr = THR( pBegin->MoveAdjacentToElement( pElem, ELEM_ADJ_AfterBegin));
    if ( hr )
        goto Cleanup;

    hr = THR( pEnd->MoveAdjacentToElement( pElem, ELEM_ADJ_BeforeEnd));
    if ( hr )
        goto Cleanup;

Cleanup:
    RRETURN( hr );
}

//-----------------------------------------------------------------------
//  AccStepRight
//  
//  DESCRIPTION:
//      Moves the markup pointer to the right one step. A step is considered
//      moving from one before/end tag position to the other. If the tag
//      to the right of the markup pointer belongs to a supported element
//      the function steps over the complete supported element tag. NOscope
//      elements are considered single steps
//      The method increments the counter if it steps over something that
//      would cause the child count to increase.
//  
//  PARAMETERS:
//      pMarkup :   Markup pointer to be moved
//      pCounter:   Pointer to the child counter on the caller side
//-----------------------------------------------------------------------
HRESULT
AccStepRight( CMarkupPointer* pMarkup, long * pCounter )
{
    HRESULT             hr = S_OK;
    MARKUP_CONTEXT_TYPE context;
    CTreeNode *         pElemNode = NULL;

    //get what is to our right
    hr = THR( pMarkup->Right( TRUE, &context, &pElemNode, NULL, NULL, NULL));

    switch ( context )
    {

#if DBG==1            
        case CONTEXT_TYPE_None:     
            //if there was nothing on this side at all,
            AssertSz(FALSE, "We have hit the root..");
            break;

        case CONTEXT_TYPE_ExitScope://don't care ...
            break;
#endif

        case CONTEXT_TYPE_EnterScope:
            //check the element to see if it is a supported element
            if ( IsSupportedElement( pElemNode->Element() ) )
            {
                //go to the end of this element, since we will handle it as a container
                hr = THR( pMarkup->MoveAdjacentToElement( pElemNode->Element(), ELEM_ADJ_AfterEnd));
                if ( hr )
                    goto Cleanup;

                (*pCounter)++;      
            }
            //don't do anything if it is not a supported element.
            break;  
            
        //we jumped over text or noscope
        case CONTEXT_TYPE_NoScope:
            if ( !IsSupportedElement( pElemNode->Element() ))
                break;
            
        case CONTEXT_TYPE_Text:
            (*pCounter)++;      
            break;
    }

Cleanup:
    RRETURN( hr );
}

//+---------------------------------------------------------------------------
//  DESCRIPTION:
//      Returns the total number of acc. children between the two markup 
//      locations
//----------------------------------------------------------------------------
HRESULT 
GetChildCount( CMarkupPointer* pStart, CMarkupPointer* pEnd, long* pChildCnt)
{
    HRESULT             hr = S_OK;
    long                lChildCnt = 0;
    MARKUP_CONTEXT_TYPE context;
    CTreeNode *         pElemNode = NULL;

    Assert( pStart );
    Assert( pEnd );
    Assert( pChildCnt );
        
    *pChildCnt = 0;
//
//TODO:   FerhanE :   We can change this for perf, to compare the left elements and
//                    contexts of two pointers.
///                   1- Get left context and element of end ptr,
//                    2- Move startptr to right and keep track of the element
//                          and context. 
//                    3- Compare the context and element of the two pointers. 
//                       if they are the same, we have the child count.
//.                     

    //we walk the pStart through the HTML using this loop
    for (; ; )
    {
        // if the two pointers are pointing to the same location, or if
        // we passed the pEnd location. If the pEnd location is inside
        // a supported element tag, then we will not have a chance to 
        // stop there, we will jump over it. 
        if ( !pStart->IsLeftOf( pEnd ) )
            break;

        //get what is to our right
        hr = THR( pStart->Right( TRUE, &context, &pElemNode, NULL, NULL, NULL));

        switch ( context )
        {
#if DBG==1            
            case CONTEXT_TYPE_None:     
                //if there was nothing on this side at all,
                AssertSz(FALSE, "We have hit the root..");
                break;

            case CONTEXT_TYPE_ExitScope://don't care ...
                break;
#endif

            case CONTEXT_TYPE_EnterScope:
                //check the element to see if it is a supported element
                if ( IsSupportedElement( pElemNode->Element() ) )
                {
                    //go to the end of this element, since we will handle it as a container
                    hr = THR( pStart->MoveAdjacentToElement( pElemNode->Element(), ELEM_ADJ_AfterEnd));
                    if ( hr )
                        goto Cleanup;

                    lChildCnt++;      
                }
                //don't do anything if it is not a supported element.
                break;  
            
            //we jumped over text or noscope
            case CONTEXT_TYPE_NoScope:
                if ( !IsSupportedElement( pElemNode->Element() ))
                    break;
            
            case CONTEXT_TYPE_Text:
                lChildCnt++;      
                break;
        }
    }

Cleanup:
    if ( hr == S_OK )
        *pChildCnt = lChildCnt;

    RRETURN( hr );
}

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
HRESULT 
GetHitChildID( CElement* pElem, CMessage * pMsg, VARIANT * pvarChild )
{
    HRESULT             hr = S_OK;
    CMarkupPointer      walkerMarkup( pElem->Doc() );
    CMarkupPointer      HitMarkup(pElem->Doc());
    BOOL                bNotAtBOL;
    BOOL                bAtLogicalBOL;
    BOOL                bRightOfCp;
    MARKUP_CONTEXT_TYPE contextleft, 
                        contextright;
    CTreeNode *         pElemNodeLeft = 0;
    CTreeNode *         pElemNodeRight = 0;
    POINT               ptContent;
    CLayoutContext *    pLayoutContext;

    Assert( pvarChild );

    CDispNode* pDispNode;
    // position the markup pointer at the point that we are given
    // $$ktam: does pMsg have correct layout context already?  what about correct tree node?
    CTreeNode * pTreeNode = pElem->Doc()->GetNodeFromPoint( pMsg->pt, &pLayoutContext, FALSE, &ptContent,
                                                            NULL, NULL, NULL, &pDispNode );
    if( pTreeNode == NULL )
        goto Cleanup;

    // $$ktam: passing pTreeNode->GetFlowLayout() as the containing layout seems fishy to me..?
    hr = THR( pElem->Doc()->MovePointerToPointInternal( ptContent, 
                                                        pTreeNode,
                                                        pLayoutContext,
                                                        &HitMarkup, 
                                                        &bNotAtBOL,
                                                        &bAtLogicalBOL,
                                                        &bRightOfCp, 
                                                        FALSE,
                                                        pTreeNode->GetFlowLayout( pLayoutContext ),
                                                        NULL,
                                                        TRUE,
                                                        NULL,
                                                        pDispNode ));
    if (hr)
        goto Cleanup;
    
    // Set the HitMarkup to the current context.
    //-------------------------------------------------------------------------------
    hr = THR( HitMarkup.Left( FALSE, &contextleft, &pElemNodeLeft, NULL, NULL, NULL));
    if ( hr )
        goto Cleanup;

    hr = THR( HitMarkup.Right( FALSE, &contextright, &pElemNodeRight, NULL, NULL, NULL));
    if ( hr )
        goto Cleanup;

    //if there is text to the hit markups left, move to the closest tag on its left.
    //otherwise we are ready to count children.
    if ( contextleft == CONTEXT_TYPE_Text )
    {
        hr = THR( HitMarkup.Left( TRUE, &contextleft, NULL, NULL, NULL, NULL));
        if ( hr )
            goto Cleanup;
    }
    else
    {
        // after placing the Markup pointers, if we are not over text then we are in one
        // of the "gutters" on our parent and possibly sitting next to a noscope supported
        // thing.  In this case we don't need to continue with context correctness of text
        // children, and can instead, just return.
        //--------------------------------------------------------------------------------
        if ((contextright != CONTEXT_TYPE_Text ) && 
                pElemNodeRight && 
                (pElem == pElemNodeRight->Element() || !IsSupportedElement(pElemNodeRight->Element()) ))
        {
            V_VT( pvarChild ) = VT_I4;
            V_I4(pvarChild) = CHILDID_SELF;
            goto Cleanup;
        }
    }
      
    //place the markup pointer to the beginning of the element's scope.
    hr = THR( walkerMarkup.MoveAdjacentToElement( pElem, ELEM_ADJ_AfterBegin));
    if ( hr )
        goto Cleanup;

    // Count the number of children from the point we start to the point we hit
    // that number is the child id for the spot we hit.
    hr = THR( GetChildCount( &walkerMarkup, &HitMarkup, &(V_I4(pvarChild)) ) );
    if ( hr )
        goto Cleanup;

    // the contents of this variable is a 0 based index, but we want
    // the child id to be 1 based.
    V_VT(pvarChild) = VT_I4;
    V_I4(pvarChild)++;
   
  
Cleanup:

    //
    // MovePointerToPointInternal now returns CTL_E_INVALIDLINE when you attempt to
    // position a markup pointer in a table or TR.  Previously, this was returning
    // E_FAIL.  We want to prop the E_FAIL return code for compat.
    //
    if( hr == CTL_E_INVALIDLINE )
    {
        hr = E_FAIL;
    }
    
    RRETURN( hr );
}

//----------------------------------------------------------------------------
//  GetTextFromMarkup
//  
//  DESCRIPTION:
//      Given a markup pointer that has text to its right, retrieves the text
//      and fills a BSTR with the text read.
//      This function assumes that the markup pointer passed was placed properly
//      before text.
//      
//  PARAMETERS:
//      pMarkup     :   Markup pointer to start reading from
//      pbstrText   :   Address of the bstr to receive the text.
//----------------------------------------------------------------------------
HRESULT
GetTextFromMarkup( CMarkupPointer *pMarkup, BSTR * pbstrText )
{
    HRESULT                 hr = E_INVALIDARG;
    MARKUP_CONTEXT_TYPE     context;
    long                    lchCnt =-1;
    
    if ( !pMarkup || !pbstrText)
        goto Cleanup;

    // get the number of characters available for reading.
    hr = THR( pMarkup->Right( FALSE, &context, NULL, &lchCnt, NULL, NULL));
    if ( hr )
        goto Cleanup;

    Assert( context == CONTEXT_TYPE_Text );   //just checking.. !

    //allocate buffer
    hr = FormsAllocStringLen ( NULL, lchCnt, pbstrText );
    if ( hr )
        goto Cleanup;

    //read text
    hr = THR( pMarkup->Right( TRUE, &context, NULL, &lchCnt, *pbstrText, NULL));
    if ( hr )
    {
        // release the BSTR and reset the pointer if we have failed to get
        // the text from tree services.
        FormsFreeString( *pbstrText );
        *pbstrText = NULL;
    }    
    
Cleanup:
    RRETURN( hr );
}


//-----------------------------------------------------------------------
//  GetAccObjOfElement
//
//  DESCRIPTION:
//
//      Returns the element's associated Lookaside acc. object. 
//      if the acc object is not created it creates it and adds it to the
//      lookaside list.
//
//  PARAMETERS:
//
//      pElem   :   pointer to the element
//
//  RETURNS:
//
//      Pointer to the CAccBase that is created/looked up.
//
//  NOTES:
//
//
//----------------------------------------------------------------------------
CAccBase * GetAccObjOfElement( CElement* pElem )
{
    CAccBase *  pAccObj = NULL;

    //we require a valid pointer.
    if ( !pElem )
        return NULL;

    //if the element has a lookaside ptr, return that.
    //otherwise, create one, add to the list and return.
    if (pElem->HasAccObjPtr())
    {
        // get the acc. lookaside ptr for this element.
        pAccObj = pElem->GetAccObjPtr();

        goto Cleanup;
    }

    //
    // If the element is not a FRAME or IFRAME tag, then create the element
    // using the lookaside pointer functions in the CElement.
    // Otherwise, create a CAccWindow that represents the frame or iframe 
    //
    if (( pElem->Tag() != ETAG_FRAME ) && 
        (pElem->Tag()!=ETAG_IFRAME))
    {
        //create the accessible object
        //the CAccBase constructor also adds the object to the lookaside list,
        pAccObj = pElem->CreateAccObj();
        if ( !pAccObj )
        {
            goto Cleanup;
        }
    }
    else
    {
        // If there is a behavior attached to this frame, then create a CAccBehavior
        // instead of a CAccFrame.
        if (pElem->HasPeerHolder())
        {
            pAccObj = pElem->AccObjForBehavior();
        }

        // if there is not a behavior, or if we could not create an accessible
        // object for that behavior, then do what we would do if we did not have
        // a behavior attached.
        // In the weird case of HTML code with more FRAME tags than the ones defined, 
        // there won't be a window, so we check here if we have a valid proxy 
        // before continuing. see bug #8673 in IEv60 database
        COmWindowProxy * pWindowProxy = (DYNCAST( CFrameSite, pElem))->_pWindow;

        if (!pAccObj && pWindowProxy)
        {   
            // get the inner window for a frame tag.
            CWindow * pInnerWnd = pWindowProxy->Window();

            Assert(pInnerWnd);

            if (!(pInnerWnd->_pAccWindow))
            {
                pInnerWnd->_pAccWindow = (CAccWindow *) new CAccFrame( pInnerWnd, pElem);
                if (!pInnerWnd->_pAccWindow)
                    goto Cleanup;
            }

            pAccObj = pInnerWnd->_pAccWindow;
        }
    }

//[FerhanE]    
//we could AddRef the element here, but we don't, since most of the
//users of this function are internal, and they don't need reference
//counts on the element or they may chose not to increment (WM_GETOBJECT case)
    
Cleanup:
    return pAccObj;
}

//+---------------------------------------------------------------------------
//  SelectText
//  
//  DESCRIPTION:
//      Selects the text starting from the markup pointer that is passed, 
//      until the next tag in the markup stream.
//      
//  PARAMETERS:
//      pElem   :   The element that parents the text. 
//      pBegin  :   Pointer to the beginning markup position
//      
//----------------------------------------------------------------------------
HRESULT
SelectText( CElement* pElem, IMarkupPointer * pMarkupBegin )
{
    HRESULT             hr;
    IMarkupPointer *    pMarkupEnd = NULL;
    MARKUP_CONTEXT_TYPE context;

    if ( !pElem || !pMarkupBegin )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Since the selection manager is in another module, we have to use 
    // (IMarkupPointer *) values instead of (CMarkupPointer) values.(ferhane)

    // Create another markup pointer and place to the same spot the
    // first one is placed.
    hr = THR( pElem->Doc()->CreateMarkupPointer( &pMarkupEnd ) );
    if ( hr )
        goto Cleanup;
    
    hr = THR( pMarkupEnd->MoveToPointer( pMarkupBegin ) );
    if ( hr ) 
        goto Cleanup;

    // move the end pointer next to the first tag in the HTML stream.
    hr = THR( pMarkupEnd->Right( TRUE, &context, NULL, NULL, NULL ) );
    if ( hr )
        goto Cleanup;

    // select the range marked by the two markup pointers we have.
    hr = THR( pElem->Doc()->Select( pMarkupBegin, pMarkupEnd, SELECTION_TYPE_Text) );

Cleanup:
    ReleaseInterface( pMarkupEnd );
    RRETURN( hr );
}
