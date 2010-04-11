/*
Wmb Asm, the SDK, and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 - 2010 yellowstar

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

//This module is for experimenting with second stage loaders protocols. In this case, LoZ: Spirit Tracks.
//The data transferred in the Spirit Tracks second stage loader protocol, is some code,(roughly 0x1b0 bytes) a NARC, and some other data. None of this seems to be signed.

//#define DLLMAIN
#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#include "wmb_asm_sdk_plugin.h"

#undef fflushdebug
#define fflushdebug(f) fclose(f); f = fopen("secstagedump_log", "a");

int SecondStageDump_ProcessData(unsigned char *data, int length);

sAsmSDK_Config *SECSTAGEDUMPCONFIG = NULL;
bool *SECSTAGEDUMPDEBUG = NULL;
FILE **SecStageDumpLog = NULL;
FILE *secstagedumplog;
FILE *fbindump, *fbincode;

volatile Nds_data *secstagedump_nds_data;
unsigned short current_seq = 0, current_seq2 = 0;

void Init();
int CheckFrameExt(unsigned char *data, unsigned char command, unsigned short *size, unsigned char *pos, unsigned char firstmgc_byte, unsigned char flags);

//Change the names of the functions on-the-fly when compiling for DS, since everything is in one binary.
//#ifdef NDS
	#define AsmPlug_GetID SecStageDump_AsmPlug_GetID
	#define AsmPlug_GetIDStr SecStageDump_AsmPlug_GetIDStr
	#define AsmPlug_GetPriority SecStageDump_AsmPlug_GetPriority
	#define AsmPlug_GetStatus SecStageDump_AsmPlug_GetStatus
	#define AsmPlug_QueryFailure SecStageDump_AsmPlug_QueryFailure
	#define AsmPlug_Handle802_11 SecStageDump_AsmPlug_Handle802_11
	#define AsmPlug_Init SecStageDump_AsmPlug_Init
	#define AsmPlug_DeInit SecStageDump_AsmPlug_DeInit
	#define AsmPlug_GetNdsData SecStageDump_AsmPlug_GetNdsData
	#define AsmPlug_Reset SecStageDump_AsmPlug_Reset
	#define AsmPlug_GetModeStatus SecStageDump_AsmPlug_GetModeStatus
	#define AsmPlug_SwitchMode SecStageDump_AsmPlug_SwitchMode
//#endif

int AsmPlug_GetID()
{
    return 0;
}

char *AsmPlug_GetIDStr()
{
    return (char*)"SECSTAGEDUMP";
}

int AsmPlug_GetPriority()
{
    return ASMPLUG_PRI_LOW;//Why the lowest priority of all the default Wmb Asm plugins? The DS Download Station plugin needs to check some WMB packets. If the priority of this WMB plugin was normal, the DS Download Station plugin most likely wouldn't work correctly, if at all.
}

char *AsmPlug_GetStatus(int *error_code)
{
	*error_code=-1;
	return NULL;
}

int AsmPlug_QueryFailure()
{
    return 0;
}

int AsmPlug_GetModeStatus(int mode)//Queries whether or not the specified mode is available in this packet module.
{
    if(mode == MODE_ASM)return 1;
    if(mode != MODE_ASM)return 0;

	return 0;
}

int AsmPlug_SwitchMode(int mode)
{
    if(mode != MODE_ASM)return 3;

    return 1;
}

int AsmPlug_Handle802_11(unsigned char *data, int length)
{
     return SecondStageDump_ProcessData(data,length);
}

bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &secstagedump_nds_data);//Allocates memory for the nds_data struct
    memset((void*)secstagedump_nds_data, 0, sizeof(Nds_data));

    SECSTAGEDUMPDEBUG = config->DEBUG;
    SecStageDumpLog = &secstagedumplog;
    secstagedumplog = fopen("secstagedump_log", "w");
    SECSTAGEDUMPCONFIG = config;
    fbindump = fopen("secstagedump.bin", "wb");
    fbincode = fopen("secstagedumpcode.bin", "wb");

    return 1;
}

bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&secstagedump_nds_data);
    fclose(secstagedumplog);
    fclose(fbindump);
    fclose(fbincode);

    return 1;
}

volatile Nds_data *AsmPlug_GetNdsData()
{
    return secstagedump_nds_data;
}

void AsmPlug_Reset()
{
    
}

unsigned char secstagedump_buffer [0x81<<1];

int SecondStageDump_ProcessData(unsigned char *data, int length)
{
     unsigned short size = 0;
     unsigned char pos = 0;
     unsigned short seq;
     unsigned char *dat=NULL;
     unsigned short flags;
     int i;
     unsigned char *unks;
	int type = 0;     
	seq=0;

	iee80211_framehead2 *fh = (iee80211_framehead2*)data;

	if(CheckFrameExt(data, 0x05, &size, &pos, 0xe6, 0x1c))
	{
	}
	else if(CheckFrameExt(data, 0x07, &size, &pos, 0xe6, 0x9c))
        {
		type = 1;
 	}
	else
	{
		return 0;
	}

    dat=&data[(int)pos];
     unks = dat;
     memcpy(&flags, dat, 2);
     dat+=2;
     dat+=5;
     memcpy(&seq, dat, 2);
     dat+=2;
     dat+=2;

     	if(*SECSTAGEDUMPDEBUG)
     	{
	fprintfdebug(*SecStageDumpLog,"FOUND DATA(%d) SEQ %x FLAGS %x UNKS %02x %02x %02x %02x %02x\n", type, seq, flags, unks[2], unks[3], unks[4], unks[5], unks[6]);
	fflushdebug(*SecStageDumpLog);
        }

     memcpy(secstagedump_buffer, dat, size - 0x11 - 0x3);

     if(type==0)
     {
     if(seq==current_seq)
     {
	fwrite(secstagedump_buffer, 1, size - 0x11 - 0x3, fbindump);
	current_seq++;
	fprintfdebug(*SecStageDumpLog,"WROTE DATA(0)TO DUMP FILE\n");
     }
     else
     {
	fprintfdebug(*SecStageDumpLog,"IGNORED DATA(0), MISSED A PKT OR OLD PKT\n");
     }
     }
     else
     {
	if(seq==current_seq2)
     {
	fwrite(secstagedump_buffer, 1, size - 0x11 - 0x3, fbincode);
	current_seq2++;
	fprintfdebug(*SecStageDumpLog,"WROTE DATA(1) TO DUMP FILE\n");
     }
     else
     {
	fprintfdebug(*SecStageDumpLog,"IGNORED DATA(1), MISSED A PKT OR OLD PKT\n");
     }
     }

     return 0;
}

unsigned char secondstage_mgc[4] = {0x06,0x01,0x02,0x00};
//******************CHECK FRAME*********************************************************
int CheckFrameExt(unsigned char *data, unsigned char command, unsigned short *size, unsigned char *pos, unsigned char firstmgc_byte, unsigned char flags)
{
     iee80211_framehead2 *fh = (iee80211_framehead2*)data;
     unsigned char rsize=0;
     unsigned short Size=0;
     int data_pos = 24;

     if (((FH_FC_TYPE(fh->frame_control) == 2) && (FH_FC_SUBTYPE(fh->frame_control) == 2)))
     {
          if(CheckFlow(fh->mac1,0))
          {
				   
				   secondstage_mgc[0] = firstmgc_byte;
                                   if(memcmp(&data[data_pos],secondstage_mgc,4)==0)
                                   {
                                   data_pos+=4;

                                   rsize=data[data_pos];
                                   Size = ((unsigned short)(rsize<<1));

                                   data_pos++;

                                     if(data[data_pos]==flags)
                                     {
                                     data_pos++;
                                       if(data[data_pos]==command)
                                       {

                                       data_pos++;

                                       if(pos)
                                       *pos=(unsigned char)data_pos;
                                       if(size)*size=Size;

                                       return 1;
                                       }
                                     }
                                   }
          }
     }

     return 0;
}

