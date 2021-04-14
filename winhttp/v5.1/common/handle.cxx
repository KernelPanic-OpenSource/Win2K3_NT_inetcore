/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    handle.cxx

Abstract:

    Contains functions to allocate and deallocate handle values

    Contents:
        HandleInitialize
        HandleTerminate
        AllocateHandle
        FreeHandle
        MapHandleToAddress
        DereferenceObject

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Revision History:

    12-Mar-2001 rajeevd
        Gutted

    11-Jan-1996 rfirth
        Use fixed memory instead of moveable (Win95 has a bug w/ LocalUnlock)

    31-Oct-1994 rfirth
        Created
--*/

#include <wininetp.h>


//
// private prototypes
//

DEBUG_ONLY (PRIVATE void LogHandleClassSizes(); )

//
// functions
//

DWORD HandleInitialize (VOID)
{
    DEBUG_ONLY (LogHandleClassSizes(); )
    return ERROR_SUCCESS;
}


VOID HandleTerminate(VOID)
{
    // nothing to do
}


DWORD AllocateHandle (IN LPVOID Address,OUT LPHINTERNET lpHandle)
{
    HINTERNET Handle = (HINTERNET) Address;
    *lpHandle = Handle;
    return ERROR_SUCCESS;
}


DWORD FreeHandle(IN HINTERNET Handle)
{
    UNREFERENCED_PARAMETER(Handle);    
    return ERROR_SUCCESS;
}


DWORD
MapHandleToAddress(
    IN HINTERNET Handle,
    OUT LPVOID * lpAddress,
    IN BOOL Invalidate
    )

/*++

Routine Description:

    The handle object represented by Handle is referenced

    Assumes:    1. only HINTERNETs visible at the API are presented to this
                   function.
                   
Arguments:

    Handle      - handle value generated by AllocateHandle()

    lpAddress   - place to store mapped address. If the handle has been closed
                  and unmapped, NULL is returned. If the handle is still
                  mapped, even though it has been invalidated, its address will
                  be returned, and its reference count incremented

    Invalidate  - TRUE if we are invalidating this handle

Return Value:

    LPVOID
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE
                    if *lpAddress == NULL then the handle has been closed and
                    unmapped, else it is still mapped, but invalidated. In
                    this case, we incremented the reference count

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "MapHandleToAddress",
                "%#x, %#x, %B",
                Handle,
                lpAddress,
                Invalidate
                ));

    DWORD error;

    // Cast the handle to an address and validate
    LPVOID address = (LPVOID) Handle;
    if (address)
    {
        error = ((HANDLE_OBJECT *)address)->IsValid(TypeWildHandle);
    }
    else
    {
        error = ERROR_INVALID_HANDLE;
    }
    
    if (error != ERROR_SUCCESS)
    {
        DEBUG_PRINT(HANDLE,
                    ERROR,
                    ("invalid handle object: %#x [%#x]\n",
                    Handle,
                    address
                    ));
        error = ERROR_INVALID_HANDLE;
        address = NULL;
        goto quit;
    }

    // Attempt to increment the reference count on the handle.
    
    error = ((HANDLE_OBJECT *)address)->Reference();
    DEBUG_PRINT(HANDLE, ERROR, ("Reference() returns %d\n", error));
    switch (error)
    {
        case ERROR_SUCCESS: // handle was refcounted but is not tombstoned

            if (Invalidate)
            {
                // we were called from a handle close API.
                // Subsequent API calls will discover that the
                // handle is already invalidated and will quit
                ((HANDLE_OBJECT *)address)->Invalidate();
            }
            break;

        case ERROR_INVALID_HANDLE: // handle was refcounted but was tombstoned.
        
            break;

        default:
            INET_ASSERT (false);

        // intentional fall through
            
        case ERROR_ACCESS_DENIED: // handle is being destroyed
            
            DEBUG_PRINT(HANDLE,
                        ERROR,
                        ("Reference() failed - handle %#x [%#x] about to be deleted\n",
                        Handle,
                        address
                        ));
                        
            error = ERROR_INVALID_HANDLE;
            address = NULL;
            break;
    }

 quit:
    *lpAddress = address;
    DEBUG_LEAVE(error);
    return error;
}


DWORD
DereferenceObject(
    IN LPVOID lpObject
    )

/*++

Routine Description:

    Undoes the reference added to the handle object by MapHandleToAddress(). May
    result in the handle object being deleted

Arguments:

    lpObject    - address of object to dereference. This MUST be the mapped
                  object address as returned by MapHandleToAddress()

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    The handle object was destroyed

        Failure - ERROR_INVALID_HANDLE
                    The object was not a valid handle

--*/

{
    DEBUG_ENTER((DBG_HANDLE,
                Dword,
                "DereferenceObject",
                "%#x",
                lpObject
                ));

    INET_ASSERT(lpObject != NULL);

    HANDLE_OBJECT * object = (HANDLE_OBJECT *)lpObject;
    DWORD error = object->IsValid(TypeWildHandle);

    // Serialize with MapHandleToAddress
    
    if (error == ERROR_SUCCESS)
    {
        object->Dereference();
    }
    else
    {
        //
        // IsValid() should never return an error if the reference counts
        // are correct
        //
        INET_ASSERT(FALSE);
    }

    DEBUG_LEAVE(error);

    return error;
}


#if INET_DEBUG
PRIVATE
void
LogHandleClassSizes()
{
    DEBUG_PRINT(HANDLE,
                INFO,
                ("sizeof(HANDLE_OBJECT)                  = %d bytes\n",
                 sizeof(HANDLE_OBJECT)
                ));
    DEBUG_PRINT(HANDLE,
                INFO,
                ("sizeof(INTERNET_HANDLE_OBJECT)         = %d bytes\n",
                 sizeof(INTERNET_HANDLE_OBJECT)
                ));
    DEBUG_PRINT(HANDLE,
                INFO,
                ("sizeof(INTERNET_CONNECT_HANDLE_OBJECT) = %d bytes\n",
                 sizeof(INTERNET_CONNECT_HANDLE_OBJECT)
                ));
    DEBUG_PRINT(HANDLE,
                INFO,
                ("sizeof(HTTP_REQUEST_HANDLE_OBJECT)     = %d bytes\n",
                 sizeof(HTTP_REQUEST_HANDLE_OBJECT)
                ));
}
#endif
