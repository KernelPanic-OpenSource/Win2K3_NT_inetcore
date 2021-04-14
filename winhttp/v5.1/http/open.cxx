/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    open.cxx

Abstract:

    This file contains the implementation of the HttpOpenRequestA API.

    The following functions are exported by this module:

        HttpOpenRequestA
        WinHttpOpenRequest
        ParseHttpUrl
        ParseHttpUrl_Fsm

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    Modified to make HttpOpenRequestA remotable. madana (2/8/95)

--*/

#include <wininetp.h>
#include "httpp.h"

//
// functions
//

BOOL IsInappropriateHTTPPort (INTERNET_PORT port)
/*++
Routine Description:
The following outgoing ports should be blocked for HTTP:
21 - FTP,25 - SMTP,70 - GOPHER,110 - POP3,119 - NNTP,143 - IMAP
Arguments: Port number
Return Value: TRUE- Need to be blocked
			  FALSE-Not to be blocked
--*/
{
    if (port > INTERNET_MAX_WELL_KNOWN_PORT)
        return FALSE;
    switch (port) {
        case INTERNET_DEFAULT_FTP_PORT:
        case INTERNET_DEFAULT_GOPHER_PORT:
        case INTERNET_DEFAULT_SMTP_PORT:
        case INTERNET_DEFAULT_POP3_PORT:
        case INTERNET_DEFAULT_NNTP_PORT:
        case INTERNET_DEFAULT_IMAP_PORT:
            return TRUE;
        default:
            return FALSE;
    }
}



INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb OPTIONAL,
    IN LPCSTR lpszObjectName OPTIONAL,
    IN LPCSTR lpszVersion OPTIONAL,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Creates a new HTTP request handle and stores the specified parameters
    in that context.

Arguments:

    hConnect            - An open Internet handle returned by InternetConnect()

    lpszVerb            - The verb to use in the request. May be NULL in which
                          case "GET" will be used

    lpszObjectName      - The target object for the specified verb. This is
                          typically a file name, an executable module, or a
                          search specifier. May be NULL in which case the empty
                          string will be used

    lpszVersion         - The version string for the request. May be NULL in
                          which case "HTTP/1.0" will be used

    lpszReferrer        - Specifies the address (URI) of the document from
                          which the URI in the request (lpszObjectName) was
                          obtained. May be NULL in which case no referer is
                          specified

    lplpszAcceptTypes   - Points to a NULL-terminated array of LPCTSTR pointers
                          to content-types accepted by the client. This value
                          may be NULL in which case the default content-type
                          (text/html) is used

    dwFlags             - open options

    dwContext           - app-supplied context value for call-backs
                        - ignored in winhttp.

    BUGBUG: WHAT IS THE DEFAULT CONTENT-TRANSFER-ENCODING?

Return Value:

    HINTERNET

        Success - non-NULL (open) handle to an HTTP request

        Failure - NULL. Error status is available by calling GetLastError()

--*/

{
    UNREFERENCED_PARAMETER(dwContext);
    DEBUG_ENTER((DBG_API,
                     Handle,
                     "HttpOpenRequestA",
                     "%#x, %.80q, %.80q, %.80q, %.80q, %#x, %#08x, %#08x",
                     hConnect,
                     lpszVerb,
                     lpszObjectName,
                     lpszVersion,
                     lpszReferrer,
                     lplpszAcceptTypes,
                     dwFlags,
                     dwContext
                     ));

    DWORD error;
    HINTERNET hConnectMapped = NULL;
    HINTERNET hRequest = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();

    //
    // map the handle
    //

    error = MapHandleToAddress(hConnect, (LPVOID *)&hConnectMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // find path from internet handle and validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters. Allow lpszVerb to default to "GET" if a NULL pointer
    // is supplied
    //

    if (!ARGUMENT_PRESENT(lpszVerb) || (*lpszVerb == '\0')) {
        lpszVerb = DEFAULT_HTTP_REQUEST_VERB;
    }

    //
    // if a NULL pointer or empty string is supplied for the object name, then
    // convert to the default object name (root object)
    //

    if (!ARGUMENT_PRESENT(lpszObjectName) || (*lpszObjectName == '\0')) {
        lpszObjectName = "/";
    }

    // check the rest of the parameters
    if (dwFlags & ~WINHTTP_OPEN_REQUEST_FLAGS_MASK)
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    // default to the current supported version
    char versionBuffer[sizeof("HTTP/4294967295.4294967295")];
    DWORD verMajor;
    DWORD verMinor;

    if (!ARGUMENT_PRESENT(lpszVersion) || (*lpszVersion == '\0')) {
        wsprintf(versionBuffer,
                 "HTTP/%d.%d",
                 HttpVersionInfo.dwMajorVersion,
                 HttpVersionInfo.dwMinorVersion
                 );
        lpszVersion = versionBuffer;
        verMajor = HttpVersionInfo.dwMajorVersion;
        verMinor = HttpVersionInfo.dwMinorVersion;
    } else if (strnicmp(lpszVersion, "HTTP/", sizeof("HTTP/") - 1) == 0) {

        LPSTR p = (LPSTR)lpszVersion + sizeof("HTTP/") - 1;

        ExtractInt(&p, 0, (LPINT)&verMajor);
        while (!isdigit(*p) && (*p != '\0')) {
            ++p;
        }
        ExtractInt(&p, 0, (LPINT)&verMinor);
    } else {
        verMajor = 1;

        if (IsHttp1_1())
            verMinor = 1;
        else
            verMinor = 0;
    }

    //
    // allow empty strings to be equivalent to NULL pointer
    //

    if (ARGUMENT_PRESENT(lpszReferrer) && (*lpszReferrer == '\0')) {
        lpszReferrer = NULL;
    }

    // get the target port
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped;
    INTERNET_PORT hostPort;
    hostPort = pConnect->GetHostPort();

    // Security: Block http access to ports: 21, 25, 70, 110, 119 and 143
    if (IsInappropriateHTTPPort(hostPort))
    {
        error = ERROR_WINHTTP_INVALID_URL;
        goto quit;
    }

    //
    // set the per-thread info: parent handle object
    //

    _InternetSetObjectHandle(lpThreadInfo, hConnect, hConnectMapped);

    //
    // make local HTTP request handle object before we can add headers to it
    //

    error = RMakeHttpReqObjectHandle(hConnectMapped,
                                     &hRequest,
                                     NULL,  // (CLOSE_HANDLE_FUNC)wHttpCloseRequest
                                     dwFlags & ~INTERNET_FLAG_NO_AUTO_REDIRECT
                                     );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;

    //
    // add the request line
    //

    INET_ASSERT((lpszVerb != NULL) && (*lpszVerb != '\0'));
    INET_ASSERT((lpszObjectName != NULL) && (*lpszObjectName != '\0'));
    INET_ASSERT((lpszVersion != NULL) && (*lpszVersion != '\0'));

    if (!pRequest->LockHeaders())
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // encode the URL-path
    //

    error = pRequest->AddRequest((LPSTR)lpszVerb,
                                 (LPSTR)lpszObjectName,
                                 (LPSTR)lpszVersion
                                 );
    if (error != ERROR_SUCCESS) {
        pRequest->UnlockHeaders();
        goto quit;
    }

    //
    // set the method type from the verb
    //

    pRequest->SetMethodType(lpszVerb);

    //
    // add the headers
    //

    if (lpszReferrer != NULL) {
        error = pRequest->AddRequestHeader(HTTP_QUERY_REFERER,
                                           (LPSTR)lpszReferrer,
                                           lstrlen(lpszReferrer),
                                           0,
                                           CLEAN_HEADER
                                           );
        if (error != ERROR_SUCCESS) {
            pRequest->UnlockHeaders();
            goto quit;
        }
    }

    if (lplpszAcceptTypes != NULL) {
        while (*lplpszAcceptTypes) {
            error = pRequest->AddRequestHeader(HTTP_QUERY_ACCEPT,
                                               (LPSTR)*lplpszAcceptTypes,
                                               lstrlen(*(LPSTR*)lplpszAcceptTypes),
                                               0,
                                               CLEAN_HEADER | COALESCE_HEADER_WITH_COMMA
                                               );
            if (error != ERROR_SUCCESS) {
                pRequest->UnlockHeaders();
                goto quit;
            }
            ++lplpszAcceptTypes;
        }
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    pRequest->UnlockHeaders();

    //
    // change the object state to opened
    //

    pRequest->SetState(HttpRequestStateOpen);
    ((HTTP_REQUEST_HANDLE_OBJECT *)hRequest)->SetRequestUsingProxy(
                                                    FALSE
                                                    );

    if (hostPort == INTERNET_INVALID_PORT_NUMBER)
    {
        if (dwFlags & WINHTTP_FLAG_SECURE)
        {
            pRequest->SetHostPort(INTERNET_DEFAULT_HTTPS_PORT);
        }
        else
        {
            pRequest->SetHostPort(INTERNET_DEFAULT_HTTP_PORT);
        }
    }
    else
    {
        pRequest->SetHostPort(hostPort);
    }

    if (dwFlags & INTERNET_FLAG_NO_AUTO_REDIRECT)
    {
        pRequest->SetDwordOption(WINHTTP_OPTION_REDIRECT_POLICY, WINHTTP_OPTION_REDIRECT_POLICY_NEVER);
    }

    //
    // if the object name is not set then all cache methods fail
    //

    URLGEN_FUNC fn;
    fn = (URLGEN_FUNC)pHttpGetUrlString;

    //
    // BUGBUG - change prototype to take LPCSTR
    //

    error = pRequest->SetObjectName((LPSTR)lpszObjectName,
                                    NULL,
                                    &fn
                                    );

quit:

    _InternetDecNestingCount(1);

done:

    if (error != ERROR_SUCCESS) {
        if (hRequest != NULL) {
            ((HANDLE_OBJECT *)hRequest)->Dereference(); // release the ref claimed by this function
            WinHttpCloseHandle(((HANDLE_OBJECT *)hRequest)->GetPseudoHandle());
        }

        DEBUG_ERROR(HTTP, error);

        SetLastError(error);
        hRequest = NULL;
    } else {

        //
        // success - don't return the object address, return the pseudo-handle
        // value we generated
        //

        BOOL fDeleted = ((HANDLE_OBJECT *)hRequest)->Dereference(); // release the ref claimed by this function
        if (fDeleted)
        {
            hRequest = NULL;
            error = ERROR_WINHTTP_OPERATION_CANCELLED;
        }
        else
        {
            hRequest = ((HANDLE_OBJECT *)hRequest)->GetPseudoHandle();
        }
        
    }

    if (hConnectMapped != NULL) {
        DereferenceObject((LPVOID)hConnectMapped);
    }

    DEBUG_LEAVE(hRequest);

    return hRequest;
}


INTERNETAPI
HINTERNET
WINAPI
WinHttpOpenRequest(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Creates a new HTTP request handle and stores the specified parameters
    in that context.

Arguments:

    hHttpSession        - An open Internet handle returned by InternetConnect()

    lpszVerb            - The verb to use in the request

    lpszObjectName      - The target object for the specified verb. This is
                          typically a file name, an executable module, or a
                          search specifier

    lpszVersion         - The version string for the request

    lpszReferrer        - Specifies the address (URI) of the document from
                          which the URI in the request (lpszObjectName) was
                          obtained. May be NULL in which case no referer is
                          specified

    lplpszAcceptTypes   - Points to a NULL-terminated array of LPCTSTR pointers
                          to content-types accepted by the client. This value
                          may be NULL in which case the default content-type
                          (text/html) is used

    dwFlags             - open options

    BUGBUG: WHAT IS THE DEFAULT CONTENT-TRANSFER-ENCODING?

Return Value:

    !NULL - An open handle to an HTTP request.

    NULL - The operation failed. Error status is available by calling
        GetLastError().

Comments:

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "WinHttpOpenRequest",
                     "%#x, %.80wq, %.80wq, %.80wq, %.80wq, %#x, %#08x",
                     hConnect,
                     lpszVerb,
                     lpszObjectName,
                     lpszVersion,
                     lpszReferrer,
                     lplpszAcceptTypes,
                     dwFlags
                     ));

    HINTERNET hConnectMapped = NULL;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpVerb, mpObjectName, mpVersion, mpReferrer;
    MEMORYPACKETTABLE mptAcceptTypes;
    BOOL isLocal;
    BOOL isAsync;

    if (dwFlags &~ (WINHTTP_OPEN_REQUEST_FLAGS_MASK))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    // map the handle
    dwErr = MapHandleToAddress(hConnect, (LPVOID *)&hConnectMapped, FALSE);
    if (dwErr != ERROR_SUCCESS) 
    {
        goto cleanup;
    }

    // find path from internet handle and validate handle
    dwErr = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpConnectHandle
                           );
    if (dwErr != ERROR_SUCCESS) 
    {
        goto cleanup;
    }

    if (lpszVerb)
    {
        if (IsBadStringPtrW(lpszVerb, (UINT_PTR)-1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(lpszVerb,0,mpVerb);
        if (!mpVerb.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszVerb,mpVerb);
    }
    if (lpszObjectName)
    {
        if (IsBadStringPtrW(lpszObjectName, (UINT_PTR)-1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        pConnect = (INTERNET_CONNECT_HANDLE_OBJECT*)hConnectMapped;
        DWORD dwCodePage = pConnect->GetCodePage();
        dwErr = ConvertUnicodeToMultiByte(lpszObjectName, dwCodePage, &mpObjectName, 
                dwFlags&(
					WINHTTP_FLAG_ESCAPE_PERCENT |
					WINHTTP_FLAG_NULL_CODEPAGE |
					WINHTTP_FLAG_ESCAPE_DISABLE |
					WINHTTP_FLAG_ESCAPE_DISABLE_QUERY)); 

        if (dwErr != ERROR_SUCCESS)
        {
            goto cleanup;
        }
    }
    if (lpszVersion)
    {
        if (IsBadStringPtrW(lpszVersion, (UINT_PTR)-1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(lpszVersion,0,mpVersion);
        if (!mpVersion.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszVersion,mpVersion);
    }
    if (lpszReferrer)
    {
        if (IsBadStringPtrW(lpszReferrer, (UINT_PTR)-1))
        {
            dwErr = ERROR_INVALID_PARAMETER;
            goto cleanup;
        }
        ALLOC_MB(lpszReferrer,0,mpReferrer);
        if (!mpReferrer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszReferrer,mpReferrer);
    }

    // Create a table of ansi strings
    if (lplpszAcceptTypes)
    {
        WORD csTmp=0;
        do
        {
            if (IsBadReadPtr(lplpszAcceptTypes+csTmp*sizeof(LPCWSTR), sizeof(LPCWSTR)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            if (lplpszAcceptTypes[csTmp])
            {
                if (IsBadStringPtrW(lplpszAcceptTypes[csTmp++], (UINT_PTR)-1))
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                }
            }
            else
                break;
        }
        while (TRUE);

        mptAcceptTypes.SetUpFor(csTmp);
        for (WORD ce=0; ce < csTmp; ce++)
        {
            mptAcceptTypes.pdwAlloc[ce] = (lstrlenW(lplpszAcceptTypes[ce]) + 1)*sizeof(WCHAR);
            ALLOC_BYTES(mptAcceptTypes.ppsStr[ce], mptAcceptTypes.pdwAlloc[ce]*sizeof(CHAR));
            if (!mptAcceptTypes.ppsStr[ce])
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            mptAcceptTypes.pdwSize[ce] = WideCharToMultiByte(CP_ACP,
                                                                0,
                                                                lplpszAcceptTypes[ce],
                                                                mptAcceptTypes.pdwAlloc[ce]/sizeof(WCHAR),
                                                                mptAcceptTypes.ppsStr[ce],
                                                                mptAcceptTypes.pdwAlloc[ce],NULL,NULL);
        }
    }

    hInternet = HttpOpenRequestA(hConnect, mpVerb.psStr, mpObjectName.psStr, mpVersion.psStr,
                               mpReferrer.psStr, (LPCSTR*)mptAcceptTypes.ppsStr,
                               dwFlags, NULL);

cleanup:
    if (hConnectMapped != NULL)
    {
        DereferenceObject((LPVOID)hConnectMapped);
    }

    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(HTTP, dwErr);
    }
    
    TRACE_SET_ID_FROM_HANDLE(hInternet);
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}

