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
#define STAGEDL_MENU_DL 3

unsigned char mgc_num[4] = {0x06, 0x01, 0x02, 0x00};

int stage = STAGEDL_MENU_REQ;
bool IsSpot = 0;//Zero if the protocol for the packets being handled are DS Download Station pacekts, 1 if it's Japanese Nintendo Spot packets.(New DLStations in japan)

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;
FILE *loG;

int HandleDL_AssocResponse(unsigned char *data, int length);
int HandleWMB_Data(unsigned char *data, int length);
int HandleWMB_DataAck(unsigned char *data, int length);

int HandleDL_MenuRequest(unsigned char *data, int length);
int HandleDL_MenuDownload(unsigned char *data, int length);

struct DLClient
{
    unsigned char clientID;
    unsigned char mac[6];
    int is_dl;//Is this client really using the DLStation/N Spot protocol, not WMB or some other protocol?
    bool has_data;//Does this entry contain any data?
};

struct WMBHost
{
    unsigned char mac[6];//Any host that sends binary data over WMB, has their MAC put into the WMBHosts array. If any of the DLClients ack the WMB Host, they are kicked from the DLClients list, as they are using WMB, not DLStation/N Spot protocol.
    bool sent_data;
    int data_seq;
    bool has_data;
};

DLClient DLClients[15];
WMBHost WMBHosts[256];
int total_clients = 0;
int total_wmbhosts = 0;

unsigned char host_mac[6];

void AddClient(DLClient client);
int AddWMBHost(WMBHost host);
void RemoveClient(int index);
void RemoveWMBHost(int index);
void RemoveClients();
void RemoveWMBHosts();

#ifdef __cplusplus
  extern "C" {
#endif

DLLIMPORT int AsmPlug_GetID()
{
    return 1;
}

DLLIMPORT char *AsmPlug_GetIDStr()
{
    return (char*)"DLSTATION";
}

DLLIMPORT int AsmPlug_GetPriority()
{
    return ASMPLUG_PRI_NORMAL;
}

DLLIMPORT char *AsmPlug_GetStatus(int *error_code)
{
    
    if(!IsSpot)
    {
        if(stage==STAGEDL_MENU_REQ)
        {
            *error_code = STAGEDL_MENU_REQ;
            return (char*)"02: DS DL Station: Failed to find the Menu Request packet.\n";
        }
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    if(!IsSpot)
    {
        if(stage==STAGEDL_MENU_REQ)return 3;
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
        
        ret = HandleWMB_Data(data, length);
        if(ret)return 0;//This plugin doesn't use WMB data packets, for there actual data - only kicking clients in the DLClients list when they ack/reply to WMB data packets. This plugin must return 0, not 1, for these WMB packets, otherwise this plugin would interfer with the WMB plugin. 
        
        ret = HandleWMB_DataAck(data, length);
        if(ret)return 0;
        
        if(stage==STAGEDL_MENU_REQ)return HandleDL_MenuRequest(data, length);
        if(stage==STAGEDL_MENU_DL)return HandleDL_MenuDownload(data, length);
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
    
    Log = &loG;
    loG = fopen("dlstation_debug.txt","w");
    
    return 1;
}

DLLIMPORT bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&nds_data);
    
    RemoveClients();
    RemoveWMBHosts();
    
    fclose(loG);
    
    return 1;
}

DLLIMPORT volatile Nds_data *AsmPlug_GetNdsData()
{
    return nds_data;
}

DLLIMPORT void AsmPlug_Reset()
{
    IsSpot = 0;
    stage=STAGEDL_MENU_REQ;
    
    memset(DLClients, 0, sizeof(DLClient) * 15);
    memset(WMBHosts, 0, sizeof(WMBHost) * 256);
    total_clients = 0;
    total_wmbhosts = 0;
    
    memset(host_mac,0,6);
}

#ifdef __cplusplus
  }
#endif

void AddClient(DLClient client)
{
    bool found = 0;
    client.has_data = 1;
    
    for(int i=0; i<15; i++)
    {
        if(!DLClients[i].has_data)
        
        if(memcmp(DLClients[i].mac, client.mac, 6))found = 1;
    }
    if(found)return;
    
    if(!DLClients[client.clientID - 1].has_data)
    {
        memcpy(&DLClients[client.clientID - 1], &client, sizeof(DLClient));
        total_clients++;
    }
}

void RemoveClient(int index)
{
    if(index<0 || index>14)return;
    
    memset(&DLClients[index], 0, sizeof(DLClient));
    
    total_clients--;
}

void RemoveClients()
{
    for(int i=0; i<15; i++)
        RemoveClient(i);
}

int AddWMBHost(WMBHost host)
{
    int found = -1;
    int index = 0;
    host.has_data = 1;
    
    for(int i=0; i<256; i++)
    {
        if(!WMBHosts[i].has_data)continue;
        
        if(memcmp(WMBHosts[i].mac, host.mac, 6))found = i;
    }
    if(found>=0)return found;
    
    for(int i=0; i<15; i++)
    {
        if(!WMBHosts[i].has_data)
        {
            memcpy(&WMBHosts[i], &host, sizeof(WMBHost));
            index = i;
            break;
        }
    }
    
    total_wmbhosts++;
    return index;
}

void RemoveWMBHost(int index)
{
    if(index<0 || index>255)return;

    memset(&WMBHosts[index], 0, sizeof(WMBHost));
    
    total_wmbhosts--;
}

void RemoveWMBHosts()
{
    for(int i=0; i<256; i++)
        RemoveWMBHost(i);
}

int HandleDL_AssocResponse(unsigned char *data, int length)
{
    iee80211_framehead2 *fh = (iee80211_framehead2*)data;
    DLClient client;
    memset(&client,0, sizeof(DLClient));
    
    if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 1)))
    {
        client.clientID = data[0x5C];
        memcpy(client.mac, fh->mac1, 6);
        AddClient(client);
        
        return 1;
    }
    else
    {
        
    }
    
    return 0;
}

int HandleWMB_Data(unsigned char *data, int length)
{
    iee80211_framehead2 *dat = (iee80211_framehead2*)data;
    unsigned short size = 0;
    unsigned char pos = 0;
    WMBHost host;
    int host_index = 0;
    
    if(CheckFrame(data, dat->mac3, 0x04, &size, &pos))
    {
        memcpy(&host.mac, dat->mac3, 6);
        host_index = AddWMBHost(host);
        WMBHosts[host_index].sent_data = 1;
        WMBHosts[host_index].data_seq = ReadSeq(&dat->sequence_control);
        
        return 1;
    }
    
    return 0;
}

int HandleWMB_DataAck(unsigned char *data, int length)
{
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    unsigned char *dat = &data[0x18];
    unsigned char mgc1[3] = {0x04, 0x81, 0x09};
    unsigned char mgc2[3] = {0x00, 0x00, 0x00};
    int index = 0;
    bool found = 0;
    
    if(CheckFrameControl(frm, 2, 1))
    {
        if(CheckFlow(frm->mac3, 10))
        {
            if(memcmp(dat, mgc1, 3) && memcmp(dat + 7, mgc2, 3))
            {
                for(int i=0; i<15; i++)
                {
                    if(!DLClients[i].has_data)continue;
                    
                    if(memcmp(frm->mac2, DLClients[i].mac, 6))
                    {
                        index = i;
                        found = 1;
                        break;
                    }
                }
                
                if(found)
                {
                    RemoveClient(index);
                    return 1;
                }
            }
        }
    }
    
    return 0;
}

unsigned char *CheckDLFrame(unsigned char *data, int length, unsigned char type, int *size, unsigned short *seq, unsigned char *clientID)
{
    int sz = 0;
    unsigned short *Seq = NULL;
    unsigned char *dat = &data[0x18];
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    
    if(CheckFrameControl(frm, 2, 1))
    {
        if(CheckFlow(frm->mac3, 0x10))
        {
            if(memcmp(dat, mgc_num, 4))
            {
                dat+=4;
                
                if(dat[1]==type)
                {
                    sz = (int)dat[0];
                    sz *= 2;
                    sz -= 2;//Remove what seems to be the sequence number for the next byte, from the total length.
                    
                    if(type==0x1f)sz+=32;//The sizes need increased for data packets
                    *size = sz;
                    
                    dat+=2;
                    
                    Seq = (unsigned short*)dat;
                    *seq = *Seq;
                    
                    dat+=2;
                    
                    if(type==0x1f)*clientID = dat[sz+2];

                    unsigned int checksum = CalcCRC32(data, length - 4);
                    unsigned int *chk = (unsigned int*)&data[length-4];
                    
                    if(checksum!=*chk)
                    {
                        fprintf(*Log, "CHECKSUM FAIL %d %d\n",checksum,*chk);
                        return NULL;
                    }
                    
                    return dat;
                }
                else
                {
                    fprintf(*Log, "MTYPE FAIL %x wanted %x\n",(int)dat[1],(int)type);
                }
            }
        }
    }
    
    return NULL;
}

int HandleDL_MenuRequest(unsigned char *data, int length)
{
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    unsigned char *dat = &data[0x18];
    char req[256];
    memset(req, 0, 256);
    
    if(CheckFrameControl(frm, 2, 1))
    {
        if(CheckFlow(frm->mac3, 0x10))
        {
            if(dat[0]==0x04 && dat[2]!=0xFF)
            {
                memcpy(req, &dat[2], 9);
                
                if(strstr(req, "menu"))
                {
                    fprintf(*Log ,"DLSTATION: FOUND DL REQ %s\n", req);
                    stage = STAGEDL_MENU_DL;
                }
            }
        }
    }
    
    return 0;
}

int HandleDL_MenuDownload(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    int size = 0;
    unsigned short seq = 0;
    unsigned char clientID = 0;
    
    dat = CheckDLFrame(data, length, 0x1e, &size, &seq, &clientID);
    if(dat)
    {
        fprintf(*Log, "FOUND MENU PKT SEQ %d SZ %d CID %d\n",(int)seq, size, (int)clientID);
    }

    return 0;
}
