
#ifndef __DISPIDS_H__
#define __DISPIDS_H__

#define DISPID_WINHTTPTEST_BASE                    0x00000100
#define DISPID_WINHTTPTEST_HELPER_BASE             0x00000200
#define DISPID_WIN32ERRORCODE_BASE                 0x00000300
#define DISPID_BUFFEROBJECT_BASE                   0x00000400
#define DISPID_URLCOMPONENTS_BASE                  0x00000500
#define DISPID_SYSTEMTIME_BASE                     0x00000600

#define DISPID_WINHTTPTEST_OPEN                    (DISPID_WINHTTPTEST_BASE)
#define DISPID_WINHTTPTEST_CONNECT                 (DISPID_WINHTTPTEST_BASE + 1)
#define DISPID_WINHTTPTEST_OPENREQUEST             (DISPID_WINHTTPTEST_BASE + 2)
#define DISPID_WINHTTPTEST_SENDREQUEST             (DISPID_WINHTTPTEST_BASE + 3)
#define DISPID_WINHTTPTEST_RECEIVERESPONSE         (DISPID_WINHTTPTEST_BASE + 4)
#define DISPID_WINHTTPTEST_CLOSEHANDLE             (DISPID_WINHTTPTEST_BASE + 5)
#define DISPID_WINHTTPTEST_READDATA                (DISPID_WINHTTPTEST_BASE + 6)
#define DISPID_WINHTTPTEST_WRITEDATA               (DISPID_WINHTTPTEST_BASE + 7)
#define DISPID_WINHTTPTEST_QUERYDATAAVAILABLE      (DISPID_WINHTTPTEST_BASE + 8)
#define DISPID_WINHTTPTEST_QUERYOPTION             (DISPID_WINHTTPTEST_BASE + 9)
#define DISPID_WINHTTPTEST_SETOPTION               (DISPID_WINHTTPTEST_BASE + 10)
#define DISPID_WINHTTPTEST_SETTIMEOUTS             (DISPID_WINHTTPTEST_BASE + 11)
#define DISPID_WINHTTPTEST_ADDREQUESTHEADERS       (DISPID_WINHTTPTEST_BASE + 12)
#define DISPID_WINHTTPTEST_SETCREDENTIALS          (DISPID_WINHTTPTEST_BASE + 13)
#define DISPID_WINHTTPTEST_QUERYAUTHSCHEMES        (DISPID_WINHTTPTEST_BASE + 14)
#define DISPID_WINHTTPTEST_QUERYHEADERS            (DISPID_WINHTTPTEST_BASE + 15)
#define DISPID_WINHTTPTEST_TIMEFROMSYSTEMTIME      (DISPID_WINHTTPTEST_BASE + 16)
#define DISPID_WINHTTPTEST_TIMETOSYSTEMTIME        (DISPID_WINHTTPTEST_BASE + 17)
#define DISPID_WINHTTPTEST_CRACKURL                (DISPID_WINHTTPTEST_BASE + 18)
#define DISPID_WINHTTPTEST_CREATEURL               (DISPID_WINHTTPTEST_BASE + 19)
#define DISPID_WINHTTPTEST_SETSTATUSCALLBACK       (DISPID_WINHTTPTEST_BASE + 20)

#define DISPID_WINHTTPTEST_HELPER_GETBUFFEROBJECT  (DISPID_WINHTTPTEST_HELPER_BASE)
#define DISPID_WINHTTPTEST_HELPER_GETURLCOMPONENTS (DISPID_WINHTTPTEST_HELPER_BASE + 1)
#define DISPID_WINHTTPTEST_HELPER_GETSYSTEMTIME    (DISPID_WINHTTPTEST_HELPER_BASE + 2)
#define DISPID_WINHTTPTEST_HELPER_GETLASTERROR     (DISPID_WINHTTPTEST_HELPER_BASE + 3)

#define DISPID_WIN32ERRORCODE_ERRORCODE            (DISPID_WIN32ERRORCODE_BASE + 1)
#define DISPID_WIN32ERRORCODE_ERRORSTRING          (DISPID_WIN32ERRORCODE_BASE + 2)
#define DISPID_WIN32ERRORCODE_ISEXCEPTION          (DISPID_WIN32ERRORCODE_BASE + 3)

#define DISPID_BUFFEROBJECT_SIZE                   (DISPID_BUFFEROBJECT_BASE)
#define DISPID_BUFFEROBJECT_TYPE                   (DISPID_BUFFEROBJECT_BASE + 1)
#define DISPID_BUFFEROBJECT_BYTESTRANSFERRED       (DISPID_BUFFEROBJECT_BASE + 2)
#define DISPID_BUFFEROBJECT_FLAGS                  (DISPID_BUFFEROBJECT_BASE + 3)

#define DISPID_URLCOMPONENTS_STRUCTSIZE            (DISPID_URLCOMPONENTS_BASE)
#define DISPID_URLCOMPONENTS_SCHEME                (DISPID_URLCOMPONENTS_BASE + 1)
#define DISPID_URLCOMPONENTS_SCHEMELENGTH          (DISPID_URLCOMPONENTS_BASE + 2)
#define DISPID_URLCOMPONENTS_SCHEMEID              (DISPID_URLCOMPONENTS_BASE + 3)
#define DISPID_URLCOMPONENTS_HOSTNAME              (DISPID_URLCOMPONENTS_BASE + 4)
#define DISPID_URLCOMPONENTS_HOSTNAMELENGTH        (DISPID_URLCOMPONENTS_BASE + 5)
#define DISPID_URLCOMPONENTS_PORT                  (DISPID_URLCOMPONENTS_BASE + 6)
#define DISPID_URLCOMPONENTS_USERNAME              (DISPID_URLCOMPONENTS_BASE + 7)
#define DISPID_URLCOMPONENTS_USERNAMELENGTH        (DISPID_URLCOMPONENTS_BASE + 8)
#define DISPID_URLCOMPONENTS_PASSWORD              (DISPID_URLCOMPONENTS_BASE + 9)
#define DISPID_URLCOMPONENTS_PASSWORDLENGTH        (DISPID_URLCOMPONENTS_BASE + 10)
#define DISPID_URLCOMPONENTS_URLPATH               (DISPID_URLCOMPONENTS_BASE + 11)
#define DISPID_URLCOMPONENTS_URLPATHLENGTH         (DISPID_URLCOMPONENTS_BASE + 12)
#define DISPID_URLCOMPONENTS_EXTRAINFO             (DISPID_URLCOMPONENTS_BASE + 13)
#define DISPID_URLCOMPONENTS_EXTRAINFOLENGTH       (DISPID_URLCOMPONENTS_BASE + 14)

#define DISPID_SYSTEMTIME_YEAR                     (DISPID_SYSTEMTIME_BASE)
#define DISPID_SYSTEMTIME_MONTH                    (DISPID_SYSTEMTIME_BASE + 1)
#define DISPID_SYSTEMTIME_DAYOFWEEK                (DISPID_SYSTEMTIME_BASE + 2)
#define DISPID_SYSTEMTIME_DAY                      (DISPID_SYSTEMTIME_BASE + 3)
#define DISPID_SYSTEMTIME_HOUR                     (DISPID_SYSTEMTIME_BASE + 4)
#define DISPID_SYSTEMTIME_MINUTE                   (DISPID_SYSTEMTIME_BASE + 5)
#define DISPID_SYSTEMTIME_SECOND                   (DISPID_SYSTEMTIME_BASE + 6)
#define DISPID_SYSTEMTIME_MSEC                     (DISPID_SYSTEMTIME_BASE + 7)

#endif /* __DISPIDS_H__ */
