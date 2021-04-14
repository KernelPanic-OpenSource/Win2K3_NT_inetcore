/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetapia.cxx

Abstract:

    Contains the ANSI and character-mode-independent Internet APIs

    Contents:
        WinHttpCloseHandle
        WinHttpReadData
        WinHttpWriteData
        WinHttpQueryDataAvailable
        
    
        WinHttpCrackUrlA
        WinHttpCreateUrlA
        InternetCanonicalizeUrlA
        InternetCombineUrlA
        InternetOpenA
        _InternetCloseHandle
        _InternetCloseHandleNoContext
        InternetConnectA
        InternetOpenUrlA
        ReadFile_End
        InternetQueryOptionA
        InternetSetOptionA
        InternetGetLastResponseInfoA
        (wInternetCloseConnectA)
        (CreateDeleteSocket)

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

    07-Mar-1995 madana


--*/


#include <wininetp.h>
#include <perfdiag.hxx>

//  because wininet doesnt know IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>


//
// private manifests
//

//
// private prototypes
//

PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    );

PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    );

PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    );

PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    );


PRIVATE
DWORD
wInternetCloseConnectA(
    IN HINTERNET lpConnectHandle,
    IN DWORD ServiceType
    );

PRIVATE
BOOL
InternetParseCommon(
    IN LPCTSTR lpszBaseUrl,
    IN LPCTSTR lpszRelativeUrl,
    OUT LPTSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );


//
// WinHttpCrackUrlA
//


BOOL winHttpCrackUrlAVerifyStringBuffer( LPSTR szStr, DWORD dwStrSize, DWORD dwFlags)
{

    if (szStr == NULL 
        && dwStrSize != 0
        && (dwFlags & (ICU_DECODE | ICU_ESCAPE)))
    {
        return FALSE;
    }
    else
        return TRUE;
}


DWORD winHttpCrackUrlACopyResultToBuffer( LPSTR szResult, DWORD dwResultLength,
                                          LPSTR* pszBuffer, DWORD* pdwBufferSize,
                                          DWORD dwSchemeType, BOOL fDecode, BOOL fEscape, BOOL* pfCopyFailure)
{
    DWORD dwError = ERROR_WINHTTP_INTERNAL_ERROR;
    LPSTR szAlloc = NULL;
    
    if (*pszBuffer != NULL 
        && *pdwBufferSize > dwResultLength+1
        && szResult != NULL
        && szResult[0] != '\0')
    {
        memcpy(*pszBuffer, szResult, dwResultLength);
        
        (*pszBuffer)[dwResultLength] = '\0';

        if (fDecode)
        {
            HRESULT hr = UrlUnescapeInPlace((*pszBuffer), 0);
            if (FAILED(hr))
            {
                dwError = HRESULT_CODE(hr);
                goto quit;
            }
        }

        if (fEscape)
        {
            DWORD dwAllocSize = (*pdwBufferSize);
            szAlloc = (LPSTR)ALLOCATE_MEMORY(dwAllocSize);
            if(szAlloc == NULL)
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            dwError = EncodeUrlPath(0, dwSchemeType, 
                                  (*pszBuffer), strlen((*pszBuffer)) + 1, 
                                  &szAlloc, &dwAllocSize);

            //  dwAllocSize was the size of the buffer going in,
            //but after calling EncodeUrlPath it is now the length (without '\0')
            //of the result.

            if(dwError != ERROR_SUCCESS)
                goto quit;
                                  
            if(dwAllocSize + 1 <= (*pdwBufferSize))
            {
                memcpy((*pszBuffer), szAlloc, dwAllocSize+1);
                (*pdwBufferSize) = dwAllocSize;  // if we had enough space, return length not including '\0'
            }
            else
            {
                (*pszBuffer)[0] = '\0';
                (*pdwBufferSize) = dwAllocSize+1;
                *pfCopyFailure = TRUE;  // if we didn't have enough space, ask for enough to include '\0'
            }
        }
        else
        {
            (*pdwBufferSize) = strlen((*pszBuffer));
        }
    } 
    else if (*pszBuffer != NULL)
    {
        //  If there wasn't room for the copy, return an empty string
        //with the size parameter returning a sufficient size to store
        //the full result.
        (*pszBuffer)[0] = '\0';

        if (dwResultLength > 0)
        {
            DWORD dwPossibleExpansion = fEscape ? 3 : 1;
            (*pdwBufferSize) = dwResultLength * dwPossibleExpansion + 1;
            *pfCopyFailure = TRUE;
        }
        else
        {
            *pdwBufferSize = 0;
        }
    }        
    else if ((*pdwBufferSize) != 0)
    {
        //  *pswBuffer == NULL && *pdwBufferSize != 0 indicates
        //user wants a pointer to the result in the original string.  This
        //also indicates WinHttpCrackUrlA called CrackUrl on the
        //original string.
        *pszBuffer = szResult;
        *pdwBufferSize = dwResultLength;
    }

    dwError = ERROR_SUCCESS;
quit:
    if (szAlloc != NULL)
        DEL_STRING(szAlloc);
    
    return dwError;
}
    


INTERNETAPI
BOOL
WINAPI
WinHttpCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN LPURL_COMPONENTSA lpUrlComponents
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts. Optionally escapes the url-path.
    We assume that the user has supplied large enough buffers for the various
    URL parts

Arguments:

    lpszUrl         - pointer to URL to crack

    dwUrlLength     - 0 if lpszUrl is ASCIIZ string, else length of lpszUrl

    dwFlags         - flags controlling operation

    lpUrlComponents - pointer to URL_COMPONENTS

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCrackUrlA",
                     "%q, %#x, %#x, %#x",
                     lpszUrl,
                     dwUrlLength,
                     dwFlags,
                     lpUrlComponents
                     ));

    DWORD error;

    LPSTR lpUrl;
    LPSTR urlCopy = NULL;
    //
    // validate parameters
    //
    if (!dwUrlLength)
        dwUrlLength = lstrlen(lpszUrl);

    if (!winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszScheme, lpUrlComponents->dwSchemeLength, dwFlags)
        || !winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszHostName, lpUrlComponents->dwHostNameLength, dwFlags)
        || !winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszUserName, lpUrlComponents->dwUserNameLength, dwFlags)
        || !winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszPassword, lpUrlComponents->dwPasswordLength, dwFlags)
        || !winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszUrlPath, lpUrlComponents->dwUrlPathLength, dwFlags)
        || !winHttpCrackUrlAVerifyStringBuffer(lpUrlComponents->lpszExtraInfo, lpUrlComponents->dwExtraInfoLength, dwFlags))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }
   
    //
    //  Below are variables to be used as out params with CrackUrl()
    //

    LPSTR schemeName = NULL;
    DWORD schemeNameLength = 0;
    LPSTR hostName = NULL;
    DWORD hostNameLength = 0;
    LPSTR userName = NULL;
    DWORD userNameLength = 0;
    LPSTR password = NULL;
    DWORD passwordLength = 0;
    LPSTR urlPath = NULL;
    DWORD urlPathLength = 0;
    LPSTR extraInfo = NULL;
    DWORD extraInfoLength = 0;

    INTERNET_SCHEME schemeType;
    INTERNET_PORT nPort;
    BOOL havePort;
    
    //
    //  If we're escaping or decoding, create a separate work buffer
    //because we can't do it in place.
    //
    if (dwFlags & (ICU_ESCAPE | ICU_DECODE))
    {
        //
        // create a copy of the URL. CrackUrl() will modify this in situ. We
        // need to copy the results back to the user's buffer(s)
        //

        urlCopy = NewString((LPSTR)lpszUrl, dwUrlLength);
        if (urlCopy == NULL)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        lpUrl = urlCopy;
    }
    else
    {
        lpUrl = (LPSTR)lpszUrl;
        urlCopy = NULL;
    }

    //
    // crack the URL into its constituent parts
    //

    error = CrackUrl(lpUrl,
                     dwUrlLength,
                     FALSE,
                     &schemeType,
                     &schemeName,
                     &schemeNameLength,
                     &hostName,
                     &hostNameLength,
                     FALSE,
                     &nPort,
                     &userName,
                     &userNameLength,
                     &password,
                     &passwordLength,
                     &urlPath,
                     &urlPathLength,
                     lpUrlComponents->dwExtraInfoLength ? &extraInfo : NULL,
                     lpUrlComponents->dwExtraInfoLength ? &extraInfoLength : 0,
                     &havePort
                     );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }


    //
    //  Transfer the results from CrackUrl() to lpUrlComponents
    //

    BOOL fCopyFailure;
    fCopyFailure = FALSE;

    error = winHttpCrackUrlACopyResultToBuffer(
                schemeName, schemeNameLength,
                &lpUrlComponents->lpszScheme, &lpUrlComponents->dwSchemeLength,
                schemeType, dwFlags & ICU_DECODE, FALSE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    error = winHttpCrackUrlACopyResultToBuffer(
                hostName, hostNameLength,
                &lpUrlComponents->lpszHostName, &lpUrlComponents->dwHostNameLength,
                schemeType, dwFlags & ICU_DECODE, FALSE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    error = winHttpCrackUrlACopyResultToBuffer(
                userName, userNameLength,
                &lpUrlComponents->lpszUserName, &lpUrlComponents->dwUserNameLength,
                schemeType, dwFlags & ICU_DECODE, FALSE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    error = winHttpCrackUrlACopyResultToBuffer(
                password, passwordLength,
                &lpUrlComponents->lpszPassword, &lpUrlComponents->dwPasswordLength,
                schemeType, dwFlags & ICU_DECODE, FALSE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    error = winHttpCrackUrlACopyResultToBuffer(
                urlPath, urlPathLength,
                &lpUrlComponents->lpszUrlPath, &lpUrlComponents->dwUrlPathLength,
                schemeType, dwFlags & ICU_DECODE, dwFlags & ICU_ESCAPE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    error = winHttpCrackUrlACopyResultToBuffer(
                extraInfo, extraInfoLength,
                &lpUrlComponents->lpszExtraInfo, &lpUrlComponents->dwExtraInfoLength,
                schemeType, dwFlags & ICU_DECODE, dwFlags & ICU_ESCAPE, &fCopyFailure);

    if (error != ERROR_SUCCESS)
        goto quit;
                
    //
    // we may have failed to copy one or more components because we didn't have
    // enough buffer space.
    //

    if (fCopyFailure)
    {
        error = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // copy the scheme type
    //

    lpUrlComponents->nScheme = schemeType;

    //
    // convert 0 port (not in URL) to default value for scheme
    //

    if (nPort == INTERNET_INVALID_PORT_NUMBER && !havePort) {
        switch (schemeType) {
        case INTERNET_SCHEME_HTTP:
            nPort = INTERNET_DEFAULT_HTTP_PORT;
            break;

        case INTERNET_SCHEME_HTTPS:
            nPort = INTERNET_DEFAULT_HTTPS_PORT;
            break;
        }
    }
    lpUrlComponents->nPort = nPort;

quit:
    if (urlCopy != NULL)
    {
        DEL_STRING(urlCopy);
    }

    BOOL success = (error==ERROR_SUCCESS);

    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}


INTERNETAPI
BOOL
WINAPI
WinHttpCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl OPTIONAL,
    IN OUT LPDWORD lpdwUrlLength
    )

/*++

Routine Description:

    Creates an URL from its constituent parts

Arguments:

    lpUrlComponents - pointer to URL_COMPONENTS structure containing pointers
                      and lengths of components of interest

    dwFlags         - flags controlling function:

                        ICU_ESCAPE  - the components contain characters that
                                      must be escaped in the output URL

    lpszUrl         - pointer to buffer where output URL will be written

    lpdwUrlLength   - IN: number of bytes in lpszUrl buffer
                      OUT: if success, number of characters in lpszUrl, else
                           number of bytes required for buffer

Return Value:

    BOOL
        Success - URL written to lpszUrl

        Failure - call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCreateUrlA",
                     "%#x, %#x, %#x, %#x",
                     lpUrlComponents,
                     dwFlags,
                     lpszUrl,
                     lpdwUrlLength
                     ));

#if INET_DEBUG

    LPSTR lpszUrlOriginal = lpszUrl;

#endif

    DWORD error = ERROR_SUCCESS;
    LPSTR encodedUrlPath = NULL;
    LPSTR encodedExtraInfo = NULL;

    //
    // validate parameters
    //

    if (!ARGUMENT_PRESENT(lpszUrl)) {
        *lpdwUrlLength = 0;
    }

    //
    // allocate large buffers from heap
    //

    encodedUrlPath = (LPSTR)ALLOCATE_MEMORY(INTERNET_MAX_URL_LENGTH + 1);
    encodedExtraInfo = (LPSTR)ALLOCATE_MEMORY(INTERNET_MAX_URL_LENGTH + 1);
    if ((encodedUrlPath == NULL) || (encodedExtraInfo == NULL)) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // if we get an exception, we return ERROR_INVALID_PARAMETER
    //
    
    __try {

        //
        // get the individual components to copy
        //

        LPSTR schemeName;
        DWORD schemeNameLength;
        DWORD schemeFlags;
        LPSTR hostName;
        DWORD hostNameLength;
        BOOL  bracketsNeeded;
        INTERNET_PORT nPort = 0;
        DWORD portLength;
        LPSTR userName;
        DWORD userNameLength;
        LPSTR password;
        DWORD passwordLength;
        LPSTR urlPath;
        DWORD urlPathLength;
        DWORD extraLength;
        DWORD encodedUrlPathLength;
        LPSTR extraInfo = NULL;
        DWORD extraInfoLength = 0;
        DWORD encodedExtraInfoLength;
        LPSTR schemeSep;
        DWORD schemeSepLength;
        INTERNET_SCHEME schemeType;
        INTERNET_PORT defaultPort;

        //
        // if the scheme name is absent then we use the default
        //

        schemeName = lpUrlComponents->lpszScheme;
        schemeType = lpUrlComponents->nScheme;

        if (schemeName == NULL) {
            if (schemeType == INTERNET_SCHEME_DEFAULT){
                schemeName = DEFAULT_URL_SCHEME_NAME;
                schemeNameLength = sizeof(DEFAULT_URL_SCHEME_NAME) - 1;
            }
            else {
                schemeName = MapUrlScheme(schemeType, &schemeNameLength);
            }
        } else {
            schemeNameLength = lpUrlComponents->dwSchemeLength;
            if (schemeNameLength == 0) {
                schemeNameLength = lstrlen(schemeName);
            }
        }

        if (schemeNameLength == 0)
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        

        //
        // doesn't have to be a host name
        //

        hostName = lpUrlComponents->lpszHostName;
        portLength = 0;
        if (hostName != NULL) {
            hostNameLength = lpUrlComponents->dwHostNameLength;
            if (hostNameLength == 0) {
                hostNameLength = lstrlen(hostName);
            }

            //
            // If a hostname was supplied and it contains at least two colons then
            // it will be assumed to be an IPv6 literal address.  To comply with RFC 2732 
            // it will need square brackets around it when we construct the URL string.
            //
            DWORD colonCount, index;
            bracketsNeeded = FALSE;
            for (colonCount = 0, index = 0; index < hostNameLength; index++) {
                if (hostName[index] == ':') {
                    colonCount++;
                }
                if (colonCount > 1) {
                    bracketsNeeded = TRUE;
                    break;
                }
            }
            
            // Don't add square brackets to IPv6 literal if they are already there.
            
            if (bracketsNeeded && (hostName[0] == '[') && (hostName[hostNameLength-1] == ']')) {
                    bracketsNeeded = FALSE;
            }
            
        //
        // if the port is default then we don't add it to the URL, else we need to
        // copy it as a string
        //
        // there won't be a port unless there's host.

            schemeType = MapUrlSchemeName(schemeName, schemeNameLength ? schemeNameLength : -1);
            switch (schemeType) {
            case INTERNET_SCHEME_HTTP:
                defaultPort = INTERNET_DEFAULT_HTTP_PORT;
                break;

            case INTERNET_SCHEME_HTTPS:
                defaultPort = INTERNET_DEFAULT_HTTPS_PORT;
                break;

            default:
                defaultPort = INTERNET_INVALID_PORT_NUMBER;
                break;
            }

            if (lpUrlComponents->nPort != defaultPort) {

                INTERNET_PORT divisor;

                nPort = lpUrlComponents->nPort;
                if (nPort) {
                    divisor = 10000;
                    portLength = 6; // max is 5 characters, plus 1 for ':'
                    while ((nPort / divisor) == 0) {
                        --portLength;
                        divisor /= 10;
                    }
                } else {
                    portLength = 2;         // port is ":0"
                }
            }
        } else {
            hostNameLength = 0;
            bracketsNeeded = FALSE;
        }


        //
        // doesn't have to be a user name
        //

        userName = lpUrlComponents->lpszUserName;
        if (userName != NULL) {
            userNameLength = lpUrlComponents->dwUserNameLength;
            if (userNameLength == 0) {
                userNameLength = lstrlen(userName);
            }
        } else {

            userNameLength = 0;
        }

        //
        // doesn't have to be a password
        //

        password = lpUrlComponents->lpszPassword;
        if (password != NULL) {
            passwordLength = lpUrlComponents->dwPasswordLength;
            if (passwordLength == 0) {
                passwordLength = lstrlen(password);
            }
        } else {

            passwordLength = 0;
        }

        //
        // but if there's a password without a user name, then its an error
        //

        if (password && !userName) {
            error = ERROR_INVALID_PARAMETER;
        } else {

            //
            // determine the scheme type for possible uses below
            //

            schemeFlags = 0;
            if (strnicmp(schemeName, "http", schemeNameLength) == 0) {
                schemeFlags = SCHEME_HTTP;
            } else if (strnicmp(schemeName, "ftp", schemeNameLength) == 0) {
                schemeFlags = SCHEME_FTP;
            } else if (strnicmp(schemeName, "gopher", schemeNameLength) == 0) {
                schemeFlags = SCHEME_GOPHER;
            }

            //
            // doesn't have to be an URL-path. Empty string is default
            //

            urlPath = lpUrlComponents->lpszUrlPath;
            if (urlPath != NULL) {
                urlPathLength = lpUrlComponents->dwUrlPathLength;
                if (urlPathLength == 0) {
                    urlPathLength = lstrlen(urlPath);
                }
                if ((*urlPath != '/') && (*urlPath != '\\')) {
                    extraLength = 1;
                } else {
                    extraLength = 0;
                }

                //
                // if requested, we will encode the URL-path
                //

                if (dwFlags & ICU_ESCAPE) {

                    //
                    // only encode the URL-path if it's a recognized scheme
                    //

                    if (schemeFlags != 0) {
                        encodedUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
                        error = EncodeUrlPath(NO_ENCODE_PATH_SEP,
                                              schemeFlags,
                                              urlPath,
                                              urlPathLength,
                                              &encodedUrlPath,
                                              &encodedUrlPathLength
                                              );
                        if (error == ERROR_SUCCESS) {
                            urlPath = encodedUrlPath;
                            urlPathLength = encodedUrlPathLength;
                        }
                    }
                }
            } else {
                urlPathLength = 0;
                extraLength = 0;
            }

            //
            // handle extra info if present
            //

            if (error == ERROR_SUCCESS) {
                extraInfo = lpUrlComponents->lpszExtraInfo;
                if (extraInfo != NULL) {
                    extraInfoLength = lpUrlComponents->dwExtraInfoLength;
                    if (extraInfoLength == 0) {
                        extraInfoLength = lstrlen(extraInfo);
                    }

                    //
                    // if requested, we will encode the extra info
                    //

                    if (dwFlags & ICU_ESCAPE) {

                        //
                        // only encode the extra info if it's a recognized scheme
                        //

                        if (schemeFlags != 0) {
                            encodedExtraInfoLength = INTERNET_MAX_URL_LENGTH + 1;
                            error = EncodeUrlPath(0,
                                                  schemeFlags,
                                                  extraInfo,
                                                  extraInfoLength,
                                                  &encodedExtraInfo,
                                                  &encodedExtraInfoLength
                                                  );
                            if (error == ERROR_SUCCESS) {
                                extraInfo = encodedExtraInfo;
                                extraInfoLength = encodedExtraInfoLength;
                            }
                        }
                    }
                } else {
                    extraInfoLength = 0;
                }
            }

            DWORD requiredSize = 0;

            if (error == ERROR_SUCCESS) {

                //
                // Determine if we have a protocol scheme that requires slashes
                //

                if (DoesSchemeRequireSlashes(schemeName, schemeNameLength, (hostName != NULL))) {
                    schemeSep = "://";
                    schemeSepLength = sizeof("://") - 1;
                } else {
                    schemeSep = ":";
                    schemeSepLength = sizeof(":") - 1;
                }

                //
                // ensure we have enough buffer space
                //

                requiredSize = schemeNameLength
                             + schemeSepLength
                             + hostNameLength
                             + (bracketsNeeded ? 2 : 0)
                             + portLength
                             + (userName ? userNameLength + 1 : 0) // +1 for '@'
                             + (password ? passwordLength + 1 : 0) // +1 for ':'
                             + urlPathLength
                             + extraLength
                             + extraInfoLength
                             + 1                                // +1 for '\0'
                             ;

                //
                // if there is enough buffer, copy the URL
                //

                if (*lpdwUrlLength >= requiredSize) {
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeName, schemeNameLength);
                    lpszUrl += schemeNameLength;
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeSep, schemeSepLength);
                    lpszUrl += schemeSepLength;
                    if (userName) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)userName, userNameLength);
                        lpszUrl += userNameLength;
                        if (password) {
                            *lpszUrl++ = ':';
                            memcpy((LPVOID)lpszUrl, (LPVOID)password, passwordLength);
                            lpszUrl += passwordLength;
                        }
                        *lpszUrl++ = '@';
                    }
                    if (hostName) {
                        if (bracketsNeeded)
                            *lpszUrl++ = '[';
                        memcpy((LPVOID)lpszUrl, (LPVOID)hostName, hostNameLength);
                        lpszUrl += hostNameLength;
                        if (bracketsNeeded)
                            *lpszUrl++ = ']';
                        // We won't attach a port unless there's a host to go with it.
                        if (portLength) {
                            lpszUrl += wsprintf(lpszUrl, ":%d", nPort & 0xffff);
                        }

                    }
                    if (urlPath) {

                        //
                        // Only do extraLength if we've actually copied something
                        // after the scheme.
                        //

                        if (extraLength != 0 && (userName || hostName || portLength)) {
                            *lpszUrl++ = '/';
                        } else if (extraLength != 0) {
                            --requiredSize;
                        }
                        memcpy((LPVOID)lpszUrl, (LPVOID)urlPath, urlPathLength);
                        lpszUrl += urlPathLength;
                    } else if (extraLength != 0) {
                        --requiredSize;
                    }
                    if (extraInfo) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)extraInfo, extraInfoLength);
                        lpszUrl += extraInfoLength;
                    }

                    //
                    // terminate string
                    //

                    *lpszUrl = '\0';

                    //
                    // -1 for terminating '\0'
                    //

                    --requiredSize;
                } else {

                    //
                    // not enough buffer space - just return the required buffer length
                    //

                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            }

            //
            // update returned parameters
            //

            *lpdwUrlLength = requiredSize;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT
quit:

    //
    // clear up the buffers we allocated
    //


    if (encodedUrlPath != NULL) {
        FREE_MEMORY(encodedUrlPath);
    }
    if (encodedExtraInfo != NULL) {
        FREE_MEMORY(encodedExtraInfo);
    }

    BOOL success = (error==ERROR_SUCCESS);

    if (success) {

        DEBUG_PRINT_API(API,
                        INFO,
                        ("URL = %q\n",
                        lpszUrlOriginal
                        ));
    } else {

        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}

//
//  ICUHrToWin32Error() is specifically for converting the return codes for
//  Url* APIs in shlwapi into win32 errors.
//  WARNING:  it should not be used for any other purpose.
//
DWORD
ICUHrToWin32Error(HRESULT hr)
{
    DWORD err = ERROR_INVALID_PARAMETER;
    switch(hr)
    {
    case E_OUTOFMEMORY:
        err = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case E_POINTER:
        err = ERROR_INSUFFICIENT_BUFFER;
        break;

    case S_OK:
        err = ERROR_SUCCESS;
        break;

    default:
        break;
    }
    return err;
}


INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszUrl             - pointer to URL to be canonicalize
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER((DBG_API,
                     Bool,
                     "InternetCanonicalizeUrlA",
                     "%q, %#x, %#x [%d], %#x",
                     lpszUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet = TRUE;;

    INET_ASSERT(lpszUrl);
    INET_ASSERT(lpszBuffer);
    INET_ASSERT(lpdwBufferLength && (*lpdwBufferLength > 0));

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszUrl || !lpszBuffer || !lpdwBufferLength || *lpdwBufferLength == 0 || IsBadWritePtr(lpszBuffer, *lpdwBufferLength*sizeof(CHAR)))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCanonicalizeA(lpszUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }

    DEBUG_LEAVE(bRet);

    return bRet;
}


INTERNETAPI
BOOL
WINAPI
InternetCombineUrlA(
    IN LPCSTR lpszBaseUrl,
    IN LPCSTR lpszRelativeUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszBaseUrl         - pointer to base URL
    lpszRelativeUrl     - pointer to relative URL
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER((DBG_API,
                     Bool,
                     "InternetCombineUrlA",
                     "%q, %q, %#x, %#x [%d], %#x",
                     lpszBaseUrl,
                     lpszRelativeUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet;

    INET_ASSERT(lpszBaseUrl);
    INET_ASSERT(lpszRelativeUrl);
    INET_ASSERT(lpdwBufferLength);

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszBaseUrl || !lpszRelativeUrl || !lpdwBufferLength || (lpszBuffer && IsBadWritePtr(lpszBuffer, *lpdwBufferLength*sizeof(CHAR))))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCombineA(lpszBaseUrl, lpszRelativeUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }
    else
        bRet = TRUE;

    IF_DEBUG_CODE() {
        if (bRet) {
            DEBUG_PRINT_API(API,
                            INFO,
                            ("URL = %q\n",
                            lpszBuffer
                            ));
        }
    }

    DEBUG_LEAVE(bRet);

    return bRet;
}


INTERNETAPI
HINTERNET
WINAPI
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Opens a root Internet handle from which all HINTERNET objects are derived

Arguments:

    lpszAgent       - name of the application making the request (arbitrary
                      identifying string). Used in "User-Agent" header when
                      communicating with HTTP servers, if the application does
                      not add a User-Agent header of its own

    dwAccessType    - type of access required. Can be

                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY
                            - Gets the configuration from the registry

                        WINHTTP_ACCESS_TYPE_NO_PROXY
                            - Requests are made directly to the nominated server

                        WINHTTP_ACCESS_TYPE_NAMED_PROXY
                            - Requests are made via the nominated proxy


    lpszProxy       - if INTERNET_OPEN_TYPE_PROXY, a list of proxy servers to
                      use

    lpszProxyBypass - if INTERNET_OPEN_TYPE_PROXY, a list of servers which we
                      will communicate with directly

    dwFlags         - flags to control the operation of this API or potentially
                      all APIs called on the handle generated by this API.
                      Currently supported are:

                        WINHTTP_FLAG_ASYNC - Not supported in WinHttpX v6.


Return Value:

    HINTERNET
        Success - handle of Internet object

        Failure - NULL. For more information, call GetLastError()

--*/

{
    PERF_INIT();

    DEBUG_ENTER((DBG_API,
                     Handle,
                     "InternetOpenA",
                     "%q, %s (%d), %q, %q, %#x",
                     lpszAgent,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     lpszProxy,
                     lpszProxyBypass,
                     dwFlags
                     ));

    DWORD error;
    HINTERNET hInternet = NULL;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // validate parameters
    //

    if (!(   (dwAccessType == WINHTTP_ACCESS_TYPE_DEFAULT_PROXY)
          || (dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
          || (   (dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
              && (ARGUMENT_PRESENT(lpszProxy))
              && (*lpszProxy != '\0'))))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if( 0 != (dwFlags & ~WINHTTP_OPEN_FLAGS_MASK))
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }


    INTERNET_HANDLE_OBJECT * lpInternet;

    lpInternet = New INTERNET_HANDLE_OBJECT(lpszAgent,
                                            dwAccessType,
                                            (LPSTR)lpszProxy,
                                            (LPSTR)lpszProxyBypass,
                                            dwFlags
                                            );
    if (lpInternet == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }
    error = lpInternet->GetStatus();
    if (error == ERROR_SUCCESS) {
        hInternet = (HINTERNET)lpInternet;

        //
        // success - don't return the object address, return the pseudo-handle
        // value we generated
        //

        hInternet = ((HANDLE_OBJECT *)hInternet)->GetPseudoHandle();
        
    } else {

        //
        // hack fix to stop InternetIndicateStatus (called from the handle
        // object destructor) blowing up if there is no handle object in the
        // thread info block. We can't call back anyway
        //

        LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

        if (lpThreadInfo) {

            //
            // BUGBUG - incorrect handle value
            //

            _InternetSetObjectHandle(lpThreadInfo, lpInternet, lpInternet);
        }

        //
        // we failed during initialization. Kill the handle using Dereference()
        // (in order to stop the debug version complaining about the reference
        // count not being 0. Invalidate for same reason)
        //

        lpInternet->Invalidate();
        lpInternet->Dereference();

        INET_ASSERT(hInternet == NULL);

    }

quit:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
    }

    DEBUG_LEAVE(hInternet);

    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
WinHttpCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Closes any open internet handle object

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpCloseHandle",
                     "%#x",
                     hInternet
                     ));

    PERF_ENTER(InternetCloseHandle);

    DWORD error;
    BOOL success = FALSE;
    HINTERNET hInternetMapped = NULL;

    INTERNET_THREAD_INFO    LocalThreadInfoOnStack;
    LPINTERNET_THREAD_INFO  lpThreadInfo = NULL;


    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    lpThreadInfo = InternetGetThreadInfo();

    // If allocation of the thread info object failed, then
    // use a threadinfo object preallocated on our stack.
    if (!lpThreadInfo)
    {
        lpThreadInfo = InternetCreateThreadInfo(TRUE, &LocalThreadInfoOnStack);
        INET_ASSERT(lpThreadInfo->fStaticAllocation);
    }


    //
    // map the handle. Don't invalidate it (_InternetCloseHandle() does this)
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped == NULL) {

            //
            // the handle never existed or has been completely destroyed
            //

            DEBUG_PRINT(API,
                        ERROR,
                        ("Handle %#x is invalid\n",
                        hInternet
                        ));

            //
            // catch invalid handles - may help caller
            //

            DEBUG_BREAK(INVALID_HANDLES);

        } else {

            //
            // this handle is already being closed (it's invalidated). We only
            // need one InternetCloseHandle() operation to invalidate the handle.
            // All other threads will simply dereference the handle, and
            // eventually it will be destroyed
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }
        goto quit;
    }

    //
    // the handle is not invalidated
    //

    HANDLE_OBJECT * pHandle;

    pHandle = (HANDLE_OBJECT *)hInternetMapped;

    DEBUG_PRINT(INET,
                INFO,
                ("handle %#x == %#x == %s\n",
                hInternet,
                hInternetMapped,
                InternetMapHandleType(pHandle->GetHandleType())
                ));

    //
    // clear the handle object last error variables
    //

    InternetClearLastError();

    //
    // decrement session count here rather than in destructor, since 
    // the session is ref-counted and there may still be outstanding
    // references from request/connect handles on async fsms.
    //
    if (pHandle->GetHandleType() == TypeInternetHandle)
    {
        InterlockedDecrement(&g_cSessionCount);
    }

    //
    // remove the reference added by MapHandleToAddress(), or the handle won't
    // be destroyed by _InternetCloseHandle()
    //

    DereferenceObject((LPVOID)hInternetMapped);

    //
    // use _InternetCloseHandle() to do the work
    //

    success = _InternetCloseHandle(hInternet);

quit:

    // SetLastError must be called after PERF_LEAVE !!!
    PERF_LEAVE(InternetCloseHandle);

    //
    // If the local stack threadinfo object is in use,
    // then clear out the pointer from TLS here. So when this thread
    // detaches, the InternetDestroyThreadInfo called
    // from DLLMain(DLL_THREAD_DETACH) will have no effect--it will
    // not find a threadinfo object in TLS on this thread.
    //
    if (lpThreadInfo == &LocalThreadInfoOnStack)
    {
        InternetSetThreadInfo(NULL);
    }

    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


BOOL
_InternetCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as InternetCloseHandle() except does not clear out the last error text.
    Mainly for FTP

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandle",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    BOOL success;
    HINTERNET hInternetMapped = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {
        if (InDllCleanup) {
            error = ERROR_WINHTTP_SHUTDOWN;
        } else {

            INET_ASSERT(FALSE);

            error = ERROR_WINHTTP_INTERNAL_ERROR;
        }
        goto quit;
    }

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_BASE *)hInternetMapped)->AbortSocket();

    //
    // we need the parent handle - we will set this as the handle object being
    // processed by this thread. This is required for async worker threads (see
    // below)
    //

    HINTERNET hParent = NULL;
    HINTERNET hParentMapped;
    DWORD_PTR dwParentContext;

    hParentMapped = ((HANDLE_OBJECT *)hInternetMapped)->GetParent();
    if (hParentMapped != NULL) {
        hParent = ((HANDLE_OBJECT *)hParentMapped)->GetPseudoHandle();
        dwParentContext = ((HANDLE_OBJECT *)hParentMapped)->GetContext();
    }

    //
    // set the object handle in the per-thread data structure
    //

    _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

    //
    // now set the object to be the parent. This is necessary for e.g.
    // FtpGetFile() and async requests (where the async worker thread will make
    // an extra callback to deliver the results of the async request)
    //

    if (hParentMapped != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hParent, hParentMapped);
    }

    //
    // if the handle was still alive after dereferencing it then we will inform
    // the app that the close is pending
    //

quit:

    success = (error==ERROR_SUCCESS);
    if (!success) {
        SetLastError(error);
        DEBUG_ERROR(INET, error);
    }
    DEBUG_LEAVE(success);
    return success;
}


DWORD
_InternetCloseHandleNoContext(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as _InternetCloseHandle() except does not change the per-thread info
    structure handle/context values

    BUGBUG - This should be handled via a parameter to _InternetCloseHandle(),
             but its close to shipping...

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandleNoContext",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    HINTERNET hInternetMapped = NULL;

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_BASE *)hInternetMapped)->AbortSocket();

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

quit:

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
HINTERNET
WINAPI
InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Opens a connection with a server, logging-on the user in the process.

Arguments:

    hInternet       - Internet handle, returned by InternetOpen()

    lpszServerName  - name of server with which to connect

    nServerPort     - port at which server listens

    dwFlags         - protocol-specific flags. The following are defined:
                        - INTERNET_FLAG_KEEP_CONNECTION (HTTP)
                        - WINHTTP_FLAG_SECURE (HTTP)

    dwContext       - application-supplied value used to identify this
                      request in callbacks
                    - ignored in WinHttp

Return Value:

    HINTERNET
        Success - address of a new handle object

        Failure - NULL. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER((DBG_API,
                     Handle,
                     "InternetConnectA",
                     "%#x, %q, %d, %#08x, %#x",
                     hInternet,
                     lpszServerName,
                     nServerPort,
                     dwFlags,
                     dwContext
                     ));

    HINTERNET connectHandle = NULL;
    HINTERNET hInternetMapped = NULL;

    LPINTERNET_THREAD_INFO lpThreadInfo;

    INTERNET_CONNECT_HANDLE_OBJECT * pConnect = NULL;

    BOOL isAsync;

    DWORD error = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER(dwContext);

    if (!GlobalDataInitialized) {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    
    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hInternetMapped == NULL)) {
        goto quit;
    }

    //
    // set the info and clear the last error info
    //

    _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle object is invalidated
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate the handle & discover sync/async
    //

    error = RIsHandleLocal(hInternetMapped,
                            NULL,
                            &isAsync,
                            TypeInternetHandle
                            );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we allow all valid flags to be passed in
    //

    if ((dwFlags & ~WINHTTP_CONNECT_FLAGS_MASK)
        || (lpszServerName == NULL)
        || (*lpszServerName == '\0')) 
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // app thread or in async worker thread but being called from another
    // async API, such as InternetOpenUrl()
    //

    INET_ASSERT(connectHandle == NULL);
    INET_ASSERT(error == ERROR_SUCCESS);
        
    error = RMakeInternetConnectObjectHandle(
                hInternetMapped,
                &connectHandle,
                (LPSTR) lpszServerName,
                nServerPort,
                dwFlags
                );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // this new handle will be used in callbacks
    //

    _InternetSetObjectHandle(lpThreadInfo,
                                ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle(),
                                connectHandle
                                );

    //
    // based on whether we have been asked to perform async I/O AND we are not
    // in an async worker thread context AND the request is to connect with an
    // FTP service (currently only FTP because this request performs network
    // I/O - gopher and HTTP just allocate & fill in memory) AND there is a
    // valid context value, we will queue the async request, or execute the
    // request synchronously
    //

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle;
    

    INET_ASSERT(error == ERROR_SUCCESS);

quit:

    _InternetDecNestingCount(1);


done:

    if (error == ERROR_SUCCESS) {

        //
        // success - return generated pseudo-handle
        //

        BOOL fDeleted = ((HANDLE_OBJECT *)connectHandle)->Dereference(); // release the ref claimed by the InternetConnectA() API
        if (fDeleted)
        {
            connectHandle = NULL;
            error = ERROR_WINHTTP_OPERATION_CANCELLED;
        }
        else
        {
            connectHandle = ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle();
        }
    }

    if (hInternetMapped != NULL) {
        DereferenceObject((LPVOID)hInternetMapped);
    }
    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }
    DEBUG_LEAVE(connectHandle);
    return connectHandle;
}



INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    // this is dead code
    UNREFERENCED_PARAMETER(hInternet);
    UNREFERENCED_PARAMETER(lpszUrl);
    UNREFERENCED_PARAMETER(lpszHeaders);
    UNREFERENCED_PARAMETER(dwHeadersLength);
    UNREFERENCED_PARAMETER(dwFlags);
    UNREFERENCED_PARAMETER(dwContext);

    return FALSE;
}



INTERNETAPI
BOOL
WINAPI
WinHttpReadData(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    This functions reads the next block of data from the file object.

Arguments:

    hFile                   - handle returned from Open function

    lpBuffer                - pointer to caller's buffer

    dwNumberOfBytesToRead   - size of lpBuffer in BYTEs

    lpdwNumberOfBytesRead   - returned number of bytes read into lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpReadData",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    HINTERNET hFileMapped = NULL;
    DWORD bytesRead = 0;
    BOOL bEndRead = TRUE;
    BOOL b2ndDeref = FALSE;
    BOOL isAsync = FALSE;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {
        INET_ASSERT(FALSE);
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    _InternetIncNestingCount();
    nestingLevel = 1;

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    // set the handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed

    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        PERF_LOG(PE_CLIENT_REQUEST_START,
                 AR_INTERNET_READ_FILE,
                 lpThreadInfo->ThreadId,
                 hFile
                 );
    }

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    // validate parameters
    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        if (lpdwNumberOfBytesRead)
        {
            error = ProbeAndSetDword(lpdwNumberOfBytesRead, 0);
            if (error != ERROR_SUCCESS)
            {
                goto quit;
            }
        }
        error = ProbeWriteBuffer(lpBuffer, dwNumberOfBytesToRead);
        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }

        // *lpdwNumberOfBytesRead = 0;

    } // end if (!lpThreadInfo->IsAsyncWorkerThread)


    INET_ASSERT(error == ERROR_SUCCESS);

    // just call the underlying API: return whatever it returns, and let it
    // handle setting the last error

    CFsm_ReadFile *pFsm;

    pFsm = New CFsm_ReadFile(lpBuffer,
                             dwNumberOfBytesToRead,
                             /*lpdwNumberOfBytesRead*/ NULL
                             );

    HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;
    if (pFsm != NULL)
    {
        if (isAsync)
        {
            pRequest->Reference();
            b2ndDeref = TRUE;
        }

        error = StartFsmChain(pFsm, pRequest, FALSE, isAsync);

        //if error == ERROR_IO_PENDING, DO NOT TOUCH this fsm or any pRequest contents.
        //another async thread could be operating on this fsm.
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    bEndRead = FALSE;

    if (error == ERROR_SUCCESS)
    {
        bytesRead = pRequest->GetBytesRead();

        success = TRUE;
        goto sync_success;
    }
    else
    {
        success = FALSE;
    }
    
quit:

    _InternetDecNestingCount(nestingLevel);

    if (bEndRead)
    {
        //
        // if handleType is not HttpRequest or File then we are making this
        // request in the context of an uninterruptable async worker thread.
        // HTTP and file requests use the normal mechanism. In the case of non-
        // HTTP and file requests, we need to treat the request as if it were
        // sync and deref the handle
        //

        ReadFile_End(!lpThreadInfo->IsAsyncWorkerThread,
                     success,
                     hFileMapped,
                     bytesRead,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     );
    }

    if (lpThreadInfo && !lpThreadInfo->IsAsyncWorkerThread)
    {
        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_READ_FILE,
                 bytesRead,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

done:

    if (b2ndDeref)
    {
        // So that we have a refcount on the object going into the callback.
        INET_ASSERT (isAsync);
        DereferenceObject((LPVOID)hFileMapped);
    }
    
    // if error is not ERROR_SUCCESS then this function returning the error,
    // otherwise the error has already been set by the API we called,
    // irrespective of the value of success
    if (error != ERROR_SUCCESS)
    {
        if (error == ERROR_IO_PENDING)
        {
            SetLastError(ERROR_SUCCESS);
            success = TRUE;
        }
        else
        {
            DEBUG_ERROR(API, error);

            SetLastError(error);
            success = FALSE;
        }
    }

    DEBUG_LEAVE_API(success);
    return success;
    
sync_success:

    if (isAsync)
    {
        InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_READ_COMPLETE,
                               lpBuffer,
                               bytesRead
                               );
    }

    if (lpdwNumberOfBytesRead)
    {
        *lpdwNumberOfBytesRead = bytesRead;
    }
    
    goto quit;
}


PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    )

/*++

Routine Description:

    Common end-of-read processing:

        - update bytes read parameter
        - dump data if logging & API data requested
        - dereference handle if not async request

Arguments:

    bDeref                  - TRUE if handle should be dereferenced (should be
                              FALSE for async request)

    bSuccess                - TRUE if Read completed successfully

    hFileMapped             - mapped file handle

    dwBytesRead             - number of bytes read

    lpBuffer                - into this buffer

    dwNumberOfBytesToRead   - originally requested bytes to read

    lpdwNumberOfBytesRead   - where bytes read is stored

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(lpBuffer);
    DEBUG_ENTER((DBG_INET,
                 None,
                 "ReadFile_End",
                 "%B, %B, %#x, %d, %#x, %d, %#x",
                 bDeref,
                 bSuccess,
                 hFileMapped,
                 dwBytesRead,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead
                 ));

    if (bSuccess) {

        //
        // update the amount of immediate data available only if we succeeded
        //

        ((INTERNET_HANDLE_BASE *)hFileMapped)->ReduceAvailableDataLength(dwBytesRead);

        if (lpdwNumberOfBytesRead != NULL) {
            *lpdwNumberOfBytesRead = dwBytesRead;

            DEBUG_PRINT(API,
                        INFO,
                        ("*lpdwNumberOfBytesRead = %d\n",
                        *lpdwNumberOfBytesRead
                        ));

            //
            // dump API data only if requested
            //

            IF_DEBUG_CONTROL(DUMP_API_DATA) {
                DEBUG_DUMP_API(API,
                               "Received data:\n",
                               lpBuffer,
                               *lpdwNumberOfBytesRead
                               );
            }

            /* Likely redundant:
            TRACE_DUMP_API_IF_REQUEST(API,
                           "Received data:\n",
                           lpBuffer,
                           *lpdwNumberOfBytesRead,
                            (HANDLE_OBJECT *)(hFileMapped)
                           );
           */

        }
        if (dwBytesRead < dwNumberOfBytesToRead) {

            DEBUG_PRINT(API,
                        INFO,
                        ("(!) bytes read (%d) < bytes requested (%d)\n",
                        dwBytesRead,
                        dwNumberOfBytesToRead
                        ));

        }
    }

    //
    // if async request, handle will be deref'd after REQUEST_COMPLETE callback
    // is delivered
    //

    if (bDeref && (hFileMapped != NULL)) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    PERF_LOG(PE_CLIENT_REQUEST_END,
             AR_INTERNET_READ_FILE,
             dwBytesRead,
             0,
             (!bDeref && hFileMapped) ? ((INTERNET_HANDLE_BASE *)hFileMapped)->GetPseudoHandle() : NULL
             );

    DEBUG_LEAVE(0);
}




DWORD
CFsm_ReadFile::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFile::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFile * stateMachine = (CFsm_ReadFile *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFile_Fsm(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        INET_ASSERT (error == ERROR_WINHTTP_OPERATION_CANCELLED);
        Fsm->SetDone();
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFile_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFile & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }

    // the operation has gone sync, let's store the Bytes read in the request handle, otherwise we will
    // lose it as the fsm will be deleted before returning to the API call.
    HTTP_REQUEST_HANDLE_OBJECT* pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT*) fsm.GetMappedHandle();

    pRequest->SetBytesRead(fsm.m_dwBytesRead);
    }
    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 fsm.m_dwBytesRead,
                 fsm.m_lpBuffer,
                 fsm.m_dwNumberOfBytesToRead,
                 fsm.m_lpdwNumberOfBytesRead
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_ReadFileEx::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFileEx::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFileEx * stateMachine = (CFsm_ReadFileEx *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFileEx_Fsm(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        INET_ASSERT (error == ERROR_WINHTTP_OPERATION_CANCELLED);
        Fsm->SetDone();
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFileEx_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFileEx & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        fsm.m_dwNumberOfBytesToRead = fsm.m_lpBuffersOut->dwBufferLength;
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffersOut->lpvBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             (fsm.m_dwFlags & IRF_NO_WAIT)
                               ? SF_NO_WAIT
                               : 0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }
    }

    //
    // if we are asynchronously completing a no-wait read then we don't update
    // any app parameters - we simply return the indication that we completed.
    // The app will then make another no-wait read to get the data
    //

    BOOL bNoOutput;

    bNoOutput = ((fsm.m_dwFlags & IRF_NO_WAIT)
                && fsm.GetThreadInfo()->IsAsyncWorkerThread)
                    ? TRUE
                    : FALSE;

    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 bNoOutput ? 0    : fsm.m_dwBytesRead,
                 bNoOutput ? NULL : fsm.m_lpBuffersOut->lpvBuffer,
                 bNoOutput ? 0    : fsm.m_dwNumberOfBytesToRead,
                 bNoOutput ? NULL : &fsm.m_lpBuffersOut->dwBufferLength
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
BOOL
WINAPI
WinHttpWriteData(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )

/*++

Routine Description:

    This function write next block of data to the internet file. Currently it
    supports the following protocol data:

        HttpWriteFile

Arguments:

    hFile                       - handle that was obtained by OpenFile Call

    lpBuffer                    - pointer to the data buffer

    dwNumberOfBytesToWrite      - number of bytes in the above buffer

    lpdwNumberOfBytesWritten    -  pointer to a DWORD where the number of bytes
                                   of data actually written is returned

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpWriteData",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToWrite,
                     lpdwNumberOfBytesWritten
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    BOOL fNeedDeref = TRUE;
    HINTERNET hFileMapped = NULL;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        goto done;
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {

        //informational INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    //
    // set the handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    // validate handle and its type
    BOOL isAsync;
    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        INET_ASSERT(FALSE);
        goto quit;
    }

    //
    // validate parameters - write length cannot be 0
    //

    if (!lpThreadInfo->IsAsyncWorkerThread)
    {
        if (dwNumberOfBytesToWrite != 0)
        {
            error = ProbeReadBuffer((LPVOID)lpBuffer, dwNumberOfBytesToWrite);
            if (error == ERROR_SUCCESS)
            {
                if (lpdwNumberOfBytesWritten)
                {
                    error = ProbeAndSetDword(lpdwNumberOfBytesWritten, 0);
                }
            }
        }
        else
        {
            error = ERROR_INVALID_PARAMETER;
        }         

        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }
    }


    // # 62953
    // If the authentication state of the handle is Negotiate,
    // don't submit data to the server but return success.
    // ** Added test for NTLM or Negotiate - Adriaanc.
    //
    
    HTTP_REQUEST_HANDLE_OBJECT *pRequest;
    pRequest = (HTTP_REQUEST_HANDLE_OBJECT*) hFileMapped;

    DWORD dwBytesWritten = 0;
    if (pRequest->GetAuthState() == AUTHSTATE_NEGOTIATE
        || pRequest->GetProxyTunnelingSuppressWrite())
    {
        dwBytesWritten = dwNumberOfBytesToWrite;
        error = ERROR_SUCCESS;
        success = TRUE;
        goto sync_success;
    }
        

    INET_ASSERT(error == ERROR_SUCCESS);

    CFsm_HttpWriteData *pFsm = New CFsm_HttpWriteData((LPVOID)lpBuffer,
                                                      dwNumberOfBytesToWrite,
                                                      NULL/*lpdwNumberOfBytesWritten*/,
                                                      0,
                                                      pRequest
                                                      );

    if (pFsm != NULL)
    {
        HTTP_REQUEST_HANDLE_OBJECT *pRequest2 = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;

        error = StartFsmChain(pFsm, pRequest2, FALSE, isAsync);

        //if error == ERROR_IO_PENDING, DO NOT TOUCH this fsm or any pRequest contents.
        //another async thread could be operating on this fsm/request.
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Don't Derefrence if we're going pending cause the FSM will do
    //  it for us.
    //

    if ( error == ERROR_IO_PENDING )
    {
        fNeedDeref = FALSE;
    }

    if (error == ERROR_SUCCESS)
    {
        dwBytesWritten = pRequest->GetBytesWritten();
        success = TRUE;
        goto sync_success;
    }
    else
    {
        success = FALSE;
    }
    
quit:

    if (hFileMapped != NULL && fNeedDeref)
    {
        DereferenceObject((LPVOID)hFileMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    if (error != ERROR_SUCCESS)
    {
        if (error == ERROR_IO_PENDING)
        {
            SetLastError(ERROR_SUCCESS);
            success = TRUE;
        }
        else
        {
            DEBUG_ERROR(API, error);
        
            SetLastError(error);
        }
    }

    DEBUG_LEAVE_API(success);

    return success;

sync_success:

    if (isAsync)
    {
        DWORD dwResult = dwBytesWritten;
        InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE,
                               &dwResult,
                               sizeof (DWORD)
                               );
    }

    if (lpdwNumberOfBytesWritten)
    {
        *lpdwNumberOfBytesWritten = dwBytesWritten;
    }
    
    goto quit;
}



INTERNETAPI
BOOL
WINAPI
WinHttpQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    Determines the amount of data currently available to be read on the handle

Arguments:

    hFile                       - handle of internet object

    lpdwNumberOfBytesAvailable  - pointer to returned bytes available

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "WinHttpQueryDataAvailable",
                     "%#x, %#x, %#x",
                     hFile,
                     lpdwNumberOfBytesAvailable
                     ));

    BOOL success;
    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    HINTERNET hFileMapped = NULL;
    BOOL bDeref = TRUE;
    DWORD dwNumBytes = (DWORD)-1;

    if (!GlobalDataInitialized)
    {
        error = ERROR_WINHTTP_NOT_INITIALIZED;
        bDeref = FALSE;
        goto quit;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL)
    {

        //informational INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    PERF_LOG(PE_CLIENT_REQUEST_START,
             AR_INTERNET_QUERY_DATA_AVAILABLE,
             lpThreadInfo->ThreadId,
             hFile
             );

    //
    // validate parameters
    //

    error = MapHandleToAddress(hFile, &hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL))
    {
        goto quit;
    }

    INET_ASSERT(hFileMapped);

    //
    // set the handle values in the per-thread info block (this API
    // can't return extended error info, so we don't care about it)
    //

    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);

    //
    // if the handle is invalid, quit now
    //

    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    //
    // validate rest of parameters
    //

    if (lpdwNumberOfBytesAvailable)
    {
        error = ProbeAndSetDword(lpdwNumberOfBytesAvailable, 0);
        if (error != ERROR_SUCCESS)
        {
            goto quit;
        }
    }

    BOOL isAsync;
    error = RIsHandleLocal(hFileMapped, NULL, &isAsync, TypeHttpRequestHandle);
    if (error != ERROR_SUCCESS)
    {
        goto quit;
    }

    //
    // since the async worker thread doesn't come back through this API, the
    // following test is sufficient. Note that we only go async if there is
    // no data currently available on the handle
    //

    BOOL dataAvailable;
    dataAvailable = ((INTERNET_HANDLE_BASE *)hFileMapped)->IsDataAvailable();

    BOOL eof;
    eof = ((INTERNET_HANDLE_BASE *)hFileMapped)->IsEndOfFile();

    DWORD available;

    if (dataAvailable || eof)
    {
        available = ((INTERNET_HANDLE_BASE *)hFileMapped)->AvailableDataLength();

        DEBUG_PRINT(API,
                    INFO,
                    ("%d bytes are immediately available\n",
                    available
                    ));

        // *lpdwNumberOfBytesAvailable = available;
        success = TRUE;
        goto sync_success;
    }

    INET_ASSERT(hFileMapped);

    //
    // sync path. wInternetQueryDataAvailable will set the last error code
    // if it fails
    //

    CFsm_QueryAvailable *pFsm;

    pFsm = New CFsm_QueryAvailable(NULL/*lpdwNumberOfBytesAvailable*/,
                                   0,
                                   NULL
                                   );

    HTTP_REQUEST_HANDLE_OBJECT *pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;

    if (pFsm != NULL)
    {
        error = StartFsmChain(pFsm, pRequest, FALSE, isAsync);
    }
    else
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (error == ERROR_SUCCESS)
    {
        available = pRequest->AvailableDataLength();
        success = TRUE;
        goto sync_success;
    }
    else
    {
        if (error == ERROR_IO_PENDING)
        {
            bDeref = FALSE;
        }
        goto quit;
    }

finish:

    DEBUG_PRINT_API(API,
                    INFO,
                    ("*lpdwNumberOfBytesAvailable (%#x) = %d\n",
                    lpdwNumberOfBytesAvailable,
                    dwNumBytes
                    ));

    if (bDeref && (hFileMapped != NULL))
    {
        DereferenceObject((LPVOID)hFileMapped);
    }

    if (lpThreadInfo)
    {

        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_QUERY_DATA_AVAILABLE,
                 dwNumBytes,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

    if (error == ERROR_IO_PENDING)
    {
        SetLastError(ERROR_SUCCESS);
        success = TRUE;
    }
    
    DEBUG_LEAVE_API(success);
    return success;

quit:

    if (error != ERROR_IO_PENDING)
    {
        DEBUG_ERROR(API, error);
    }

    SetLastError(error);
    success = FALSE;

    goto finish;

sync_success:

    dwNumBytes = available;
    
    if (isAsync)
    {
        DWORD dwResult = available;
        InternetIndicateStatus(WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE,
                               &dwResult,
                               sizeof (DWORD)
                               );
    }

    if (lpdwNumberOfBytesAvailable)
    {
        *lpdwNumberOfBytesAvailable = available;
    }
    
    goto finish;
}


DWORD
CFsm_QueryAvailable::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_QueryAvailable::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_QueryAvailable * stateMachine = (CFsm_QueryAvailable *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = QueryAvailable_Fsm(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = Fsm->GetError();
        INET_ASSERT (error == ERROR_WINHTTP_OPERATION_CANCELLED);
        Fsm->SetDone();
        break;

    default:
        error = ERROR_WINHTTP_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_WINHTTP_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "QueryAvailable_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_QueryAvailable & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)fsm.GetMappedHandle();

    if (fsm.GetState() == FSM_STATE_INIT) {
        error = pRequest->QueryDataAvailable(fsm.m_lpdwNumberOfBytesAvailable);
    }
    if (error == ERROR_SUCCESS) {
        pRequest->SetAvailableDataLength(*fsm.m_lpdwNumberOfBytesAvailable);

        DEBUG_PRINT(INET,
                    INFO,
                    ("%d bytes available\n",
                    *fsm.m_lpdwNumberOfBytesAvailable
                    ));

        fsm.SetApiData(*fsm.m_lpdwNumberOfBytesAvailable);
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}



INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwErrorCategory,
    IN LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    This function returns the per-thread last internet error description text
    or server response.

    If this function is successful, *lpdwBufferLength contains the string length
    of lpszBuffer.

    If this function returns a failure indication, *lpdwBufferLength contains
    the number of BYTEs required to hold the response text

Arguments:

    lpdwErrorCategory   - pointer to DWORD location where the error catagory is
                          returned

    lpszBuffer          - pointer to buffer where the error text is returned

    lpdwBufferLength    - IN: length of lpszBuffer
                          OUT: number of characters in lpszBuffer if successful
                          else size of buffer required to hold response text

Return Value:

    BOOL
        Success - TRUE
                    lpszBuffer contains the error text. The caller must check
                    *lpdwBufferLength: if 0 then there was no text to return

        Failure - FALSE
                    Call GetLastError() for more information

--*/

{
    DEBUG_ENTER((DBG_API,
                     Bool,
                     "InternetGetLastResponseInfoA",
                     "%#x, %#x, %#x [%d]",
                     lpdwErrorCategory,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD error;
    BOOL success;
    DWORD textLength;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    //
    // validate parameters
    //

    if (IsBadWritePtr(lpdwErrorCategory, sizeof(*lpdwErrorCategory))
    || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
    || (ARGUMENT_PRESENT(lpszBuffer)
        ? IsBadWritePtr(lpszBuffer, *lpdwBufferLength)
        : FALSE)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // if the buffer pointer is NULL then its the same as a zero-length buffer
    //

    if (!ARGUMENT_PRESENT(lpszBuffer)) {
        *lpdwBufferLength = 0;
    } else if (*lpdwBufferLength != 0) {
        *lpszBuffer = '\0';
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        DEBUG_PRINT(INET,
                    ERROR,
                    ("failed to get INTERNET_THREAD_INFO\n"
                    ));

        //informational INET_ASSERT(FALSE);

        error = ERROR_WINHTTP_INTERNAL_ERROR;
        goto quit;
    }

    //
    // there may not be any error text for this thread - either no server
    // error/response has been received, or the error text has been cleared by
    // an intervening API
    //

    if (lpThreadInfo->hErrorText != NULL) {

        //
        // copy as much as we can fit in the user supplied buffer
        //

        textLength = lpThreadInfo->ErrorTextLength;
        if (*lpdwBufferLength && lpszBuffer != NULL) {

            LPBYTE errorText;

            errorText = (LPBYTE)LOCK_MEMORY(lpThreadInfo->hErrorText);
            if (errorText != NULL) {
                textLength = min(textLength, *lpdwBufferLength) - 1;
                memcpy(lpszBuffer, errorText, textLength);

                //
                // the error text should always be zero terminated, so the
                // calling app can treat it as a string
                //

                lpszBuffer[textLength] = '\0';

                UNLOCK_MEMORY(lpThreadInfo->hErrorText);

                if (textLength == lpThreadInfo->ErrorTextLength - 1) {
                    error = ERROR_SUCCESS;
                } else {

                    //
                    // returned length is amount of buffer required
                    //

                    textLength = lpThreadInfo->ErrorTextLength;
                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            } else {

                DEBUG_PRINT(INET,
                            ERROR,
                            ("failed to lock hErrorText (%#x): %d\n",
                            lpThreadInfo->hErrorText,
                            GetLastError()
                            ));

                error = ERROR_WINHTTP_INTERNAL_ERROR;
            }
        } else {

            //
            // user's buffer is not large enough to hold the info. We'll
            // let them know the required length
            //

            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else {

        INET_ASSERT(lpThreadInfo->ErrorTextLength == 0);

        textLength = 0;
        error = ERROR_SUCCESS;
    }

    *lpdwErrorCategory = lpThreadInfo->ErrorNumber;
    *lpdwBufferLength = textLength;

    IF_DEBUG(ANY) {
        if ((error == ERROR_SUCCESS)
        || ((textLength != 0) && (lpszBuffer != NULL))) {

            DEBUG_DUMP_API(API,
                           "Last Response Info:\n",
                           lpszBuffer,
                           textLength
                           );

        }
    }

    if ((error == ERROR_SUCCESS)
    || ((textLength != 0) && (lpszBuffer != NULL))) {

        TRACE_DUMP_API_IF_REQUEST(API,
                       "Last Response Info:\n",
                       lpszBuffer,
                       textLength,
                       (HANDLE_OBJECT *) (lpThreadInfo->hObjectMapped)
                       );

    }

quit:
    success = (error == ERROR_SUCCESS);
    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE(success);

    return success;
}


BOOL
internalWinHttpGetDefaultProxyConfigurationA( IN OUT WINHTTP_PROXY_INFOA * pProxyInfo)
/*++

Routine Description:

    Reads the settings for WinHTTP's default proxy mode.
    
Arguments:

    pProxyInfo  - Pointer to structure to receive proxy settings

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/
{
    DWORD dwError;
    
    INTERNET_PROXY_INFO_EX proxyInfoEx;
    memset( &proxyInfoEx, 0, sizeof( proxyInfoEx));
    proxyInfoEx.dwStructSize = sizeof( proxyInfoEx);

    dwError = ReadProxySettings( &proxyInfoEx);

    if( dwError == ERROR_SUCCESS)
    {
        //  reset access type result to one of two known..
        pProxyInfo->dwAccessType = WINHTTP_ACCESS_TYPE_NO_PROXY;
        if( proxyInfoEx.dwFlags == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
            pProxyInfo->dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        
        pProxyInfo->lpszProxy = (LPSTR)proxyInfoEx.lpszProxy;  // allocated by GlobalAlloc()
        pProxyInfo->lpszProxyBypass = (LPSTR)proxyInfoEx.lpszProxyBypass;  // allocated by GlobalAlloc()
    }

    if( dwError != ERROR_SUCCESS)
        SetLastError( dwError);

    return dwError == ERROR_SUCCESS ? TRUE : FALSE;
}


BOOL
internalWinHttpSetDefaultProxyConfigurationA( IN WINHTTP_PROXY_INFOA * pProxyInfo)
/*++

Routine Description:

    Writes settings for WinHTTP's default proxy mode.
    
Arguments:

    pProxyInfo  - Pointer to structure describing proxy settings

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/
{
    BOOL blReturnValue = FALSE;

    //
    //  Parameter validation
    //

    //  If AccessType is NO_PROXY, make sure no proxy information is given.
    if( pProxyInfo->dwAccessType == WINHTTP_ACCESS_TYPE_NO_PROXY)
    {
        if( pProxyInfo->lpszProxy != NULL
            || pProxyInfo->lpszProxyBypass != NULL)
        {
            SetLastError( ERROR_INVALID_PARAMETER);
            goto done;
        }
    }
    //  If AccessType is NAMED_PROXY, make sure a proxy is given.  ProxyBypass list is optional.
    else if( pProxyInfo->dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY)
    {
        if( pProxyInfo->lpszProxy == NULL)
        {
            SetLastError( ERROR_INVALID_PARAMETER);
            goto done;
        }
    }
    else
    {
        //  AccessType is not NAMED_PROXY or NO_PROXY
        SetLastError( ERROR_INVALID_PARAMETER);
        goto done;
    }

    //  verify validity of proxy server list
    if( pProxyInfo->lpszProxy != NULL)
    {
        //  verify validity of proxy server list
        PROXY_SERVER_LIST proxyServerList( pProxyInfo->lpszProxy);

        if( ERROR_SUCCESS != proxyServerList.GetError())
        {
            SetLastError( ERROR_INVALID_PARAMETER);
            goto done;
        }
    }
    
    //  verify validity of proxy server bypass list
    if( pProxyInfo->lpszProxyBypass != NULL)
    {
        PROXY_BYPASS_LIST proxyBypassList( pProxyInfo->lpszProxyBypass);
        if( ERROR_SUCCESS != proxyBypassList.GetError())
        {
            SetLastError( ERROR_INVALID_PARAMETER);
            goto done;
        }
    }

    INTERNET_PROXY_INFO_EX proxyInfoEx;
    memset( &proxyInfoEx, 0, sizeof( proxyInfoEx));
    proxyInfoEx.dwStructSize = sizeof( proxyInfoEx);
    proxyInfoEx.dwFlags = pProxyInfo->dwAccessType;
    proxyInfoEx.lpszProxy = pProxyInfo->lpszProxy;
    proxyInfoEx.lpszProxyBypass = pProxyInfo->lpszProxyBypass;

    DWORD dwError;
    dwError = WriteProxySettings( &proxyInfoEx);
    if( ERROR_SUCCESS != dwError)
    {
        SetLastError( dwError);
        goto done;
    }
    
    blReturnValue = TRUE;

done:
    return blReturnValue;
}

