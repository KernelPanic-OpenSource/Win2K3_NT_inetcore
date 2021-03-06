//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       cnetftp.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <iapp.h>
#ifdef unix
#include <unistd.h>
#endif /* unix */

#include <shlwapip.h>

static DWORD dwLstError;

PerfDbgTag(tagCINetFile,    "Urlmon", "Log CINetFile",         DEB_PROT);
    DbgTag(tagCINetFileErr, "Urlmon", "Log CINetFile Errors",  DEB_PROT|DEB_ERROR);
PerfDbgTag(tagCINetSimple,  "Urlmon", "Log CINetSimple",       DEB_PROT);

#ifdef unix
extern "C" void unixForceAutoProxSync();
#endif /* unix */

LPWSTR FindFileExtensionW(LPWSTR pwzFileName)
{
    DEBUG_ENTER((DBG_APP,
                Pointer,
                "FindFileExtensionW",
                "%.80wq",
                pwzFileName
                ));
                
    LPWSTR pStr = NULL;
    LPWSTR lpF = pwzFileName + wcslen(pwzFileName); //Point to null

    if (lpF)
    {
        //Strip all trailing dots.
        for (lpF--; lpF >= pwzFileName && (*lpF == L'.' || *lpF == L' '); lpF --)
            *lpF = 0;

        for (; lpF >= pwzFileName && *lpF != L'.'; lpF --)
            if (*lpF == L'\\')
                return NULL;
    }

    if (lpF > pwzFileName)
    {
        pStr = lpF;
    }

    DEBUG_LEAVE(pStr);
    return pStr;

}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::CINetFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetFile::CINetFile(REFCLSID rclsid, IUnknown *pUnkOuter) : CINet(rclsid,pUnkOuter)
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetFile::CINetFile",
                "this=%#x, %#x, %#x",
                this, &rclsid, pUnkOuter
                ));
                
    PerfDbgLog(tagCINetFile, this, "+CINetFile::CINetFile");

    _dwIsA = DLD_PROTOCOL_FILE;
    _wzFileName[0] = '\0';
    
    PerfDbgLog(tagCINetFile, this, "-CINetFile::CINetFile");

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::~CINetFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetFile::~CINetFile()
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetFile::~CINetFile",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetFile, this, "+CINetFile::~CINetFile");

    if (_hFile && (_hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(_hFile);
    }

    _wzFileName[0] = '\0';
    PerfDbgLog(tagCINetFile, this, "-CINetFile::~CINetFile");

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::INetAsyncOpen
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetFile::INetAsyncOpen()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetFile::INetAsyncOpen",
                "this=%#x",
                this
                ));
                
    HRESULT hr = NOERROR;
    DWORD dwAttr;
    PerfDbgLog1(tagCINetFile, this, "+CINetFile::INetAsyncOpen(szObject:%ws)", GetObjectNameW());

    ReportNotification(BINDSTATUS_SENDINGREQUEST, GetServerName());

    // nothing to do - just call
    dwAttr = GetFileAttributesWrapW(GetObjectNameW());

#ifdef unix
    unixForceAutoProxSync();
    if(access(GetObjectName(),R_OK) == -1)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        dwAttr = 0xffffffff;
    }
#endif /* unix */

    if (   (dwAttr != 0xffffffff)
        && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)
       )
    {
        HANDLE htemp = 0;
        DWORD dwSize = 0;
        {
            WIN32_FIND_DATAW finddata;

            htemp = FindFirstFileWrapW(GetObjectNameW(), &finddata);
            dwSize = finddata.nFileSizeLow;
        }
        _cbTotalBytesRead = dwSize;
        _cbDataSize = dwSize;

        if (htemp  && (htemp != INVALID_HANDLE_VALUE))
        {
            ReportNotificationW(BINDSTATUS_CACHEFILENAMEAVAILABLE, GetObjectNameW());
            
            // if the file has a file extension, try to
            // determine the MIME type that way...
            //
            LPWSTR pwsz = GetObjectNameW();
            pwsz = FindFileExtensionW(pwsz);
            if (pwsz)
            {
                char psz[MAX_PATH];
                psz[0] = '\0';

                W2A(pwsz, psz, MAX_PATH);
                char szMime[MAX_PATH];
                DWORD cb = MAX_PATH;

                if (SUCCEEDED(GetMimeFromExt(psz,szMime,&cb)))
                {
                    if( _grfBindF & BINDF_FROMURLMON )
                    {
                        ReportNotification(BINDSTATUS_VERIFIEDMIMETYPEAVAILABLE, szMime);
                    }
                    else
                    {
                        ReportNotification(BINDSTATUS_MIMETYPEAVAILABLE, szMime);

                    }
    
                }
            }

            if (_pCTrans)
            {
                _bscf |= BSCF_LASTDATANOTIFICATION;
                _pCTrans->ReportData(_bscf, _cbTotalBytesRead, _cbDataSize);
                ReportResultAndStop(NOERROR, _cbTotalBytesRead, _cbDataSize );
            }                
         
            _hrError = INET_E_DONE;
            hr = NOERROR;
            FindClose(htemp);
        }
        else
        {
            SetCNetBindResult(GetLastError());
            hr = _hrError = INET_E_DATA_NOT_AVAILABLE;
        }
    }
    else if(   (dwAttr != 0xffffffff)
            && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
    {
        hr = _hrError = INET_E_REDIRECT_TO_DIR;
    }
    else
    {
        // BUGBUG do authentication if this call failed due to
        // net permission being denied
        DbgLog2(tagCINetFileErr, this, "-CINetFile::INetAsyncOpen failed (dwAttr:%ld,filename:%ws)",dwAttr,GetObjectNameW());

        // If you pass \\servername to GetFileAttributesWrap, on NT4 it will return ERROR_FILE_NOT_FOUND
        // but ERROR_INVALID_NAME on all other platforms.
        // We remap this error so that iframes can navigate to \\servername on NT4
        // Reference: IE5/102590
        DWORD dwError = GetLastError();
        if ((dwError==ERROR_FILE_NOT_FOUND)  
            && PathIsUNCServer(GetObjectName()))
        {
            dwError = ERROR_INVALID_NAME;
        }
        // set the exact error for GetBindResult
        SetCNetBindResult(dwError);
        hr = _hrError = INET_E_RESOURCE_NOT_FOUND;
    }

    PerfDbgLog1(tagCINetFile, this, "-CINetFile::INetAsyncOpen (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::Read
//
//  Synopsis:
//
//  Arguments:  [pBuffer] --
//              [cbBytes] --
//              [pcbBytes] --
//
//  Returns:
//
//  History:    2-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CINetFile::Read(void *pBuffer, DWORD cbBytes, DWORD *pcbBytes)
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetFile::IInternetProtocol::Read",
                "this=%#x, %#x, %#x, %#x",
                this, pBuffer, cbBytes, pcbBytes
                ));
                
    PerfDbgLog(tagCINetFile, this, "+CINetFile::Read");
    HRESULT hr = NOERROR;

    PProtAssert((cbBytes && pcbBytes));

    if (!_hFile)
    {
        LPWSTR wszFile = GetObjectNameW();

        PProtAssert((wszFile));

        if (wszFile)
        {
            // fill the internal buffer for data sniffing
            _hFile = CreateFileWrapW(wszFile, GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, NULL);

        }

    }

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwRead;
        if (ReadFile(_hFile, pBuffer, cbBytes, &dwRead, NULL))
        {
            *pcbBytes = dwRead;
            if (cbBytes != dwRead)
            {
                hr = S_FALSE;
            }
        }
        else
        {
            DbgLog2(tagCINetFileErr, this, " -CINetFile::Read could not read file (sniffing:%ld) (hr:%lx)", dwRead, hr);
            SetCNetBindResult(GetLastError());
            hr = _hrError = INET_E_DATA_NOT_AVAILABLE;
            ReportResultAndStop(hr);
        }
    }
    else
    {
        SetCNetBindResult(GetLastError());
        hr = _hrError = INET_E_DATA_NOT_AVAILABLE;
        ReportResultAndStop(hr);
        DbgLog2(tagCINetFileErr, this, "-CINetFile::Read could not open (file:%ws, hr:%lx)", GetObjectNameW(), hr);
    }

    PerfDbgLog4(tagCINetFile, this, "-CINetFile::Read (_hrError:%lx, [hr:%lx,cbBytesAsked:%ld,cbBytesReturned:%ld])",
                                    _hrError, hr, cbBytes, *pcbBytes);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::LockFile
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetFile::LockFile(BOOL fRetrieve)
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetFile::LockFile",
                "this=%#x, %B",
                this, fRetrieve
                ));
                
    PerfDbgLog(tagCINetFile, this, "+CINetFile::LockFile");
    HRESULT hr = NOERROR;

    if (!_hFile )
    {
        _hFile = CreateFileWrapW( GetObjectNameW(), GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    }

    if (_hFile != INVALID_HANDLE_VALUE)
    {
        _fLocked = TRUE;
    }

    PerfDbgLog1(tagCINetFile, this, "-CINetFile::LockFile (hr:%lx,)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFile::UnlockFile
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-13-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetFile::UnlockFile()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetFile::UnlockFile",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetFile, this, "IN CINetFile::UnlockFile");
    HRESULT hr = NOERROR;

    if (_fLocked)
    {
        PProtAssert((_hFile));
        CloseHandle(_hFile);
        _hFile = NULL;
        _fLocked = FALSE;
    }

    PerfDbgLog1(tagCINetFile, this, "-CINetFile::UnlockFile (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::CINetSimple
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetSimple::CINetSimple(REFCLSID rclsid, IUnknown *pUnkOuter) : CINet(rclsid,pUnkOuter)
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetSimple::CINetSimple",
                "this=%#x, %#x, %#x",
                this, &rclsid, pUnkOuter
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::CINetSimple");

    _dwState = INetState_START;
    _dwIsA = DLD_PROTOCOL_NONE;

    PerfDbgLog(tagCINetSimple, this, "-CINetSimple::CINetSimple");

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::~CINetSimple
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetSimple::~CINetSimple()
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetSimple::~CINetSimple",
                "this=%#x",
                this
                ));

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::INetAsyncOpenRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::INetAsyncOpenRequest()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::INetAsyncOpenRequest",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::INetAsyncOpenRequest");
    HRESULT hr = NOERROR;
    DWORD dwBindF = 0;

    SetINetState(INetState_PROTOPEN_REQUEST);
    PProtAssert((g_hSession != NULL));

    if (_hRequest && _fDoSimpleRetry)
    {
        // Abort the previous request if needed.
        // Note:  This is CINetSimple, so the behavior of closing the previous
        //        handle and retrying only applies to FTP and gopher requests.
        _fDoSimpleRetry = FALSE;
        _fHandlesRecycled = TRUE;
        TerminateRequest();
    }
 
    // get the open flags
    dwBindF = GetBindFlags();

    if (dwBindF & BINDF_GETNEWESTVERSION)
    {
        _dwOpenFlags |= INTERNET_FLAG_RELOAD;
    }

    if (   (dwBindF & BINDF_NOWRITECACHE)
        //BUG-WORK
        //&& !_pCTransData->IsFileRequired()
        )
    {
        _dwOpenFlags |= INTERNET_FLAG_DONT_CACHE ;
    }

    // BUGBUG OFFLINE, RELOAD, RESYNCHRONIZE and HYPERLINK are mutually
    // exclusive. But inside wininet there is priority, so
    // the priority is OFFLINE, RELOAD, RESYNCHRONIZE, HYPERLINK in that order

    if (dwBindF & BINDF_RESYNCHRONIZE)
    {
        // caller asking to do if-modified-since
        _dwOpenFlags |= INTERNET_FLAG_RESYNCHRONIZE;
    }

    if (dwBindF & BINDF_HYPERLINK)
    {
        // caller syas this is a hyperlink access
        _dwOpenFlags |= INTERNET_FLAG_HYPERLINK;
    }


    PrivAddRef();
    _HandleStateRequest = HandleState_Pending;

    // we always request keep-alive, use an existing connection (FTP) and
    // passive mode transfers (also FTP)
    _dwOpenFlags |= INTERNET_FLAG_KEEP_CONNECTION
                    | INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_PASSIVE;

    HINTERNET hRequestTmp = InternetOpenUrl(g_hSession, _pszFullURL, NULL, 0, _dwOpenFlags, (DWORD_PTR) this);
    if ( hRequestTmp == 0)
    {
        dwLstError = GetLastError();
        if (dwLstError == ERROR_IO_PENDING)
        {
            // wait async for the handle
            hr = E_PENDING;
            SetStatePending(E_PENDING);

        }
        else
        {
            _hrError = INET_E_CANNOT_CONNECT;
            hr = E_FAIL;
        }
    }
    else
    {
        _hRequest = hRequestTmp;
        _HandleStateRequest = HandleState_Initialized;
        hr = INetAsyncSendRequest();
    }

    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop(NOERROR);
    }

    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::INetAsyncOpenRequest (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::INetAsyncSendRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::INetAsyncSendRequest()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::INetAsyncSendRequest",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::INetAsyncSendRequest");

    HRESULT hr = NOERROR;
    hr = QueryStatusOnResponse();
    
    if (! _fDoSimpleRetry && hr == NOERROR)
    {

        SetINetState(INetState_SEND_REQUEST);

        if (OperationOnAparmentThread(INetState_SEND_REQUEST))
        {
            TransitState(INetState_READ);
        }
        else
        {
            hr = INetRead();
        }
    }
    
    if (_hrError != INET_E_OK)
    {
        // we need to terminate here
        ReportResultAndStop(NOERROR);
    }

    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::INetAsyncSendRequest (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::INetAsyncConnect
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-27-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::INetAsyncConnect()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::INetAsyncConnect",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::INetAsyncConnect");
    HRESULT hr = NOERROR;

    //just go into OpenRequest state
    hr = INetAsyncOpenRequest();

    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::INetAsyncConnect (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::QueryStatusOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    11-14-97   OliverW (Oliver Wallace)   Created
//              09-18-98   MeadH (Mead Himelstein) copy over to shipping code
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::QueryStatusOnResponse()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::QueryStatusOnResponse",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::QueryStatusOnResponse");
    HRESULT hr = NOERROR;

    DWORD dwStatus = 0;
    DWORD cbLen = sizeof(DWORD);

    if (HttpQueryInfo(
            _hRequest,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            (LPVOID) &dwStatus,
            &cbLen,
            NULL
            ))
    {
        if (dwStatus == HTTP_STATUS_PROXY_AUTH_REQ)
        {
            _hrINet = INET_E_AUTHENTICATION_REQUIRED;
            TransitState(INetState_AUTHENTICATE, TRUE);
            hr = E_PENDING;
        }
    }

    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::QueryStatusOnResponse (hr:%lx)", hr);

    DEBUG_LEAVE(hr);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::QueryStatusOnResponseDefault
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-02-98   OliverW   Created
//              09-18-98   MeadH (Mead Himelstein) copy over to shipping code
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::QueryStatusOnResponseDefault(DWORD dwStat)
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::QueryStatusOnResponseDefault",
                "this=%#x, %#x",
                this, dwStat
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::QueryStatusOnResponseDefault");
    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::QueryStatusOnResponseDefault (hr:%lx)", NOERROR);

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}

    

//+---------------------------------------------------------------------------
//
//  Method:     CINetSimple::QueryHeaderOnResponse
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-02-98   OliverW   Created
//              09-18-98   MeadH (Mead Himelstein) copy over to shipping code
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetSimple::QueryHeaderOnResponse()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetSimple::QueryHeaderOnResponse",
                "this=%#x",
                this
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetSimple::QueryHeaderOnResponse");
    PerfDbgLog1(tagCINetSimple, this, "-CINetSimple::QueryHeaderOnResponse (hr:%lx)", NOERROR);

    DEBUG_LEAVE(NOERROR);
    return NOERROR;
}



//+---------------------------------------------------------------------------
//
//  Method:     CINetFtp::CINetFtp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetFtp::CINetFtp(REFCLSID rclsid, IUnknown *pUnkOuter) : CINetSimple(rclsid, pUnkOuter)
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetFtp::CINetFtp",
                "this=%#x, %#x, %#x",
                this, &rclsid, pUnkOuter
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetFtp::CINetFtp");

    _dwIsA = DLD_PROTOCOL_FTP;

    PerfDbgLog(tagCINetSimple, this, "-CINetFtp::CINetFtp");

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFtp::INetAsyncSendRequest
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    3-13-98   VincentR  Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CINetFtp::INetAsyncSendRequest()
{
    DEBUG_ENTER((DBG_APP,
                Hresult,
                "CINetFtp::INetAsyncSendRequest",
                "this=%#x",
                this
                ));
                
    HRESULT hr = NOERROR;
    

    DWORD dwLowSize = 0;
    DWORD dwHighSize = 0;
    
    dwLowSize = FtpGetFileSize(_hRequest, &dwHighSize);
    if(dwLowSize != 0xffffffff)
        _cbDataSize = dwLowSize;

    hr = CINetSimple::INetAsyncSendRequest();

    DEBUG_LEAVE(hr);
    return(hr);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetFtp::~CINetFtp
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetFtp::~CINetFtp()
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetFtp::~CINetFtp",
                "this=%#x",
                this
                ));

    DEBUG_LEAVE(0);
}


//+---------------------------------------------------------------------------
//
//  Method:     CINetGopher::CINetGopher
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetGopher::CINetGopher(REFCLSID rclsid, IUnknown *pUnkOuter) : CINetSimple(rclsid,pUnkOuter)
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetGopher::CINetGopher",
                "this=%#x, %#x, %#x",
                this, &rclsid, pUnkOuter
                ));
                
    PerfDbgLog(tagCINetSimple, this, "+CINetGopher::CINetGopher");

    _dwIsA = DLD_PROTOCOL_GOPHER;

    PerfDbgLog(tagCINetSimple, this, "-CINetGopher::CINetGopher");

    DEBUG_LEAVE(0);
}

//+---------------------------------------------------------------------------
//
//  Method:     CINetGopher::~CINetGopher
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    2-06-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CINetGopher::~CINetGopher()
{
    DEBUG_ENTER((DBG_APP,
                None,
                "CINetGopher::~CINetGopher",
                "this=%#x",
                this
                ));

    DEBUG_LEAVE(0);
}

