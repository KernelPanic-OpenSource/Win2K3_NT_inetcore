#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

extern "C" void InitChkStk(unsigned long dwFill);
#define DLL_MAIN_FUNCTION_NAME  _DllMainStartupChkStk
#define DLL_MAIN_PRE_CINIT      InitChkStk(0xCCCCCCCC);
#define DLL_MAIN_PRE_CEXIT      /* Nothing */
#define DLL_MAIN_POST_CEXIT     /* Nothing */
#include "dllmain.cxx"
