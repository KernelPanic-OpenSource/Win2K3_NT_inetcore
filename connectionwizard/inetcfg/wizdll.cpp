/*****************************************************************/
/**          Microsoft Windows for Workgroups        **/
/**          Copyright (C) Microsoft Corp., 1991-1992      **/
/*****************************************************************/ 

//
//  WIZDLL.C - 32-bit stubs for functions that call into 16-bit DLL
//

//  HISTORY:
//  
//  11/20/94  jeremys  Created.
//  96/03/13  markdu  Added IcfgSetInstallSourcePath().
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//  96/05/28  markdu  InitConfig and DeInitConfig in DllEntryPoint.
//

#include "wizard.h"

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)
HINSTANCE ghInstance=NULL;
#pragma data_seg(DATASEG_DEFAULT)

typedef UINT RETERR;

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

#ifdef __cplusplus
}
#endif // __cplusplus

/*******************************************************************

  NAME:    DllEntryPoint

  SYNOPSIS:  Entry point for DLL.

  NOTES:    Initializes thunk layer to WIZ16.DLL

********************************************************************/
BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
  if( fdwReason == DLL_PROCESS_ATTACH )
  {
    ghInstance = hInstDll;

	//
	// 7/22/97 jmazner Olympus #9903
	//
	TCHAR szPath[MAX_PATH + 1];
	BOOL fPathAlreadySet = FALSE;

	GetICW11Path( szPath, &fPathAlreadySet );

	if( !fPathAlreadySet && szPath[0] )
	{
		SetICWRegKeysToPath( szPath );
	}

	if( IsParentICW10() )
	{
		DEBUGMSG("DllEntryPoint, INETCFG called from old component, bailing out!");
		return FALSE;
	}


	// load the config dll proc addresses
    BOOL fRet = InitConfig(NULL);
    if (FALSE == fRet)
    {
      // Error message was already displayed in InitConfig.
      return FALSE;
    }
  }

  if( fdwReason == DLL_PROCESS_DETACH )
  {
    DeInitConfig();
  }

  return TRUE;
}


#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void __cdecl main() {};

#ifdef __cplusplus
}
#endif // __cplusplus
