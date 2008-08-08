#define DLLMAIN
#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#include "..\..\SDK\include\wmb_asm_sdk_plugin.h"

#undef DLLIMPORT
#define DLLIMPORT __declspec (dllexport)

#define STAGE_ASSOC_RESPONSE 1
#define STAGE_MENU_REQ 2

int stage = STAGE_ASSOC_RESPONSE;

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;
FILE *DLog = NULL;

bool DidInit = 0;

bool Handle_AssocResponse(unsigned char *data, int length);
bool Handle_MenuRequest(unsigned char *data, int length);

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



DLLIMPORT char *GetStatus(int *error_code)
{
    if(stage==STAGE_ASSOC_RESPONSE)
    {
        *error_code = STAGE_ASSOC_RESPONSE;
        return (char*)"02: Failed to find the Association Response packet.\n";
    }
    
    if(stage==STAGE_MENU_REQ)
    {
        *error_code = STAGE_MENU_REQ;
        return (char*)"03: Failed to find the Menu Request packet.\n";
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
     bool ret = 0;
     
     ret = Handle_AssocResponse(data, length);
     
     if(ret)return ret;
     
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
        
        DLog = fopen("DLog.txt","w");
    }

    stage=STAGE_ASSOC_RESPONSE;
    
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

bool Handle_AssocResponse(unsigned char *data, int length)
{
    iee80211_framehead2 *fh = (iee80211_framehead2*)data;

    if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 1)))
    {
        DLClients[total_clients].clientID = data[0x5C];
        memcpy(DLClients[total_clients].mac,fh->mac1, 6);
        total_clients++;
        
        if(stage==STAGE_ASSOC_RESPONSE)stage = STAGE_MENU_REQ;
        
        printf("FOUND ASSOC RESPONSE!\n");
        
        return 1;
    }
    else
    {
        if(DLog!=NULL)
        {
            fprintf(DLog,"Fail %d %d num %d\n", FH_FC_TYPE(fh->frame_control), FH_FC_SUBTYPE(fh->frame_control));
        }
    }
    
    return 0;
}

bool Handle_MenuRequest(unsigned char *data, int length)
{
    return 0;
}
