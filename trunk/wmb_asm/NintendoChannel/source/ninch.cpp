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

#define STAGE_CLIENTHELLO 1
#define STAGE_SERVERHELLO 2

int stage = STAGE_CLIENTHELLO;

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;

bool DidInit = 0;

unsigned int client, host;

#ifdef __cplusplus
  extern "C" {
#endif

int Handle_ClientHello(unsigned char *data, int length);

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
    if(stage==STAGE_CLIENTHELLO)
    {
        *error_code = STAGE_CLIENTHELLO;
        return (char*)"02: Nintendo Channel: Failed to find the TLS Client Hello packet.\n";
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    if(stage==STAGE_CLIENTHELLO)return 3;
    if(stage==STAGE_SERVERHELLO)return 1;
    
    return 0;
}

DLLIMPORT int AsmPlug_Handle802_11(unsigned char *data, int length)
{
     if(stage==STAGE_CLIENTHELLO)return Handle_ClientHello(data, length);

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
    stage=STAGE_CLIENTHELLO;
    
    client = 0;
    host = 0;
    
    /*last_seq=0;

    memset(host_mac,0,6);
	memset(client_mac,0,6);*/
}

#ifdef __cplusplus
  }
#endif

int did_dump=0;

int Handle_ClientHello(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    unsigned char *Dat = NULL;
    unsigned char version = 0, ip_length = 0;
    
    dat = GetEthernet(data,length, 8);
    if(dat==NULL)return 0;
    length-=sizeof(EthernetHeader);
    
    Dat = GetIP(dat,length);
    if(Dat==NULL)return 0;
    length-=sizeof(IPHeader);
    IPHeader *iphdr = (IPHeader*)dat;
    //Dat+=sizeof(IPHeader);
    
    unsigned char *payload;
    TCPHeader *tcpheader = GetTCP(iphdr, Dat, length, &payload);
    if(tcpheader==NULL)return 0;
    length-=sizeof(TCPHeader);
    Dat+=sizeof(TCPHeader);
    
    version = iphdr->version;
    ip_length = iphdr->length * 4;
    client = iphdr->src;
    host = iphdr->dst;
    
    stage = STAGE_SERVERHELLO;
    
    printf("FOUND CLIENT HELLO!\n");
    
    return 1;
}
