/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    logon.h

Abstract:

    This interface abstracts a Passport Logon Session.

Author:

    Biao Wang (biaow) 01-Oct-2000

--*/

#ifndef LOGON_H
#define LOGON_H

#include "xstring.h"

void DecryptPassword ( WCHAR* pClearPassword, PVOID pPassword, DWORD cbSize );


class SESSION;

class LOGON
{
public:
    LOGON(SESSION*, DWORD dwParentFlags);
    virtual ~LOGON(void);

    BOOL Open(PCWSTR pwszPartnerInfo);
    void Close(void);

    BOOL SetCredentials(
        PCWSTR      pwszRealm,
        PCWSTR      pwszTarget,
        PCWSTR      pwszSignIn,
        PCWSTR      pwszPassword,
        PSYSTEMTIME pTimeCredsEntered
        );

    BOOL GetLogonHost(
    	PWSTR       pwszHostName,
    	OUT PDWORD  pdwHostNameLen
        ) const;

    DWORD Logon(BOOL fAnonymous);

    PLIST_ENTRY GetPrivacyEvents(void) const { return (PLIST_ENTRY)&m_PrivacyEventList; }

    BOOL GetChallengeInfo(
		HBITMAP*		 phBitmap,
        PBOOL            pfPrompt,
    	PWSTR			 pwszCbText,
        PDWORD           pdwTextLen,
        PWSTR            pwszRealm,
        DWORD            dwMaxRealmLen,
        PWSTR            pwszReqUserName,
        PDWORD           pdwReqUserNameLen
        ) const;

    BOOL GetChallengeContent(
      	PBYTE    	     pContent,
        OUT PDWORD       pdwContentLen
        ) const;


    BOOL GetAuthorizationInfo(
        PWSTR   pwszTicket,       // e.g. "from-PP = ..."
        PDWORD  pdwTicketLen,
        PBOOL   pfKeepVerb, // if TRUE, no data will be copied into pwszUrl
        PWSTR   pwszUrl,    // user supplied buffer ...
        PDWORD  pdwUrlLen  // ... and length (will be updated to actual length 
                                        // on successful return)
        ) const;

    VOID StatusCallback(
        IN HINTERNET hInternet,
        IN DWORD dwInternetStatus,
        IN LPVOID lpvStatusInformation,
        IN DWORD dwStatusInformationLength);

protected:
    void GetCachedCreds(
        PCWSTR	pwszRealm,
        PCWSTR  pwszTarget,
        PCREDENTIALW** pppCreds,
        DWORD* pdwCreds
        );

    BOOL DownLoadCoBrandBitmap(
        PWSTR pwszChallenge
        );

    DWORD Handle401FromDA(
        HINTERNET   hRequest, 
        BOOL        fTicketRequest
        );

    DWORD Handle200FromDA(
        HINTERNET hRequest
        );

    void CheckForVersionChange(
        HINTERNET hRequest
        );

protected:

    SESSION*    m_pSession;

    HINTERNET   m_hConnect;
    BOOL        m_fCredsPresent;
    PWSTR       m_pwszSignIn;
    CSecureStr  m_Password;
    WCHAR       m_wNewDAUrl[1024];
    PWSTR       m_pwszTicketRequest;
    PWSTR       m_pwszAuthInfo;
    PWSTR		m_pwszReturnUrl;
    // PWSTR       m_pwszCbUrl;
	BOOL		m_fWhistler;
    HBITMAP     m_hBitmap;
    BOOL        m_fPrompt;
    WCHAR       m_wRealm[128];
    WCHAR       m_wTimeSkew[16];
    PWSTR       m_pwszAuthHeader;
    DWORD       m_dwParentFlags;
    PWSTR       m_pwszCbtxt;

    WCHAR       m_wDAHostName[256];
    LPSTREAM    m_p401Content;
    SYSTEMTIME  m_TimeCredsEntered;

    LIST_ENTRY  m_PrivacyEventList;
};

#endif // LOGON_H
