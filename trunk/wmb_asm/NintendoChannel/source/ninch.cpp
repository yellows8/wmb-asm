/*
Wmb Asm, the SDK, and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#define DLLMAIN
#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#include "..\..\SDK\include\wmb_asm_sdk_plugin.h"

#undef DLLIMPORT
#define DLLIMPORT __declspec (dllexport)

#ifdef DLLIMPORT
	#ifdef NDS
		#undef DLLIMPORT
		#define DLLIMPORT
	#endif
#endif

//Change the names of the functions on-the-fly when compiling for DS, since everything is in one binary.
#ifdef NDS
	#define AsmPlug_GetID NINCH_AsmPlug_GetID
	#define AsmPlug_GetIDStr NINCH_AsmPlug_GetIDStr
	#define AsmPlug_GetPriority NINCH_AsmPlug_GetPriority
	#define AsmPlug_GetStatus NINCH_AsmPlug_GetStatus
	#define AsmPlug_QueryFailure NINCH_AsmPlug_QueryFailure
	#define AsmPlug_Handle802_11 NINCH_AsmPlug_Handle802_11
	#define AsmPlug_Init NINCH_AsmPlug_Init
	#define AsmPlug_DeInit NINCH_AsmPlug_DeInit
	#define AsmPlug_GetNdsData NINCH_AsmPlug_GetNdsData
	#define AsmPlug_Reset NINCH_AsmPlug_Reset
	#define AsmPlug_GetModeStatus NINCH_AsmPlug_GetModeStatus
	#define AsmPlug_SwitchMode NINCH_AsmPlug_SwitchMode
#endif

int ninch_stage = 0;

sAsmSDK_Config *NINCHCONFIG = NULL;
bool *NINCHDEBUG = NULL;
FILE **NINCHLog = NULL;

bool DidInit = 0;

volatile Nds_data *ninch_nds_data;

#ifdef __cplusplus
  extern "C" {
#endif

DLLIMPORT int AsmPlug_GetID()
{
    return 2;
}

DLLIMPORT char *AsmPlug_GetIDStr()
{
    return (char*)"NINCH";
}

DLLIMPORT int AsmPlug_GetPriority()
{
    return ASMPLUG_PRI_NORMAL;
}

DLLIMPORT char *AsmPlug_GetStatus(int *error_code)
{
	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    return 0;
}

DLLIMPORT int AsmPlug_Handle802_11(unsigned char *data, int length)
{
     return 0;
}

DLLIMPORT bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &ninch_nds_data);

	#ifndef NDS
    ResetAsm = config->ResetAsm;
    #endif
	NINCHDEBUG = config->DEBUG;
    NINCHLog = config->Log;
    NINCHCONFIG = config;

    return 1;
}

DLLIMPORT bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&ninch_nds_data);

    return 1;
}

DLLIMPORT volatile Nds_data *AsmPlug_GetNdsData()
{
    return ninch_nds_data;
}

DLLIMPORT void AsmPlug_Reset()
{

}

DLLIMPORT int AsmPlug_GetModeStatus(int mode)//Queries whether or not the specified mode is available in this packet module.
{
    if(mode == MODE_ASM)return 0;
    if(mode != MODE_ASM)return 1;

	return 0;
}

DLLIMPORT int AsmPlug_SwitchMode(int mode)
{
    if(mode == MODE_ASM)return 3;

    return 1;
}

#ifdef __cplusplus
  }
#endif
