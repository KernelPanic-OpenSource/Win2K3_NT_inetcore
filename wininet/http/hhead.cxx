/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hhead.cxx

Abstract:

    This file contains autogenerated table values of a perfect hash function
    DO NOT, DO NOT EDIT THIS FILE, TO ADD HEADERS SEE hashgen.cpp
    Contents:
      GlobalKnownHeaders
      GlobalHeaderHashs

Author:

   Arthur Bierer (arthurbi) 19-Dec-1997 (AND) my code generator[hashgen.exe]

Revision History:

--*/


#include <wininetp.h>
#include "httpp.h"

#ifdef HEADER_HASH_SEED
#if (HEADER_HASH_SEED != 1291949)
#error HEADER_HASH_SEED has not been updated in the header file, please copy this number to the header
#endif
#else
#define HEADER_HASH_SEED 1291949
#endif

#ifdef MAX_HEADER_HASH_SIZE
#if (MAX_HEADER_HASH_SIZE != 153)
#error MAX_HEADER_HASH_SIZE has not been updated in the header file, please copy this number to the header
#endif
#else
#define MAX_HEADER_HASH_SIZE 153
#endif

#ifdef HTTP_QUERY_MAX
#if (HTTP_QUERY_MAX != 78)
#error HTTP_QUERY_MAX is not the same as the value used in wininet.h, this indicates mismatched headers, see hashgen.cpp
#endif
#endif

//
// GlobalHeaderHashs - array of precalculated hashes on case-sensetive set of known headers.
// This array must be used with the same hash function used to generate it.
// Note, all entries in this array are biased (++'ed) by 1 from HTTP_QUERY_ manifests in wininet.h.
//   0-ed entries indicate error values
//

const BYTE GlobalHeaderHashs[MAX_HEADER_HASH_SIZE] = {
      0,   0,   0,  71,   0,   0, 
     53,   0,   0,   0,   0,   0, 
     40,  60,  72,   0,   0,   0, 
     64,  61,   0,  28,   4,   0, 
     76,  57,   0,  30,   0,   8, 
      0,  15,   0,   0,   0,   0, 
     24,   0,  66,  29,   0,   0, 
      0,   0,   0,   0,  13,  14, 
     16,  33,   0,   0,   0,  68, 
     41,   0,   7,   0,   0,   0, 
      0,   0,  32,  17,  51,  48, 
     67,  11,   0,   0,   5,   0, 
     65,   2,   0,   0,  35,   6, 
      0,   0,  31,  50,   0,   0, 
      0,  49,  78,  26,   0,   0, 
     42,   0,   0,  43,   0,  27, 
     69,   9,   1,   0,   0,  18, 
     10,  79,   0,   3,  47,  55, 
      0,  44,   0,  56,   0,  70, 
     54,  52,   0,   0,   0,   0, 
      0,   0,  36,  62,   0,   0, 
     45,   0,   0,  12,   0,  73, 
     77,   0,  63,   0,  59,   0, 
      0,   0,   0,  58,  38,   0, 
      0,   0,  39,  25,   0,  37, 
      0,  34,   0, 
   };

//
// GlobalKnownHeaders - array of HTTP request and response headers that we understand.
// This array must be in the same order as the HTTP_QUERY_ manifests in WININET.H
//

#define HEADER_ENTRY(String, Flags, HashVal) String, sizeof(String) - 1, Flags, HashVal

const struct KnownHeaderType GlobalKnownHeaders[HTTP_QUERY_MAX+1] = {
    HEADER_ENTRY("Mime-Version",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x179ED708),
    HEADER_ENTRY("Content-Type",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x8A50E357),
    HEADER_ENTRY("Content-Transfer-Encoding",  HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x562B730E),
    HEADER_ENTRY("Content-Id",                 HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xE7ADCA82),
    HEADER_ENTRY("Content-Description",        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x9D4EB3D9),
    HEADER_ENTRY("Content-Length",             (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_NUMBER), 0x4E02E517),
    HEADER_ENTRY("Content-Language",           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xA6681019),
    HEADER_ENTRY("Allow",                      HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x297A46CC),
    HEADER_ENTRY("Public",                     HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7C5DD44C),
    HEADER_ENTRY("Date",                       (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0xBB71C70B),
    HEADER_ENTRY("Expires",                    HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC153144D),
    HEADER_ENTRY("Last-Modified",              (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0x23617A4F),
    HEADER_ENTRY("Message-id",                 HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC9AE6FAC),
    HEADER_ENTRY("Uri",                        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xCF60D45D),
    HEADER_ENTRY("Derived-From",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x6AAF4091),
    HEADER_ENTRY("Cost",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB717626),
    HEADER_ENTRY("Link",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB764B5B),
    HEADER_ENTRY("Pragma",                     HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7C26E9A5),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_NUMBER, 0x0),
    HEADER_ENTRY("",                           0, 0x0),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("Connection",                 HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x190A507D),
    HEADER_ENTRY("Accept",                     HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x581B295D),
    HEADER_ENTRY("Accept-Charset",             HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC87564B4),
    HEADER_ENTRY("Accept-Encoding",            HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x81EEF031),
    HEADER_ENTRY("Accept-Language",            HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x2827D6EE),
    HEADER_ENTRY("Authorization",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xEC768B9E),
    HEADER_ENTRY("Content-Encoding",           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x2F295C),
    HEADER_ENTRY("Forwarded",                  HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x420550EB),
    HEADER_ENTRY("From",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB732781),
    HEADER_ENTRY("If-Modified-Since",          (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0x88B69529),
    HEADER_ENTRY("Location",                   HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x8DD3A2C6),
    HEADER_ENTRY("Orig-Uri",                   HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x47CCA2FB),
    HEADER_ENTRY("Referer",                    HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7CFEFF98),
    HEADER_ENTRY("Retry-After",                (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0xD2881822),
    HEADER_ENTRY("Server",                     HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x8244B644),
    HEADER_ENTRY("Title",                      HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x2AD094CF),
    HEADER_ENTRY("User-Agent",                 HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x968679A8),
    HEADER_ENTRY("WWW-Authenticate",           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x21BED5E),
    HEADER_ENTRY("Proxy-Authenticate",         HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBE0F117B),
    HEADER_ENTRY("Accept-Ranges",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC3BF828A),
    HEADER_ENTRY("Set-Cookie",                 HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC704A760),
    HEADER_ENTRY("Cookie",                     HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x5DA54DC7),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("Refresh",                    0, 0x7D05EAFC),
    HEADER_ENTRY("Content-Disposition",        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xDCB6FC4A),
    HEADER_ENTRY("Age",                        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xCF607DDA),
    HEADER_ENTRY("Cache-Control",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBBD3B86F),
    HEADER_ENTRY("Content-Base",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x8A469ED0),
    HEADER_ENTRY("Content-Location",           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xC201A76E),
    HEADER_ENTRY("Content-Md5",                HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xDD672BFB),
    HEADER_ENTRY("Content-Range",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xD43BEC42),
    HEADER_ENTRY("Etag",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB72A1CE),
    HEADER_ENTRY("Host",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB74340B),
    HEADER_ENTRY("If-Match",                   HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x971A5776),
    HEADER_ENTRY("If-None-Match",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xCF6E9D3),
    HEADER_ENTRY("If-Range",                   HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x9774B8F6),
    HEADER_ENTRY("If-Unmodified-Since",        (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0xC946042C),
    HEADER_ENTRY("Max-Forwards",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x3C693EC8),
    HEADER_ENTRY("Proxy-Authorization",        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x96A221ED),
    HEADER_ENTRY("Range",                      HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x2AA7E69A),
    HEADER_ENTRY("Transfer-Encoding",          HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBD09B166),
    HEADER_ENTRY("Upgrade",                    HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7DAF65B5),
    HEADER_ENTRY("Vary",                       HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xBB7BA5AF),
    HEADER_ENTRY("Via",                        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xCF60D76D),
    HEADER_ENTRY("Warning",                    HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xF56B7D23),
    HEADER_ENTRY("Expect",                     (HTTP_QUERY_FLAG_REQUEST_HEADERS | HTTP_QUERY_FLAG_SYSTEMTIME), 0x62F2EDB6),
    HEADER_ENTRY("Proxy-Connection",           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xD79F12C),
    HEADER_ENTRY("Unless-Modified-Since",      HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x8E53B934),
    HEADER_ENTRY("Ms-Echo-Request",            0, 0xBB7AF4AF),
    HEADER_ENTRY("Ms-Echo-Reply",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xA5326612),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("",                           HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x0),
    HEADER_ENTRY("Proxy-Support",              HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7C4F7F99),
    HEADER_ENTRY("Authentication-Info",        HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xB03F47E6),
    HEADER_ENTRY("PassportURLs",               HTTP_QUERY_FLAG_REQUEST_HEADERS, 0xCF7C676F),
    HEADER_ENTRY("PassportConfig",             HTTP_QUERY_FLAG_REQUEST_HEADERS, 0x7601C9BF),
    };


