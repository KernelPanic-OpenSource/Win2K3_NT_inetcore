#include "priv.h"
#include "advpub.h"
#include "sdsutils.h"
#include "utils.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

/*----------------------------------------------------------
Purpose: Behaves just like RegQueryValueEx, except if the
         data type is REG_EXPAND_SZ, then this goes ahead
         and expands out the string.  *pdwType will always
         be massaged to REG_SZ if this happens.

Returns: 
Cond:    --
*/
DWORD
SDSQueryValueExA(
    IN     HKEY    hkey,
    IN     LPCSTR  pszValue,
    IN     LPDWORD lpReserved,
    OUT    LPDWORD pdwType,
    OUT    LPVOID  pvData,
    IN OUT LPDWORD pcbData)
{
    DWORD dwRet;
    DWORD cbSize;
    DWORD dwType;
    LPSTR lpsz;

    if (pvData) 
    {
        // Trying to get back data

        cbSize = *pcbData;     // Size of output buffer
        dwRet = RegQueryValueExA(hkey, pszValue, lpReserved, &dwType,
                                 (LPBYTE)pvData, &cbSize);

        // Normally, we'd be done with this.  But do some extra work
        // if this is an expandable string (something that has system
        // variables in it), or if we need to pad the buffer.

        if (NO_ERROR == dwRet)
        {
            // Note: on Win95, RegSetValueEx will always write the 
            // full string out, including the null terminator.  On NT,
            // it won't unless the write length was specified.  
            // Hence, we have the following check.

            // Pad the buffer, in case the string didn't have a null
            // terminator when it was stored?
            if (REG_SZ == dwType)
            {
                // Yes
                if (cbSize < *pcbData) 
                {
                    LPSTR lpszData = (LPSTR)pvData;
                    lpszData[cbSize] = '\0';
                }
            }
            // Expand the string?
            else if (REG_EXPAND_SZ == dwType)
            {
                // Yes

                // Use a temporary buffer to expand
                lpsz = (LPSTR)LocalAlloc(LPTR, *pcbData);    
                if ( !lpsz )
                    return ERROR_OUTOFMEMORY;

                cbSize = ExpandEnvironmentStringsA((LPSTR)pvData, lpsz, *pcbData);

                // BUGBUG:: NT screws up the cbSize returned...
                if (cbSize > 0)
                    cbSize = lstrlen(lpsz) + 1;
                if (cbSize > 0 && cbSize <= *pcbData) 
                    lstrcpynA((LPSTR)pvData, lpsz, *pcbData);
                else
                    dwRet = GetLastError();

                LocalFree(lpsz);

                // Massage dwType so that callers always see REG_SZ
                dwType = REG_SZ;
            }
        }
    } 
    else 
    {
        // Trying to find out how big of a buffer to use

        cbSize = 0;
        dwRet = RegQueryValueExA(hkey, pszValue, lpReserved, &dwType,
                                 NULL, &cbSize);
        if (NO_ERROR == dwRet && REG_EXPAND_SZ == dwType)
        {
            CHAR szBuff[1];

            // Find out the length of the expanded string
            //
            lpsz = (LPSTR)LocalAlloc(LPTR, cbSize);
            if (!lpsz)
                return ERROR_OUTOFMEMORY;

            dwRet = RegQueryValueExA(hkey, pszValue, lpReserved, NULL,
                                     (LPBYTE)lpsz, &cbSize);

            if (NO_ERROR == dwRet)
            {
                cbSize = ExpandEnvironmentStringsA(lpsz, szBuff, ARRAYSIZE(szBuff));

                // BUGBUG:: NT screws up the cbSize returned...
                if (cbSize > 0)
                    cbSize = lstrlen(lpsz) + 1;
            }

            LocalFree(lpsz);

            // Massage dwType so that callers always see REG_SZ
            dwType = REG_SZ;
        }
    }

    if (pdwType)
        *pdwType = dwType;

    if (pcbData)
        *pcbData = cbSize;

    return dwRet;
}

