/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    fsm.hxx

Abstract:

    Contains Finite State Machine class definition

Author:

     Richard L Firth (rfirth) 11-Apr-1997

Revision History:

    11-Apr-1997 rfirth
        Created

--*/

//
// types
//

//
// FSM_STATE - states FSMs can be in. We have some states defined for all FSMs,
// e.g. FSM_STATE_INIT, and other states that are used internally by the FSMs,
// e.g. FSM_STATE_1 through FSM_STATE_10
//

typedef enum {
    FSM_STATE_BAD = -1,
    FSM_STATE_INIT,
    FSM_STATE_WAIT,
    FSM_STATE_DONE,
    FSM_STATE_ERROR,
    FSM_STATE_CONTINUE,
    FSM_STATE_FINISH,
    FSM_STATE_1,
    FSM_STATE_2,
    FSM_STATE_3,
    FSM_STATE_4,
    FSM_STATE_5,
    FSM_STATE_6,
    FSM_STATE_7,
    FSM_STATE_8,
    FSM_STATE_9,
    FSM_STATE_10
} FSM_STATE;

//
// FSM_HINT - QUICK if next operation is expected to complete quickly
//

typedef enum {
    FSM_HINT_SLOW,  // default
    FSM_HINT_QUICK
} FSM_HINT;

//
// FSM_ACTION - type of (socket) action an FSM is waiting on
//

typedef enum {
    FSM_ACTION_NONE = 0,
    FSM_ACTION_CONNECT = 1,
    FSM_ACTION_SEND,
    FSM_ACTION_RECEIVE
} FSM_ACTION;

//
// FSM_TYPE - for debugging purposes
//

typedef enum {
    FSM_TYPE_NONE,
    FSM_TYPE_WAIT_FOR_COMPLETION,
    FSM_TYPE_RESOLVE_HOST,
    FSM_TYPE_SOCKET_CONNECT,
    FSM_TYPE_SOCKET_SEND,
    FSM_TYPE_SOCKET_RECEIVE,
    FSM_TYPE_SOCKET_QUERY_AVAILABLE,
    FSM_TYPE_SECURE_CONNECT,
    FSM_TYPE_SECURE_HANDSHAKE,
    FSM_TYPE_SECURE_NEGOTIATE,
    FSM_TYPE_NEGOTIATE_LOOP,
    FSM_TYPE_SECURE_SEND,
    FSM_TYPE_SECURE_RECEIVE,
    FSM_TYPE_GET_CONNECTION,
    FSM_TYPE_HTTP_SEND_REQUEST,
    FSM_TYPE_MAKE_CONNECTION,
    FSM_TYPE_OPEN_CONNECTION,
    FSM_TYPE_OPEN_PROXY_TUNNEL,
    FSM_TYPE_SEND_REQUEST,
    FSM_TYPE_RECEIVE_RESPONSE,
    FSM_TYPE_HTTP_READ,
    FSM_TYPE_HTTP_WRITE,
    FSM_TYPE_READ_DATA,
    FSM_TYPE_HTTP_QUERY_AVAILABLE,
    FSM_TYPE_DRAIN_RESPONSE,
    FSM_TYPE_REDIRECT,
    FSM_TYPE_READ_LOOP,
    FSM_TYPE_PARSE_HTTP_URL,
    FSM_TYPE_PARSE_URL_FOR_HTTP,
    FSM_TYPE_READ_FILE,
    FSM_TYPE_READ_FILE_EX,
    FSM_TYPE_WRITE_FILE,
    FSM_TYPE_QUERY_DATA_AVAILABLE,
    FSM_TYPE_FTP_CONNECT,    
    FSM_TYPE_FTP_FIND_FIRST_FILE,
    FSM_TYPE_FTP_GET_FILE,
    FSM_TYPE_FTP_PUT_FILE,
    FSM_TYPE_FTP_DELETE_FILE,
    FSM_TYPE_FTP_RENAME_FILE,
    FSM_TYPE_FTP_GET_FILE_SIZE,
    FSM_TYPE_FTP_OPEN_FILE,
    FSM_TYPE_FTP_COMMAND,
    FSM_TYPE_FTP_CREATE_DIRECTORY,
    FSM_TYPE_FTP_REMOVE_DIRECTORY,
    FSM_TYPE_FTP_SET_CURRENT_DIRECTORY,
    FSM_TYPE_FTP_GET_CURRENT_DIRECTORY,
    FSM_TYPE_GOPHER_FIND_FIRST_FILE,
    FSM_TYPE_GOPHER_OPEN_FILE,
    FSM_TYPE_GOPHER_GET_ATTRIBUTE,
    FSM_TYPE_INTERNET_PARSE_URL,
    FSM_TYPE_INTERNET_FIND_NEXT_FILE,
    FSM_TYPE_INTERNET_QUERY_DATA_AVAILABLE,
    FSM_TYPE_INTERNET_WRITE_FILE,
    FSM_TYPE_INTERNET_READ_FILE,
    FSM_TYPE_BACKGROUND_TASK
} FSM_TYPE;

//
// API_TYPE - what type of parameter API returns. Used in conjunction with
// SetApi()
//

typedef enum {
    ApiType_None,
    ApiType_Handle,
    ApiType_Bool
} API_TYPE;

//
// macros
//

#define COPY_MANDATORY_PARAM(y,x) \
            if (x) {  \
                y = NewString(x); \
                if ( y == NULL) { \
                    SetError(ERROR_NOT_ENOUGH_MEMORY); \
                }                 \
            }                     \
            else {                \
                y = NULL;         \
                INET_ASSERT(FALSE); \
            }

#define COPY_MANDATORY_PARAMW(y,x) \
            if (x) {  \
                y = NewStringW(x); \
                if ( y == NULL) { \
                    SetError(ERROR_NOT_ENOUGH_MEMORY); \
                }                 \
            }                     \
            else {                \
                y = NULL;         \
                INET_ASSERT(FALSE); \
            }


#define COPY_OPTIONAL_PARAM(y,x) \
            if (x) { \
                y = NewString(x); \
                if ( y == NULL) { \
                    SetError(ERROR_NOT_ENOUGH_MEMORY); \
                }                 \
            }                     \
            else {                \
                y = NULL;         \
            }

#define DELETE_MANDATORY_PARAM(x) \
            if (x) { \
                x=(LPSTR)FREE_MEMORY(x); \
            } \
            else { \
                INET_ASSERT(FALSE); \
            }

#define DELETE_MANDATORY_PARAMW(x) \
            if (x) { \
                x=(LPWSTR)FREE_MEMORY(x); \
            } \
            else { \
                INET_ASSERT(FALSE); \
            }


#define DELETE_OPTIONAL_PARAM(x) \
            if (x) { \
                x=(LPSTR)FREE_MEMORY(x); \
            } 

//
// functions
//

BOOL
wInternetQueryDataAvailable(
    IN LPVOID hFileMapped,
    OUT LPDWORD lpdwNumberOfBytesAvailable,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );


//
// classes
//

//
// CFsm - the finite state machine class. Describes the basic work unit,
// assumable by any available thread.
//
// State machines are chainable on a stack: the head of the stack is always the
// currently executing state machine
//
// For non-blocking socket operations, state machines are associated with the
// socket handle blocking the state machine
//
// State machines have a priority which is used in deciding which runnable state
// machine is executed next
//

class CFsm : public CPriorityListEntry {

private:

    CFsm * m_Link;                              // 0x10
    DWORD m_dwError;                            // 0x14
    LPINTERNET_THREAD_INFO m_lpThreadInfo;      // 0x18
    DWORD_PTR m_dwContext;                      // 0x1C
    HINTERNET m_hObject;                        // 0x20
    INTERNET_HANDLE_OBJECT * m_hObjectMapped;   // 0x24
    DWORD m_dwMappedErrorCode;                  // 0x28
    FSM_STATE m_State;                          // 0x2C
    FSM_STATE m_NextState;                      // 0x30
    FSM_STATE m_FunctionState;                  // 0x34
    DWORD (*m_lpfnHandler)(CFsm *);             // 0x38
    LPVOID m_lpvContext;                        // 0x3C
    FSM_HINT m_Hint;                            // 0x40
    SOCKET m_Socket;                            // 0x44
    FSM_ACTION m_Action;                        // 0x48
    DWORD_PTR m_dwBlockId;                      // 0x4C
    DWORD m_dwTimeout;                          // 0x50
    BOOL m_fTimeoutWraps; 
    DWORD m_dwTimer;                            // 0x54
    BOOL m_bTimerStarted;                       // 0x58
    BOOL m_bIsApi;                              // 0x5C
    // indicates a non-yielding fsm, that blocks a full thread while it executes
    BOOL m_bIsBlockingFsm;                      // 0x60
    API_TYPE m_ApiType;                         // 0x64
    union {
        BOOL Bool;
        HINTERNET Handle;
    } m_ApiResult;                              // 0x68
    DWORD m_dwApiData;                      // 0x6C

    BOOL m_bHasTimeout;
    BOOL m_bOnAsyncList;
    BOOL m_bPushPop;

//#if INET_DEBUG
#ifdef STRESS_BUG_DEBUG

    DWORD m_Signature;                          // 0x70

public:

    DWORD m_ThreadId;                           // 0x74

#define INET_ASSERT_X(x)      if ( !(x) ) {  OutputDebugString("WinHttp5.DLL: FSM still in use, contact venkatk, x54275 \r\n"); \
                                DebugBreak(); } 
                                      
#define FSM_SIGNATURE       0x5f4d5346  // "FSM_"
#define INIT_FSM()          m_Signature = FSM_SIGNATURE; \
                            m_Type = FSM_TYPE_NONE; \
                            m_ThreadId = 0
#define CHECK_FSM()         INET_ASSERT_X(m_Signature == FSM_SIGNATURE)
#define SET_OWNED()         m_ThreadId = GetCurrentThreadId()
#define RESET_OWNED()       m_ThreadId = 0
#define CHECK_OWNED()       INET_ASSERT_X(m_ThreadId == GetCurrentThreadId())
#define CHECK_UNOWNED()     INET_ASSERT_X(m_ThreadId == 0)
#define SET_FSM_OWNED(p)    if (p) p->m_ThreadId = GetCurrentThreadId()
#define RESET_FSM_OWNED(p)  if (p) p->m_ThreadId = 0
#define CHECK_FSM_OWNED(p)  if (p) INET_ASSERT_X(p->m_ThreadId == GetCurrentThreadId())
#define CHECK_FSM_UNOWNED(p) if (p) INET_ASSERT_X(p->m_ThreadId == 0)

#else

#define INIT_FSM()          /* NOTHING */
#define CHECK_FSM()         /* NOTHING */
#define SET_OWNED()         /* NOTHING */
#define RESET_OWNED()       /* NOTHING */
#define CHECK_OWNED()       /* NOTHING */
#define CHECK_UNOWNED()     /* NOTHING */
#define SET_FSM_OWNED(p)    /* NOTHING */
#define RESET_FSM_OWNED(p)  /* NOTHING */
#define CHECK_FSM_OWNED(p)  /* NOTHING */
#define CHECK_FSM_UNOWNED(p) /* NOTHING */

#endif

protected:

    FSM_TYPE m_Type;                            // 0x78

#define SET_FSM_TYPE(type)  m_Type = FSM_TYPE_ ## type

public:

    CFsm(DWORD (* lpfnHandler)(CFsm *), LPVOID lpvContext, BOOL fPushPop = TRUE);
    virtual ~CFsm();

    VOID
    Push(
        VOID
        );

    VOID
    Pop(
        VOID
        );

    VOID
    SetPushPop(BOOL fPushPop) {
        m_bPushPop = fPushPop;
    }

    DWORD GetError(VOID) const {
        return m_dwError;
    }

    VOID SetError(DWORD Error) {
        m_dwError = Error;
    }

    BOOL IsInvalid(VOID) {

        INET_ASSERT(m_hObjectMapped != NULL);

        return (m_hObjectMapped != NULL)
            ? m_hObjectMapped->IsInvalidated()
            : FALSE;
    }

    DWORD GetMappedError(VOID) const {
        return m_dwMappedErrorCode;
    }

    VOID SetMappedError(DWORD dwError) {
        m_dwMappedErrorCode = dwError;
    }

    LPINTERNET_THREAD_INFO GetThreadInfo(VOID) const {
        return m_lpThreadInfo;
    }

    VOID SetThreadInfo(LPINTERNET_THREAD_INFO lpThreadInfo) {
        m_lpThreadInfo = lpThreadInfo;
    }

    DWORD_PTR GetAppContext(VOID) const {
        return m_dwContext;
    }

    HINTERNET GetAppHandle(VOID) const {
        return m_hObject;
    }

    HINTERNET GetMappedHandle(VOID) const {
        return m_hObjectMapped;
    }

    INTERNET_HANDLE_OBJECT * GetMappedHandleObject(VOID) const {
        return (INTERNET_HANDLE_OBJECT *)m_hObjectMapped;
    }

    FSM_STATE GetState(VOID) const {
        return m_State;
    }

    VOID SetState(FSM_STATE State) {
        m_State = State;
    }

    FSM_STATE GetNextState(VOID) const {
        return m_NextState;
    }

    VOID SetNextState(FSM_STATE State) {
        m_NextState = State;
    }

    FSM_STATE GetFunctionState(VOID) const {

        //
        //  We should never enter into this state, cause the FSMs
        //   themselves must be correct enough to always set the
        //   next function state before exiting their state.
        //

        //INET_ASSERT(m_FunctionState != FSM_STATE_BAD);

        return m_FunctionState;
    }

    VOID SetFunctionState(FSM_STATE State) {
        m_FunctionState = State;
    }

    VOID SetWait(VOID) {
        SetState(FSM_STATE_WAIT);
    }

    BOOL IsWait(VOID) {
        return (m_State == FSM_STATE_WAIT) ? TRUE : FALSE;
    }

    VOID SetErrorState(DWORD Error) {
        SetError(Error);
        SetState(FSM_STATE_ERROR);
    }

    VOID SetDone(VOID) {
        SetState(FSM_STATE_DONE);
    }

    VOID SetDone(DWORD Error) {
        SetError(Error);
        SetState(FSM_STATE_DONE);
    }

    BOOL IsDone(VOID) {
        return (m_State == FSM_STATE_DONE) ? TRUE : FALSE;
    }

    LPVOID GetContext(VOID) const {
        return m_lpvContext;
    }

    VOID SetContext(LPVOID lpvContext) {
        m_lpvContext = lpvContext;
    }

    LPVOID GetHandler(VOID) const {
        return (LPVOID)m_lpfnHandler;
    }

    VOID SetHandler(LPVOID lpfnHandler) {
        m_lpfnHandler = (DWORD (*)(CFsm *))lpfnHandler;
    }

    VOID SetHandler2(DWORD (* lpfnHandler)(CFsm *)) {
        m_lpfnHandler = lpfnHandler;
    }

    SOCKET GetSocket(VOID) const {
        return m_Socket;
    }

    VOID SetSocket(SOCKET Socket) {

        INET_ASSERT(m_Socket == INVALID_SOCKET);

        m_Socket = Socket;
    }

    VOID ResetSocket(VOID) {
        m_Socket = INVALID_SOCKET;
    }

    BOOL IsActive(VOID) {
        return (m_Socket == INVALID_SOCKET) ? FALSE : TRUE;
    }

    FSM_ACTION GetAction(VOID) const {
        return m_Action;
    }

    VOID SetAction(FSM_ACTION Action) {
        m_Action = Action;
    }

    DWORD_PTR GetBlockId(VOID) const {
        return m_dwBlockId;
    }

    VOID SetBlockId(DWORD_PTR dwBlockId) {
        m_dwBlockId = dwBlockId;
    }

    BOOL IsBlockedOn(DWORD_PTR dwBlockId) {
        return (m_dwBlockId == dwBlockId) ? TRUE : FALSE;
    }

    DWORD GetTimeout(VOID) const {
        return m_dwTimeout;
    }

    // This code needs to handle system time roll over.
    // SetTimeout is passed the duration, and we calculate the ultimate time
    // However, this may result in a rollover -- e.g. if the current time is 
    // 0xffffff00, the ultimate time could be 0x000000fd

    // IsTimedOut is passed the current tick count, however, and in the past
    // would return TRUE immediately.
    // Thus we set a flag is we need to wait for system time rollover to happen,

    VOID SetTimeout(DWORD dwTimeout) {

        if (dwTimeout != INFINITE)
            m_bHasTimeout = TRUE;
        else
            m_bHasTimeout = FALSE;
        DWORD dw = GetTickCountWrap();
        m_dwTimeout = (dwTimeout == INFINITE)
                    ? dwTimeout
                    : (dw + dwTimeout);
        m_fTimeoutWraps = dw > m_dwTimeout;
    }

    BOOL HasTimeout()
    {
        return m_bHasTimeout;
    }

    VOID SetOnAsyncList(BOOL bOnAsyncList)
    {
        m_bOnAsyncList = bOnAsyncList;
    }

    BOOL IsOnAsyncList()
    {
        return m_bOnAsyncList;
    }
    
    BOOL IsTimedOut(DWORD dwTime) {
        if (m_fTimeoutWraps)
        {
            m_fTimeoutWraps = ((LONG)dwTime < 0);
        }
        return ((m_dwTimeout == INFINITE) || m_fTimeoutWraps)
            ? FALSE
            : (dwTime > m_dwTimeout);
    }

    VOID StartTimer(VOID) {
        m_dwTimer = GetTickCountWrap();
        m_bTimerStarted = TRUE;
    }

    DWORD StopTimer(VOID) {
        if (m_bTimerStarted) {
            m_dwTimer = GetTickCountWrap() - m_dwTimer;
            m_bTimerStarted = FALSE;
        }
        return m_dwTimer;
    }

    DWORD GetElapsedTime(VOID) {
        return (GetTickCountWrap() - m_dwTimer);
    }

    DWORD StopAndStartTimer(VOID) {

        DWORD tNow = GetTickCountWrap();
        DWORD tElapsed = (tNow - m_dwTimer);

        m_dwTimer = tNow;
        m_bTimerStarted = TRUE;
        return tElapsed;
    }

    DWORD ReadTimer(VOID) {
        return m_bTimerStarted ? GetElapsedTime() : m_dwTimer;
    }

    VOID SetBlocking(BOOL fBlocking = TRUE) {
        m_bIsBlockingFsm = fBlocking;
    }

    BOOL IsBlocking(VOID) const {
        return m_bIsBlockingFsm;
    }

    VOID SetApi(API_TYPE ApiType) {
        m_bIsApi = TRUE;
        m_ApiType = ApiType;
    }

    BOOL IsApi(VOID) const {
        return m_bIsApi;
    }

    API_TYPE GetApiType(VOID) const {
        return m_ApiType;
    }

    VOID SetApiResult(BOOL bResult) {
        m_ApiResult.Bool = bResult;
    }

    VOID SetApiResult(HINTERNET hResult) {
        m_ApiResult.Handle = hResult;
    }

    DWORD GetApiResult(VOID) {
        // SUNDOWN: typecast problem
        return (m_ApiType == ApiType_Handle)
                    ? PtrToUlong(GetHandleResult())
                    : (m_ApiType == ApiType_Bool)
                        ? (DWORD) GetBoolResult()
                        : 0;
    }

    BOOL GetBoolResult(VOID) const {
        return m_ApiResult.Bool;
    }

    HINTERNET GetHandleResult(VOID) const {
        return m_ApiResult.Handle;
    }

    DWORD GetApiData(VOID) const {
        return m_dwApiData;
    }

    VOID SetApiData(DWORD dwApiData) {
        m_dwApiData = dwApiData;
    }

    DWORD
    QueueWorkItem(
        VOID
        );

    static
    DWORD
    RunWorkItem(
        IN CFsm * pFsm
        );

    DWORD
    Run(
        IN LPINTERNET_THREAD_INFO lpThreadInfo,
        OUT DWORD *lpdwApiResult OPTIONAL,
        OUT DWORD *lpdwApiData OPTIONAL
        );

    FSM_TYPE GetType(VOID) const {
        return m_Type;
    }

#if INET_DEBUG

    DEBUG_FUNCTION
    LPSTR
    MapType(
        VOID
        );

    DEBUG_FUNCTION
    LPSTR
    StateName(
        IN DWORD State
        );

    DEBUG_FUNCTION
    LPSTR MapState(VOID) {
        return StateName(m_State);
    }

    DEBUG_FUNCTION
    LPSTR MapFunctionState(VOID) {
        return StateName(m_FunctionState);
    }

#else

    LPSTR MapType(VOID) {
        return "";
    }

    LPSTR MapState(VOID) {
        return "";
    }

    LPSTR MapFunctionState(VOID) {
        return "";
    }

#endif

};

/*DWORD
RunSM_Wrapper(
    IN*/


//
// Derived State Machines
//
// The following state machines contain the parameters and variables that are
// maintained across machine states and thread switches
//
// In order to make the code more readable, the state machine is friends with
// the object class for which it operates (e.g. ICSocket or ICHttpRequest)
//

//
// CAddressList FSMs
//

class CFsm_ResolveHost : public CFsm {

friend class CAddressList;

private:

    //
    // parameters
    //

    LPSTR m_lpszHostName;
    LPDWORD m_lpdwResolutionId;
    DWORD m_dwFlags;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ResolveHost(
        IN LPSTR lpszHostName,
        IN LPDWORD lpdwResolutionId,
        IN DWORD dwFlags,
        IN CAddressList * pAddressList
        ) : CFsm(RunSM, (LPVOID)pAddressList) {

        SET_FSM_TYPE(RESOLVE_HOST);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }
        m_lpszHostName = lpszHostName;
        m_lpdwResolutionId = lpdwResolutionId;
        m_dwFlags = dwFlags;
    }
};



//
//  Internet API FSMs
//

class CFsm_InternetParseUrl : public CFsm {

private:

    //
    // parameters
    //

    HINTERNET m_hConnectHandle;
    DWORD_PTR m_dwContext;
    LPCSTR m_lpszUrl;
    LPCSTR m_lpszHeaders;
    DWORD m_dwHeadersLength;
    DWORD m_dwFlags;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        )
    {
        CFsm_InternetParseUrl *pFsm  = (CFsm_InternetParseUrl *)Fsm;

        INET_ASSERT(Fsm->GetState() == FSM_STATE_INIT);

        pFsm->SetApiResult(InternetOpenUrlA(
                            pFsm->m_hConnectHandle,
                            pFsm->m_lpszUrl,
                            pFsm->m_lpszHeaders,
                            pFsm->m_dwHeadersLength,
                            pFsm->m_dwFlags,
                            pFsm->m_dwContext
                            ));

        pFsm->SetDone();

        return ((pFsm->GetHandleResult()) ? ERROR_SUCCESS : GetLastError());        
    }

    CFsm_InternetParseUrl(
        IN HINTERNET hConnectHandle,
        IN DWORD_PTR dwContext,
        IN LPCSTR lpszUrl,
        IN LPCSTR lpszHeaders,
        IN DWORD dwHeadersLength,
        IN DWORD dwFlags
        ) : CFsm(RunSM, (LPVOID)hConnectHandle) {

        SET_FSM_TYPE(INTERNET_PARSE_URL);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Handle);
        SetBlocking();

        m_hConnectHandle = hConnectHandle;
        m_dwContext = dwContext;
        m_lpszUrl = lpszUrl;
        m_lpszHeaders = lpszHeaders;
        m_dwHeadersLength = dwHeadersLength;
        m_dwFlags = dwFlags;
    }
};


class CFsm_InternetQueryDataAvailable : public CFsm {

private:

    //
    // parameters
    //

    HINTERNET m_hSessionHandle;
    LPDWORD m_lpdwNumberOfBytesAvailable;
    DWORD m_dwNumberOfBytesAvailable;
    DWORD m_dwFlags;
    DWORD_PTR m_dwContext;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        )
    {
        CFsm_InternetQueryDataAvailable *pFsm  = (CFsm_InternetQueryDataAvailable *)Fsm;

        INET_ASSERT(Fsm->GetState() == FSM_STATE_INIT);

        pFsm->SetApiResult(wInternetQueryDataAvailable(
                            pFsm->m_hSessionHandle,
                            pFsm->m_lpdwNumberOfBytesAvailable,
                            pFsm->m_dwFlags,
                            pFsm->m_dwContext
                            ));

        pFsm->SetApiData(*pFsm->m_lpdwNumberOfBytesAvailable);

        pFsm->SetDone();

        return ((pFsm->GetBoolResult()) ? ERROR_SUCCESS : GetLastError());
    }

    CFsm_InternetQueryDataAvailable(
        IN HINTERNET hSessionHandle,
        OUT LPDWORD lpdwNumberOfBytesAvailable,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        ) : CFsm(RunSM, (LPVOID)hSessionHandle) {

        SET_FSM_TYPE(INTERNET_QUERY_DATA_AVAILABLE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        SetBlocking();

        m_hSessionHandle = hSessionHandle;
        m_lpdwNumberOfBytesAvailable = &m_dwNumberOfBytesAvailable;
        m_dwNumberOfBytesAvailable = *lpdwNumberOfBytesAvailable;
        m_dwFlags = dwFlags;
        m_dwContext = dwContext;
    }
};



class CFsm_InternetWriteFile : public CFsm {

private:

    //
    // parameters
    //

    HINTERNET m_hSessionHandle;
    LPCVOID m_lpBuffer;
    DWORD m_dwNumberOfBytesToWrite;
    LPDWORD m_lpdwNumberOfBytesWritten;
    DWORD m_dwNumberOfBytesWritten;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        )
    {
        CFsm_InternetWriteFile *pFsm  = (CFsm_InternetWriteFile *)Fsm;

        INET_ASSERT(Fsm->GetState() == FSM_STATE_INIT);

        pFsm->SetApiResult(WinHttpWriteData(
                            pFsm->m_hSessionHandle,
                            pFsm->m_lpBuffer,
                            pFsm->m_dwNumberOfBytesToWrite,
                            pFsm->m_lpdwNumberOfBytesWritten
                            ));

        pFsm->SetApiData(*(pFsm->m_lpdwNumberOfBytesWritten));

        pFsm->SetDone();

        return ((pFsm->GetBoolResult()) ? ERROR_SUCCESS : GetLastError());
    }

    CFsm_InternetWriteFile(
        IN HINTERNET hSessionHandle,
        IN LPCVOID lpBuffer,
        IN DWORD dwNumberOfBytesToWrite,
        OUT LPDWORD lpdwNumberOfBytesWritten
        ) : CFsm(RunSM, (LPVOID)hSessionHandle) {

        SET_FSM_TYPE(INTERNET_WRITE_FILE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        SetBlocking();

        m_hSessionHandle = hSessionHandle;
        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToWrite = dwNumberOfBytesToWrite;
        m_dwNumberOfBytesWritten = *lpdwNumberOfBytesWritten;
        m_lpdwNumberOfBytesWritten = &m_dwNumberOfBytesWritten;
    }
};

class CFsm_InternetReadFile  : public CFsm {

private:

    //
    // parameters
    //

    HINTERNET m_hSessionHandle;
    LPVOID m_lpBuffer;
    DWORD m_dwNumberOfBytesToRead;
    LPDWORD m_lpdwNumberOfBytesRead;
    DWORD m_dwNumberOfBytesRead;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        )
    {
        CFsm_InternetReadFile *pFsm  = (CFsm_InternetReadFile *)Fsm;

        INET_ASSERT(Fsm->GetState() == FSM_STATE_INIT);

        pFsm->SetApiResult(WinHttpReadData(
                                pFsm->m_hSessionHandle,
                                pFsm->m_lpBuffer,
                                pFsm->m_dwNumberOfBytesToRead,
                                pFsm->m_lpdwNumberOfBytesRead
                                ));

        pFsm->SetApiData(*(pFsm->m_lpdwNumberOfBytesRead));

        pFsm->SetDone();

        return ((pFsm->GetBoolResult()) ? ERROR_SUCCESS : GetLastError());
    }

    CFsm_InternetReadFile(
        IN HINTERNET hSessionHandle,
        IN LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead
        ) : CFsm(RunSM, (LPVOID)hSessionHandle) {

        SET_FSM_TYPE(INTERNET_READ_FILE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        SetBlocking();

        m_hSessionHandle = hSessionHandle;
        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToRead = dwNumberOfBytesToRead;
        m_dwNumberOfBytesRead = *lpdwNumberOfBytesRead;
        m_lpdwNumberOfBytesRead = &m_dwNumberOfBytesRead;
    }
};

//
// ICSocket FSMs
//


//
// CFsm_SocketConnect -
//

class CFsm_SocketConnect : public CFsm {

friend class ICSocket;

private:

    //
    // parameters
    //

    LONG m_Timeout;
    INT m_Retries;
    DWORD m_dwFlags;

    //
    // local variables
    //

    BOOL m_bStopOfflineTimer;
    LONG m_lPreviousTime;
    BOOL m_bResolved;
    DWORD m_dwResolutionId;
    DWORD m_dwAddressIndex;
    char m_AddressBuffer[CSADDR_BUFFER_LENGTH];
    LPCSADDR_INFO m_pAddress;
    CServerInfo * m_pServerInfo;
    CServerInfo * m_pOriginServer;
    BOOL m_fTimeout;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SocketConnect(
        IN LONG Timeout,
        IN INT Retries,
        IN DWORD dwFlags,
        IN ICSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SOCKET_CONNECT);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }
        m_Timeout = Timeout;
        m_fTimeout = Timeout ? TRUE : FALSE;
        m_Retries = Retries;
        m_dwFlags = dwFlags;
        m_bStopOfflineTimer = FALSE;
        m_lPreviousTime = (LONG)GetTickCountWrap();
        m_bResolved = FALSE;
        m_dwResolutionId = (DWORD)-1;
        m_dwAddressIndex = (DWORD)-1;
        m_pAddress = (LPCSADDR_INFO)m_AddressBuffer;
        m_pServerInfo = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetServerInfo();
        m_pOriginServer = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetOriginServer();
    }

    VOID SetServerInfo(CServerInfo * pServerInfo) {
        m_pServerInfo = pServerInfo;
    }

    BOOL IsCountedOut(VOID) {
        return (--m_Retries <= 0) ? TRUE : FALSE;
    }

    BOOL IsTimedOut(VOID) {
        return (m_fTimeout != INFINITE) ? (((m_Timeout -= (LONG)ReadTimer()) <= 0) ? TRUE : FALSE) : FALSE;
    }

    LONG GetTimeout(VOID) const
    {
        return m_Timeout;
    }
};

//
// CFsm_SocketSend -
//

class CFsm_SocketIOCP : public CFsm 
{
public:
    BOOL  bIOCPSuccess;
    DWORD dwBytesTransferred;
    DWORD dwIOCPError;
    BOOL  bIOCPInited;

    CFsm_SocketIOCP(DWORD (* lpfnHandler)(CFsm *), LPVOID lpvContext):CFsm(lpfnHandler, lpvContext)
    {
        bIOCPSuccess = FALSE;
        dwBytesTransferred = 0;
        dwIOCPError = 0;
        bIOCPInited = FALSE;
    }
};
    
class CFsm_SocketSend : public CFsm_SocketIOCP 
{
friend class ICSocket;

private:

    //
    // parameters
    //

    LPVOID m_lpBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwFlags;

    //
    // local variables
    //

    INT m_iTotalSent;
    BOOL m_bStopOfflineTimer;
    CServerInfo * m_pServerInfo;
    CServerInfo * m_pOriginServer;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SocketSend(
        IN LPVOID lpBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwFlags,
        IN ICSocket * pSocket
        ) : CFsm_SocketIOCP(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SOCKET_SEND);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        INET_ASSERT(lpBuffer != NULL);
        INET_ASSERT((int)dwBufferLength > 0);

        m_lpBuffer = lpBuffer;
        m_dwBufferLength = dwBufferLength;
        m_dwFlags = dwFlags;
        m_iTotalSent = 0;
        m_bStopOfflineTimer = FALSE;
        m_pServerInfo = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetServerInfo();
        m_pOriginServer = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetOriginServer();
    }
};

//
// CFsm_SocketReceive -
//

class CFsm_SocketReceive : public CFsm_SocketIOCP 
{
friend class ICSocket;

private:

    //
    // parameters
    //

    LPVOID * m_lplpBuffer;
    LPDWORD m_lpdwBufferLength;
    LPDWORD m_lpdwBufferRemaining;
    LPDWORD m_lpdwBytesReceived;
    DWORD m_dwExtraSpace;
    DWORD m_dwFlags;
    LPBOOL m_lpbEof;

    //
    // local variables
    //

    HLOCAL m_hBuffer;
    LPBYTE m_lpBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwBufferLeft;
    DWORD m_dwBytesReceived;
    DWORD m_dwBytesRead;
    BOOL m_bEof;
    BOOL m_bAllocated;
    CServerInfo * m_pServerInfo;
    CServerInfo * m_pOriginServer;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SocketReceive(
        IN OUT LPVOID * lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwBufferRemaining,
        IN OUT LPDWORD lpdwBytesReceived,
        IN DWORD dwExtraSpace,
        IN DWORD dwFlags,
        OUT LPBOOL lpbEof,
        IN ICSocket * pSocket
        ) : CFsm_SocketIOCP(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SOCKET_RECEIVE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        //
        // sanity check
        //

        INET_ASSERT(lplpBuffer != NULL);
        INET_ASSERT(lpdwBufferLength != NULL);
        INET_ASSERT((*lpdwBufferLength == 0) ? (dwFlags & SF_EXPAND) : TRUE);

        m_lplpBuffer = lplpBuffer;
        m_lpdwBufferLength = lpdwBufferLength;
        m_lpdwBufferRemaining = lpdwBufferRemaining;
        m_lpdwBytesReceived = lpdwBytesReceived;
        m_dwExtraSpace = dwExtraSpace;
        m_dwFlags = dwFlags;
        m_lpbEof = lpbEof;
        m_hBuffer = *lplpBuffer;
        m_lpBuffer = (LPBYTE)m_hBuffer;
        m_dwBufferLength = *lpdwBufferLength;
        m_dwBufferLeft = *lpdwBufferRemaining;
        m_dwBytesReceived = *lpdwBytesReceived;
        m_dwBytesRead = 0;
        m_bEof = FALSE;
        m_bAllocated = FALSE;
        m_pServerInfo = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetServerInfo();
        m_pOriginServer = ((HTTP_REQUEST_HANDLE_OBJECT *)
                            GetThreadInfo()->hObjectMapped)->GetOriginServer();
    }
};

//
// ICSecureSocket FSMs
//

//
// CFsm_SecureConnect -
//

class CFsm_SecureConnect : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    LONG m_Timeout;
    INT m_Retries;
    DWORD m_dwFlags;

    //
    // locals
    //

    BOOL m_bAttemptReconnect;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SecureConnect(
        IN LONG Timeout,
        IN INT Retries,
        IN DWORD dwFlags,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SECURE_CONNECT);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_Timeout = Timeout;
        m_Retries = Retries;
        m_dwFlags = dwFlags;
        m_bAttemptReconnect = FALSE;
    }
};

//
// CFsm_SecureHandshake -
//

class CFsm_SecureHandshake : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    DWORD m_dwFlags;
    LPBOOL m_lpbAttemptReconnect;

    //
    // locals
    //

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SecureHandshake(
        IN DWORD dwFlags,
        OUT LPBOOL lpbAttemptReconnect,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SECURE_HANDSHAKE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_dwFlags = dwFlags;
        m_lpbAttemptReconnect = lpbAttemptReconnect;
    }
};

//
// CFsm_SecureNegotiate -
//

class CFsm_SecureNegotiate : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    DWORD m_dwFlags;
    LPBOOL m_lpbAttemptReconnect;

    //
    // locals
    //

    SecBufferDesc m_OutBuffer;
    SecBuffer m_OutBuffers[1];
    CredHandle m_hCreds;
    BOOL m_bDoingClientAuth;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SecureNegotiate(
        IN DWORD dwFlags,
        OUT LPBOOL lpbAttemptReconnect,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SECURE_NEGOTIATE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_dwFlags = dwFlags;
        m_lpbAttemptReconnect = lpbAttemptReconnect;
        ClearCreds(m_hCreds);
        m_bDoingClientAuth = FALSE;
    }


    ~CFsm_SecureNegotiate() {
        //INET_ASSERT(IsCredClear(m_hCreds));
        INET_ASSERT(!m_bDoingClientAuth);

        if ( m_bDoingClientAuth &&
             !IsCredClear(m_hCreds))
        {
            // Look at comments before CliAuthSelectCredentials
            // g_FreeCredentialsHandle(&m_hCreds);
            m_bDoingClientAuth = FALSE;

        }
    }

};

//
// CFsm_NegotiateLoop -
//

class CFsm_NegotiateLoop : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    DBLBUFFER * m_pdblbufBuffer;
    DWORD m_dwFlags;
    BOOL m_bDoInitialRead;

    //
    // locals
    //

    SECURITY_STATUS m_scRet;
    DWORD m_dwProviderIndex;
    LPSTR m_lpszBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwBufferLeft;
    DWORD m_dwBytesReceived;
    BOOL m_bEofReceive;
    BOOL m_bDoingClientAuth;
    BOOL m_bDoRead;
    SecBuffer m_InBuffers[2];
    SecBuffer m_OutBuffers[1];
    SecBufferDesc m_OutBuffer;
    CredHandle m_hCreds;
    DWORD m_dwSSPIFlags;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_NegotiateLoop(
        IN DBLBUFFER * pdblbufBuffer,
        IN DWORD dwFlags,
        IN BOOL bDoInitialRead,
        IN BOOL bDoingClientAuth,
        IN CredHandle hCreds,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(NEGOTIATE_LOOP);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_pdblbufBuffer = pdblbufBuffer;
        m_dwFlags = dwFlags;
        m_bDoInitialRead = bDoInitialRead;
        m_scRet = SEC_E_SECPKG_NOT_FOUND;
        m_lpszBuffer = NULL;
        m_dwBufferLength = 0;
        m_dwBufferLeft = 0;
        m_dwBytesReceived = 0;
        m_bEofReceive = FALSE;
        m_bDoingClientAuth = bDoingClientAuth;
        m_bDoRead = m_bDoInitialRead;

        if (bDoingClientAuth)
            m_hCreds = hCreds;
        else
            ClearCreds(m_hCreds);
    }

    ~CFsm_NegotiateLoop() {

        if (m_bDoingClientAuth)
        {
            INET_ASSERT(!IsCredClear(m_hCreds));
            // Look at comments before CliAuthSelectCredentials
            // g_FreeCredentialsHandle(&m_hCreds);
            m_bDoingClientAuth = FALSE;
        }
    }
};

//
// CFsm_SecureSend -
//

class CFsm_SecureSend : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    LPVOID m_lpBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwFlags;

    //
    // locals
    //

    LPVOID m_lpCryptBuffer;
    DWORD m_dwCryptBufferLength;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SecureSend(
        IN LPVOID lpBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwFlags,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SECURE_SEND);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpBuffer = lpBuffer;
        m_dwBufferLength = dwBufferLength;
        m_dwFlags = dwFlags;
        m_lpCryptBuffer = NULL;
        m_dwCryptBufferLength = 0;
    }
};

//
// CFsm_SecureReceive -
//

class CFsm_SecureReceive : public CFsm {

friend class ICSecureSocket;

private:

    //
    // parameters
    //

    LPVOID * m_lplpBuffer;
    LPDWORD m_lpdwBufferLength;
    LPDWORD m_lpdwBufferRemaining;
    LPDWORD m_lpdwBytesReceived;
    DWORD m_dwExtraSpace;
    DWORD m_dwFlags;
    LPBOOL m_lpbEof;

    //
    // locals
    //

    HLOCAL m_hBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwBufferLeft;
    DWORD m_dwBytesReceived;
    DWORD m_dwBytesRead;
    LPBYTE m_lpBuffer;
    BOOL m_bEof;
    BOOL m_bAllocated;
    DWORD m_dwDecryptError;
    DWORD m_dwReadFlags;
    LPBYTE m_lpBufferDummy;
    DWORD m_dwBufferLengthDummy;
    DWORD m_dwBufferReceivedDummy;
    DWORD m_dwBufferLeftDummy;
    DWORD m_dwBufferReceivedPre;
    DWORD m_dwInputBytesLeft;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SecureReceive(
        IN OUT LPVOID* lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwBufferRemaining,
        IN OUT LPDWORD lpdwBytesReceived,
        IN DWORD dwExtraSpace,
        IN DWORD dwFlags,
        OUT LPBOOL lpbEof,
        IN ICSecureSocket * pSocket
        ) : CFsm(RunSM, (LPVOID)pSocket) {

        SET_FSM_TYPE(SECURE_RECEIVE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lplpBuffer = lplpBuffer;
        m_lpdwBufferLength = lpdwBufferLength;
        m_lpdwBufferRemaining = lpdwBufferRemaining;
        m_lpdwBytesReceived = lpdwBytesReceived;
        m_dwExtraSpace = dwExtraSpace;
        m_dwFlags = dwFlags;
        m_lpbEof = lpbEof;
        m_hBuffer = (HLOCAL)*lplpBuffer;
        m_dwBufferLength = *lpdwBufferLength;
        m_dwBufferLeft = *lpdwBufferRemaining;
        m_dwBytesReceived = *lpdwBytesReceived;
        m_dwBytesRead = 0;
        m_bEof = FALSE;
        m_bAllocated = FALSE;
        m_dwDecryptError = ERROR_SUCCESS;
    }
};

//
// CServerInfo FSMs
//

//
// CFsm_GetConnection -
//

class CFsm_GetConnection : public CFsm {

friend class CServerInfo;

private:

    //
    // parameters
    //

    DWORD m_dwSocketFlags;
    INTERNET_PORT m_nPort;
    DWORD m_dwTimeout;
    BOOL m_fTimeoutWraps;
    DWORD m_dwLimitTimeout;
    ICSocket * * m_lplpSocket;
    LPSTR m_lpszSecureTunnelHost;

    //
    // locals
    //

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_GetConnection(
        IN DWORD dwSocketFlags,
        IN INTERNET_PORT nPort,
        IN DWORD dwTimeout,
        IN DWORD dwLimitTimeout,
        OUT ICSocket * * lplpSocket,
        IN CServerInfo * lpServerInfo,
        IN LPSTR lpszSecureTunnelHost = NULL
        ) : CFsm(RunSM, (LPVOID)lpServerInfo) {

        SET_FSM_TYPE(GET_CONNECTION);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_dwSocketFlags = dwSocketFlags;
        m_nPort = nPort;
        m_dwTimeout = dwTimeout;
        m_fTimeoutWraps = GetTickCount() > dwTimeout;
        m_dwLimitTimeout = dwLimitTimeout; // This is unused
        m_lplpSocket = lplpSocket;
        if (lpszSecureTunnelHost)
        {
            int ccSecureTunnelHost = strlen(lpszSecureTunnelHost) + 1;
            m_lpszSecureTunnelHost = (LPSTR) ALLOCATE_FIXED_MEMORY(ccSecureTunnelHost);
            if (m_lpszSecureTunnelHost)
                memcpy(m_lpszSecureTunnelHost, lpszSecureTunnelHost, ccSecureTunnelHost);
        }
        else
        {
            m_lpszSecureTunnelHost = NULL;
        }
    }

    ~CFsm_GetConnection() {
        if (m_lpszSecureTunnelHost)
            FREE_MEMORY(m_lpszSecureTunnelHost);
    }
};

//
// HTTP_REQUEST_HANDLE_OBJECT FSMs
//

//
// CFsm_HttpSendRequest -
//

class CFsm_HttpSendRequest : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

public:
    AR_TYPE m_arRequest;

private:

    //
    // parameters
    //

    LPVOID m_lpOptional;
    DWORD m_dwOptionalLength;

    //
    // local variables
    //

    HINTERNET m_hRequestMapped;
    HTTP_REQUEST_HANDLE_OBJECT * m_pRequest;
    BOOL m_bFinished;
    BOOL m_bAuthNotFinished;
    BOOL m_bRedirectCountedOut;
    BOOL m_bCancelRedoOfProxy;
    BOOL m_bRedirected;
    BOOL m_bSink;
    DWORD m_dwRedirectCount;
    AUTO_PROXY_ASYNC_MSG *m_pProxyInfoQuery;
    BOOL m_fOwnsProxyInfoQueryObj;
    INTERNET_HANDLE_OBJECT * m_pInternet;
    LPVOID m_pBuffer;
    DWORD m_dwBytesDrained;
    DWORD m_iRetries;
    BOOL m_bWasKeepAlive;
    HTTP_METHOD_TYPE m_tMethodRedirect;
    DWORD m_dwCookieIndex;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_HttpSendRequest(
        IN LPVOID lpOptional OPTIONAL,
        IN DWORD dwOptionalLength,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest,
        IN AR_TYPE arRequest
        ) : CFsm(RunSM, (LPVOID)pRequest, FALSE) {

        SET_FSM_TYPE(HTTP_SEND_REQUEST);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        m_hRequestMapped = (HINTERNET)pRequest;
        m_lpOptional = lpOptional;
        m_dwOptionalLength = dwOptionalLength;
        m_pRequest = pRequest;
        m_bFinished = FALSE;
        m_bAuthNotFinished = FALSE;
        m_bRedirectCountedOut = FALSE;
        m_bCancelRedoOfProxy = FALSE;
        m_bRedirected = FALSE;
        m_bSink = FALSE;
        m_dwRedirectCount = GlobalMaxHttpRedirects;
        m_pProxyInfoQuery = NULL;
        m_arRequest = arRequest;
        m_fOwnsProxyInfoQueryObj = TRUE;
        m_dwCookieIndex = 0;

        m_pInternet = GetRootHandle(pRequest);

        INET_ASSERT(m_pInternet != NULL);
        INET_ASSERT(m_pInternet->IsValid(TypeInternetHandle) == ERROR_SUCCESS);

        m_pBuffer = NULL;
        m_iRetries = 2;
        m_tMethodRedirect = HTTP_METHOD_TYPE_UNKNOWN;
    }

    ~CFsm_HttpSendRequest(
        VOID
        )
    {
        if ( m_fOwnsProxyInfoQueryObj && m_pProxyInfoQuery && m_pProxyInfoQuery->IsAlloced())
        {
            delete m_pProxyInfoQuery;
        }

        if (m_pBuffer != NULL)
        {
            m_pBuffer = (LPVOID)FREE_MEMORY(m_pBuffer);
        }
    }

};

//
// CFsm_MakeConnection -
//

class CFsm_MakeConnection : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    //
    // local variables
    //

    HTTP_REQUEST_HANDLE_OBJECT * m_pRequest;
    BOOL m_bAttemptReconnect;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_MakeConnection(
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(MAKE_CONNECTION);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_pRequest = pRequest;
    }
};

//
// CFsm_OpenConnection -
//

class CFsm_OpenConnection : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    BOOL m_bNewConnection;

    //
    // local variables
    //

    BOOL m_bCreatedSocket;
    BOOL m_fNoCreate;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_OpenConnection(
        IN BOOL bNewConnection,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest,
        IN BOOL fNoCreate = FALSE
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(OPEN_CONNECTION);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_bNewConnection = bNewConnection;
        m_bCreatedSocket = FALSE;
        m_fNoCreate = fNoCreate;
    }
};

//
// CFsm_OpenProxyTunnel -
//

class CFsm_OpenProxyTunnel : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    //
    // local variables
    //

    HINTERNET m_hConnect;
    HINTERNET m_hRequest;
    HINTERNET m_hRequestMapped;
    HTTP_REQUEST_HANDLE_OBJECT * m_pRequest;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_OpenProxyTunnel(
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(OPEN_PROXY_TUNNEL);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_hConnect = NULL;
        m_hRequest = NULL;
        m_hRequestMapped = NULL;
    }
};

//
// CFsm_SendRequest -
//

class CFsm_SendRequest : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPVOID m_lpOptional;
    DWORD m_dwOptionalLength;


    //
    // local variables
    //

    LPSTR m_pRequestBuffer;
    DWORD m_dwRequestLength;
    BOOL m_bExtraCrLf;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_SendRequest(
        IN LPVOID lpOptional,
        IN DWORD dwOptionalLength,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(SEND_REQUEST);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpOptional = lpOptional;
        m_dwOptionalLength = dwOptionalLength;
        m_pRequestBuffer = NULL;
        m_bExtraCrLf = FALSE;
    }

    ~CFsm_SendRequest() {
        if (m_pRequestBuffer != NULL) {
            m_pRequestBuffer = (LPSTR)FREE_MEMORY(m_pRequestBuffer);

            INET_ASSERT(m_pRequestBuffer == NULL);

        }
    }
};

//
// CFsm_ReceiveResponse -
//

class CFsm_ReceiveResponse : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    //
    // local variables
    //

    HTTP_REQUEST_HANDLE_OBJECT * m_pRequest;
    DWORD m_dwResponseLeft;
    BOOL m_bEofResponseHeaders;
    BOOL m_bDrained;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ReceiveResponse(
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(RECEIVE_RESPONSE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_pRequest = pRequest;
        m_bDrained = FALSE;
    }
};

//
// CFsm_HttpReadData -
//

class CFsm_HttpReadData : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPVOID m_lpBuffer;
    DWORD m_dwNumberOfBytesToRead;
    LPDWORD m_lpdwNumberOfBytesRead;
    DWORD m_dwSocketFlags;

    //
    // local variables
    //

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_HttpReadData(
        IN LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead,
        IN DWORD dwSocketFlags,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(HTTP_READ);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToRead = dwNumberOfBytesToRead;
        m_lpdwNumberOfBytesRead = lpdwNumberOfBytesRead;
        m_dwSocketFlags = dwSocketFlags;
    }
};

//
// CFsm_HttpWriteData -
//

class CFsm_HttpWriteData : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPVOID m_lpBuffer;
    DWORD m_dwNumberOfBytesToWrite;
    LPDWORD m_lpdwNumberOfBytesWritten;
    DWORD m_dwSocketFlags;

    //
    // local variables
    //

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_HttpWriteData(
        IN LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToWrite,
        OUT LPDWORD lpdwNumberOfBytesWritten,
        IN DWORD dwSocketFlags,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest, FALSE) {

        SET_FSM_TYPE(HTTP_WRITE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToWrite = dwNumberOfBytesToWrite;
        m_lpdwNumberOfBytesWritten = lpdwNumberOfBytesWritten;
        m_dwSocketFlags = dwSocketFlags;
        SetApi(ApiType_Bool);
    }
};


//
// CFsm_ReadData -
//

class CFsm_ReadData : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPVOID m_lpBuffer;
    DWORD m_dwNumberOfBytesToRead;
    LPDWORD m_lpdwNumberOfBytesRead;
    BOOL m_fNoAsync;
    DWORD m_dwSocketFlags;

    //
    // local variables
    //

    DWORD m_nBytes;
    DWORD m_nBytesCopied;
    DWORD m_dwBufferLeft;
    DWORD m_dwBytesRead;
    BOOL m_bEof;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ReadData(
        IN LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead,
        IN BOOL fNoAsync,
        IN DWORD dwSocketFlags,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(READ_DATA);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToRead = dwNumberOfBytesToRead;
        m_lpdwNumberOfBytesRead = lpdwNumberOfBytesRead;
        m_fNoAsync = fNoAsync;
        m_dwSocketFlags = dwSocketFlags;
        m_nBytesCopied = 0;
    }
};

//
// CFsm_HttpQueryAvailable -
//

class CFsm_HttpQueryAvailable : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPDWORD m_lpdwNumberOfBytesAvailable;

    //
    // local variables
    //

    LPVOID m_lpBuffer;
    DWORD m_dwBufferLength;
    DWORD m_dwBufferLeft;
    BOOL m_bEof;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_HttpQueryAvailable(
        IN LPDWORD lpdwNumberOfBytesAvailable,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(HTTP_QUERY_AVAILABLE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpdwNumberOfBytesAvailable = lpdwNumberOfBytesAvailable;
    }
};

//
// CFsm_DrainResponse -
//

class CFsm_DrainResponse : CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    LPBOOL m_lpbDrained;

    //
    // local variables
    //

    DWORD m_dwAmountToRead;
    DWORD m_dwBufferLeft;
    DWORD m_dwPreviousBytesReceived;
    DWORD m_dwAsyncFlags;
    DWORD m_dwBytesReceived;
    BOOL m_bEof;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_DrainResponse(
        IN LPBOOL lpbDrained,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(DRAIN_RESPONSE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_lpbDrained = lpbDrained;
        m_bEof = FALSE;
        m_dwBytesReceived = 0;
    }
};

//
// CFsm_Redirect -
//

class CFsm_Redirect : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    HTTP_METHOD_TYPE m_tMethod;

    //
    // local variables
    //

    BOOL m_bDrained;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_Redirect(
        IN HTTP_METHOD_TYPE tMethod,
        IN BOOL fRedirectToProxy,
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(REDIRECT);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_tMethod = tMethod;

    }
};

//
// CFsm_ReadLoop -
//

class CFsm_ReadLoop : public CFsm {

friend class HTTP_REQUEST_HANDLE_OBJECT;

private:

    //
    // parameters
    //

    DWORD m_dwSocketFlags;
    PBYTE m_pRead;
    DWORD m_cbReadIn;
    DWORD* m_pcbReadOut;

    //
    // local variables
    //

    LPVOID m_pBuf;      // read buffer
    DWORD  m_cbBuf;     // size of read buffer
    DWORD  m_dwReadEnd; // read offset goal
    DWORD  m_cbRead;    // bytes to read
    DWORD  m_cbRecv;    // bytes received

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ReadLoop(
        IN HTTP_REQUEST_HANDLE_OBJECT * pRequest,
        IN DWORD dwSocketFlags,
        IN PBYTE pRead,
        IN DWORD cbReadIn,
        OUT DWORD* pcbReadOut
        ) : CFsm(RunSM, (LPVOID)pRequest) {

        SET_FSM_TYPE(READ_LOOP);

        m_dwSocketFlags = dwSocketFlags;
        m_pRead = pRead;
        m_cbReadIn = cbReadIn;
        m_pcbReadOut = pcbReadOut;

        if (GetError() != ERROR_SUCCESS) {
            return;
        }
    }

};

//
// CFsm_ParseHttpUrl -
//

class CFsm_ParseHttpUrl : public CFsm {

friend
DWORD
ParseHttpUrl_Fsm(
    IN CFsm_ParseHttpUrl * Fsm
    );

private:

    //
    // parameters
    //

    LPHINTERNET m_phInternet;
    LPSTR m_lpszUrl;
    DWORD m_dwSchemeLength;
    LPSTR m_lpszHeaders;
    DWORD m_dwHeadersLength;
    DWORD m_dwFlags;
    DWORD_PTR m_dwContext;

    //
    // locals
    //

    HINTERNET m_hConnect;
    HINTERNET m_hRequest;

public:

    CFsm_ParseHttpUrl(
        IN OUT LPHINTERNET phInternet,
        IN LPSTR lpszUrl,
        IN DWORD dwSchemeLength,
        IN LPSTR lpszHeaders,
        IN DWORD dwHeadersLength,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        ) : CFsm((DWORD (*)(CFsm *))ParseHttpUrl_Fsm, NULL) {

        SET_FSM_TYPE(PARSE_HTTP_URL);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_phInternet = phInternet;
        m_lpszUrl = lpszUrl;
        m_dwSchemeLength = dwSchemeLength;
        m_lpszHeaders = lpszHeaders;
        m_dwHeadersLength = dwHeadersLength;
        m_dwFlags = dwFlags;
        m_dwContext = dwContext;
    }
};

//
// CFsm_OpenUrl -
//

class CFsm_OpenUrl : public CFsm {

private:

    //
    // parameters
    //

    //
    // locals
    //

public:

    CFsm_OpenUrl();
};

//
// CFsm_ParseUrlForHttp -
//

class CFsm_ParseUrlForHttp : public CFsm {

friend
DWORD
ParseUrlForHttp_Fsm(
    IN CFsm_ParseUrlForHttp * Fsm
    );

private:

    //
    // parameters
    //

    LPHINTERNET m_lphInternet;              // 0x7C
    HINTERNET m_hInternet;                  // 0x80
    LPVOID m_hInternetMapped;               // 0x84
    LPSTR m_lpcszUrl;                       // 0x88
    LPSTR m_lpcszHeaders;                   // 0x8C
    DWORD m_dwHeadersLength;                // 0x90
    DWORD m_dwFlags;                        // 0x94
    DWORD_PTR m_dwContext;                  // 0x98

    //
    // locals
    //

    PROXY_STATE * m_pProxyState;            // 0x9C
    LPSTR m_lpszUrlCopy;                    // 0xA0
    LPFN_URL_PARSER m_pUrlParser;           // 0xA4
    INTERNET_SCHEME m_SchemeType;           // 0xA8
    DWORD m_dwSchemeLength;                 // 0xAC
    AUTO_PROXY_ASYNC_MSG *m_pProxyInfoQuery;// 0xB0
    BOOL m_fFirstCall;                      // 0xB4
    HINTERNET m_hInternetCopy;              // 0xB8

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ParseUrlForHttp(
        IN OUT LPHINTERNET lphInternet,
        IN LPVOID hMapped,
        IN LPCSTR lpcszUrl,
        IN LPCSTR lpcszHeaders,
        IN DWORD dwHeadersLength,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        ) : CFsm(RunSM, NULL) {

        SET_FSM_TYPE(PARSE_URL_FOR_HTTP);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        //
        // ParseUrlForHttp() is the function that returns the API result for
        // InternetOpenUrl() in the new scheme. Make this FSM return the API
        // result
        //

        SetApi(ApiType_Handle);
        m_lphInternet = lphInternet;
        m_hInternet = *lphInternet;
        m_hInternetMapped = hMapped;
        m_hInternetCopy = *lphInternet;
        m_dwHeadersLength = dwHeadersLength;
        m_dwFlags = dwFlags;
        m_dwContext = dwContext;
        m_pProxyState = NULL;
        m_pUrlParser = NULL;
        m_SchemeType = INTERNET_SCHEME_DEFAULT;
        m_dwSchemeLength = 0;
        m_pProxyInfoQuery = NULL;
        m_fFirstCall = TRUE;

        COPY_MANDATORY_PARAM(m_lpcszUrl, lpcszUrl);

        if (lpcszHeaders) 
        {
            m_lpcszHeaders = NewString(lpcszHeaders, dwHeadersLength);
            if (!m_lpcszHeaders)
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
            m_lpcszHeaders = NULL;
    }

    ~CFsm_ParseUrlForHttp() {
        DELETE_MANDATORY_PARAM(m_lpcszUrl);
        DELETE_OPTIONAL_PARAM(m_lpcszHeaders);
    }

    BOOL IsOnApiCall(VOID) {
        return m_fFirstCall;
    }

    VOID ClearOnApiCall(VOID) {
        m_fFirstCall = FALSE;
    }

    DWORD
    QueryProxySettings(
        IN CFsm_ParseUrlForHttp * Fsm,
        IN BOOL fCallback
        );

    DWORD
    BuildProxyMessage(
        IN CFsm_ParseUrlForHttp * Fsm
        );

    DWORD
    ScanProxyUrl(
        IN CFsm_ParseUrlForHttp * Fsm
        );

    DWORD 
    CompleteParseUrl(
        IN CFsm_ParseUrlForHttp * Fsm,
        IN LPINTERNET_THREAD_INFO lpThreadInfo,
        IN DWORD error
        );
};

//
// InternetReadFile API
//

class CFsm_ReadFile : public CFsm {

friend
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    );

public:
    LPVOID m_lpBuffer;
    LPDWORD m_lpdwNumberOfBytesRead;


private:

    //
    // parameters
    //

    DWORD m_dwNumberOfBytesToRead;

    //
    // local variables
    //

    DWORD m_dwBytesRead;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ReadFile(
        IN LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead
        ) : CFsm(RunSM, NULL, FALSE) {

        SET_FSM_TYPE(READ_FILE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        m_lpBuffer = lpBuffer;
        m_dwNumberOfBytesToRead = dwNumberOfBytesToRead;
        m_lpdwNumberOfBytesRead = lpdwNumberOfBytesRead;
    }
};

//
// InternetReadFileEx API
//

class CFsm_ReadFileEx : public CFsm {

friend
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    );

private:

    //
    // parameters
    //

    LPINTERNET_BUFFERS m_lpBuffersOut;
    DWORD m_dwFlags;
    DWORD_PTR m_dwContext;

    //
    // local variables
    //

    DWORD m_dwNumberOfBytesToRead;
    DWORD m_dwBytesRead;

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_ReadFileEx(
        IN LPINTERNET_BUFFERS lpBuffersOut,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        ) : CFsm(RunSM, NULL) {

        SET_FSM_TYPE(READ_FILE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        m_lpBuffersOut = lpBuffersOut;
        m_dwFlags = dwFlags;
        m_dwContext = dwContext;
    }
};

//
// InternetQueryDataAvailable API
//

class CFsm_QueryAvailable : public CFsm {

friend
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    );

public:
    LPDWORD m_lpdwNumberOfBytesAvailable;

private:

    //
    // parameters
    //

    DWORD m_dwFlags;
    DWORD_PTR m_dwContext;

    //
    // local variables
    //

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );

    CFsm_QueryAvailable(
        IN LPDWORD lpdwNumberOfBytesAvailable,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        ) : CFsm(RunSM, NULL, FALSE) {

        SET_FSM_TYPE(QUERY_DATA_AVAILABLE);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        SetApi(ApiType_Bool);
        m_lpdwNumberOfBytesAvailable = lpdwNumberOfBytesAvailable;
        m_dwFlags = dwFlags;
        m_dwContext = dwContext;
    }
};

//
// Fsm to Background tasks 
//

class BackgroundTaskMgr;   // defined in bgtask.hxx
class CFsm_BackgroundTask : public CFsm {
friend class BackgroundTaskMgr;

friend
DWORD
BackgroundTask_Fsm(
    IN CFsm_BackgroundTask * Fsm
    );

private:

    //
    // parameters
    //
    BackgroundTaskMgr*  m_pMgr;
    LPCSTR              m_lpszUrl;

    //
    // local variables
    //

    // 
    // internal methods

    DWORD DoSendReq();

    // 
    // this private func can only
    // be called from Mgr 
    //
    CFsm_BackgroundTask(
        IN BackgroundTaskMgr*   pMgr,
        IN LPCSTR               lpszUrl
        ) : CFsm(RunSM, NULL) {

        SET_FSM_TYPE(BACKGROUND_TASK);

        if (GetError() != ERROR_SUCCESS) {
            return;
        }

        m_pMgr = pMgr;
        COPY_MANDATORY_PARAM(m_lpszUrl, lpszUrl);
    }

public:

    static
    DWORD
    RunSM(
        IN CFsm * Fsm
        );


    ~CFsm_BackgroundTask(); 
};


//
// prototypes
//

CFsm *
ContainingFsm(
    IN LPVOID lpAddress
    );

DWORD
RunAll(
    VOID
    );

DWORD
DoFsm(
    IN CFsm * pFsm
    );

DWORD
DoAsyncFsm(
    IN CFsm * pFsm,
    IN HTTP_REQUEST_HANDLE_OBJECT *pRequest
    );
