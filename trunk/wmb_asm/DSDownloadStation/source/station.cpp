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

#include <lzo\lzo1.h>

#include "..\..\SDK\include\wmb_asm_sdk_plugin.h"

#undef DLLIMPORT
#define DLLIMPORT __declspec (dllexport)

#define STAGEDL_ASSOC_RESPONSE 1
#define STAGEDL_MENU_REQ 2

int stage = STAGEDL_ASSOC_RESPONSE;
bool IsSpot = 0;//Zero if the protocol for the packets being handled are DS Download Station pacekts, 1 if it's Japanese Nintendo Spot packets.(New DLStations in japan)

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;

int HandleDL_AssocResponse(unsigned char *data, int length);
int HandleDL_MenuRequest(unsigned char *data, int length);

struct DLClient
{
    unsigned char clientID;
    unsigned char mac[6];
};
DLClient DLClients[15];
int total_clients = 0;

#ifdef __cplusplus
  extern "C" {
#endif

DLLIMPORT int AsmPlug_GetID()
{
    return 1;
}

DLLIMPORT char *AsmPlug_GetIDStr()
{
    return (char*)"DSDLSTATN";
}

DLLIMPORT int AsmPlug_GetPriority()
{
    return ASMPLUG_PRI_NORMAL;
}

DLLIMPORT char *AsmPlug_GetStatus(int *error_code)
{
    
    if(!IsSpot)
    {
        if(stage==STAGEDL_ASSOC_RESPONSE)
        {
            *error_code = STAGEDL_ASSOC_RESPONSE;
            return (char*)"02: DS DL Station: Failed to find the Association Response packet.\n";
        }
        
        if(stage==STAGEDL_MENU_REQ)
        {
            *error_code = STAGEDL_MENU_REQ;
            return (char*)"03: DS DL Station: Failed to find the Menu Request packet.\n";
        }
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    if(!IsSpot)
    {
        if(stage==STAGEDL_ASSOC_RESPONSE)return 3;
        if(stage==STAGEDL_MENU_REQ)return 1;
    }
        
    return 0;
}

DLLIMPORT int AsmPlug_Handle802_11(unsigned char *data, int length)
{
     
     if(!IsSpot)
     {
        int ret = 0;
        
        ret = HandleDL_AssocResponse(data, length);
        
        if(ret)return ret;
        
        if(stage==STAGEDL_MENU_REQ)return HandleDL_MenuRequest(data, length);
     }
     
     return 0;
}

DLLIMPORT bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &nds_data);
    
    ResetAsm = config->ResetAsm;
    DEBUG = config->DEBUG;
    Log = config->Log;
    CONFIG = config;
    
    return 1;
}

DLLIMPORT bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&nds_data);
    
    return 1;
}

DLLIMPORT volatile Nds_data *AsmPlug_GetNdsData()
{
    return nds_data;
}

DLLIMPORT void AsmPlug_Reset()
{

    IsSpot = 0;
    stage=STAGEDL_ASSOC_RESPONSE;
    
    memset(DLClients, 0, sizeof(DLClient) * 15);
    total_clients = 0;
    
    /*
    last_seq=0;
    memset(host_mac,0,6);
    */
}

#ifdef __cplusplus
  }
#endif

int HandleDL_AssocResponse(unsigned char *data, int length)
{
    iee80211_framehead2 *fh = (iee80211_framehead2*)data;

    if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 1)))
    {
        DLClients[total_clients].clientID = data[0x5C];
        memcpy(DLClients[total_clients].mac,fh->mac1, 6);
        total_clients++;
        
        if(stage==STAGEDL_ASSOC_RESPONSE)stage = STAGEDL_MENU_REQ;
        
        //printf("FOUND ASSOC RESPONSE!\n");
        
        return 1;
    }
    else
    {
        
    }
    
    return 0;
}

int HandleDL_MenuRequest(unsigned char *data, int length)
{
    return 0;
}
