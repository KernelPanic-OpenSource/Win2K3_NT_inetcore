//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       iapp.h
//
//  Contents:   precompiled header file for the trans directory
//
//  Classes:
//
//  Functions:
//
//  History:    12-22-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#include <urlmon.hxx>
#ifndef unix
#include "..\iapp\curl.hxx"
#include "..\iapp\cnet.hxx"
#else
#include "../iapp/curl.hxx"
#include "../iapp/cnet.hxx"
#endif /* unix */
 

