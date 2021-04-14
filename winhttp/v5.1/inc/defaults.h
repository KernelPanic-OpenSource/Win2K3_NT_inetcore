/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    defaults.h

Abstract:

    Contains all default manifests for WININET

Author:

    Richard L Firth (rfirth) 15-Jul-1995

Revision History:

    15-Jul-1995 rfirth
        Created

--*/

//
// default timeout values and retry counts
//

#define DEFAULT_RESOLVE_TIMEOUT             INFINITE
#define DEFAULT_CONNECT_TIMEOUT             ((LONG)1*60*1000)   // 1 minute
#define DEFAULT_CONNECT_RETRIES             5
#define DEFAULT_SEND_TIMEOUT                ((LONG)30*1000)     // 30 seconds
#define DEFAULT_RECEIVE_TIMEOUT             ((LONG)30*1000)     // 30 seconds
#define DEFAULT_KEEP_ALIVE_TIMEOUT          (1 * 60 * 1000)         // 1 minute
#define DEFAULT_FROM_CACHE_TIMEOUT          (5 * 1000)              // 5 seconds
#define DEFAULT_DNS_CACHE_ENTRIES           32
#define DEFAULT_DNS_CACHE_TIMEOUT           (5 * 60)               // 5 minutes
#define DEFAULT_MAX_HTTP_REDIRECTS          10
#define DEFAULT_MAX_CONNECTIONS_PER_SERVER  2                       // default HTTP 1.1
#define DEFAULT_MAX_CONS_PER_1_0_SERVER     4                       // default HTTP 1.0
#define DEFAULT_CONNECTION_LIMIT_TIMEOUT    (1 * 60 * 1000)         // 1 minute
#define DEFAULT_CONNECTION_INACTIVE_TIMEOUT (10 * 1000)             // 10 seconds
#define DEFAULT_SERVER_INFO_TIMEOUT         (2 * 60 * 1000)         // 2 minutes
#define DEFAULT_IDLE_TIMEOUT                1000                    // 1 second
#define DEFAULT_RECEIVE_RESPONSE_TIMEOUT    (90 * 1000)             // 90 seconds
#define DEFAULT_ASYNC_SELECT_THREAD_TIMEOUT (5 * 1000)              // 5 seconds

//
// thread pool default constants
//

#define DEFAULT_MINIMUM_THREADS     0
#define DEFAULT_MAXIMUM_THREADS     4   // arbitrary
#define DEFAULT_INITIAL_THREADS     1
#define DEFAULT_THREAD_IDLE_TIMEOUT (2 * 60 * 1000) // 2 minutes
#define DEFAULT_WORK_QUEUE_LIMIT    8
#define DEFAULT_WORK_ITEM_PRIORITY  0

//
// async scheduler thread default constants
//

#define DEFAULT_WORKER_THREAD_TIMEOUT       (2 * 60 * 1000)        // 2 minutes
#define DEFAULT_MAXIMUM_QUEUE_DEPTH         2
#define DEFAULT_FIBER_STACK_SIZE            (16 K)
#define DEFAULT_CREATE_FIBER_ATTEMPTS       4

//
// default sizes
//

#define DEFAULT_RECEIVE_BUFFER_INCREMENT        (1 K)
#define DEFAULT_TRANSPORT_PACKET_LENGTH         (1 K)
#define DEFAULT_HTML_QUERY_BUFFER_LENGTH        (4 K)
#define DEFAULT_SOCKET_SEND_BUFFER_LENGTH       ((DWORD)-1)
#define DEFAULT_SOCKET_RECEIVE_BUFFER_LENGTH    ((DWORD)-1)
#define DEFAULT_SOCKET_QUERY_BUFFER_LENGTH      (8 K)

//
// default strings
//

#define DEFAULT_HTTP_REQUEST_VERB       "GET"
#define DEFAULT_EMAIL_NAME              "user@domain"
#define DEFAULT_URL_SCHEME_NAME         "http"

// default SSL protocols
#define DEFAULT_SECURE_PROTOCOLS        (WINHTTP_FLAG_SECURE_PROTOCOL_SSL3 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1)


// Maximum size allowed on a proxy auto-config script.
#define MAX_PAC_SCRIPT_SIZE             (1 * 1024 * 1024)   // 1 megabyte
#define MAX_PAC_SCRIPT_RUN_TIMEOUT      (60 * 1000)         // 60 seconds
