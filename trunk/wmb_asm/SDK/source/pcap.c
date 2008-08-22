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

#define BUILDING_SDK

#include "..\include\wmb_asm_sdk.h"

#ifndef lib_pcap_h

char PCAP_ERROR_BUFFER[PCAP_ERRBUF_SIZE];

int PacketNumber=1;
struct spcap_pkthdr pktheader;
struct pcap_file_header capheader;

struct pcap_t *cap = NULL;

bool PCAP_CheckAVS = 1;

DLLIMPORT struct pcap_t *pcap_open_offline(const char *filename, char *errbuf)
{
    cap = (struct pcap_t*)malloc(sizeof(struct pcap_t));
    unsigned int magic = 0xa1b2c3d4;
    if(cap==NULL)return NULL;
    memset(&capheader,0,sizeof(struct pcap_file_header));
    PacketNumber=1;
    
    cap->file = fopen(filename,"rb");
    if(cap->file==NULL)
    {
        free(cap);
        return NULL;
    }
    
    if(fread(&capheader,1,sizeof(struct pcap_file_header),cap->file)!=sizeof(struct pcap_file_header))
    {
        return NULL;
    }
    
    if(capheader.magic!=magic)cap->swap=1;//If the magic number is not the same as our magic number, endians in everything needed swapped.
    if(capheader.magic==magic)cap->swap=0;
    
    if(cap->swap)
    {
        ConvertEndian(&capheader.majorVersion,&capheader.majorVersion,sizeof(unsigned short));
        ConvertEndian(&capheader.majorVersion,&capheader.minorVersion,sizeof(unsigned short));
        ConvertEndian(&capheader.timeZoneOffset,&capheader.timeZoneOffset,sizeof(unsigned int));
        ConvertEndian(&capheader.timeStampAccuracy,&capheader.timeStampAccuracy,sizeof(unsigned int));
        ConvertEndian(&capheader.snapshotLength,&capheader.snapshotLength,sizeof(unsigned int));
        ConvertEndian(&capheader.linkLayerType,&capheader.linkLayerType,sizeof(unsigned int));
    }
    
    if(capheader.majorVersion!=PCAP_VERSION_MAJOR || capheader.minorVersion!=PCAP_VERSION_MINOR)
    {
        //Unsupported version
        fclose(cap->file);
        free(cap);
        return NULL;
    }
    
    if(capheader.linkLayerType!=163 && capheader.linkLayerType!=1)
    {
        //Unsupported link type; This implemention only supports 802.11, with AVS, or
        //Ethernet/Wireless, IP, TCP.
        fclose(cap->file);
        free(cap);
        return NULL;
    }
    
    if(capheader.linkLayerType==163)PCAP_CheckAVS = 1;
    if(capheader.linkLayerType==1)PCAP_CheckAVS = 0;
    
    memset(&cap->header,0,sizeof(struct pcap_file_header));
    cap->pktdata = (unsigned char*)malloc(capheader.snapshotLength);
    if(cap->pktdata==NULL)return NULL;
    memset(cap->pktdata,0,capheader.snapshotLength);
    
    cap->error_buffer = errbuf;
    
    return cap;
}

DLLIMPORT int pcap_next_ex(struct pcap_t *fcap, pcap_pkthdr **hdr, const u_char **pktdata)
{
    if(fcap==NULL){strcpy(PCAP_ERROR_BUFFER,"FILE POINTER NULL");return -1;}
    if(fcap->file==NULL){strcpy(PCAP_ERROR_BUFFER,"CAPTURE FILE NOT OPENED");return -1;}
    
    trunc:
    
    if(feof(fcap->file)!=0)return -2;//Reached end of capture
    
    if(fread(&pktheader,1,sizeof(pcap_pkthdr),fcap->file)!=sizeof(pcap_pkthdr))
    return -2;
    
    if(fcap->swap)
    {
        ConvertEndian(&pktheader.tv_sec,&pktheader.tv_sec,sizeof(long));
        ConvertEndian(&pktheader.tv_usec,&pktheader.tv_usec,sizeof(long));
        ConvertEndian(&pktheader.caplen,&pktheader.caplen,sizeof(long));
        ConvertEndian(&pktheader.len,&pktheader.len,sizeof(long));
    }
    
    PacketNumber++;
    
    if(pktheader.caplen!=pktheader.len)
    {
        //Truncated packet. Skip it.
        fseek(fcap->file,(long)pktheader.caplen,SEEK_CUR);
        goto trunc;
    }
    
    if(fread(fcap->pktdata,1,pktheader.caplen,fcap->file)!=(size_t)pktheader.caplen)
    return -2;
    
    *hdr = &pktheader;
    *pktdata = fcap->pktdata;
    
    return 0;
}

DLLIMPORT void pcap_close(struct pcap_t *fcap)
{
    if(fcap==NULL)return;
    if(fcap->file==NULL)return;
    
    fclose(fcap->file);
    free(fcap->pktdata);
    //free(cap);//<-------- Hangs when debugging with gdb and wxDev-Cpp, no clue why
}

DLLIMPORT char *pcap_geterr(struct pcap_t *file)
{
    return PCAP_ERROR_BUFFER;//file->error_buffer
}

DLLIMPORT int GetPacketNumber()
{
    return PacketNumber;
}

DLLIMPORT int GetSnapshotLength()
{
    return (int)capheader.snapshotLength;
}

DLLIMPORT bool GetPacketHasAVS()
{
    return PCAP_CheckAVS;
}

DLLIMPORT void SetPacketHasAVS(bool val)
{
    PCAP_CheckAVS = val;
}

#endif
