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
#define STAGE_CLIENTKEY_EXCHANGE 3
#define STAGE_SERVER_CHANGECIPHERSPEC 4
#define STAGE_APPLICATION_DATA 5

int stage = STAGE_CLIENTHELLO;

sAsmSDK_Config *CONFIG = NULL;
bool *DEBUG = NULL;
FILE **Log = NULL;

bool DidInit = 0;

unsigned int client, host;

unsigned char Random_Bytes[28];
unsigned char *sessionID;
unsigned char sessionIDLen;

#ifdef __cplusplus
  extern "C" {
#endif

int Handle_ClientHello(unsigned char *data, int length);
int Handle_ServerHello(unsigned char *data, int length);
int Handle_ClientKeyExchange(unsigned char *data, int length);
int Handle_ServerChangeCipherSpec(unsigned char *data, int length);
int Handle_ApplicationData(unsigned char *data, int length);

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
     if(stage==STAGE_SERVERHELLO)return Handle_ServerHello(data, length);
     if(stage==STAGE_CLIENTKEY_EXCHANGE)return Handle_ClientKeyExchange(data, length);
     if(stage==STAGE_SERVER_CHANGECIPHERSPEC)return Handle_ServerChangeCipherSpec(data, length);
     if(stage==STAGE_APPLICATION_DATA)return Handle_ApplicationData(data, length);
     
     return 0;
}

DLLIMPORT bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &nds_data);
    
    ResetAsm = config->ResetAsm;
    DEBUG = config->DEBUG;
    Log = config->Log;
    CONFIG = config;
    
    memset(Random_Bytes, 0, 28);
    
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
    unsigned char tls_length = 0;
    unsigned char version_major, version_minor;
    unsigned short cipherspec_length;
    unsigned short sessionID_length;
    unsigned short challenge_length;
    
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
    Dat = payload;
    
    if(tcpheader->dest_port!=443 && tcpheader->src_port!=443)
    return 0;
    
    if(*Dat!=0x80)return 0;//Not exactly sure what this is...
    Dat++;
    
    tls_length = *Dat;
    Dat++;
    
    if(*Dat!=0x01)return 3;//This is not a Client Hello packet, ignore it.
    Dat++;
    
    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;
    
    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;
    
    memcpy(&cipherspec_length, Dat, sizeof(unsigned short));
    ConvertEndian(&cipherspec_length, &cipherspec_length, sizeof(unsigned short));
    Dat+=2;
    memcpy(&sessionID_length, Dat, sizeof(unsigned short));
    ConvertEndian(&sessionID_length, &sessionID_length, sizeof(unsigned short));
    Dat+=2;
    memcpy(&challenge_length, Dat, sizeof(unsigned short));
    ConvertEndian(&challenge_length, &challenge_length, sizeof(unsigned short));
    Dat+=2;
    
    bool found = 0;
    for(int i=0; i<(int)cipherspec_length/3; i++)
    {
        
        if(*Dat!=0 || *(Dat+1)!=0)return 3;
        if(*(Dat+2)==0x35)found = 1;
        
        Dat+=3;
    }
    
    if(!found)return 3;//Ignore this Client Hello packet since Cipher Spec: TLS_RSA_WITH_AES_256_CBC_SHA is not supported.
    
    client = iphdr->src;
    
    stage = STAGE_SERVERHELLO;
    
    printf("FOUND CLIENT HELLO!\n");
    
    return 1;
}

int Handle_ServerHello(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    unsigned char *Dat = NULL;
    unsigned char version_major, version_minor;
    unsigned short len;
    
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
    Dat = payload;

    if(tcpheader->dest_port!=443 && tcpheader->src_port!=443)
    return 0;
    
    if(iphdr->dst!=client)return 0;
    
    if(*Dat!=0x16)return 0;//Ignore non-handshake packets
    Dat++;
    
    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;
    
    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    if(*Dat!=0x02)return 3;//This is not a Server Hello record
    Dat++;
    
    Dat+=3;
    
    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;
    
    Dat+=4;
    memcpy(Random_Bytes, Dat, 28);
    Dat+=28;
    
    sessionIDLen = *Dat;
    Dat++;
    sessionID = (unsigned char*)malloc(sessionIDLen);
    memcpy(sessionID, Dat, sessionIDLen);
    Dat+=sessionIDLen;
    if(*Dat!=0x00)return 3;
    Dat++;
    if(*Dat!=0x35)return 3;
    Dat++;
    
    host = iphdr->src;
    
    stage = STAGE_CLIENTKEY_EXCHANGE;
    
    printf("FOUND SERVER HELLO!\n");
    
    return 1;
}

int Handle_ClientKeyExchange(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    unsigned char *Dat = NULL;
    unsigned char version_major, version_minor;
    unsigned short len;
    unsigned int Len;
    unsigned char *buffer;

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
    Dat = payload;

    if(tcpheader->dest_port!=443 && tcpheader->src_port!=443)
    return 0;
    
    if(iphdr->src!=client)return 0;
    if(iphdr->dst!=host)return 0;

    if(*Dat!=0x16)return 0;//Ignore non-handshake packets
    Dat++;

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;
    
    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;

    if(*Dat!=0x10)return 3;//This is not a Client Key Exchange record
    Dat++;
    
    unsigned char temp;
    unsigned char *ptr;
    
    Dat++;
    memcpy(&len, Dat, 2);
    ConvertEndian(&len, &len, sizeof(unsigned short));
    ptr = (unsigned char*)&len;
    /*temp = *ptr;
    *ptr = (*(ptr+2));
    (*(ptr+2)) = temp;
    */
    Dat+=2;
    
    Dat+=len;
    
    if(*Dat!=0x14)return 3;//This is not a Change Cipher Spec record
    Dat++;

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;
    
    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;

    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    if(*Dat!=0x01)return 3;//This is not a Change Cipher Spec message
    Dat++;
    
    if(*Dat!=0x16)return 3;//This is not a Handshake record
    Dat++;

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;

    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    buffer = (unsigned char*)malloc(len);
    if(buffer==NULL)return 5;
    memcpy(buffer, Dat, len);
    
    //This data is an encrypted handshake message. Decryption code will go here when the key/code used to generate the key for this are found.
    
    free(buffer);
    
    stage = STAGE_SERVER_CHANGECIPHERSPEC;
    
    printf("FOUND CLIENT KEY EXCHANGE!\n");
    
    return 1;
}

int Handle_ServerChangeCipherSpec(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    unsigned char *Dat = NULL;
    unsigned char version_major, version_minor;
    unsigned short len;
    unsigned char *buffer;
    
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
    Dat = payload;

    if(tcpheader->dest_port!=443 && tcpheader->src_port!=443)
    return 0;

    if(iphdr->src!=host)return 0;
    if(iphdr->dst!=client)return 0;
    
    if(*Dat!=0x14)return 0;//Ignore non-change cipher spec packets
    Dat++;

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;
    
    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    if(*Dat!=0x01)return 3;//Ignore non-change cipher spec messages.
    Dat++;
    
    if(*Dat!=0x16)return 0;//Ignore non-handshake records.
    Dat++;

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;

    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    buffer = (unsigned char*)malloc(len);
    if(buffer==NULL)return 5;
    memcpy(buffer, Dat, len);

    //This data is an encrypted handshake message. Decryption code will go here when the key/code used to generate the key for this are found.

    free(buffer);
    
    stage = STAGE_APPLICATION_DATA;
    
    printf("FOUND SERVER CHANGE CIPHER SPEC!\n");
    
    return 1;
}

int Handle_ApplicationData(unsigned char *data, int length)
{
    unsigned char *dat = NULL;
    unsigned char *Dat = NULL;
    unsigned char version_major, version_minor;
    unsigned short len;
    unsigned char *buffer;

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
    Dat = payload;

    if(tcpheader->dest_port!=443 && tcpheader->src_port!=443)
    return 0;
    
    if(((iphdr->src!=client) && (iphdr->dst!=host)) && ((iphdr->src!=host) && (iphdr->dst!=client)))
    return 0;
    
    //printf("A %x %d\n", *Dat, GetPacketNum());
    
    if(*Dat!=0x17)return 0;//Ignore non-application data packets
    Dat++;

    //printf("B\n");

    version_major = *Dat;
    Dat++;
    version_minor = *Dat;
    Dat++;

    if(version_major != 0x03)return 3;
    if(version_minor != 0x01)return 3;

    //printf("C\n");
    
    memcpy(&len, Dat, sizeof(unsigned short));
    ConvertEndian(&len, &len, sizeof(unsigned short));
    Dat+=2;
    
    buffer = (unsigned char*)malloc(len);
    if(buffer==NULL)return 5;
    memcpy(buffer, Dat, len);

    //This data is an encrypted application data message. Decryption code will go here when the code used to generate the key for this are found.

    free(buffer);
    
    printf("FOUND APPLICATION DATA!\n");
    
    return 1;
}
