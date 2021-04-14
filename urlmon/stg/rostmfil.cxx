//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       ROSTMFIL.CXX
//
//  Contents:
//
//  Classes:    Implements the CReadOnlyStreamFile class.
//
//  Functions:
//
//  History:    03-02-96    JoeS (Joe Souza)    Created
//
//----------------------------------------------------------------------------
#include <urlint.h>
#ifndef unix
#include "..\trans\transact.hxx"
#else
#include "../trans/transact.hxx"
#endif /* unix */
#include "rostmdir.hxx"
#include "rostmfil.hxx"

//+---------------------------------------------------------------------------
//
//  Method:     CReadOnlyStreamFile::GetFileName
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    7-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPWSTR CReadOnlyStreamFile::GetFileName()
{
    DEBUG_ENTER((DBG_STORAGE,
                String,
                "CReadOnlyStreamFile::GetFileName",
                "this=%#x",
                this
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::GetFileName\n", this));
    LPWSTR pwzDupname = NULL;

    if (_pszFileName)
    {
        pwzDupname = DupA2W(_pszFileName);
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::GetFileName (Filename:%ws)\n", this, pwzDupname));

    DEBUG_LEAVE(pwzDupname);
    return pwzDupname;
}


HRESULT CReadOnlyStreamFile::Create(LPSTR pszFileName, CReadOnlyStreamFile **ppCRoStm, LPCWSTR pwzURL)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::Create",
                "%.80q, %#x, %.80wq",
                pszFileName, ppCRoStm, pwzURL
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Create (pszFileName:%s)\n", NULL,pszFileName));
    HRESULT hr = NOERROR;

    *ppCRoStm = NULL;

    if (pszFileName == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LPSTR  pszStr = new CHAR [strlen(pszFileName) + 1];

        if (pszStr == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {

            HANDLE handle = CreateFileA(pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

            if (handle == INVALID_HANDLE_VALUE)
            {
                delete pszStr;
                hr = E_FAIL;
            }
            else
            {
                LPWSTR pwzStr = NULL;
                if (pwzURL)
                {
                    pwzStr = OLESTRDuplicate(pwzURL);
                }
                    
                strcpy(pszStr, pszFileName);
                CReadOnlyStreamFile *pCRoStm = new CReadOnlyStreamFile(pszStr, handle, pwzStr);

                if (pCRoStm == NULL)
                {
                    delete pszStr;
                }
                else
                {
                    *ppCRoStm = pCRoStm;
                }
            }
        }
    }
    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Create (hr:%lx)\n", *ppCRoStm, hr));

    DEBUG_LEAVE(hr);
    return hr;
}

CReadOnlyStreamFile::CReadOnlyStreamFile(LPSTR pszFileName, HANDLE handle, LPWSTR pwzURL)
                        : CReadOnlyStreamDirect(NULL, 0, TRUE)
{
    DEBUG_ENTER((DBG_STORAGE,
                None,
                "CReadOnlyStreamFile::CReadOnlyStreamFile",
                "this=%#x, %.80q, %#x, %.80wq",
                this, pszFileName, handle, pwzURL
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::CReadOnlyStreamFile (pszFileName:%s)\n", NULL,pszFileName));

    _hFileHandle = handle;
    _pszFileName = pszFileName;
    _fDataFullyAvailable = FALSE;
    _hWinInetLockHandle = NULL;
    _pwzURL = pwzURL;
    _fDeleteFile = FALSE;

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::CReadOnlyStreamFile \n", this));

    DEBUG_LEAVE(0);
}

CReadOnlyStreamFile::~CReadOnlyStreamFile(void)
{
    DEBUG_ENTER((DBG_STORAGE,
                None,
                "CReadOnlyStreamFile::~CReadOnlyStreamFile",
                "this=%#x",
                this
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::~CReadOnlyStreamFile (_pszFileName:%s)\n", this,_pszFileName));

    if (_hFileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hFileHandle);
    }

    if (_pwzURL)
    {
        if (_fDeleteFile)
        {
            DeleteUrlCacheEntryW(_pwzURL);
        }

        delete [] _pwzURL;
    }

    if (_hWinInetLockHandle)
    {
        InternetUnlockRequestFile(_hWinInetLockHandle);
        _hWinInetLockHandle = NULL;
    }

    if (_pszFileName)
    {
        delete _pszFileName;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::~CReadOnlyStreamFile\n", this));

    DEBUG_LEAVE(0);
}

HRESULT CReadOnlyStreamFile::Read(THIS_ VOID HUGEP *pv, ULONG cb, ULONG FAR *pcbRead)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::Read",
                "this=%#x, %#x, %x, %#x",
                this, pv, cb, pcbRead
                ));
                
    HRESULT hresult = NOERROR;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Read (cbBuffer:%lx)\n", this,cb));

    DWORD dwRead = 0;

    if (!ReadFile(_hFileHandle, pv, cb, &dwRead, NULL))
    {
        if (GetLastError() == ERROR_LOCK_VIOLATION)
        {
            hresult = STG_E_ACCESSDENIED;
        }
        else
        {
            hresult = E_FAIL;
        }

        if (pcbRead)
        {
            *pcbRead = 0;
        }
    }
    else
    {
        if (pcbRead)
        {
            *pcbRead = dwRead;
        }

        if (dwRead == 0)
        {
            hresult = (_fDataFullyAvailable) ? S_FALSE : E_PENDING;
        }
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Read (hr:%lx, cdRead:%lx)\n", this, hresult, dwRead));

    DEBUG_LEAVE(hresult);
    return(hresult);
}

HRESULT CReadOnlyStreamFile::Seek(THIS_ LARGE_INTEGER dlibMove, DWORD dwOrigin,
            ULARGE_INTEGER FAR *plibNewPosition)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::Seek",
                "this=%#x, %#x, %x, %#x",
                this, dlibMove, dwOrigin, plibNewPosition
                ));
                
    HRESULT hresult = NOERROR;
    DWORD   offslow, offshigh;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Seek\n", this));

    offshigh = dlibMove.HighPart;
    offslow = dlibMove.LowPart;

    offslow = SetFilePointer(_hFileHandle, offslow, (LONG *)&offshigh, dwOrigin);
    if (offslow == -1 && GetLastError() != NO_ERROR)
    {
        hresult = E_FAIL;
    }
    else if (plibNewPosition)
    {
        plibNewPosition->HighPart = offshigh;
        plibNewPosition->LowPart = offslow;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Seek\n", this));

    DEBUG_LEAVE(hresult);
    return(hresult);
}

HRESULT CReadOnlyStreamFile::CopyTo(THIS_ LPSTREAM pStm, ULARGE_INTEGER cb,
            ULARGE_INTEGER FAR *pcbRead, ULARGE_INTEGER FAR *pcbWritten)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::CopyTo",
                "this=%#x, %#x, %x, %#x, %#x",
                this, pStm, cb, pcbRead, pcbWritten
                ));
                
    HRESULT hresult = STG_E_INSUFFICIENTMEMORY;
    LPVOID  memptr = NULL;
    DWORD   readcount, writecount;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::CopyTo\n", this));

    if (cb.HighPart || (pStm == NULL))
    {
        hresult = E_INVALIDARG;
        goto CopyToExit;
    }

    // do not need to copy to ourself
    if (pStm == this)
    {
        hresult = NOERROR;
        goto CopyToExit;
    }

    memptr = new BYTE [cb.LowPart];

    if (memptr)
    {
        if (!ReadFile(_hFileHandle, memptr, cb.LowPart, &readcount, NULL))
        {
            if (GetLastError() == ERROR_LOCK_VIOLATION)
            {
                hresult = STG_E_ACCESSDENIED;
            }
            else
            {
                hresult = E_FAIL;
            }

            goto CopyToExit;
        }

        if (pcbRead)
        {
            pcbRead->HighPart = 0;
            pcbRead->LowPart = readcount;
        }

        hresult = pStm->Write(memptr, readcount, &writecount);

        if (pcbWritten && !hresult)
        {
            pcbWritten->HighPart = 0;
            pcbWritten->LowPart = writecount;
        }
    }

CopyToExit:

    if (memptr)
    {
        delete memptr;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::CopyTo\n", this));

    DEBUG_LEAVE(hresult);
    return(hresult);
}

HRESULT CReadOnlyStreamFile::LockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::LockRegion",
                "this=%#x, %#x, %x, %x",
                this, libOffset, cb, dwLockType
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamFile::LockRegion (NoOp)\n", this));

    DEBUG_LEAVE(E_NOTIMPL);
    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamFile::UnlockRegion(THIS_ ULARGE_INTEGER libOffset,
            ULARGE_INTEGER cb, DWORD dwLockType)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::UnlockRegion",
                "this=%#x, %#x, %x, %x",
                this, libOffset, cb, dwLockType
                ));
                
    UrlMkDebugOut((DEB_ISTREAM, "%p CReadOnlyStreamFile::UnlockRegion (NoOp)\n", this));

    DEBUG_LEAVE(E_NOTIMPL);
    return(E_NOTIMPL);
}

HRESULT CReadOnlyStreamFile::Stat(THIS_ STATSTG FAR *pStatStg, DWORD grfStatFlag)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::Stat",
                "this=%#x, %#x, %x",
                this, pStatStg, grfStatFlag
                ));
                
    HRESULT hresult = E_FAIL;
    DWORD   sizelow, sizehigh;
    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Stat\n", this));

    if (pStatStg)
    {
        memset(pStatStg, 0, sizeof(STATSTG));

        pStatStg->type = STGTY_STREAM;
        pStatStg->clsid = IID_IStream;

        sizelow = GetFileSize(_hFileHandle, &sizehigh);
        if (sizelow == -1 && GetLastError() != NOERROR)
        {
            goto StatExit;
        }

        pStatStg->cbSize.HighPart = sizehigh;
        pStatStg->cbSize.LowPart = sizelow;
        pStatStg->pwcsName = GetFileName();

        if (GetFileTime(_hFileHandle, &pStatStg->ctime, &pStatStg->atime, &pStatStg->mtime))
        {
            pStatStg->grfMode = GENERIC_READ;
            pStatStg->grfLocksSupported = 0;
            pStatStg->clsid = IID_IStream;
            pStatStg->grfStateBits = 0;

            hresult = NOERROR;
        }
    }

StatExit:

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Stat\n", this));

    DEBUG_LEAVE(hresult);
    return(hresult);
}

HRESULT CReadOnlyStreamFile::Clone(THIS_ LPSTREAM FAR *ppStm)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IStream::Clone",
                "this=%#x, %#x",
                this, ppStm
                ));
                
    HRESULT hr = NOERROR;
    CReadOnlyStreamFile *pCRoStm;

    UrlMkDebugOut((DEB_ISTREAM, "%p IN CReadOnlyStreamFile::Clone\n", this));

    hr = Create(_pszFileName, &pCRoStm, _pwzURL);

    if (hr == NOERROR)
    {
        *ppStm = pCRoStm;
    }
    else
    {
        *ppStm = NULL;
    }

    UrlMkDebugOut((DEB_ISTREAM, "%p OUT CReadOnlyStreamFile::Clone (hr:%lx)\n", this, hr));

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CReadOnlyStreamFile::SetHandleForUnlock(THIS_ DWORD_PTR hWinInetLockHandle, DWORD_PTR dwReserved)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IWinInetFileStream::SetHandleForUnlock",
                "this=%#x, handle=%#x",
                this, hWinInetLockHandle
                ));

    HRESULT hr = E_FAIL;

    if (hWinInetLockHandle)
    {
        _hWinInetLockHandle = (HANDLE)hWinInetLockHandle;
        hr = S_OK;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CReadOnlyStreamFile::SetDeleteFile(THIS_ DWORD_PTR dwReserved)
{
    DEBUG_ENTER((DBG_STORAGE,
                Hresult,
                "CReadOnlyStreamFile::IWinInetFileStream::SetDeleteFile",
                "this=%#x, URL=%.80wq",
                this, _pwzURL
                ));

    _fDeleteFile = TRUE;

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

