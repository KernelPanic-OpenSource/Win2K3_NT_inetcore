//
//
//
#include "stdafx.h"

// This file cannot be compiled as a C++ file, otherwise the linker
// will bail on unresolved externals (even with extern "C" wrapping 
// this).

// Define some things for debug.h
//
#define SZ_DEBUGINI         "ccshell.ini"
#define SZ_DEBUGSECTION     "trialoc"
#define SZ_MODULE           "trialoc"
#define DECLARE_DEBUG
#include "..\inc\debug.h"


