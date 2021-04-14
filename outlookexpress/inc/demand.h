/*
**	d e m a n d . h
**	
**	Purpose: create an intelligent method of defer loading functions
**
**  Creators: jimsch, brimo, t-erikne
**  Created: 5/15/97
**	
**	Copyright (C) Microsoft Corp. 1997
*/

//
// IF YOU #INCLUDE A FILE HERE YOU PROBABLY ARE CONFUSED.
// THIS FILE IS INCLUDED BY LOTS OF PEOPLE.  THINK THRICE
// BEFORE #INCLUDING *ANYTHING* HERE.  MAKE GOOD USE
// OF FORWARD REFS INSTEAD.
//

#define USE_CRITSEC

#ifdef IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return err; \
           return VAR_##name args2;                     \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;        \
        extern TYP_##name VAR_##name;                   \
        ret WINAPI LOADER_##name args1                  \
        {                                               \
           DemandLoad##dll();                           \
           if (VAR_##name == LOADER_##name) return;     \
           VAR_##name args2;                            \
           return;                                      \
        }                                               \
        TYP_##name VAR_##name = LOADER_##name;

#else  // !IMPLEMENT_LOADER_FUNCTIONS

#define LOADER_FUNCTION(ret, name, args1, args2, err, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;			\
        extern TYP_##name VAR_##name;                   

#define LOADER_FUNCTION_VOID(ret, name, args1, args2, dll)  \
        typedef ret (WINAPI * TYP_##name) args1;			\
        extern TYP_##name VAR_##name;

#endif // IMPLEMENT_LOADER_FUNCTIONS

extern HINSTANCE g_hInst;

void InitDemandLoadedLibs();
void FreeDemandLoadedLibs();

/////////////////////////////////////
// CRYPT32.DLL

#define _CRYPT32_

BOOL DemandLoadCrypt32(void);

extern  BOOL    g_FSupportV3;
BOOL IsSMIME3Supported(void);

typedef void *HCERTSTORE;
typedef const struct _CERT_CONTEXT *PCCERT_CONTEXT;
typedef ULONG_PTR HCRYPTPROV;
typedef struct _CERT_INFO *PCERT_INFO;
typedef struct _CERT_RDN_ATTR *PCERT_RDN_ATTR;
typedef struct _CERT_NAME_INFO *PCERT_NAME_INFO;
typedef void *HCRYPTMSG;
typedef struct _CMSG_STREAM_INFO *PCMSG_STREAM_INFO;
typedef struct _CERT_RDN_ATTR *PCERT_RDN_ATTR;
typedef struct _CERT_NAME_INFO *PCCERT_NAME_INFO;

LOADER_FUNCTION( DWORD, CertRDNValueToStrA,
    (DWORD dwValueType, PCERT_RDN_VALUE_BLOB pValue, LPTSTR pszValueString, DWORD cszValueString),
    (dwValueType, pValue, pszValueString, cszValueString),
    NULL, Crypt32)
#define CertRDNValueToStrA VAR_CertRDNValueToStrA

LOADER_FUNCTION( BOOL, CertAddCertificateContextToStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pCertContext, DWORD dwAddDisposition, PCCERT_CONTEXT *ppStoreContext),
    (hCertStore, pCertContext, dwAddDisposition, ppStoreContext),
    FALSE, Crypt32)
#define CertAddCertificateContextToStore VAR_CertAddCertificateContextToStore

LOADER_FUNCTION( BOOL, CertAddEncodedCertificateToStore,
    (HCERTSTORE hCertStore,
     DWORD dwCertEncodingType,
     BYTE *pbCertEncoded,
     DWORD cbCertEncoded,
     DWORD dwAddDisposition,
     PCCERT_CONTEXT *ppStoreContext),
    (hCertStore, dwCertEncodingType, pbCertEncoded, cbCertEncoded, dwAddDisposition, ppStoreContext),
    FALSE, Crypt32)
#define CertAddEncodedCertificateToStore VAR_CertAddEncodedCertificateToStore

LOADER_FUNCTION( PCCERT_CONTEXT, CertEnumCertificatesInStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, pPrevCertContext),
    NULL, Crypt32)
#define CertEnumCertificatesInStore VAR_CertEnumCertificatesInStore

LOADER_FUNCTION( BOOL, CryptDecodeObject,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, void *pvStructInfo, DWORD *pcbStructInfo),
    (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, pvStructInfo, pcbStructInfo),
    FALSE, Crypt32)
#define CryptDecodeObject VAR_CryptDecodeObject

LOADER_FUNCTION( BOOL, CryptEncodeObject,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const void * pvStructInfo, BYTE * pbEncoded, DWORD * pcbEncoded),
    (dwCertEncodingType, lpszStructType, pvStructInfo, pbEncoded, pcbEncoded),
    FALSE, Crypt32)
#define CryptEncodeObject VAR_CryptEncodeObject

LOADER_FUNCTION( BOOL, CryptDecodeObjectEx,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const BYTE *pbEncoded, DWORD cbEncoded, DWORD dwFlags, PCRYPT_DECODE_PARA pDecodePara, void *pvStructInfo, DWORD *pcbStructInfo),
    (dwCertEncodingType, lpszStructType, pbEncoded, cbEncoded, dwFlags, pDecodePara, pvStructInfo, pcbStructInfo),
    FALSE, Crypt32)
#define CryptDecodeObjectEx VAR_CryptDecodeObjectEx

LOADER_FUNCTION( BOOL, CryptEncodeObjectEx,
    (DWORD dwCertEncodingType, LPCSTR lpszStructType, const void * pvStructInfo, DWORD dwFlags, PCRYPT_ENCODE_PARA pEncodePara, void * pbEncoded, DWORD * pcbEncoded),
    (dwCertEncodingType, lpszStructType, pvStructInfo, dwFlags, pEncodePara, pbEncoded, pcbEncoded),
    FALSE, Crypt32)
#define CryptEncodeObjectEx VAR_CryptEncodeObjectEx


LOADER_FUNCTION(PCERT_RDN_ATTR, CertFindRDNAttr,
    (LPCSTR pszObjId, PCERT_NAME_INFO pName),
    (pszObjId, pName),
    NULL, Crypt32)
#define CertFindRDNAttr VAR_CertFindRDNAttr

LOADER_FUNCTION( BOOL, CertFreeCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext),
    TRUE, Crypt32)  // return success since GLE() is meaningless
#define CertFreeCertificateContext VAR_CertFreeCertificateContext

LOADER_FUNCTION( PCCERT_CONTEXT, CertDuplicateCertificateContext,
    (PCCERT_CONTEXT pCertContext),
    (pCertContext), NULL, Crypt32)
#define CertDuplicateCertificateContext VAR_CertDuplicateCertificateContext

LOADER_FUNCTION( PCCERT_CONTEXT, CertFindCertificateInStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, DWORD dwFindFlags, DWORD dwFindType, const void *pvFindPara, PCCERT_CONTEXT pPrevCertContext),
    (hCertStore, dwCertEncodingType, dwFindFlags, dwFindType, pvFindPara, pPrevCertContext),
    NULL, Crypt32)
#define CertFindCertificateInStore VAR_CertFindCertificateInStore

LOADER_FUNCTION( LONG, CertVerifyTimeValidity,
    (LPFILETIME pTimeToVerify, PCERT_INFO pCertInfo),
    (pTimeToVerify, pCertInfo),
    1, Crypt32)
#define CertVerifyTimeValidity VAR_CertVerifyTimeValidity

LOADER_FUNCTION( BOOL, CertCompareCertificate,
    (DWORD dwCertEncodingType, PCERT_INFO pCertId1, PCERT_INFO pCertId2),
    (dwCertEncodingType, pCertId1, pCertId2),
    FALSE, Crypt32)
#define CertCompareCertificate VAR_CertCompareCertificate

LOADER_FUNCTION( BOOL, CertCompareCertificateName,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pCertName1, PCERT_NAME_BLOB pCertName2),
    (dwCertEncodingType, pCertName1, pCertName2),
    FALSE, Crypt32)
#define CertCompareCertificateName VAR_CertCompareCertificateName

LOADER_FUNCTION( BOOL, CertCompareIntegerBlob,
    (PCRYPT_INTEGER_BLOB pInt1, PCRYPT_INTEGER_BLOB pInt2),
    (pInt1, pInt2),
    FALSE, Crypt32)
#define CertCompareIntegerBlob VAR_CertCompareIntegerBlob

LOADER_FUNCTION( HCERTSTORE, CertOpenStore,
    (LPCSTR lpszStoreProvider, DWORD dwEncodingType, HCRYPTPROV hCryptProv, DWORD dwFlags, const void *pvPara),
    (lpszStoreProvider, dwEncodingType, hCryptProv, dwFlags, pvPara),
    NULL, Crypt32)
#define CertOpenStore VAR_CertOpenStore

LOADER_FUNCTION( HCERTSTORE, CertDuplicateStore,
    (HCERTSTORE hCertStore),
    (hCertStore),
    NULL, Crypt32)
#define CertDuplicateStore VAR_CertDuplicateStore

LOADER_FUNCTION( BOOL, CertCloseStore,
    (HCERTSTORE hCertStore, DWORD dwFlags),
    (hCertStore, dwFlags),
    FALSE, Crypt32)
#define CertCloseStore VAR_CertCloseStore

LOADER_FUNCTION( BOOL, CertControlStore,
    (
        HCERTSTORE hCertStore,
        DWORD dwFlags,
        DWORD dwCtrlType,
        void const *pvCtrlPara
    ),
    (
        hCertStore,
        dwFlags,
        dwCtrlType,
        pvCtrlPara
    ),
    FALSE, Crypt32)
#define CertControlStore VAR_CertControlStore

LOADER_FUNCTION( PCCERT_CONTEXT, CertGetSubjectCertificateFromStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, PCERT_INFO pCertId),
    (hCertStore, dwCertEncodingType, pCertId),
    NULL, Crypt32)
#define CertGetSubjectCertificateFromStore VAR_CertGetSubjectCertificateFromStore

LOADER_FUNCTION( PCCERT_CONTEXT, CertGetIssuerCertificateFromStore,
    (HCERTSTORE hCertStore, PCCERT_CONTEXT pSubjectContext, PCCERT_CONTEXT pPrevIssuerContext, DWORD *pdwFlags),
    (hCertStore, pSubjectContext, pPrevIssuerContext, pdwFlags),
    NULL, Crypt32)
#define CertGetIssuerCertificateFromStore VAR_CertGetIssuerCertificateFromStore

LOADER_FUNCTION( BOOL, CertGetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, void *pvData, DWORD *pcbData),
    (pCertContext, dwPropId, pvData, pcbData),
    FALSE, Crypt32)
#define CertGetCertificateContextProperty VAR_CertGetCertificateContextProperty

LOADER_FUNCTION( HCRYPTMSG, CryptMsgOpenToEncode,
    (DWORD dwMsgEncodingType, DWORD dwFlags, DWORD dwMsgType, void const *pvMsgEncodeInfo, LPSTR pszInnerContentObjID, PCMSG_STREAM_INFO pStreamInfo),
    (dwMsgEncodingType, dwFlags, dwMsgType, pvMsgEncodeInfo, pszInnerContentObjID, pStreamInfo),
    NULL, Crypt32)
#define CryptMsgOpenToEncode VAR_CryptMsgOpenToEncode

LOADER_FUNCTION( HCRYPTMSG, CryptMsgOpenToDecode,
    (DWORD dwMsgEncodingType, DWORD dwFlags, DWORD dwMsgType, HCRYPTPROV hCryptProv, PCERT_INFO pRecipientInfo, PCMSG_STREAM_INFO pStreamInfo),
    (dwMsgEncodingType, dwFlags, dwMsgType, hCryptProv, pRecipientInfo, pStreamInfo),
    NULL, Crypt32)
#define CryptMsgOpenToDecode VAR_CryptMsgOpenToDecode

LOADER_FUNCTION( BOOL, CryptMsgUpdate,
    (HCRYPTMSG hCryptMsg, const BYTE *pbData, DWORD cbData, BOOL fFinal),
    (hCryptMsg, pbData, cbData, fFinal),
    FALSE, Crypt32)
#define CryptMsgUpdate VAR_CryptMsgUpdate

LOADER_FUNCTION( BOOL, CryptMsgGetParam,
    (HCRYPTMSG hCryptMsg, DWORD dwParamType, DWORD dwIndex, void *pvData, DWORD *pcbData),
    (hCryptMsg, dwParamType, dwIndex, pvData, pcbData),
    FALSE, Crypt32)
#define CryptMsgGetParam VAR_CryptMsgGetParam

LOADER_FUNCTION( BOOL, CryptMsgControl,
    (HCRYPTMSG hCryptMsg, DWORD dwFlags, DWORD dwCtrlType, void const *pvCtrlPara),
    (hCryptMsg, dwFlags, dwCtrlType, pvCtrlPara),
    FALSE, Crypt32)
#define CryptMsgControl VAR_CryptMsgControl

LOADER_FUNCTION( BOOL, CryptMsgClose,
    (HCRYPTMSG hCryptMsg),
    (hCryptMsg),
    TRUE, Crypt32)  // return success since GLE() is meaningless
#define CryptMsgClose VAR_CryptMsgClose

LOADER_FUNCTION( BOOL, CertAddEncodedCRLToStore,
    (HCERTSTORE hCertStore, DWORD dwCertEncodingType, const BYTE *pbCrlEncoded, DWORD cbCrlEncoded, DWORD dwAddDisposition, PCCRL_CONTEXT *ppCrlContext),
    (hCertStore, dwCertEncodingType, pbCrlEncoded, cbCrlEncoded, dwAddDisposition, ppCrlContext),
    FALSE, Crypt32)
#define CertAddEncodedCRLToStore VAR_CertAddEncodedCRLToStore

LOADER_FUNCTION( PCCRL_CONTEXT, CertEnumCRLsInStore,
    (HCERTSTORE hCertStore, PCCRL_CONTEXT pPrevCrlContext),
    (hCertStore, pPrevCrlContext),
    NULL, Crypt32)
#define CertEnumCRLsInStore VAR_CertEnumCRLsInStore

LOADER_FUNCTION( PCERT_EXTENSION, CertFindExtension,
    (LPCSTR pszObjId, DWORD cExtensions, CERT_EXTENSION rgExtensions[]),
    (pszObjId, cExtensions, rgExtensions),
    NULL, Crypt32)
#define CertFindExtension VAR_CertFindExtension

LOADER_FUNCTION( PCERT_CONTEXT, CertCreateCertificateContext,
    (DWORD dwCertEncodingType, const BYTE * pbCertEncoded, DWORD cbCertEncoded),
    (dwCertEncodingType, pbCertEncoded, cbCertEncoded),
    NULL, Crypt32)
#define CertCreateCertificateContext VAR_CertCreateCertificateContext

LOADER_FUNCTION( DWORD, CertNameToStrA,
    (DWORD dwCertEncodingType, PCERT_NAME_BLOB pName, DWORD dwStrType,
    LPSTR psz, DWORD csz),
    (dwCertEncodingType, pName, dwStrType, psz, csz),
    NULL, Crypt32)
#define CertNameToStrA VAR_CertNameToStrA

LOADER_FUNCTION( BOOL, CertGetEnhancedKeyUsage,
    (PCCERT_CONTEXT pCertContext, DWORD dwFlags, PCERT_ENHKEY_USAGE pUsage,
     DWORD *pcbUsage),
    (pCertContext, dwFlags, pUsage, pcbUsage),
    FALSE, Crypt32)
#define CertGetEnhancedKeyUsage VAR_CertGetEnhancedKeyUsage

LOADER_FUNCTION( DWORD, CertAddStoreToCollection,
    (HCERTSTORE hCollectionStore, HCERTSTORE hSiblingStore, DWORD dwUpdateFlags,
     DWORD dwPriority),
    (hCollectionStore, hSiblingStore, dwUpdateFlags, dwPriority),
    NULL, Crypt32)
#define CertAddStoreToCollection VAR_CertAddStoreToCollection

LOADER_FUNCTION( BOOL, CertSetCertificateContextProperty,
    (PCCERT_CONTEXT pCertContext, DWORD dwPropId, DWORD dwFlags, const void *pvData),
    (pCertContext, dwPropId, dwFlags, pvData),
    NULL, Crypt32)
#define CertSetCertificateContextProperty VAR_CertSetCertificateContextProperty

LOADER_FUNCTION( BOOL, CertVerifySubjectCertificateContext,
    (PCCERT_CONTEXT pSubject, PCCERT_CONTEXT pIssuer, DWORD *pdwFlags),
    (pSubject, pIssuer, pdwFlags),
    FALSE, Crypt32)
#define CertVerifySubjectCertificateContext VAR_CertVerifySubjectCertificateContext

LOADER_FUNCTION_VOID( void, CertFreeCertificateChain,
    (PCCERT_CHAIN_CONTEXT pChainContext),
    (pChainContext),
    Crypt32)
#define CertFreeCertificateChain VAR_CertFreeCertificateChain

LOADER_FUNCTION( BOOL, CertGetCertificateChain,
    (HCERTCHAINENGINE hChainEngine, PCCERT_CONTEXT pCertContext, LPFILETIME pTime, HCERTSTORE hAdditionalStore, 
     PCERT_CHAIN_PARA pChainPara, DWORD dwFlags, LPVOID pvReserved, PCCERT_CHAIN_CONTEXT* ppChainContext),
    (hChainEngine, pCertContext, pTime, hAdditionalStore, pChainPara, dwFlags, pvReserved, ppChainContext),
    FALSE, Crypt32)
#define CertGetCertificateChain VAR_CertGetCertificateChain

LOADER_FUNCTION( BOOL, CertVerifyCertificateChainPolicy,
    (LPCSTR pszPolicyOID, PCCERT_CHAIN_CONTEXT pChainContext, PCERT_CHAIN_POLICY_PARA pPolicyPara, PCERT_CHAIN_POLICY_STATUS pPolicyStatus),
    (pszPolicyOID, pChainContext, pPolicyPara, pPolicyStatus),
    FALSE, Crypt32)
#define CertVerifyCertificateChainPolicy VAR_CertVerifyCertificateChainPolicy

/////////////////////////////////////
// CRYPTDLG.DLL

#define _CRYPTDLG_

// Old cert dialogs
typedef struct tagCERT_VIEWPROPERTIES_STRUCT_A *PCERT_VIEWPROPERTIES_STRUCT_A;
typedef struct tagCSSA *PCERT_SELECT_STRUCT_A;

BOOL DemandLoadCryptDlg();

LOADER_FUNCTION( BOOL, CertViewPropertiesA,
    (PCERT_VIEWPROPERTIES_STRUCT_A pCertViewInfo),
    (pCertViewInfo),
    FALSE, CryptDlg)
#define CertViewPropertiesA VAR_CertViewPropertiesA

LOADER_FUNCTION( DWORD, GetFriendlyNameOfCertA,
    (PCCERT_CONTEXT pccert, LPSTR pchBuffer, DWORD cchBuffer),
    (pccert, pchBuffer, cchBuffer),
    0, CryptDlg)
#define GetFriendlyNameOfCertA VAR_GetFriendlyNameOfCertA

LOADER_FUNCTION( BOOL, CertSelectCertificateA,
    (PCERT_SELECT_STRUCT_A pCertSelectInfo),
    (pCertSelectInfo),
    FALSE, CryptDlg)
#define CertSelectCertificateA VAR_CertSelectCertificateA

/////////////////////////////////////
// WINTRUST.DLL

BOOL DemandLoadWinTrust();

LOADER_FUNCTION( LONG, WinVerifyTrust,
    (HWND hwnd, GUID *ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData),
    0, WinTrust)
#define WinVerifyTrust VAR_WinVerifyTrust

/////////////////////////////////////
// WININET.DLL

#include <wininet.h>

#define _WININET_

typedef struct _INTERNET_CACHE_ENTRY_INFOA INTERNET_CACHE_ENTRY_INFOA;

BOOL DemandLoadWinINET();

LOADER_FUNCTION( BOOL, RetrieveUrlCacheEntryFileA,
    (LPCSTR  lpszUrlName, INTERNET_CACHE_ENTRY_INFOA *lpCacheEntryInfo, LPDWORD lpdwCacheEntryInfoBufferSize, DWORD dwReserved),
    (lpszUrlName, lpCacheEntryInfo, lpdwCacheEntryInfoBufferSize, dwReserved),
    FALSE, WinINET)
#define RetrieveUrlCacheEntryFileA VAR_RetrieveUrlCacheEntryFileA

LOADER_FUNCTION( BOOL, CommitUrlCacheEntryA,
    (LPCSTR lpszUrlName, LPCSTR lpszLocalFileName, FILETIME ExpireTime, FILETIME LastModifiedTime, DWORD CacheEntryType, LPBYTE lpHeaderInfo,
     DWORD dwHeaderSize, LPCSTR lpszFileExtension, LPCSTR lpszOriginalUrl),
    (lpszUrlName, lpszLocalFileName, ExpireTime, LastModifiedTime, CacheEntryType, lpHeaderInfo,
     dwHeaderSize, lpszFileExtension, lpszOriginalUrl),
    FALSE, WinINET)
#define CommitUrlCacheEntryA VAR_CommitUrlCacheEntryA

LOADER_FUNCTION( BOOL, CreateUrlCacheEntryA,
    (LPCSTR lpszUrlName, DWORD dwExpectedFileSize, LPCSTR lpszFileExtension, LPSTR lpszFileName, DWORD dwReserved),
    (lpszUrlName, dwExpectedFileSize, lpszFileExtension, lpszFileName, dwReserved),
    FALSE, WinINET)
#define CreateUrlCacheEntryA VAR_CreateUrlCacheEntryA
    
LOADER_FUNCTION( BOOL, DeleteUrlCacheEntryA,
    (LPCSTR lpszUrlName),
    (lpszUrlName),
    FALSE, WinINET)
#define DeleteUrlCacheEntryA VAR_DeleteUrlCacheEntryA

LOADER_FUNCTION( BOOL, UnlockUrlCacheEntryFileA,
    (LPCSTR  lpszUrlName, DWORD dwRes),
    (lpszUrlName, dwRes),
    FALSE, WinINET)
#define UnlockUrlCacheEntryFileA VAR_UnlockUrlCacheEntryFileA

LOADER_FUNCTION( BOOL, InternetQueryOptionA,
    (HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, LPDWORD lpdwBufferLength),
    (hInternet, dwOption, lpBuffer, lpdwBufferLength),
    NULL, WinINET)
#define InternetQueryOptionA VAR_InternetQueryOptionA

LOADER_FUNCTION( BOOL, InternetSetOptionA,
    (HINTERNET hInternet, DWORD dwOption, LPVOID lpBuffer, DWORD dwBufferLength),
    (hInternet, dwOption, lpBuffer, dwBufferLength),
    NULL, WinINET)
#define InternetSetOptionA VAR_InternetSetOptionA

LOADER_FUNCTION( BOOL, InternetDialA,
    (HWND hwndParent, LPSTR lpszConnectoid, DWORD dwFlags, DWORD_PTR *lpdwConnection, DWORD dwReserved),
    (hwndParent, lpszConnectoid, dwFlags, lpdwConnection, dwReserved),
    NULL, WinINET)
#define InternetDialA VAR_InternetDialA

LOADER_FUNCTION( BOOL, InternetHangUp,
    (DWORD_PTR dwConnection, DWORD dwReserved),
    (dwConnection, dwReserved),
    NULL, WinINET)
#define InternetHangUp VAR_InternetHangUp

LOADER_FUNCTION(BOOL, InternetGetConnectedStateExA, 
                (LPDWORD dwFlags,  LPTSTR szconn, DWORD size, DWORD reserved),
                (dwFlags, szconn, size, reserved),
                FALSE, WinINET)
#define InternetGetConnectedStateExA  VAR_InternetGetConnectedStateExA

LOADER_FUNCTION(BOOL, InternetCombineUrlA, 
                (LPCSTR lpszBaseUrl, LPCSTR lpszRelativeUrl, LPSTR lpszBuffer, LPDWORD lpdwBufferLength, DWORD dwFlags),
                (lpszBaseUrl, lpszRelativeUrl, lpszBuffer, lpdwBufferLength, dwFlags),
                FALSE, WinINET)
#define InternetCombineUrlA  VAR_InternetCombineUrlA

LOADER_FUNCTION(BOOL, InternetCrackUrlA, 
                (LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTS lpUrlComponents),
                (lpszUrl, dwUrlLength, dwFlags, lpUrlComponents),
                FALSE, WinINET)
#define InternetCrackUrlA  VAR_InternetCrackUrlA

LOADER_FUNCTION(BOOL, InternetCloseHandle, 
                (HINTERNET hInternet),
                (hInternet),
                FALSE, WinINET)
#define   InternetCloseHandle   VAR_InternetCloseHandle

LOADER_FUNCTION(BOOL, InternetReadFile, 
                (HINTERNET hInternet, LPVOID lpBuffer, DWORD dwNumberOfBytesToRead, LPDWORD lpdwNumberOfBytesRead),
                (hInternet, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead),
                FALSE, WinINET)
#define   InternetReadFile   VAR_InternetReadFile

LOADER_FUNCTION(HINTERNET, InternetConnectA, 
                (HINTERNET hInternet, LPCSTR lpszServerName, INTERNET_PORT nServerPort,
                            LPCSTR lpszUserName,LPCSTR lpszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext),
                (hInternet, lpszServerName, nServerPort, lpszUserName, lpszPassword, dwService, dwFlags, dwContext),
                NULL, WinINET)
#define   InternetConnectA   VAR_InternetConnectA

LOADER_FUNCTION(HINTERNET, InternetOpenA, 
                (LPCSTR lpszAgent, DWORD dwAccessType, LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags),
                (lpszAgent, dwAccessType, lpszProxy, lpszProxyBypass, dwFlags),
                NULL, WinINET)
#define   InternetOpenA   VAR_InternetOpenA

LOADER_FUNCTION(INTERNET_STATUS_CALLBACK, InternetSetStatusCallbackA,
                (HINTERNET hInternet, INTERNET_STATUS_CALLBACK lpfnInternetCallback),
                (hInternet, lpfnInternetCallback),
                NULL, WinINET)
#define   InternetSetStatusCallbackA   VAR_InternetSetStatusCallbackA

LOADER_FUNCTION(BOOL, HttpQueryInfoA, 
                (HINTERNET hRequest, DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpdwBufferLength, LPDWORD lpdwIndex),
                (hRequest, dwInfoLevel, lpBuffer, lpdwBufferLength, lpdwIndex),
                FALSE, WinINET)
#define   HttpQueryInfoA   VAR_HttpQueryInfoA

LOADER_FUNCTION(HINTERNET, HttpOpenRequestA, 
                (HINTERNET hConnect, LPCSTR lpszVerb, LPCSTR lpszObjectName, LPCSTR lpszVersion,
                            LPCSTR lpszReferrer, LPCSTR FAR * lplpszAcceptTypes, DWORD dwFlags, DWORD_PTR dwContext),
                ( hConnect, lpszVerb, lpszObjectName, lpszVersion, lpszReferrer, lplpszAcceptTypes, dwFlags, dwContext),
                NULL, WinINET)
#define   HttpOpenRequestA   VAR_HttpOpenRequestA

LOADER_FUNCTION(BOOL, HttpAddRequestHeadersA, 
                (HINTERNET hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwModifiers),
                (hRequest, lpszHeaders, dwHeadersLength, dwModifiers),
                FALSE, WinINET)
#define   HttpAddRequestHeadersA   VAR_HttpAddRequestHeadersA

LOADER_FUNCTION(BOOL, HttpSendRequestA, 
                (HINTERNET hRequest, LPCSTR lpszHeaders, DWORD dwHeadersLength, LPVOID lpOptional, DWORD dwOptionalLength),
                (hRequest, lpszHeaders, dwHeadersLength, lpOptional, dwOptionalLength),
                FALSE, WinINET)
#define   HttpSendRequestA   VAR_HttpSendRequestA

LOADER_FUNCTION(BOOL, InternetWriteFile, 
                (HINTERNET hFile, LPCVOID lpBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD lpdwNumberOfBytesWritten),
                (hFile, lpBuffer, dwNumberOfBytesToWrite, lpdwNumberOfBytesWritten),
                FALSE, WinINET)
#define   InternetWriteFile   VAR_InternetWriteFile

LOADER_FUNCTION(BOOL, HttpEndRequestA, 
                (HINTERNET hRequest, LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext),
                (hRequest, lpBuffersOut, dwFlags, dwContext),
                FALSE, WinINET)
#define   HttpEndRequestA   VAR_HttpEndRequestA

LOADER_FUNCTION(BOOL, HttpSendRequestExA, 
                (HINTERNET hRequest, LPINTERNET_BUFFERSA lpBuffersIn,
                                LPINTERNET_BUFFERSA lpBuffersOut, DWORD dwFlags, DWORD_PTR dwContext),
                (hRequest, lpBuffersIn, lpBuffersOut, dwFlags, dwContext),
                FALSE, WinINET)
#define   HttpSendRequestExA   VAR_HttpSendRequestExA

/////////////////////////////////////
// SHELL32.DLL

#include <shellapi.h>
#include <shlobj.h>
#include <shlobjp.h>

BOOL DemandLoadSHELL32();

LOADER_FUNCTION(BOOL, SHFree, // Actually a void
   (LPVOID lpv),
   (lpv),
   FALSE, SHELL32)
#define SHFree VAR_SHFree

LOADER_FUNCTION(BOOL, SHGetPathFromIDListA,
    (LPCITEMIDLIST pidl, LPSTR pszPath),
    (pidl, pszPath),
    FALSE, SHELL32)
#define SHGetPathFromIDListA VAR_SHGetPathFromIDListA

LOADER_FUNCTION(BOOL, SHGetPathFromIDListW,
    (LPCITEMIDLIST pidl, LPWSTR pwszPath),
    (pidl, pwszPath),
    FALSE, SHELL32)
#define SHGetPathFromIDListW VAR_SHGetPathFromIDListW

LOADER_FUNCTION(HRESULT, SHGetSpecialFolderLocation,
    (HWND hwndOwner, int nFolder, LPITEMIDLIST * ppidl),
    (hwndOwner, nFolder, ppidl),
    E_FAIL, SHELL32)
#define SHGetSpecialFolderLocation VAR_SHGetSpecialFolderLocation

LOADER_FUNCTION(LPITEMIDLIST, SHBrowseForFolderA,
    (LPBROWSEINFOA lpbi),
    (lpbi),
    NULL, SHELL32)
#define SHBrowseForFolderA VAR_SHBrowseForFolderA

LOADER_FUNCTION(LPITEMIDLIST, SHBrowseForFolderW,
    (LPBROWSEINFOW lpbi),
    (lpbi),
    NULL, SHELL32)
#define SHBrowseForFolderW VAR_SHBrowseForFolderW

LOADER_FUNCTION(HRESULT, SHSetUnreadMailCountW,
    (LPCWSTR pszMailAddress, DWORD dwCount, LPCWSTR pszShellExecuteCommand),
    (pszMailAddress, dwCount, pszShellExecuteCommand),
    NULL, SHELL32)
#define SHSetUnreadMailCountW VAR_SHSetUnreadMailCountW

LOADER_FUNCTION(HINSTANCE, ShellExecuteA,
    (HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd),
    (hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd),
    NULL, SHELL32)
#define ShellExecuteA VAR_ShellExecuteA

LOADER_FUNCTION(BOOL, ShellExecuteExA,
    (LPSHELLEXECUTEINFOA lpExecInfo),
    (lpExecInfo),
    FALSE, SHELL32)
#define ShellExecuteExA VAR_ShellExecuteExA

LOADER_FUNCTION(UINT, DragQueryFileA,
    (HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cb),
    (hDrop, iFile, lpszFile, cb),
    0, SHELL32)
#define DragQueryFileA VAR_DragQueryFileA

LOADER_FUNCTION(DWORD, SHGetFileInfoA,
    (LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR *psfi, UINT cbFileInfo, UINT uFlags),
    (pszPath, dwFileAttributes, psfi, cbFileInfo, uFlags),
    0, SHELL32)
#define SHGetFileInfoA VAR_SHGetFileInfoA

LOADER_FUNCTION(BOOL, Shell_NotifyIconA,
    (DWORD dwMessage, PNOTIFYICONDATAA lpData),
    (dwMessage, lpData),
    FALSE, SHELL32)
#define Shell_NotifyIconA VAR_Shell_NotifyIconA

LOADER_FUNCTION(int, SHFileOperationA,
    (LPSHFILEOPSTRUCTA lpfo),
    (lpfo),
    -1, SHELL32)
#define SHFileOperationA VAR_SHFileOperationA

LOADER_FUNCTION(HICON, ExtractIconA,
    (HINSTANCE hInst, LPCSTR lpszExeFileName, UINT nIconIndex),
    (hInst, lpszExeFileName, nIconIndex),
    NULL, SHELL32)
#define ExtractIconA VAR_ExtractIconA

/////////////////////////////////////
// COMDLG32.DLL

#include <commdlg.h>

BOOL DemandLoadCOMDLG32();

LOADER_FUNCTION(BOOL, GetSaveFileNameA,
    (LPOPENFILENAME pof),
    (pof),
    FALSE, COMDLG32)
#define GetSaveFileNameA VAR_GetSaveFileNameA

LOADER_FUNCTION(BOOL, GetOpenFileNameA,
    (LPOPENFILENAME pof),
    (pof),
    FALSE, COMDLG32)
#define GetOpenFileNameA VAR_GetOpenFileNameA

LOADER_FUNCTION(BOOL, ChooseFontA,
    (LPCHOOSEFONT pcf),
    (pcf),
    FALSE, COMDLG32)
#define ChooseFontA VAR_ChooseFontA

/////////////////////////////////////
// VERSION.DLL

BOOL DemandLoadVERSION();

LOADER_FUNCTION(BOOL, VerQueryValueA,
    (const LPVOID pBlock, LPSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen),
    (pBlock, lpSubBlock, lplpBuffer, puLen),
    FALSE, VERSION)
#define VerQueryValueA VAR_VerQueryValueA

LOADER_FUNCTION(BOOL, GetFileVersionInfoA,
    (PSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData),
    (lptstrFilename, dwHandle, dwLen, lpData),
    FALSE, VERSION)
#define GetFileVersionInfoA VAR_GetFileVersionInfoA

LOADER_FUNCTION(DWORD, GetFileVersionInfoSizeA,
    (LPSTR lptstrFilename, LPDWORD lpdwHandle),
    (lptstrFilename, lpdwHandle),
    0, VERSION)
#define GetFileVersionInfoSizeA VAR_GetFileVersionInfoSizeA

/////////////////////////////////////
// URLMON.DLL

BOOL DemandLoadURLMON();

LOADER_FUNCTION(HRESULT, CreateURLMoniker,
    (LPMONIKER pMkCtx, LPCWSTR szURL, LPMONIKER FAR *ppmk),
    (pMkCtx, szURL, ppmk),
    E_FAIL, URLMON)
#define CreateURLMoniker VAR_CreateURLMoniker

LOADER_FUNCTION(HRESULT, URLOpenBlockingStreamA,
    (LPUNKNOWN pUnk,LPCSTR pURL,LPSTREAM* ppstm,DWORD i,LPBINDSTATUSCALLBACK p),
    (pUnk, pURL, ppstm, i, p),
    E_FAIL, URLMON)
#define URLOpenBlockingStreamA VAR_URLOpenBlockingStreamA

LOADER_FUNCTION(HRESULT, FindMimeFromData,
    (LPBC pBC, LPCWSTR pwzUrl, LPVOID pBuffer, DWORD cbSize, LPCWSTR pwzMimeProposed, DWORD dwMimeFlags, LPWSTR *ppwzMimeOut, DWORD dwReserved),
    (pBC, pwzUrl, pBuffer, cbSize, pwzMimeProposed, dwMimeFlags, ppwzMimeOut, dwReserved),
    E_FAIL, URLMON)
#define FindMimeFromData VAR_FindMimeFromData

LOADER_FUNCTION( HRESULT, CoInternetCombineUrl,
    (LPCWSTR pwzBaseUrl, LPCWSTR pwzRelativeUrl, DWORD dwCombineFlags, LPWSTR pszResult, DWORD cchResult, DWORD *pcchResult, DWORD dwReserved),
    (pwzBaseUrl, pwzRelativeUrl, dwCombineFlags, pszResult, cchResult, pcchResult, dwReserved),
    E_FAIL, URLMON)
#define CoInternetCombineUrl VAR_CoInternetCombineUrl

LOADER_FUNCTION( HRESULT, RegisterBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb, IBindStatusCallback**  ppBSCBPrev, DWORD dwReserved),
    (pBC, pBSCb, ppBSCBPrev, dwReserved),
    E_FAIL, URLMON)
#define RegisterBindStatusCallback VAR_RegisterBindStatusCallback

LOADER_FUNCTION( HRESULT, RevokeBindStatusCallback,
    (LPBC pBC, IBindStatusCallback *pBSCb),
    (pBC, pBSCb),
    E_FAIL, URLMON)
#define RevokeBindStatusCallback VAR_RevokeBindStatusCallback

LOADER_FUNCTION( HRESULT, FaultInIEFeature,
    (HWND hwnd,     uCLSSPEC    *classpec, QUERYCONTEXT *pQuery, DWORD dwFlags),
    (hwnd, classpec, pQuery, dwFlags),
    E_FAIL, URLMON)
#define FaultInIEFeature VAR_FaultInIEFeature

LOADER_FUNCTION( HRESULT, CoInternetGetSecurityUrl,
    (LPCWSTR pwzUrl, LPWSTR  *ppwzSecUrl, PSUACTION  psuAction, DWORD   dwReserved),
    (pwzUrl, ppwzSecUrl, psuAction, dwReserved),
    E_FAIL, URLMON)
#define CoInternetGetSecurityUrl VAR_CoInternetGetSecurityUrl

LOADER_FUNCTION( HRESULT, ObtainUserAgentString,
    (DWORD dwOption, LPSTR pszUAOut, DWORD* cbSize),
    (dwOption, pszUAOut, cbSize),
    E_FAIL, URLMON)
#define ObtainUserAgentString VAR_ObtainUserAgentString

LOADER_FUNCTION( HRESULT, CoInternetCreateSecurityManager,
    (IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved),
    (pSP, ppSM, dwReserved),
    E_FAIL, URLMON)
#define CoInternetCreateSecurityManager VAR_CoInternetCreateSecurityManager

/////////////////////////////////////
// MLANG.DLL

#include <mlang.h>

BOOL DemandLoadMLANG(void);

LOADER_FUNCTION( HRESULT, IsConvertINetStringAvailable,
    (DWORD dwSrcEncoding, DWORD dwDstEncoding),
    (dwSrcEncoding, dwDstEncoding),
    S_FALSE, MLANG)
#define IsConvertINetStringAvailable VAR_IsConvertINetStringAvailable

LOADER_FUNCTION( HRESULT, ConvertINetString,
    (LPDWORD lpdwMode, DWORD dwSrcEncoding, DWORD dwDstEncoding, LPCSTR lpSrcStr, LPINT lpnSrcSize, LPSTR lpDstStr, LPINT lpnDstSize),
    (lpdwMode, dwSrcEncoding, dwDstEncoding, lpSrcStr, lpnSrcSize, lpDstStr, lpnDstSize),
    E_FAIL, MLANG)
#define ConvertINetString VAR_ConvertINetString

/////////////////////////////////////
// SHDOCVW.DLL

#include <shlobjp.h>
BOOL DemandLoadSHDOCVW();

LOADER_FUNCTION(HRESULT, AddUrlToFavorites,
    (HWND hwnd, LPWSTR pszUrlW, LPWSTR pszTitleW, BOOL fDisplayUI),
    (hwnd, pszUrlW, pszTitleW, fDisplayUI),
    E_FAIL, SHDOCVW)
#define AddUrlToFavorites VAR_AddUrlToFavorites

LOADER_FUNCTION(long, SetQueryNetSessionCount,
    (enum SessionOp Op),
    (Op),
    0, SHDOCVW)
#define SetQueryNetSessionCount VAR_SetQueryNetSessionCount

LOADER_FUNCTION_VOID(void, SetShellOfflineState,
    (BOOL fPutOfflineState),
    (fPutOfflineState),
    SHDOCVW)
#define SetShellOfflineState    VAR_SetShellOfflineState


/////////////////////////////////////
// INETCPL.CPL

BOOL DemandLoadINETCPL();

LOADER_FUNCTION(int, OpenFontsDialog,
    (HWND hwnd, LPCSTR lpszKey),
    (hwnd, lpszKey),
    0, INETCPL)
#define OpenFontsDialog VAR_OpenFontsDialog

LOADER_FUNCTION(BOOL, LaunchConnectionDialog,
    (HWND   hwnd),
    (hwnd),
    FALSE,  INETCPL)
#define LaunchConnectionDialog VAR_LaunchConnectionDialog

/////////////////////////////////////
// MSO9.DLL
#include "msoci.h"
BOOL DemandLoadMSO9();

LOADER_FUNCTION(BOOL, MsoFGetComponentManager,
    (IMsoComponentManager **ppicm),
    (ppicm),
    FALSE, MSO9)
#define MsoFGetComponentManager VAR_MsoFGetComponentManager

/////////////////////////////////////
// WINMM.DLL


BOOL DemandLoadWinMM();

LOADER_FUNCTION(BOOL, sndPlaySoundA,
    (LPCSTR pszSound, UINT fuSound),
    (pszSound, fuSound),
    FALSE, WinMM)
#define sndPlaySoundA VAR_sndPlaySoundA

/////////////////////////////////////
// WSOCK32.DLL

#include <winsock.h>

typedef struct WSAData FAR * LPWSADATA;
typedef unsigned int    u_int;
typedef unsigned short  u_short;
#ifdef _WIN64
typedef UINT_PTR		SOCKET;
#else
typedef u_int           SOCKET;
#endif	// _WIN64

BOOL DemandLoadWSOCK32();

LOADER_FUNCTION( int, WSAStartup,
    (WORD wVersionRequired, LPWSADATA lpWSAData),
    (wVersionRequired, lpWSAData),
    WSAVERNOTSUPPORTED, WSOCK32)
#define WSAStartup VAR_WSAStartup

LOADER_FUNCTION( int, WSACleanup,
    (void),
    (),
    SOCKET_ERROR, WSOCK32)
#define WSACleanup VAR_WSACleanup

LOADER_FUNCTION( int, WSAGetLastError,
    (void),
    (),
    0, WSOCK32)
#define WSAGetLastError VAR_WSAGetLastError

LOADER_FUNCTION( int, gethostname,
    (char FAR * name, int namelen),
    (name, namelen),
    SOCKET_ERROR, WSOCK32)
#define gethostname VAR_gethostname

LOADER_FUNCTION( struct hostent FAR *, gethostbyname,
    (const char FAR * name),
    (name),
    NULL, WSOCK32)
#define gethostbyname VAR_gethostbyname

LOADER_FUNCTION( HANDLE, WSAAsyncGetHostByName,
    (HWND hWnd, u_int wMsg, const char FAR * name, char FAR * buf, int buflen),
    (hWnd, wMsg, name, buf, buflen),
    0, WSOCK32)
#define WSAAsyncGetHostByName VAR_WSAAsyncGetHostByName

LOADER_FUNCTION( unsigned long, inet_addr,
    (const char FAR * cp),
    (cp),
    INADDR_NONE, WSOCK32)
#define inet_addr VAR_inet_addr

LOADER_FUNCTION( u_short, htons,
    (u_short hostshort),
    (hostshort),
    0, WSOCK32)
#define htons VAR_htons

LOADER_FUNCTION( int, WSACancelAsyncRequest,
    (HANDLE hAsyncTaskHandle),
    (hAsyncTaskHandle),
    SOCKET_ERROR, WSOCK32)
#define WSACancelAsyncRequest VAR_WSACancelAsyncRequest

LOADER_FUNCTION( int, send,
    (SOCKET s, const char FAR * buf, int len, int flags),
    (s, buf, len, flags),
    SOCKET_ERROR, WSOCK32)
#define send VAR_send

LOADER_FUNCTION( int, connect,
    (SOCKET s, const struct sockaddr FAR *name, int namelen),
    (s, name, namelen),
    SOCKET_ERROR, WSOCK32)
#define connect VAR_connect

LOADER_FUNCTION( int, WSAAsyncSelect,
    (SOCKET s, HWND hWnd, u_int wMsg, long lEvent),
    (s, hWnd, wMsg, lEvent),
    SOCKET_ERROR, WSOCK32)
#define WSAAsyncSelect VAR_WSAAsyncSelect

LOADER_FUNCTION( SOCKET, socket,
    (int af, int type, int protocol),
    (af, type, protocol),
    INVALID_SOCKET, WSOCK32)
#define socket VAR_socket

LOADER_FUNCTION( char FAR *, inet_ntoa,
    (struct in_addr in),
    (in),
    NULL, WSOCK32)
#define inet_ntoa VAR_inet_ntoa

LOADER_FUNCTION( int, closesocket,
    (SOCKET s),
    (s),
    SOCKET_ERROR, WSOCK32)
#define closesocket VAR_closesocket

LOADER_FUNCTION( int, recv,
    (SOCKET s, char FAR * buf, int len, int flags),
    (s, buf, len, flags),
    SOCKET_ERROR, WSOCK32)
#define recv VAR_recv

/////////////////////////////////////
// PSTOREC.DLL

#ifndef __IEnumPStoreProviders_FWD_DEFINED__
    #define __IEnumPStoreProviders_FWD_DEFINED__
    typedef interface IEnumPStoreProviders IEnumPStoreProviders;
#endif 	/* __IEnumPStoreProviders_FWD_DEFINED__ */
#ifndef __IPStore_FWD_DEFINED__
    #define __IPStore_FWD_DEFINED__
    typedef interface IPStore IPStore;
#endif 	/* __IPStore_FWD_DEFINED__ */
typedef GUID PST_PROVIDERID;

BOOL DemandLoadPStoreC();

LOADER_FUNCTION( HRESULT, PStoreCreateInstance,
    (IPStore __RPC_FAR *__RPC_FAR *ppProvider, PST_PROVIDERID __RPC_FAR *pProviderID, void __RPC_FAR *pReserved, DWORD dwFlags),
    (ppProvider, pProviderID, pReserved, dwFlags),
    E_FAIL, PStoreC)
#define PStoreCreateInstance VAR_PStoreCreateInstance

/////////////////////////////////////
// RICHED32.DLL
// note: special case as we don't use any riched functions but need to LoadLibrary it.

BOOL DemandLoadRichEdit();

/////////////////////////////////////
// RAS.DLL
#include <ras.h>
#include <raserror.h>

extern BOOL DemandLoadRAS(void);

LOADER_FUNCTION( DWORD, RasEnumEntriesA,
    (LPSTR reserved, LPSTR lpszPhoneBook, LPRASENTRYNAMEA lpEntry, LPDWORD lpcb, LPDWORD lpcEntries),
    (reserved, lpszPhoneBook, lpEntry, lpcb, lpcEntries),
    ERROR_FILE_NOT_FOUND, RAS)
#define RasEnumEntriesA VAR_RasEnumEntriesA

LOADER_FUNCTION( DWORD, RasEditPhonebookEntryA,
    (HWND hwnd, LPSTR lpszPhoneBook, LPSTR lpszEntryName),
    (hwnd, lpszPhoneBook, lpszEntryName),
    ERROR_FILE_NOT_FOUND, RAS)
#define RasEditPhonebookEntryA VAR_RasEditPhonebookEntryA

LOADER_FUNCTION( DWORD, RasCreatePhonebookEntryA,
    (HWND hwnd, LPSTR lpszPhoneBook),
    (hwnd, lpszPhoneBook),
    ERROR_FILE_NOT_FOUND, RAS)
#define RasCreatePhonebookEntryA VAR_RasCreatePhonebookEntryA

/////////////////////////////////////
// ADVAPI32.DLL

#ifndef ALGIDDEF
    #define ALGIDDEF
    typedef unsigned int ALG_ID;
#endif
typedef ULONG_PTR HCRYPTKEY;

BOOL DemandLoadAdvApi32(void);

LOADER_FUNCTION( BOOL, CryptAcquireContextW,
    (HCRYPTPROV *phProv, LPCWSTR pszContainer, LPCWSTR pszProvider, DWORD dwProvType, DWORD dwFlags),
    (phProv, pszContainer, pszProvider, dwProvType, dwFlags),
    FALSE, AdvApi32)
#define CryptAcquireContextW VAR_CryptAcquireContextW

LOADER_FUNCTION( BOOL, CryptContextAddRef,
    (HCRYPTPROV hProv, DWORD * pdwReserved, DWORD dwFlags),
    (hProv, pdwReserved, dwFlags),
    FALSE, AdvApi32)
#define CryptContextAddRef VAR_CryptContextAddRef

LOADER_FUNCTION( BOOL, CryptDuplicateKey,
    (HCRYPTKEY hkey, DWORD * pdwReserved, DWORD dwFlags, HCRYPTKEY * phKey),
    (hkey, pdwReserved, dwFlags, phKey),
    FALSE, AdvApi32)
#define CryptDuplicateKey VAR_CryptDuplicateKey

BOOL MY_CryptContextAddRef(HCRYPTPROV hProv, DWORD * pdwReserved, DWORD dwFlags);
BOOL MY_CryptDuplicateKey(HCRYPTKEY hkey, DWORD * pdwReserved, DWORD dwFlags, HCRYPTKEY * phKey);

HINSTANCE DemandLoadShlWapi(void);

/////////////////////////////////////
// CRYPTUI.DLL

#ifndef _CRYPTUI_YST
#define _CRYPTUI_YST
typedef struct _CRYPTUI_CERT_MGR_STRUCT
{
    DWORD                       dwSize;                     // REQUIRED
    HWND                        hwndParent;                 // OPTIONAL
    DWORD                       dwFlags;                    // OPTIONAL
    LPCWSTR                     pwszTitle;                   // OPTIONAL
    LPCSTR                      pszInitUsageOID;            // OPTIONAL
}CRYPTUI_CERT_MGR_STRUCT, *PCRYPTUI_CERT_MGR_STRUCT;

typedef const CRYPTUI_CERT_MGR_STRUCT *PCCRYPTUI_CERT_MGR_STRUCT;
#endif 

BOOL DemandLoadCryptUI();

LOADER_FUNCTION(BOOL, CryptUIDlgCertMgr,
    (PCCRYPTUI_CERT_MGR_STRUCT pCryptUICertMgr),
    (pCryptUICertMgr),
    0, CryptUI)
#define CryptUIDlgCertMgr VAR_CryptUIDlgCertMgr


/////////////////////////////////////
// MSI.DLL
// Windows Installer (DARWIN) - Used primarily for speller

#include <msi.h>

BOOL DemandLoadMSI();

LOADER_FUNCTION(UINT, MsiEnumComponentQualifiersA,
    (LPCSTR szComponent, DWORD iIndex, LPSTR lpQualifierBuf, DWORD *pcchQualifierBuf, LPSTR lpApplicationDataBuf, DWORD *pcchApplicationDataBuf),
    (       szComponent,       iIndex,       lpQualifierBuf,        pcchQualifierBuf,       lpApplicationDataBuf,        pcchApplicationDataBuf),
    ERROR_UNKNOWN_COMPONENT, MSI)
#define MsiEnumComponentQualifiersA VAR_MsiEnumComponentQualifiersA

LOADER_FUNCTION(UINT, MsiProvideQualifiedComponentA,
    (LPCSTR szCategory, LPCSTR szQualifier, DWORD dwInstallMode, LPSTR lpPathBuf, DWORD *pcchPathBuf),
    (       szCategory,        szQualifier,       dwInstallMode,       lpPathBuf,        pcchPathBuf),
    ERROR_UNKNOWN_COMPONENT, MSI)
#define MsiProvideQualifiedComponentA VAR_MsiProvideQualifiedComponentA

LOADER_FUNCTION(INSTALLSTATE, MsiLocateComponentA,
    (LPCSTR szComponent, LPSTR lpPathBuf, DWORD *pcchBuf),
    (       szComponent,       lpPathBuf,        pcchBuf),
    INSTALLSTATE_UNKNOWN, MSI)
#define MsiLocateComponentA VAR_MsiLocateComponentA

LOADER_FUNCTION(INSTALLUILEVEL, MsiSetInternalUI,
    (INSTALLUILEVEL dwUILevel, HWND *phwnd),
    (               dwUILevel,       phwnd),
    INSTALLUILEVEL_NOCHANGE, MSI)
#define MsiSetInternalUI VAR_MsiSetInternalUI





