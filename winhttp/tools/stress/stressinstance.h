//////////////////////////////////////////////////////////////////////
// File:  StressInstance.h
//
// Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
// Purpose:
//	StressInstance.h: interface for the StressInstance class.
//	This class is used spawn and monitor instances of the stressEXE app.
//
// History:
//	02/15/01	DennisCh	Created
//
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Includes
//////////////////////////////////////////////////////////////////////

//
// WIN32 headers
//
#define UNICODE
#define _UNICODE

#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <winhttp.h>

//
// Project headers
//
#include <debugger.h>

//////////////////////////////////////////////////////////////////////
// Constants
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRESSINSTANCE_H__806226FB_2170_4FE3_ACCA_EF8952E6A524__INCLUDED_)
#define AFX_STRESSINSTANCE_H__806226FB_2170_4FE3_ACCA_EF8952E6A524__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define	RESPONSE_HEADER__STRESS_BEGIN			_T("WinHttpStress_Begin: Begin Stress")
#define	RESPONSE_HEADER__STRESS_END				_T("WinHttpStress_End: End Stress")


// ***************************
// ** max string URL length
#define MAX_STRESS_URL							MAX_PATH * 2


// ***************************
// ** time to check the stress exe in milliseconds
#define STRESSINSTANCE_MONITOR_EXE_TIME			1200000	// 20 minutes

// ***************************
// ** Command line CreateProcess will use. Takes two params: the path+filename of the stressExe and CDB pipe name
//#define STRESSINSTANCE_DEBUG_COMMANDLINE		_T("c:\\debuggers\\remote.exe /s \"c:\\debuggers\\cdb.exe -g -G \"%s\"\" %s")
#define STRESSINSTANCE_DEBUG_COMMANDLINE		_T("\"%s\"")

// ***************************
// ** Memory dump path
#define	STRESSINSTANCE_DEFAULT_MEMORY_DUMP_PATH	_T("\\\\hairball\\dump$\\")

// ***************************
// ** relative path to the directory where the stressExe files will be downloaded to.
#define STRESSINSTANCE_STRESS_EXE_DOWNLOAD_DIR	_T("stressExe")

// ***************************
// ** Time to wait for stressExe to close after telling it
#define STRESSINSTANCE_STRESS_EXE_CLOSE_TIMEOUT	100

// ***************************
// ** Cross process event object names. We append the PID of the process to the end to prevent name collisions.
#define STRESSINSTANCE_STRESS_EXE_EVENT_EXITPROCESS			_T("ExitProcessEvent")


class StressInstance  
{
public:
				StressInstance();
	virtual		~StressInstance();

	BOOL		Begin();
	VOID		End();
	BOOL		IsRunning(DWORD);

	DWORD		Get_ID();
	LPTSTR		Get_StressExeMemoryDumpPath();

	VOID		Set_StressExeMemoryDumpPath(LPTSTR);
	VOID		Set_StressExeURL(LPTSTR);
	VOID		Set_StressExePdbURL(LPTSTR);
	VOID		Set_StressExeSymURL(LPTSTR);
	VOID		Set_StressExeID(DWORD);
	VOID		Set_PageHeapCommands(LPCTSTR);
	VOID		Set_UMDHCommands(LPCTSTR);

	BOOL		DownloadStressExe();

				// This is the timer callback proc that monitors the stressExe process.
	friend		VOID CALLBACK StressExe_TimerProc(HWND, UINT, UINT_PTR, DWORD);

				// This is the timer callback proc for the debugger object
	friend		DWORD DebuggerCallbackProc(DWORD, LPVOID, LPTSTR, LPVOID);

private:
	DWORD		m_dwStressExe_ID;					// ID from the stressAdmin DB uniquely identifying this stress EXE.
	LPTSTR		m_szStressExe_URL;					// URL to the stress app
	LPTSTR		m_szStressExe_PDB_URL;				// URL to the stress app's pdb file
	LPTSTR		m_szStressExe_SYM_URL;				// URL to the stress app's sym file
	LPTSTR		m_szStressExe_FilePath;				// Local relative path of the downloaded stress EXE
	LPTSTR		m_szStressExe_FileName;				// Local filename of the downloaded stress EXE
	LPTSTR		m_szStressExe_FilePathAndName;		// Path and filename to the local stressExe downloaded
	LPTSTR		m_szStressExe_PageHeapCommandLine;	// Command line params when enabling pageheap.
	LPTSTR		m_szStressExe_UMDHCommandLine;		// Command line params when enabling UMDH.

	LPTSTR		m_szStressExe_MemDumpPath;			// path that the minidump will dump to

	Debugger	*m_objDebugger;						// the debughelp debugger object

	PROCESS_INFORMATION 	m_piStressExeProcess;	// stuct containing info on the stressExe process
	HANDLE					m_hStressExe_ProcessExitEvent;	// Cross process event we send stressExe to tell it to exit
};


#endif // !defined(AFX_STRESSINSTANCE_H__806226FB_2170_4FE3_ACCA_EF8952E6A524__INCLUDED_)
