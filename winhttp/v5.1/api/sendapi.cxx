/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    sendapi.cxx

Abstract:

    This file contains the implementation of the HttpSendRequestA API.

    Contents:
        WinHttpSendRequest
        HttpSendRequestA
        WinHttpReceiveResponse
        HttpWrapSendRequest

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

      29-Apr-97 rfirth
        Conversion to FSM

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// private prototypes
//

PRIVATE
BOOL
HttpWrapSendRequest(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwOptionalLengthTotal,
    IN AR_TYPE arRequest,
    OUT HTTP_REQUEST_HANDLE_OBJECT** pHandleToDeref,
    IN DWORD_PTR dwContext=NULL
    );

//
// functions
//

INTERNETAPI
BOOL
WINAPI
HttpSendRequestA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

--*/

{
    DEBUG_ENTER((DBG_API,
                Bool,
                "HttpSendRequestA",
                "%#x, %.80q, %d, %#x, %d",
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength
                ));


    BOOL fRet= HttpWrapSendRequest(
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                0,
                AR_HTTP_SEND_REQUEST,
                NULL
                );


    DEBUG_LEAVE(fRet);

    return fRet;
}

INTERNETAPI
BOOL
WINAPI
WinHttpSendRequest(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwTotalLength,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

--*/

{
    DEBUG_ENTER2_API((DBG_API,
                Bool,
                "WinHttpSendRequest",
                "%#x, %.80wq, %d, %#x, %d, %d, %x",
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                dwTotalLength,
                dwContext
                ));

    TRACE_ENTER2_API((DBG_API,
                Bool,
                "WinHttpSendRequest",
                hRequest,
                "%#x, %.80wq, %d, %#x, %d, %d, %x",
                hRequest,
                !lpszHeaders || IsBadReadPtr(lpszHeaders, 1)? L"": lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                dwTotalLength,
                dwContext
                ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpHeaders;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }

    if (lpszHeaders && IsBadReadPtr(lpszHeaders, 1))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    if (dwHeadersLength == -1L)
    {
        dwHeadersLength = lpszHeaders ? lstrlenW(lpszHeaders) : 0;
    }

    if (lpszHeaders)
    {
        if ((dwHeadersLength == -1)
            ? IsBadStringPtrW(lpszHeaders, (UINT_PTR)-1)
            : IsBadReadPtr(lpszHeaders, dwHeadersLength))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(lpszHeaders, dwHeadersLength, mpHeaders);
        if (!mpHeaders.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszHeaders, mpHeaders);
    }
    if (lpOptional 
        && dwOptionalLength
        && IsBadReadPtr(lpOptional, dwOptionalLength) )
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
            
    AR_TYPE ar;
    
    // Always require a WinHttpReceiveResponse to initiate
    // FSM_STATE_4 onwards in HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Start:
    if (dwOptionalLength <= dwTotalLength)
    {
        ar = AR_HTTP_BEGIN_SEND_REQUEST;
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    HTTP_REQUEST_HANDLE_OBJECT* pRequest = NULL;
    
    fResult = HttpWrapSendRequest(hRequest, mpHeaders.psStr, mpHeaders.dwSize,
                lpOptional, dwOptionalLength, dwTotalLength, ar, &pRequest, dwContext);
    // This calls SetLastError if fResult is FALSE.

    if (fResult)
    {
        if (pRequest)
        {
            InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE,
                                   NULL,
                                   NULL
                                   );

            // So that we have a refcount on the object going into the callback.
            DereferenceObject((LPVOID)pRequest);
        }
    }
    else if ((dwErr = GetLastError()) == ERROR_IO_PENDING)
    {
        SetLastError(dwErr = ERROR_SUCCESS);
        fResult = TRUE;
    }
    
cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        DEBUG_ERROR(HTTP, dwErr);
        SetLastError(dwErr); 
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
WinHttpReceiveResponse(
    IN HINTERNET hRequest,
    IN LPVOID lpBuffersOut OPTIONAL
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hRequest        -
    lpBuffersOut    -
    dwFlags         -
    dwContext       -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpReceiveResponse",
                     "%#x, %#x",
                     hRequest,
                     lpBuffersOut
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else if (lpBuffersOut)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        HTTP_REQUEST_HANDLE_OBJECT* pRequest = NULL;
        fResult = HttpWrapSendRequest(hRequest, NULL, 0, NULL, 0, 0, AR_HTTP_END_SEND_REQUEST, &pRequest);
        
        if (fResult)
        {
            if (pRequest)
            {
                InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE,
                                       NULL,
                                       NULL
                                       );

                // So that we have a refcount on the object going into the callback.
                DereferenceObject((LPVOID)pRequest);
            }
        }
        else if ((dwErr = GetLastError()) == ERROR_IO_PENDING)
        {
            SetLastError(dwErr = ERROR_SUCCESS);
            fResult = TRUE;
        }
    }
    
    if (dwErr!=ERROR_SUCCESS) 
    { 
        DEBUG_ERROR(HTTP, dwErr);
        SetLastError(dwErr); 
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


PRIVATE
BOOL
HttpWrapSendRequest(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwOptionalLengthTotal,
    IN AR_TYPE arRequest,
    OUT HTTP_REQUEST_HANDLE_OBJECT** pHandleToDeref,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

    dwOptionalLengthTotal   - Total length need to be sent for File Upload.

    arRequest               - Which API the caller is making,
                                assumed to be HttpEndRequestA, HttpSendRequestExA, or
                                HttpSendRequestA

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

Comments:

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HttpWrapSendRequest",
                 "%#x, %.80q, %d, %#x, %d, %d, %x",
                 hRequest,
                 lpszHeaders,
                 dwHeadersLength,
                 lpOptional,
                 dwOptionalLength,
                 dwOptionalLengthTotal,
                 dwContext
                 ));

    PERF_ENTER(HttpWrapSendRequest);

    DWORD error = ERROR_SUCCESS;
    HINTERNET hRequestMapped = NULL;
    BOOL bDeref = TRUE;

    BOOL isLocal;
    BOOL isAsync = FALSE;

    if (!GlobalDataInitialized) {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    //
    // we will need the thread info for several items
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    //
    // the only FSMs that can come before this one are InternetOpenUrl() or
    // HttpSendRequest() when we are performing nested send for https://
    // tunnelling through proxy
    //

    INET_ASSERT((lpThreadInfo->Fsm == NULL)
                || (lpThreadInfo->Fsm->GetType() == FSM_TYPE_PARSE_HTTP_URL)
                || (lpThreadInfo->Fsm->GetType() == FSM_TYPE_OPEN_PROXY_TUNNEL)
                );

    INET_ASSERT( arRequest == AR_HTTP_SEND_REQUEST ||
                 arRequest == AR_HTTP_BEGIN_SEND_REQUEST ||
                 arRequest == AR_HTTP_END_SEND_REQUEST );


    //
    // map the handle
    //
    error = MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);


    if ((error != ERROR_SUCCESS) && (hRequestMapped == NULL)) {
        goto quit;
    }

    //
    // Cast it to the object that we know. We are going to do caching
    // semantics with this
    //

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequestMapped;

    //
    // set the context and handle info & reset the error variables,
    // but only if not for a ReceiveResponse call.
    //
    if (arRequest != AR_HTTP_END_SEND_REQUEST)
    {
        if (dwContext)
            pRequest->SetContext(dwContext);
        
        // We need this information to special-case for Redirects and Auth because of RR FSM changes:
        pRequest->SetWriteRequired(dwOptionalLength < dwOptionalLengthTotal);
    }

    //
    //  For a call to WinHttpSendRequest (arRequst == AR_HTTP_BEGIN_SEND_REQUEST), SetProxyTunnelingSuppressWrite()
    //will be called again setting the value to TRUE iff we really are trying to tunnel through a proxy.
    //  For a call to WinHttpReceiveResponse (arRequest == AR_HTTP_END_SEND_REQUEST), WinHttpWriteData is no longer
    //valid to be called, and so writes are suppressed by the request state.  Its appropriate to call 
    //SetProxyTunnelingSuppressWrite(FALSE) in this case to ensure the request's state logic kicks in
    //for denying the request though.
    //
    pRequest->SetProxyTunnelingSuppressWrite(FALSE);
    
    _InternetSetObjectHandle(lpThreadInfo, hRequest, hRequestMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle was invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // use RIsHandleLocal() to discover 4 things:
    //
    //  1. Handle is valid
    //  2. Handle is of expected type (HTTP Request in this case)
    //  3. Handle is local or remote
    //  4. Handle supports async I/O
    //

    error = RIsHandleLocal(hRequestMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpRequestHandle
                           );

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    if (isAsync)
    {
        error = InitializeAsyncSupport();
        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }
    }
    //
    // For SEND_REQUEST, and BEGIN_SEND_REQUEST, we need
    //  to do some basic initalization
    //

    if ( arRequest == AR_HTTP_SEND_REQUEST ||
         arRequest == AR_HTTP_BEGIN_SEND_REQUEST)
    {
        error = pRequest->InitBeginSendRequest(lpszHeaders,
                                       dwHeadersLength,
                                       &lpOptional,
                                       &dwOptionalLength,
                                       dwOptionalLengthTotal
                                       );

        if ( error != ERROR_SUCCESS)
        {
            goto quit;
        }

        // (Re)set flag to indicate WinHttpReceiveResponse needs to be called.
        pRequest->SetReceiveResponseState(FALSE);

        // RENO 35599: If sending a new request, ensure the OptionalSaved member
        // variables are cleared out.
        pRequest->ClearSavedOptionalData();
    }
    else if (arRequest == AR_HTTP_END_SEND_REQUEST)
    {
        pRequest->SetReceiveResponseState(TRUE);
    }


    //
    // send the request to the server. This may involve redirections and user
    // authentication
    //

    //error = DoFsm(New CFsm_HttpSendRequest(lpOptional, dwOptionalLength, pRequest, arRequest));
    //if (error == ERROR_IO_PENDING) {
    //    bDeref = FALSE;
    //}
    CFsm_HttpSendRequest * pFsm;

    pFsm = New CFsm_HttpSendRequest(lpOptional, dwOptionalLength, pRequest, arRequest);

    if (pFsm != NULL)
    {
        if (isAsync && !lpThreadInfo->IsAsyncWorkerThread)
        {
            error = StartFsmChain(pFsm, pRequest, TRUE, TRUE);

            if ((error == ERROR_SUCCESS)
                && pHandleToDeref)
            {
                // Deref in the outer api call for sync success in async case.
                *pHandleToDeref = pRequest;
                bDeref = FALSE;
            }
        }
        else
        {
            error = StartFsmChain(pFsm, pRequest, FALSE, FALSE);
        }
        if (error == ERROR_IO_PENDING)
        {
            bDeref = FALSE;
        }
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

quit:

    //
    // if we went async don't deref the handle
    //

    if (bDeref && (hRequestMapped != NULL)) {
        DereferenceObject((LPVOID)hRequestMapped);
    }

done:

    BOOL success = TRUE;

    // SetLastError must be called after PERF_LEAVE !!!
    PERF_LEAVE(HttpWrapSendRequest);

    if (error != ERROR_SUCCESS) 
    {
        DEBUG_ERROR(HTTP, error);
        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE(success);
    return success;
}
