/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.h

Abstract:

    Header for util.cxx

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Revision History:

    31-Oct-1994 rfirth
        Created

--*/

#if !defined(__UTIL_H__)
#define __UTIL_H__

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

#define PLATFORM_TYPE_UNKNOWN       ((DWORD)(-1))
#define PLATFORM_TYPE_WIN95         ((DWORD)(0))
#define PLATFORM_TYPE_WINNT         ((DWORD)(1))
#define PLATFORM_TYPE_UNIX          ((DWORD)(2))

#define PLATFORM_SUPPORTS_UNICODE   0x00000001

// max header allowed by wininet in the cache

#define MAX_HEADER_SUPPORTED            2048
#define MAX_USERNAME                    128
#define DEFAULT_MAX_EXTENSION_LENGTH    8

/*
 * These should surround any call to g_pSecMgr->PUA, or MUTZ calls in urlmon
 * [ potentially ANYTHING that passes a URL to the sec. mgr. ]
 * to prevent the auto_proxy queueing calls going async.
 *
 * Why not just put it in HostBypassesProxy()?
 *
 * Try only HostBypassesProxy() and then later, we'll add to the urlmon calls
 * if need be.
 *
 * Can optimize these to surround the outermost calls to these functions where
 * the fsm is already available, but in doing so, make sure that no paths
 * that would legitimately send the autoproxy async are made sync.
 *
 *  USE ONLY ONE EACH OF THESE CALLS IN A FUNCTION, and 
 *  MAKE SURE THEY ARE PAIRED UP and IN THE SAME SCOPE.
 */

#define START_GUARD_AGAINST_ASYNC_AUTOPROXY_CALL \
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();\
    BOOL bSetNonBlocking = FALSE;\
    CFsm* pFsm;\
    if (lpThreadInfo && (pFsm=lpThreadInfo->Fsm) && lpThreadInfo->IsAsyncWorkerThread && !pFsm->IsBlocking())\
    {\
        pFsm->SetBlocking(TRUE);\
        bSetNonBlocking = TRUE;\
    }

#define END_GUARD_AGAINST_ASYNC_AUTOPROXY_CALL \
{\
    if (bSetNonBlocking)\
    {\
        pFsm->SetBlocking(FALSE);\
    }\
}

//
// types
//

//
// TRI_STATE - for places where we need to differentiate between TRUE/FALSE and
// uninitialized
//

typedef enum {
    TRI_STATE_UNKNOWN = -1,
    TRI_STATE_FALSE = 0,
    TRI_STATE_TRUE = 1
} TRI_STATE;

//
// DLL_ENTRY_POINT - maps a name to an entry point in a DLL
//

typedef struct {
    LPSTR lpszProcedureName;
    FARPROC * lplpfnProcedure;
} DLL_ENTRY_POINT, * LPDLL_ENTRY_POINT;

//
// DLL_INFO - used to dynamically load/unload libraries
//

typedef struct {
    LPSTR lpszDllName;
    HINSTANCE hModule;
    LONG LoadCount;
    DWORD dwNumberOfEntryPoints;
    LPDLL_ENTRY_POINT lpEntryPoints;
} DLL_INFO, * LPDLL_INFO;

//
// macros
//

#define IsPlatformWin95() \
    (BOOL)((GlobalPlatformType == PLATFORM_TYPE_WIN95) ? TRUE : FALSE)

#define IsPlatformWinNT() \
    (BOOL)((GlobalPlatformType == PLATFORM_TYPE_WINNT) ? TRUE : FALSE)

//#define IsUnicodeSupported() \
//    (BOOL)((PlatformSupport() & PLATFORM_SUPPORTS_UNICODE) ? TRUE : FALSE)

#define DLL_ENTRY_POINT_ELEMENT(name) \
    # name, (FARPROC *)&_I_ ## name

#define DLL_INFO_INIT(name, entryPoints) { \
    name, \
    NULL, \
    0, \
    ARRAY_ELEMENTS(entryPoints), \
    (LPDLL_ENTRY_POINT)&entryPoints \
}


#define CompareFileTime(ft1, ft2)   \
    (((*(LONGLONG UNALIGNED *)&ft1) > (*(LONGLONG UNALIGNED *)&ft2)) ? 1 : \
     (((*(LONGLONG UNALIGNED *)&ft1) == (*(LONGLONG UNALIGNED *)&ft2)) ? 0 : -1 ) )



//
// prototypes
//

LPSTR
NewString(
    IN LPCSTR String,
    IN DWORD dwLen = 0
    );

LPWSTR
NewStringW(
    IN LPCWSTR String,
    IN DWORD dwLen = 0
    );


LPSTR
CatString (
    IN LPCSTR lpszLeft,
    IN LPCSTR lpszRight
    );

HLOCAL
ResizeBuffer(
    IN HLOCAL BufferHandle,
    IN DWORD Size,
    IN BOOL Moveable
    );

LPSTR
_memrchr(
    IN LPSTR lpString,
    IN CHAR cTarget,
    IN INT iLength
    );

LPSTR
strnistr(
    IN LPSTR str1,
    IN LPSTR str2,
    IN DWORD Length
    );

LPSTR
FASTCALL
PrivateStrChr(
    IN LPCSTR lpStart,
    IN WORD wMatch
    );

DWORD
PlatformType(
    IN OUT LPDWORD lpdwVersion5os = NULL
    );

DWORD
PlatformSupport(
    VOID
    );

DWORD
GetTimeoutValue(
    IN DWORD TimeoutOption
    );

DWORD
ProbeWriteBuffer(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

DWORD
ProbeReadBuffer(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

DWORD
ProbeAndSetDword(
    IN LPDWORD lpDword,
    IN DWORD dwValue
    );

DWORD
ProbeString(
    IN LPSTR lpString,
    OUT LPDWORD lpdwStringLength
    );

DWORD
ProbeStringW(
    IN LPWSTR lpString,
    OUT LPDWORD lpdwStringLength
    );

DWORD
LoadDllEntryPoints(
    IN OUT LPDLL_INFO lpDllInfo,
    IN DWORD dwFlags
    );

//
// flags for LoadDllEntryPoints()
//

#define LDEP_PARTIAL_LOAD_OK    0x00000001  // ok if not all entry points can be loaded

DWORD
UnloadDllEntryPoints(
    IN OUT LPDLL_INFO lpDllInfo,
    IN BOOL bForce
    );

DWORD
MapInternetError(
    IN DWORD ErrorCode
    );

DWORD
CalculateHashValue(
    IN LPSTR lpszString
    );

VOID GetCurrentGmtTime(
    LPFILETIME  lpFt
    );

//DWORD DwRemoveDots (
//    char * pchPath
//    );


LPSTR GetFileExtensionFromUrl(
    LPSTR lpszUrl,
    LPDWORD lpdwLength
    );

DWORD
CheckExpired(
    HINTERNET   handle,
    BOOL    *lpfIsExpired,
    LPCACHE_ENTRY_INFO lpCEI,
    LONGLONG DefaultExpiryDelta
    );


LPTSTR
FTtoString(
    IN FILETIME *pftTime
    );

BOOL
PrintFileTimeInInternetFormat(
    FILETIME *lpft,
    LPSTR lpszBuff,
    DWORD   dwSize
);

BOOL
InternetSettingsChanged(
    VOID
    );

DWORD
ConvertSecurityInfoIntoCertInfoStruct(
    IN  LPINTERNET_SECURITY_INFO   pSecInfo,
    OUT INTERNET_CERTIFICATE_INFO *pCertificate,
    IN OUT DWORD *pcbCertificate
    );

LPTSTR
FormatCertInfo(
    IN INTERNET_CERTIFICATE_INFO *pCertInfo
    );

BOOL
CertHashToStr(
    IN LPSTR lpMD5Hash,
    IN DWORD dwMD5HashSize,
    IN OUT LPSTR *lplpszHashStr
    );

//DWORD
//UnicodeToUtf8(
//    IN LPCWSTR pwszIn,
//    IN DWORD dwInLen,
//    OUT LPBYTE pszOut,
//    IN OUT LPDWORD pdwOutLen
//    );

BOOL
StringContainsHighAnsi(
    IN LPSTR pszIn,
    IN DWORD dwInLen
    );

LPSTR
ConvertMBCSToUTF8(
    IN LPCSTR lpszMBCSHostName,
    IN DWORD dwMBCSHostNameLength,
    IN DWORD dwCodePage,
    OUT DWORD * pdwUTF8HostNameLength,
    BOOL bLowerCase = TRUE
    );
    
DWORD 
GetTickCountWrap();

char *FindNamedValue(char *pszHeader, 
                     const char *pszFieldName, 
                     unsigned long *pdwValSize);

#ifdef DONT_USE_IERT
char *
StrTokEx(
    IN OUT char ** pstring, 
    IN const char * control);

double 
StrToDbl(
    IN const char *str, 
    IN OUT char **strStop);
#endif //DONT_USE_IERT

char* StrTokEx2(char** pstring, const char* control);

BOOL IsAddressValidIPString(LPCSTR pszHostIP);

BOOL
RefreshSslState(
    VOID
    );

BOOL
IsInGUIModeSetup();

#if defined(__cplusplus)
}
#endif

#endif // defined(__UTIL_H__)
