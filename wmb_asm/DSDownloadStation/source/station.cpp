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

#ifndef NDS
	#include <lzo\lzoconf.h>
	#include <lzo\lzo1x.h>
#endif

#ifdef NDS
	#include <minilzo.h>
#endif

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
	#define AsmPlug_GetID DLSTATION_AsmPlug_GetID
	#define AsmPlug_GetIDStr DLSTATION_AsmPlug_GetIDStr
	#define AsmPlug_GetPriority DLSTATION_AsmPlug_GetPriority
	#define AsmPlug_GetStatus DLSTATION_AsmPlug_GetStatus
	#define AsmPlug_QueryFailure DLSTATION_AsmPlug_QueryFailure
	#define AsmPlug_Handle802_11 DLSTATION_AsmPlug_Handle802_11
	#define AsmPlug_Init DLSTATION_AsmPlug_Init
	#define AsmPlug_DeInit DLSTATION_AsmPlug_DeInit
	#define AsmPlug_GetNdsData DLSTATION_AsmPlug_GetNdsData
	#define AsmPlug_Reset DLSTATION_AsmPlug_Reset
	#define AsmPlug_GetModeStatus DLSTATION_AsmPlug_GetModeStatus
	#define AsmPlug_SwitchMode DLSTATION_AsmPlug_SwitchMode
#endif

#define STAGEDL_ASSOC_RESPONSE 1
#define STAGEDL_MENU_REQ 2
#define STAGEDL_MENU_DL 3
#define STAGEDL_DL_REQ 4
#define STAGEDL_HEADER 5
#define STAGEDL_DATA 6

unsigned char mgc_num[4] = {0x06, 0x01, 0x02, 0x00};

int dlstation_stage = STAGEDL_MENU_REQ;
bool IsSpot = 0;//Zero if the protocol for the packets being handled are DS Download Station pacekts, 1 if it's Japanese Nintendo Spot packets.(New DLStations in japan)

sAsmSDK_Config *DLSTATIONCONFIG = NULL;
bool *DLSTATIONDEBUG = NULL;
FILE **DLSTATIONLog = NULL;
FILE *DLSTATIONloG;

volatile Nds_data *dlstation_nds_data;

int HandleDL_AssocResponse(unsigned char *data, int length);
int HandleDL_Deauth(unsigned char *data, int length);
int HandleWMB_Data(unsigned char *data, int length);
int HandleWMB_DataAck(unsigned char *data, int length);

int HandleSP_MenuDownload(unsigned char *data, int length);

int HandleDL_PacketRequest(unsigned char *data, int length);
int HandleDL_MenuDownload(unsigned char *data, int length);
int HandleDL_Header(unsigned char *data, int length);
int HandleDL_Data(unsigned char *data, int length);

struct DLClient
{
    unsigned char clientID;
    unsigned char mac[6];
    int gameID;
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

struct MenuItem
{
    unsigned char icon[512];
    unsigned short palette[16];
    char title[0x80];
    char subtitle[0x80];
    char controls[0x402];
    char id[8];
    unsigned char zeroes[2];
} __attribute__((__packed__));

DLClient DLClients[15];
WMBHost WMBHosts[256];

unsigned char dlstation_host_mac[6];

char gameIDs[15][8];

void AddClient(DLClient client);
int AddWMBHost(WMBHost host);
int GetClient(unsigned char *mac);
int GetWMBHost(unsigned char *mac);
void RemoveClient(int index);
void RemoveWMBHost(int index);
void RemoveClients();
void RemoveWMBHosts();

int LookupGameID(char *id);

unsigned char *menu_data = NULL;
int *found_menu = NULL;
int *menu_sizes = NULL;
bool assembled_menu = 0;//Set to true when the menu has been assembled.
int high_pos = 0;
int high_seq = 0;//Highest data packet seq we have seen
int highest_contigous_seq = 0;//Highest contigous, the last seq we have seen without any missed packets before it.

unsigned int data_size = 0;//Size of the LZO compressed data, excluding the LZO header.
unsigned int de_data_size = 0;//Size of the decompressed data.

unsigned char lzo_mgc_num[8] = {0x4C, 0x5A, 0x4F, 0x6E, 0x00, 0x2F, 0xF1, 0x71};

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
        if(dlstation_stage==STAGEDL_MENU_REQ)
        {
            *error_code = STAGEDL_MENU_REQ;
            return (char*)"02: DS DL Station: Failed to find the Menu Request packet.\n";
        }

        if(dlstation_stage==STAGEDL_MENU_DL)
        {
            *error_code = STAGEDL_MENU_DL;
            return (char*)"03: DS DL Station: Failed to find all of the Menu packets.\n";
        }

        if(dlstation_stage==STAGEDL_DATA)
        {
            *error_code = STAGEDL_DATA;
            return (char*)"04: DS DL Station: Failed to find all of the data packets.\n";
        }
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    if(!IsSpot)
    {
        if(dlstation_stage==STAGEDL_MENU_REQ)return 3;
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

        ret = HandleDL_Deauth(data, length);
        if(ret)return ret;

        ret = HandleWMB_Data(data, length);
        if(ret)return 0;//This plugin doesn't use WMB data packets, for there actual data - only kicking clients in the DLClients list when they ack/reply to WMB data packets. This plugin must return 0, not 1, for these WMB packets, otherwise this plugin would interfer with the WMB plugin.

        ret = HandleWMB_DataAck(data, length);
        if(ret)return 0;

        ret = HandleDL_PacketRequest(data, length);
        if(ret)return 1;

        ret = HandleSP_MenuDownload(data, length);
        if(ret)return 1;

        if(dlstation_stage==STAGEDL_MENU_DL)return HandleDL_MenuDownload(data, length);
        if(dlstation_stage==STAGEDL_HEADER)return HandleDL_Header(data, length);
        if(dlstation_stage==STAGEDL_DATA)return HandleDL_Data(data, length);
     }

     return 0;
}

DLLIMPORT bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &dlstation_nds_data);
    dlstation_nds_data->use_advert = 1;

	#ifndef NDS
    ResetAsm = config->ResetAsm;
    #endif
	DLSTATIONDEBUG = config->DEBUG;
    DLSTATIONLog = config->Log;
    DLSTATIONCONFIG = config;

    if(lzo_init()!=LZO_E_OK)
    {
        printf("Failed to initialize LZO library!\n");
        return 0;
    }

    menu_data = (unsigned char*)malloc(30 * 1000);//1000 because there doesn't seem to be any total-menu packets, or total packet length dat sent to the clients in the DLStation protocol - only the seq with ffff tells when the last menu packet was sent.
    found_menu = (int*)malloc(sizeof(int) * 1000);
    menu_sizes = (int*)malloc(sizeof(int) * 1000);

    if(menu_data==NULL || found_menu==NULL || menu_sizes==NULL)
    {
        printf("Failed to allocate memory!\n");
        return 0;
    }

    DLSTATIONLog = &DLSTATIONloG;
    if(*DLSTATIONDEBUG)
    DLSTATIONloG = fopen("dlstation_debug.txt","w");

    return 1;
}

DLLIMPORT bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&dlstation_nds_data);

    RemoveClients();
    RemoveWMBHosts();

    free(menu_data);
    free(found_menu);

    if(*DLSTATIONDEBUG)
    fclose(DLSTATIONloG);

    return 1;
}

DLLIMPORT volatile Nds_data *AsmPlug_GetNdsData()
{
    return dlstation_nds_data;
}

DLLIMPORT void AsmPlug_Reset()
{
    IsSpot = 0;
    dlstation_stage=STAGEDL_MENU_REQ;

    RemoveClients();
    RemoveWMBHosts();

    memset(menu_data, 0, 30 * 1000);
    memset(found_menu, 0, sizeof(int) * 1000);
    memset(menu_sizes, 0, sizeof(int) * 1000);
    assembled_menu = 0;

    memset(gameIDs, 0, 15 * 8);

    memset(dlstation_host_mac,0,6);
}

DLLIMPORT int AsmPlug_GetModeStatus(int mode)//Queries whether or not the specified mode is available in this packet module.
{
    if(mode == MODE_ASM)return 1;
    if(mode != MODE_ASM)return 0;

	return 0;
}

DLLIMPORT int AsmPlug_SwitchMode(int mode)
{
    if(mode != MODE_ASM)return 3;

    return 1;
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
        if(DLClients[i].has_data)
        {
            if(memcmp(DLClients[i].mac, client.mac, 6)==0)found = 1;
        }
    }
    if(found)return;

    if(!DLClients[client.clientID - 1].has_data)
    {
        memcpy(&DLClients[client.clientID - 1], &client, sizeof(DLClient));
    }
}

void RemoveClient(int index)
{
    if(index<0 || index>14)return;

    memset(&DLClients[index], 0, sizeof(DLClient));
    DLClients[index].gameID = -1;
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

        if(memcmp(WMBHosts[i].mac, host.mac, 6)==0)found = i;
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

    return index;
}

int GetClient(unsigned char *mac)
{
    int index = -1;

    for(int i=0; i<16; i++)
    {
        if(!DLClients[i].has_data)continue;

        if(memcmp(DLClients[i].mac, mac, 6)==0)
        {
            index = i;
            break;
        }
    }

    return index;
}

int GetWMBHost(unsigned char *mac)
{
    int index = -1;

    for(int i=0; i<15; i++)
    {
        if(!WMBHosts[i].has_data)continue;

        if(memcmp(WMBHosts[i].mac, mac, 6)==0)
        {
            index = i;
            break;
        }
    }

    return index;
}

void RemoveWMBHost(int index)
{
    if(index<0 || index>255)return;

    memset(&WMBHosts[index], 0, sizeof(WMBHost));
}

void RemoveWMBHosts()
{
    for(int i=0; i<256; i++)
        RemoveWMBHost(i);
}

int LookupGameID(char *id)
{
    int index = -1;

    for(int i=0; i<16; i++)
    {
        if(memcmp(id, gameIDs[i], 8)==0)
        {
            index = i;
            break;
        }
    }

    if(index != -1)
    {
        if(dlstation_nds_data->handledIDs[index])return -1;//We don't need to assemle the same demo twice.
    }

    return index;
}

int HandleDL_AssocResponse(unsigned char *data, int length)
{
    iee80211_framehead2 *fh = (iee80211_framehead2*)data;
    DLClient client;
    memset(&client,0, sizeof(DLClient));

    if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 1)))
    {
        client.clientID = data[0x1C];
        memcpy(client.mac, fh->mac1, 6);
        AddClient(client);

        return 1;
    }
    else
    {

    }

    return 0;
}

int HandleDL_Deauth(unsigned char *data, int length)
{
    iee80211_framehead2 *fh = (iee80211_framehead2*)data;
    int index = 0;

    if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 12)))
    {
        index = GetClient(fh->mac2);
        if(index != -1)
        {
            RemoveClient(index);

        }

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
                    if(*DLSTATIONDEBUG)
                    {
                        fprintf(*DLSTATIONLog, "KICKING CLIENT FOR ACKNOWLEDGING WMB DATA, CLIENT ID %d MAC %x:%x:%x:%x:%x:%x NUM %d\n", index, frm->mac2[0], frm->mac2[1], frm->mac2[2], frm->mac2[3], frm->mac2[4], frm->mac2[5], GetPacketNum());
                        fflush(*DLSTATIONLog);
                    }
                    RemoveClient(index);
                    return 1;
                }
            }
        }
    }

    return 0;
}

unsigned char *CheckDLFrame(unsigned char *data, int length, unsigned char type, int *size, unsigned short *seq, unsigned short *clientID)
{
    int sz = 0;
    unsigned short *Seq = NULL;
    unsigned char *dat = &data[0x18];
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    unsigned char fcs_mgc[2] = {0x02, 0x00};
    unsigned short *id;

    if(CheckFrameControl(frm, 2, 2))
    {
        if(CheckFlow(frm->mac1, 0x00))
        {
            if(memcmp(dat, mgc_num, 4)==0)
            {
                dat+=4;

                if(dat[1]==type)
                {
                    sz = (int)dat[0];
                    sz *= 2;
                    sz -= 2;//Remove what seems to be the sequence number for the next data packet, from the total length.

                    *size = sz;

                    dat+=2;

                    Seq = (unsigned short*)dat;
                    *seq = *Seq;

                    dat+=2;

                    if(type==0x1f)
                    {
                        id = (unsigned short*)&dat[sz+2];
                        *clientID = *id;
                    }

                    if(memcmp(&data[length-6], fcs_mgc, 2)==0)
                    {
                        unsigned int checksum = CalcCRC32(data, length - 4);
                        unsigned int *chk = (unsigned int*)&data[length-4];

                        if(checksum!=*chk)
                        {
                            if(*DLSTATIONDEBUG)
                            {
                                fprintf(*DLSTATIONLog, "CHECKSUM FAIL %d %d NUM %d\n",checksum,*chk, GetPacketNum());
                                fflush(*DLSTATIONLog);
                            }
                            return NULL;
                        }
                    }

                    return dat;
                }
            }
        }
    }

    return NULL;
}

int HandleDL_PacketRequest(unsigned char *data, int length)
{
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    unsigned char *dat = &data[0x18];
    char *req = (char*)malloc(256);
    int index = 0;
    memset(req, 0, 256);

    if(CheckFrameControl(frm, 2, 1))
    {
        if(CheckFlow(frm->mac3, 0x10))
        {
            if(dat[0]==0x04 && dat[2]!=0xFF && dat[2]!=0x09 && dat[2]!=0x08 && dat[2]!=0x07 && dat[2]!=0x00 && dat[1]!=0x81)//Check if this is a packet request packet, and make sure this is not a WMB ack packet.
            {
                memcpy(req, &dat[2], 8);

                index = GetClient(frm->mac2);
                if(index != -1)
                {
                    DLClients[index].is_dl = 1;
                }
                else
                {
                    free(req);
                    return 3;//Do not continue; We don't know what this client's clientID is.
                }

                if(strstr(req, "menu"))
                {
                    if(*DLSTATIONDEBUG)
                    {
                        fprintf(*DLSTATIONLog, "DLSTATION: FOUND MENU REQ %s NUM %d\n", req, GetPacketNum());
                        fflush(*DLSTATIONLog);
                    }

                    if(dlstation_stage==STAGEDL_MENU_REQ)
                    {
                        if(*DLSTATIONDEBUG)
                        {
                            fprintf(*DLSTATIONLog, "ENTERING MENU PACKET STAGE\n");
                            fflush(*DLSTATIONLog);
                        }

                        dlstation_stage = STAGEDL_MENU_DL;
                        dlstation_nds_data->multipleIDs = 1;
                    }
                }
                else
                {
                    if(*DLSTATIONDEBUG)
                    {
                    fprintf(*DLSTATIONLog, "DLSTATION: FOUND DL REQ %s NUM %d\n", req, GetPacketNum());
                    fflush(*DLSTATIONLog);
                    }

                    if(assembled_menu)
                    {
                        DLClients[index].gameID = LookupGameID(req);
                        if(*DLSTATIONDEBUG)
                        {
                            fprintf(*DLSTATIONLog, "DLSTATION: CLIENTID %d GAMEID SET TO %d MAC %x:%x:%x:%x:%x:%x NUM %d\n", index, DLClients[index].gameID, frm->mac2[0], frm->mac2[1], frm->mac2[2], frm->mac2[3], frm->mac2[4], frm->mac2[5], GetPacketNum());
                            fflush(*DLSTATIONLog);
                        }

                        if(dlstation_stage==STAGEDL_DL_REQ && DLClients[index].gameID != -1)
                        {
                            MenuItem *item = (MenuItem*)(((int)menu_data + 4) + (sizeof(MenuItem) * DLClients[index].gameID));
                            printf("DLSTATION: Found download request for %s. Download title: %s\n", req, item->title);

                            if(*DLSTATIONDEBUG)
                            {
                                fprintf(*DLSTATIONLog, "FOUND DL REQUEST CLIENTID %d GAMEINDEX %d GAMEID %s\n", index, DLClients[index].gameID, req);
                                fprintf(*DLSTATIONLog, "ENTERING HEADER STAGE\n");
                                fflush(*DLSTATIONLog);
                            }

                            dlstation_nds_data->gameID = (volatile unsigned char)DLClients[index].gameID;
                            dlstation_nds_data->clientID = index;
                            dlstation_stage = STAGEDL_HEADER;
                        }
                    }
                }

                free(req);

                return 1;
            }
        }
    }

    free(req);

    return 0;
}

//This function attempts to dump Nintendo Spot menu packets.
int HandleSP_MenuDownload(unsigned char *data, int length)
{
    iee80211_framehead2 *frm = (iee80211_framehead2*)data;
    unsigned char *dat;
    unsigned char sp_mgc_num[4] = {0x7e, 0x01, 0x02, 0x00};
    int size = 0;
    unsigned short *seq = 0;
    unsigned char *buffer;
    //int lzo_ret = 0;
    //lzo_uint out_len = 0;
    FILE *dump;
    char str[256];
    memset(str, 0, 256);

    if(CheckFrameControl(frm, 2, 2))
    {
        if(CheckFlow(frm->mac1, 0x00))
        {
            dat = &data[0x18];
            if(memcmp(dat, sp_mgc_num, 4)!=0)return 0;
            dat+=4;

            size = (int)*dat;
            size *= 2;
            //size-=32;
            size++;

            if(size<=0)return 3;

            dat+=1;
            seq = (unsigned short*)dat;
            dat+=2;

            buffer = (unsigned char*)malloc(100000);
            memset(buffer, 0, 100000);

            if(*DLSTATIONDEBUG)
            {
                fprintf(*DLSTATIONLog, "Nintendo Spot: Copying menu seq %d sz %d Num %d\n", (int)*seq, size, GetPacketNum());
                fflush(*DLSTATIONLog);
            }

            memcpy(buffer, dat, size);
            //lzo_ret = lzo1x_decompress(dat,size,buffer,&out_len,NULL);
            //if(lzo_ret!=LZO_E_OK)printf("Menu decompression failed: error %d\n",lzo_ret);

            sprintf(str, "SpotMenu\\menu_%d.bin",(int)*seq);
            dump = fopen(str, "wb");
            fwrite(buffer, 1, size, dump);
            fclose(dump);

            free(buffer);
        }
    }

    return 0;
}

int HandleDL_MenuDownload(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    int size = 0;
    unsigned short seq = 0;
    unsigned short clientID = 0;
    int cur_pos = 0;
    unsigned char *buffer = NULL;
    lzo_uint out_len = 0;
    int lzo_ret = 0;
    unsigned int lzon_size = 0;
    unsigned int *lz_size = NULL;
    int item_sizes = 0;
    MenuItem *item = NULL;
    int gameID = 0;
    char *str = NULL;

    if(assembled_menu)return 3;

    dat = CheckDLFrame(data, length, 0x1e, &size, &seq, &clientID);
    if(dat)
    {
        if(seq!=0xffff)
        {
            if(!found_menu[(int)seq])
            {
                if(seq!=0)
                {
                    for(int i=0; i<(int)seq; i++)
                    {
                        cur_pos+=menu_sizes[i];
                        if(menu_sizes[i]==0)
                            cur_pos+=0x10;
                    }
                }

                found_menu[(int)seq] = 1;
                menu_sizes[(int)seq] = size;

                memcpy(&menu_data[cur_pos], dat, (size_t)size);

                if(*DLSTATIONDEBUG)
                {
                    fprintf(*DLSTATIONLog, "FOUND MENU PKT SEQ %d SZ %d CID %d CURPOS %d NUM %d\n",(int)seq, size, (int)clientID, cur_pos, GetPacketNum());
                    fflush(*DLSTATIONLog);
                }
            }
        }
        else
        {
            if(!found_menu[0])return 3;
            cur_pos += menu_sizes[0];
            for(int i=1; i<1000; i++)//This is supposed to find the end of the menu data in the menu_data buffer, but this might be broken, I'm not sure. There's a bunch of zeroes at the end of my dump.
            {
                if((!found_menu[i-1] && found_menu[i]))return 3;

                if(found_menu[i])cur_pos += menu_sizes[i];
                if(!found_menu[i])cur_pos += 0x10;

                if(!found_menu[i+1] && found_menu[i-1] && found_menu[i])break;
                if(!found_menu[i] && found_menu[i-1])break;
            }

            buffer = (unsigned char*)malloc(cur_pos * 10);
            memset(buffer, 0, cur_pos);

            if(memcmp(menu_data, lzo_mgc_num, 8)!=0)return 3;
            lz_size = (unsigned int*)&menu_data[0x0C];
            lzon_size = *lz_size;
            ConvertEndian(&lzon_size, &lzon_size, sizeof(unsigned int));

            lzo_ret = lzo1x_decompress(menu_data + 0x10,lzon_size,buffer,&out_len,NULL);
            if(lzo_ret!=LZO_E_OK)
            {
                printf("Menu decompression failed: error %d\n", lzo_ret);

                if(*DLSTATIONDEBUG)
                {
                    fprintf(*DLSTATIONLog, "Menu decompression failed: error %d\n", lzo_ret);
                    fflush(*DLSTATIONLog);
                }
            }

            memset(menu_data, 0, cur_pos);
            memcpy(menu_data, buffer, out_len);
            free(buffer);
            cur_pos = out_len;

            item = (MenuItem*)((int)menu_data + 4);

            if(*DLSTATIONDEBUG)
            {
                fprintf(*DLSTATIONLog, "PROCESSING MENU...\n");
                fflush(*DLSTATIONLog);
            }

            item_sizes = (int)out_len - 4;

            str = (char*)malloc(256);
            memset(str, 0, 256);

            dlstation_nds_data->FoundGameIDs = 1;

            while(item_sizes>0)
            {
                strcpy(&gameIDs[gameID][0], item->id);
                strcpy(str, &gameIDs[gameID][0]);
                if(*DLSTATIONDEBUG)
                {
                    fprintf(*DLSTATIONLog, "PROCESSING MENU GAMEINDEX %d, ID %s TITLE %s SUBTITLE %s...\n", gameID, str, item->title, item->subtitle);
                    fflush(*DLSTATIONLog);
                }
                memset(str, 0, 256);
                memcpy((void*)&dlstation_nds_data->adverts[gameID].icon, (void*)&item->icon, 512);
                memcpy((void*)&dlstation_nds_data->adverts[gameID].icon_pallete, (void*)&item->palette, sizeof(unsigned short) * 16);

                for(int i=0; i<48; i++)
                {
                    if(item->title[i]==0)break;

                    dlstation_nds_data->adverts[gameID].game_name[i] = (unsigned short)item->title[i];
                }

                for(int i=0; i<96; i++)
                {
                    if(item->subtitle[i]==0)break;

                    dlstation_nds_data->adverts[gameID].game_description[i] = (unsigned short)item->subtitle[i];
                }

                dlstation_nds_data->foundIDs[gameID] = 1;

                item = (MenuItem*)((int)item + (int)sizeof(MenuItem));
                item_sizes-=sizeof(MenuItem);
                gameID++;
            }

            free(str);

            dlstation_stage = STAGEDL_DL_REQ;
            if(*DLSTATIONDEBUG)
            {
                fprintf(*DLSTATIONLog, "FOUND ALL MENU PACKETS!\n");
                fprintf(*DLSTATIONLog, "ENTERING DL REQUEST STAGE!\n");
                fflush(*DLSTATIONLog);
            }

            assembled_menu = 1;
        }

        return 1;
    }

    return 0;
}

//This function returns 1 if the clientID, from the header/data packets, has the same gameID, as in the value in the gameID variable.
bool LookupClientsGameID(unsigned short clientID, unsigned char gameID)
{

    for(int i=0; i<6; i++)
    {

        if(clientID > 8 * (i+1))continue;

        if(clientID != ((8 * i) + 6))//If this isn't the clientID for two IDs combined...
        {
            if(clientID == ((8 * i) + 2))
            {
                if(DLClients[(int)clientID - 2].gameID == gameID)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }

            if(clientID == ((8 * i) + 4))
            {
                if(DLClients[(int)clientID - 2].gameID == gameID)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }

            if(clientID == ((8 * i) + 8))
            {
                if(DLClients[(int)clientID - 5].gameID == gameID)
                {
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }
        else
        {

                if(DLClients[((2 * i))].gameID == gameID)
                {
                    return 1;
                }

                if(DLClients[((2 * i) + 1)].gameID == gameID)
                {
                    return 1;
                }
        }
    }

    return 0;
}

int HandleDL_Header(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    int size = 0;
    unsigned short seq = 0;
    unsigned short clientID = 0;

    dat = CheckDLFrame(data, length, 0x1f, &size, &seq, &clientID);
    if(dat)
    {
        if(seq!=0)return 3;//This is not the LZO header packet, ignore it.

        if(!LookupClientsGameID(clientID, dlstation_nds_data->gameID))
            return 3;

        if(*DLSTATIONDEBUG)
        {
            fprintf(*DLSTATIONLog, "FOUND HEADER SZ %d CID %d NUM %d\n",size, (int)clientID, GetPacketNum());
            fflush(*DLSTATIONLog);
        }

        if(memcmp(dat, lzo_mgc_num, 8)!=0)return 3;
        dat+=8;

        memcpy(&de_data_size, dat, sizeof(unsigned int));
        memcpy(&data_size, &dat[0x04], sizeof(unsigned int));
        ConvertEndian(&de_data_size, &de_data_size, sizeof(unsigned int));
        ConvertEndian(&data_size, &data_size, sizeof(unsigned int));
        if(*DLSTATIONDEBUG)
        {
            fprintf(*DLSTATIONLog, "LZO   COMPRESSED DATA SIZE %d\n", (int)data_size);
            fprintf(*DLSTATIONLog, "LZO DECOMPRESSED DATA SIZE %d\n", (int)de_data_size);
            fflush(*DLSTATIONLog);
        }

        dlstation_nds_data->saved_data = (unsigned char*)malloc(data_size);
        dlstation_nds_data->data_sizes = (int*)malloc(sizeof(int) * 32440);
        if(dlstation_nds_data->saved_data==NULL || dlstation_nds_data->data_sizes==NULL)
        {
            printf("ERROR: Failed to allocate memory.\n");
            return 5;//Fatal error. However, the Wmb Asm Module doesn't handle fatal errors from plugins returning this error code, yet.
        }
        memset((void*)dlstation_nds_data->saved_data, 0, data_size);
        memset((void*)dlstation_nds_data->data_sizes, 0, sizeof(int) * 32440);

        dat-=8;

        length-=50;
        memcpy((void*)&dlstation_nds_data->saved_data[0], dat, (size_t)length);
        dlstation_nds_data->data_sizes[0] = length;
        dlstation_nds_data->pkt_size = length;
        high_pos = length;

        dlstation_stage = STAGEDL_DATA;

        return 1;
    }

    return 0;
}

int HandleDL_Data(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    int size = 0;
    unsigned short seq = 0;
    unsigned short clientID = 0;
    unsigned char *buffer = NULL;
    int temp = dlstation_nds_data->data_sizes[0];
    int end_temp = 0;
    int lzo_ret = 0;
    lzo_uint out_len = 0;

    dat = CheckDLFrame(data, length, 0x1f, &size, &seq, &clientID);
    if(dat)
    {
        if(seq==0)return 3;//This is the LZO header packet, ignore it. Or, this is the end-of-data packet, ignore it to prevent crashes.

        if(!LookupClientsGameID(clientID, dlstation_nds_data->gameID))
            return 3;

        if(seq!=0xFFFF)
        {

            for(int i=1; i<(int)seq; i++)
            {

                temp+=dlstation_nds_data->data_sizes[i+1];

                if(dlstation_nds_data->data_sizes[i+1]==0)temp+=dlstation_nds_data->pkt_size;
            }

            if(!dlstation_nds_data->data_sizes[seq])
            {
                length-=50;

                if(*DLSTATIONDEBUG)
                {
                    fprintf(*DLSTATIONLog, "FOUND DATA PKT SEQ %d SZ %d CID %d TEMP %d ENDTEMP %d NUM %d\n",(int)seq, size, (int)clientID, temp, end_temp, GetPacketNum());
                    fflush(*DLSTATIONLog);
                }
                memcpy((void*)&dlstation_nds_data->saved_data[temp], dat, (size_t)length);
                dlstation_nds_data->data_sizes[(int)seq] = length;
            }

            temp+=dlstation_nds_data->data_sizes[(int)seq];

            if(temp>high_pos)high_pos = temp;
            if((int)seq>high_seq)high_seq = (int)seq;
            if((int)seq>highest_contigous_seq)
            {
                bool found = 0;

                for(int i=1; i<(int)seq; i++)
                {
                    if(!dlstation_nds_data->data_sizes[i])found = 1;
                }

                if(!found)highest_contigous_seq = (int)seq;
            }

        }
        else
        {
            if(high_seq==highest_contigous_seq)
            end_temp = high_pos;
        }

        if(end_temp > 0)
        {
            buffer = (unsigned char*)dlstation_nds_data->saved_data;
            data_size = end_temp;

            if(*DLSTATIONDEBUG)
            {
                fprintf(*DLSTATIONLog, "FOUND ALL DATA PACKETS!\n");
                fprintf(*DLSTATIONLog, "DECOMPRESSING DATA DECOM SIZE %d!\n", data_size);
                fflush(*DLSTATIONLog);
            }

            dlstation_nds_data->saved_data = (unsigned char*)malloc(de_data_size);
            if(dlstation_nds_data->saved_data==NULL)return 3;
            memset((void*)dlstation_nds_data->saved_data, 0, de_data_size);

            lzo_ret = lzo1x_decompress((unsigned char*)buffer + 0x10, data_size - 0x10, (unsigned char*)dlstation_nds_data->saved_data, &out_len, NULL);
            if(lzo_ret!=LZO_E_OK)
            {
                printf("DS DLSTATION: Data decompression failed: error %d\n",lzo_ret);
                if(*DLSTATIONDEBUG)
                {
                    fprintf(*DLSTATIONLog, "DS DLSTATION: Data decompression failed: error %d\n",lzo_ret);
                    fflush(*DLSTATIONLog);
                }
            }

            free(buffer);

            data_size = out_len;

            //Extract the header and rsa signature from the saved data buffer, then clear the memory that they were stored in, in the saved data buffer.
            memcpy((void*)&dlstation_nds_data->header, (void*)dlstation_nds_data->saved_data, sizeof(TNDSHeader));
            memcpy((void*)&dlstation_nds_data->rsa.signature, (void*)&dlstation_nds_data->saved_data[(data_size - 136)], 136);

            dlstation_nds_data->trigger_assembly = 1;
            dlstation_nds_data->build_raw = data_size;
        }

        return 1;
    }

    return 0;
}
