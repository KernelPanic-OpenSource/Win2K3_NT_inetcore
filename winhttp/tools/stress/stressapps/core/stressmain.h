//////////////////////////////////////////////////////////////////////
// File:  stressMain.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//		This is an empty template for WinHttp stressScheduler stress apps.
//		The stress test lives in WinHttp_StressTest() and will be called
//		repeatedly in the main function.
//
// History:
//	03/30/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////

#define	EXIT_PROCESS_EVENT_NAME	"ExitProcessEvent"
#define WIN32_LEAN_AND_MEAN 1


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

// Win32 headers
#include <windows.h>
#include <winhttp.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>


//////////////////////////////////////////////////////////////////////
// Globals
//////////////////////////////////////////////////////////////////////
extern BOOL	IsTimeToExitStress();
extern VOID	LogText(LPCSTR,	...);