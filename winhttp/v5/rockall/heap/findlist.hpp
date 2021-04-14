#ifndef _FIND_LIST_HPP_
#define _FIND_LIST_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#include "BucketList.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   The find list.                                                 */
    /*                                                                  */
    /*   The find list links all the pages in the same hash bucket      */
    /*   together so that the correct page can be found.                */
    /*                                                                  */
    /********************************************************************/

class FIND_LIST : public BUCKET_LIST
    {
		//
		//   Private data.
		//
 		LIST						  FindList;

   public:
		//
		//   Public inline functions.
		//
		//   All page descriptions contain three linked lists.
		//   These lists are all derived from a common base
		//   class.  However, this class is unable to support
		//   multiple instances in a single class a wrapper
		//   has been created for each list to make it work
		//   as required.
		//
        FIND_LIST( VOID )
			{ /* void */ };

		INLINE VOID DeleteFromFindList( LIST *HeadOfList )
			{ FindList.Delete( HeadOfList ); }

		INLINE BOOLEAN EndOfFindList( VOID )
			{ return (this == NULL); }

		STATIC INLINE PAGE *FirstInFindList( LIST *HeadOfList )
			{ return ComputePageAddress( ((CHAR*) HeadOfList -> First()) ); }

		INLINE VOID InsertInFindList( LIST *HeadOfList )
			{ FindList.Insert( HeadOfList ); }

		INLINE PAGE *NextInFindList( VOID )
			{ return ComputePageAddress( ((CHAR*) FindList.Next()) ); }

        ~FIND_LIST( VOID )
			{ /* void */ };

	private:
		//
		//   Private functions.
		//
		//   Compute the actual start address of the page
		//   and return it to allow the linked list to
		//   be correctly walked.
		//
		STATIC INLINE PAGE *ComputePageAddress( CHAR *Address )
			{
			if ( Address != NULL )
				{ return ((PAGE*) (Address - sizeof(BUCKET_LIST))); }
			else
				{ return ((PAGE*) NULL); }
			}

        //
        //   Disabled operations.
		//
		//   All copy constructors and class assignment 
		//   operations are disabled.
        //
        FIND_LIST( CONST FIND_LIST & Copy );

        VOID operator=( CONST FIND_LIST & Copy );
    };
#endif
