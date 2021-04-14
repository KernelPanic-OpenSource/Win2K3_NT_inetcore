#ifndef _THREAD_LOCAL_STORE_HPP_
#define _THREAD_LOCAL_STORE_HPP_
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

#include "Assembly.hpp"

    /********************************************************************/
    /*                                                                  */
    /*   Constants exported from the class.                             */
    /*                                                                  */
    /*   The environment constants indicate the state of thread         */
    /*   local store.                                                   */
    /*                                                                  */
    /********************************************************************/

CONST SBIT32 NoTLSMemory			  = -1;
CONST SBIT32 NoTLSStructure			  = 0;

    /********************************************************************/
    /*                                                                  */
    /*   Thread local store.                                            */
    /*                                                                  */
    /*   A wide range of applications use threads.  It is often very    */
    /*   valuable to be able to have some private per thread data.      */
    /*   This functionality is supported by the following class.        */
    /*                                                                  */
    /********************************************************************/

class THREAD_LOCAL_STORE : public ASSEMBLY
    {
        //
        //   Private data.
        //
		BOOLEAN						  Active;

		SBIT32						  TlsIndex;
		SBIT32						  TlsOffset;

    public:
        //
        //   Public functions.
        //
        THREAD_LOCAL_STORE( VOID )
			{
			if ( Active = ((TlsIndex = TlsAlloc()) != NoTLSMemory) )
				{ TlsOffset = ((TlsIndex * sizeof(void*)) + TebSlot); }
			else
				{ Failure( "No TLS available" ); }
			}

		INLINE BOOLEAN Available( VOID )
			{ return Active; }
#ifdef ASSEMBLY_X86
#ifdef ENABLE_NON_STANDARD_ASSEMBLY

		INLINE VOID *GetAddress( VOID )
			{ return (GetTlsAddress( TlsOffset )); }
#endif
#endif

		INLINE VOID *GetPointer( VOID )
			{ return (GetTlsValue( TlsIndex,TlsOffset )); }

		INLINE VOID SetPointer( VOID *NewPointer )
			{ SetTlsValue( TlsIndex,TlsOffset,NewPointer ); }

        ~THREAD_LOCAL_STORE( VOID )
			{
			Active = False;

			if ( TlsIndex != NoTLSMemory )
				{ 
				if ( ! TlsFree( TlsIndex ) )
					{ Failure( "Unable to free TLS memory" ); }
				}
			}

	private:
        //
        //   Disabled operations.
        //
        THREAD_LOCAL_STORE( CONST THREAD_LOCAL_STORE & Copy );

        VOID operator=( CONST THREAD_LOCAL_STORE & Copy );
    };
#endif
