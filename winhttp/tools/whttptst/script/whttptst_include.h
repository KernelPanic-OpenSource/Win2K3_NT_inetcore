var EXCEPTION_ACCESS_VIOLATION    = 0xc0000005;
var EXCEPTION_STACK_OVERFLOW      = 0xc00000fd;
var EXCEPTION_INT_DIVIDE_BY_ZERO  = 0xc0000094;
var EXCEPTION_ILLEGAL_INSTRUCTION = 0xc000001d;

// from resources.h
var TYPE_LPWSTR   = 0;
var TYPE_LPLPWSTR = 1;
var TYPE_LPSTR    = 2;
var TYPE_LPLPSTR  = 3;
var TYPE_DWORD    = 4;
var TYPE_LPDWORD  = 5;

// pointer value flags
var NULL_PTR      = 0;
var BAD_PTR       = 1; 
var FREE_PTR      = 2;
var UNINIT_PTR    = 3;

// data init flags
var INIT_NULL     = 0;
var INIT_SMILEY   = 1;
var INIT_HEXFF    = 2;
var INIT_GARBAGE  = 3;

// from winhttp.h
var WINHTTP_ACCESS_TYPE_DEFAULT_PROXY = 0;
var WINHTTP_ACCESS_TYPE_NO_PROXY      = 1;
var WINHTTP_ACCESS_TYPE_NAMED_PROXY   = 3;
var WINHTTP_FLAG_SYNC                 = 0x00000000; // not in header
var WINHTTP_FLAG_ASYNC                = 0x10000000;

var WINHTTP_CALLBACK_STATUS_RESOLVING_NAME          = 0x00000001;
var WINHTTP_CALLBACK_STATUS_NAME_RESOLVED           = 0x00000002;
var WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER    = 0x00000004;
var WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER     = 0x00000008;
var WINHTTP_CALLBACK_STATUS_SENDING_REQUEST         = 0x00000010;
var WINHTTP_CALLBACK_STATUS_REQUEST_SENT            = 0x00000020;
var WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE      = 0x00000040;
var WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED       = 0x00000080;
var WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION      = 0x00000100;
var WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED       = 0x00000200;
var WINHTTP_CALLBACK_STATUS_HANDLE_CREATED          = 0x00000400;
var WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING          = 0x00000800;
var WINHTTP_CALLBACK_STATUS_DETECTING_PROXY         = 0x00001000;
var WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE        = 0x00002000;
var WINHTTP_CALLBACK_STATUS_REDIRECT                = 0x00004000;
var WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE   = 0x00008000;
var WINHTTP_CALLBACK_STATUS_SECURE_FAILURE          = 0x00010000;

var WINHTTP_CALLBACK_FLAG_RESOLVE_NAME              = (WINHTTP_CALLBACK_STATUS_RESOLVING_NAME | WINHTTP_CALLBACK_STATUS_NAME_RESOLVED);
var WINHTTP_CALLBACK_FLAG_CONNECT_TO_SERVER         = (WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER | WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER);
var WINHTTP_CALLBACK_FLAG_SEND_REQUEST              = (WINHTTP_CALLBACK_STATUS_SENDING_REQUEST | WINHTTP_CALLBACK_STATUS_REQUEST_SENT);
var WINHTTP_CALLBACK_FLAG_RECEIVE_RESPONSE          = (WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE | WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED);
var WINHTTP_CALLBACK_FLAG_CLOSE_CONNECTION          = (WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION | WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED);
var WINHTTP_CALLBACK_FLAG_HANDLES                   = (WINHTTP_CALLBACK_STATUS_HANDLE_CREATED | WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING);
var WINHTTP_CALLBACK_FLAG_DETECTING_PROXY           = WINHTTP_CALLBACK_STATUS_DETECTING_PROXY;
var WINHTTP_CALLBACK_FLAG_REQUEST_COMPLETE          = WINHTTP_CALLBACK_STATUS_REQUEST_COMPLETE;
var WINHTTP_CALLBACK_FLAG_REDIRECT                  = WINHTTP_CALLBACK_STATUS_REDIRECT;
var WINHTTP_CALLBACK_FLAG_INTERMEDIATE_RESPONSE     = WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE;
var WINHTTP_CALLBACK_FLAG_SECURE_FAILURE            = WINHTTP_CALLBACK_STATUS_SECURE_FAILURE;
var WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS         = 0x0000000a; // not the same as the header file, but script doesn't like 0xffffffff