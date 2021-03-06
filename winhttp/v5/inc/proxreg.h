#include <inetreg.h>

#define BLOB_BUFF_GRANULARITY   1024

class CRegBlob
{
    private:
        HKEY    _hkey;
        BOOL    _fWrite;
        BOOL    _fCommit;
        DWORD   _dwOffset;
        DWORD   _dwBufferLimit;
        BYTE *  _pBuffer;
        LPCSTR  _pszValue;

    public:
        CRegBlob(BOOL fWrite);
        ~CRegBlob();
        DWORD Init(HKEY hBaseKey, LPCSTR pszSubKey, LPCSTR pszValue);
        DWORD Abandon();
        DWORD Commit();
        DWORD WriteString(LPCSTR pszString);
        DWORD ReadString(LPCSTR * ppszString);
        DWORD WriteBytes(LPCVOID pBytes, DWORD dwByteCount);
        DWORD ReadBytes(LPVOID pBytes, DWORD dwByteCount);

    private:
        DWORD Encrpyt();
        DWORD Decrypt();
};


typedef struct {

    //
    // dwStructSize - Structure size to handle growing list of new entries or priv/pub structures
    //

    DWORD dwStructSize;

    //
    // dwFlags - Proxy type flags
    //

    DWORD dwFlags;

    //
    // dwCurrentSettingsVersion - a counter incremented every time we change our settings
    //

    DWORD dwCurrentSettingsVersion;

    //
    // lpszConnectionName - name of the Connectoid for this connection
    //
    
    LPCSTR lpszConnectionName;

    //
    // lpszProxy - proxy server list
    //

    LPCSTR lpszProxy;

    //
    // lpszProxyBypass - proxy bypass list
    //

    LPCSTR lpszProxyBypass;

} INTERNET_PROXY_INFO_EX, * LPINTERNET_PROXY_INFO_EX;

// name of blob for saved legacy settings
#define LEGACY_SAVE_NAME            "SavedLegacySettings"

DWORD
LoadProxySettings();

DWORD
ReadProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo
    );

void
CleanProxyStruct(
    LPINTERNET_PROXY_INFO_EX pInfo
    );

DWORD
SetPerConnOptions(
    HINTERNET hInternet,    
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    );

DWORD
QueryPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    );

BOOL 
IsConnectionMatch(
    LPCSTR lpszConnection1,
    LPCSTR lpszConnection2
    );

HKEY
FindBaseProxyKey(
    VOID
    );
