//+------------------------------------------------------------------------
//
//  File:       Edcmd.cxx
//
//  Contents:   Edit Command Classes.
//
//  Contents:   CCommand, CCharCommand, CInsertCommand, CBlockCommand etc.
//      
//  History:    05-07-98 marka - created
//              07-13-98 ashrafm - split into edcmd.cxx, blockcmd.cxx, charcmd.cxx,
//                                 delcmd.cxx, and nscmd.cxx
//
//-------------------------------------------------------------------------

#ifndef _EDCMD_HXX_
#define _EDCMD_HXX_ 1

#define TEXTCHUNK_SIZE  2048
#define SINGLE_CHAR     1
#define MAX_UNDOTITLE   64

// Non-breaking space
#ifndef WCH_NBSP
  #ifdef WIN16
    #define WCH_NBSP           '\xA0'
  #else
    #define WCH_NBSP           TCHAR(0x00A0)
  #endif
#endif

#ifndef _EDPTR_
#define _EDPTR_ 1
#if DBG == 1

#define ED_PTR( name ) CEditPointer name( GetEditor(), NULL, _T(#name) )

#else

#define ED_PTR( name ) CEditPointer name( GetEditor(), NULL )

#endif
#endif

//+----------------------------------------------------------------------------
//
//  elemInfluence enum.
//
// Enum describes the way in which a given element influences a range
//
// Used in GetElementInfluenceOverRange to describe how a given element influences the range
//
//-----------------------------------------------------------------------------

enum elemInfluence
{
    elemInfluenceNone = 0,                 // element exerts NO Influence
    elemInfluenceWithin = 1,             // element is wholely contained within the Range
    elemInfluenceOverlapOutside = 2,     // element begins outside the range ( to the left), and influences part of the range
    elemInfluenceOverlapWithin  = 3,     // element begins within the range ( and influences part of the range
    elemInfluenceCompleteContain  = 4     // element completely contains the entire range.
};

class CHTMLEditor;
class CSpringLoader;
class CMshtmlEd;
class CCommand;

//
// MtExtern's
//
    
MtExtern(CCommand);
MtExtern(CCommandTable);

class CCommand
{

public:
    // Constructor

    CCommand( DWORD cmdId, CHTMLEditor * pEd );

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCommand))

    virtual ~CCommand() ;

    void Passivate();

    virtual BOOL IsValidOnControl() {return FALSE;}

    HRESULT 
    Exec(
        DWORD                     nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut,
        CMshtmlEd *              pTarget );

    HRESULT 
    QueryStatus( 
        OLECMD *                pCmd,
        OLECMDTEXT *             pcmdtext,
        CMshtmlEd *              pTarget );

    DWORD GetCommandId();

    VOID SetLeft( CCommand * inLeft);
    
    CCommand * GetLeft();
    
    VOID SetRight( CCommand* inRight);
    
    CCommand * GetRight();


    //
    // Helper Routines for MarkupServices
    //

    // TODO : MOVE ALL OF THESE TO CHTMLEDITOR AS NON-STATIC, EXPOSE AS IHTMLEDITINGSERVICES

    HRESULT GetActiveElemSegment(
        IMarkupServices *         pMarkupServices,
        IMarkupPointer **        ppStart,
        IMarkupPointer **        ppEnd);
                                      
    elemInfluence GetElementInfluenceOverPointers(
        IMarkupServices* pMarkupServices, IMarkupPointer* pStart, IMarkupPointer* pEnd,
        IHTMLElement* pFirstMatch );

    HRESULT SplitInfluenceElement(
        IMarkupServices*        pMarkupServices,
        IMarkupPointer*         pStart, 
        IMarkupPointer*         pEnd,
        IHTMLElement*             pElement, 
        elemInfluence             inElemInfluence,
        IHTMLElement **            ppNewElement = NULL );

    HRESULT GetLeftAdjacentTagId(
        IMarkupServices*        pMarkupServices,
        IMarkupPointer  *        pMarkupPointer, 
        ELEMENT_TAG_ID          tagId,
        IMarkupPointer  **        ppLeft,
        IHTMLElement    **        ppElement,
        MARKUP_CONTEXT_TYPE *    pContext );

    HRESULT GetRightAdjacentTagId(
        IMarkupServices*        pMarkupServices,
        IMarkupPointer  *        pMarkupPointer, 
        ELEMENT_TAG_ID          tagId,
        IMarkupPointer  **        ppLeft,
        IHTMLElement    **        ppElement,
        MARKUP_CONTEXT_TYPE *    pContext );

    static HRESULT SplitElement(
        IMarkupServices*        pMarkupServices,
        IHTMLElement    *        pTargetElement, 
        IMarkupPointer  *        pTagStart, 
        IMarkupPointer  *        pSegmentEnd, 
        IMarkupPointer  *        pTagEnd,
        IHTMLElement    **           ppNewElement);

    static HRESULT GetSegmentElement( 
        IMarkupServices *        pMarkupServices,
        IMarkupPointer  *        pStart, 
        IMarkupPointer  *        pEnd, 
        IHTMLElement    **        ppElement,
        BOOL                    fOuter = FALSE );

    static BOOL CanSplitBlock( IMarkupServices *pMarkupServices , IHTMLElement* pElement );

    HRESULT GetFirstSegmentPointers(
        ISegmentList *            pSegmentList,
        IMarkupPointer **         ppStart,
        IMarkupPointer **         ppEnd);

    HRESULT CreateAndInsert(ELEMENT_TAG_ID tagId, IMarkupPointer *pStart, IMarkupPointer *pEnd, IHTMLElement **ppElement);

    HRESULT GetSegmentElement(ISegment *pSegmentList, IHTMLElement **ppElement);

    HRESULT ClingToText(IMarkupPointer *pMarkupPointer, Direction direction, IMarkupPointer *pLimit, 
                        BOOL fSkipExitScopes = FALSE, BOOL fIgnoreWhiteSpace = FALSE);
                          
    HRESULT Move(
        IMarkupPointer          *pMarkupPointer, 
        Direction               direction, 
        BOOL                    fMove,
        MARKUP_CONTEXT_TYPE *   pContext,
        IHTMLElement * *        ppElement);

    HRESULT MoveBack(
        IMarkupPointer          *pMarkupPointer, 
        Direction               direction, 
        BOOL                    fMove,
        MARKUP_CONTEXT_TYPE *   pContext,
        IHTMLElement * *        ppElement);

    HRESULT ExpandToWord(IMarkupServices * pMarkupServices, IMarkupPointer * pmpStart, IMarkupPointer * pmpEnd);

#if DBG == 1
    static void DumpTree( IUnknown * pUnknown );
#endif

    HRESULT InsertBlockElement(IHTMLElement *pElement, IMarkupPointer *pStart, IMarkupPointer *pEnd);    

    CMshtmlEd*  GetCommandTarget();
  
    HRESULT AdjustPointersForAtomic( IMarkupPointer *pStart, IMarkupPointer *pEnd );

    HRESULT RemoveDoubleBullets(IHTMLElement *pLIElement);

protected:

    CHTMLEditor*        GetEditor();
    IHTMLDocument2*     GetDoc();
    IMarkupServices2*   GetMarkupServices();
    IDisplayServices*   GetDisplayServices();

    HRESULT GetSegmentList(ISegmentList **ppSegmentList);

    BOOL                SegmentListContainsPassword( ISegmentList *pISegmentList );

    BOOL IsSelectionActive();
    
    CSpringLoader* GetSpringLoader();

    virtual HRESULT 
    PrivateExec( 
        DWORD                    nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut ) = 0;
       
    virtual HRESULT 
    PrivateQueryStatus( 
        OLECMD *                pCmd,
        OLECMDTEXT *             pcmdtext ) = 0;
       
    virtual HRESULT 
    CommonPrivateExec( 
        DWORD                    nCmdexecopt,
        VARIANTARG *             pvarargIn,
        VARIANTARG *             pvarargOut );

    virtual HRESULT 
    CommonQueryStatus( 
        OLECMD *                pCmd,
        OLECMDTEXT *             pcmdtext );

    BOOL CanAcceptHTML(ISegmentList *pSegmentList);

    HRESULT AdjustSegment(IMarkupPointer *pStart, IMarkupPointer *pEnd);
    
    // Instance Variables
    DWORD             _cmdId;
    
private:
    CHTMLEditor *     _pEd;            // Editor
    CCommand*         _leftNode;
    CCommand*         _rightNode;
    
    CCommand() {}                    // Hidden default constructor
};





class CCommandTable 
{

public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(CCommandTable))

    CCommandTable(unsigned short iInitSize);

    ~CCommandTable();

    VOID Add( CCommand* pCommandEntry );

    CCommand* Get(DWORD entryKey );

private:

    VOID EnsureSize();

    short FindEntry(DWORD entryKey, CCommand** ppFoundNode );
    
    CCommand* _rootNode;
    
};


//---------------------------------------------------------------------
//
// Inlines
//
//
//---------------------------------------------------------------------


inline DWORD
CCommand::GetCommandId()
{
    return _cmdId;
}

inline VOID 
CCommand::SetLeft( CCommand * inLeft)
{
    _leftNode = inLeft;
}

inline CCommand *
CCommand::GetLeft()
{
    return _leftNode;
}

inline VOID 
CCommand::SetRight( CCommand* inRight)
{
    _rightNode = inRight;
}

inline CCommand* 
CCommand::GetRight()
{
    return _rightNode;
}

#endif

