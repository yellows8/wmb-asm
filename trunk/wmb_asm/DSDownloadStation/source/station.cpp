#define DLLMAIN
#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#include "..\..\SDK\include\wmb_asm_sdk_plugin.h"

#undef DLLIMPORT
#define DLLIMPORT __declspec (dllexport)

#define STAGE_MENU_REQ 1

int stage = STAGE_MENU_REQ;

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;

bool DidInit = 0;

bool Handle_MenuRequest(unsigned char *data, int length);

#ifdef __cplusplus
  extern "C" {
#endif



DLLIMPORT char *GetStatus(int *error_code)
{
    if(stage==STAGE_MENU_REQ)
    {
        *error_code = STAGE_MENU_REQ;
        return (char*)"02: Failed to find the Menu Request packet.\n";
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int QueryFailure()
{
    return 0;
}

DLLIMPORT bool Handle802_11(unsigned char *data, int length)
{
     if(stage==STAGE_MENU_REQ)return Handle_MenuRequest(data, length);

     return 0;
}

DLLIMPORT void Reset(sAsmSDK_Config *config)
{
    if(!DidInit)
    {
        ResetAsm = config->ResetAsm;
        nds_data = config->nds_data;
        DEBUG = config->DEBUG;
        Log = config->Log;
        CONFIG = config;
        DidInit = 1;
    }

    stage=STAGE_MENU_REQ;
    /*last_seq=0;

    memset(host_mac,0,6);
	memset(client_mac,0,6);*/
}

#ifdef __cplusplus
  }
#endif

bool Handle_MenuRequest(unsigned char *data, int length)
{
    return 0;
}
