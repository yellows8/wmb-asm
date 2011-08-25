/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
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

#ifndef _H_DEFS
#define _H_DEFS

#define ASM_MODULE_VERSION_MAJOR 2
#define ASM_MODULE_VERSION_MINOR 0
#define ASM_MODULE_VERSION_RELEASE 1
#define ASM_MODULE_VERSION_BUILD 0
#define ASM_MODULE_VERSION_STR "2.0b r2"

    #ifdef ARM9//We are compiling for Nintendo DS
		#include <nds.h>
			#ifndef NDS
				#define NDS
			#endif
    #endif

        #ifdef NDS
            #ifndef BUILDING_DLL
                #define BUILDING_DLL 1//Some trickery so the functions aren't exported. See dll.h for detals.
            #endif
        #endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

    #ifndef NDS
        #include <io.h>
        #include <cstdio>
    #endif

#include "dll.h"
#include "../include/pcap.h"

#include "../include/dirscan.h"

    #ifndef NDS
        #include <windows.h>
    #endif

            #ifdef WIN32
                #include <cstdlib>
                #include <iostream>

                #include "../include/win32dirent.h"
            #endif

                    #ifdef unix
                        #include <cstdlib>
                        #include <iostream>
                        #include <dirent.h>
                        #include <dlfcn.h>
                    #endif

    #ifdef NDS
        #include "../include/ndsdirent.h"
    #endif

            #ifndef NDS
                using namespace std;
            #endif

//*****************DEBUG*******************************
#ifdef BUILDING_DLL
extern bool DEBUG;
#endif

#define fprintfdebug fprintf
#define fflushdebug(f) fflush(f);
#define fopendebug fopen
#define fclosedebug fclose

#ifdef BUILDING_DLL
extern FILE *Log;
#endif

//<block>
//This section is from Juglak's WMB Host, to easily check the type & subtype.
#define FH_FC_TYPE(x) ((unsigned char)((x & 0xC)>>2))
#define FH_FC_SUBTYPE(x) ((unsigned char)((x & 0xF0)>>4))

#define FH_SC_FRAGNUM(x) ((unsigned char)((x & 0xF)))
#define FH_SC_SEQNUM(x) ((unsigned short)((x & 0xFFF0)>>4))
//<block/>

//From libnds
#define BIT(n) (1 << (n))

// structs

#define BEACON_INTERVAL 0xc800
#define BEACON_CAPABILITY 0x0021
#define BEACON_TAGPARAMS 0x01028204
#define BEACON_TAGPARAMS2 0x01088284

extern unsigned char host_mac[];
extern unsigned char client_mac[];
extern unsigned char host_client_mgc[];

#define STAGE_START 0
#define STAGE_BEACON 1
#define STAGE_AUTH 2
#define STAGE_RSA 3
#define STAGE_HEADER 4
#define STAGE_DATA 5
#define STAGE_ASSEMBLE 6
#define STAGE_END 7
extern int stage;

//These defines and this variable are from Masscat's WMB client code
extern unsigned char *Nin_ie;
#define NINTENDO_IE_TAG 0xDD

#define WMB_BEACON_SIZE 188
#define BEACON_NINTENDO_IE_DATA 0x34
#define BEACON_SOURCE_ADDR 0x0A
#define BEACON_BSS_ID 0x10
#define NIN_WMB_IE_BEACON_COUNT 0x23
#define NIN_WMB_IE_GAME_ID 0x18
#define NIN_WMB_IE_STREAM_ID 0x1a
#define NIN_WMB_IE_NON_ADVERT_FLAG 0x1c
#define NIN_WMB_IE_SEQ_NUMBER 0x1f
#define NIN_WMB_IE_PAYLOAD_SIZE 0x24
#define NIN_WMB_IE_PAYLOAD_DATA 0x26
#define NIN_WMB_IE_SOME_FLAGS 0x10

typedef void (*SuccessCallback)(void);
    //This most likely isn't already defined on all compilers/IDEs when compiling DLLs, but don't fret, that's already taken care of.(See above, dll.h, and dllmain.cpp)
    #ifdef BUILDING_DLL

        //In this case with building dlls, DLLIMPORT really means to export the function.
        DLLIMPORT bool HandlePacket(pcap_pkthdr *header, u_char *pkt_data, int length, char *argv[], pcap_t *fp, bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir, bool *Has_AVS);

        DLLIMPORT void InitAsm(SuccessCallback callback, bool debug);
        DLLIMPORT void ResetAsm();
        DLLIMPORT void ExitAsm();
        DLLIMPORT char *CaptureAsmReset(int *code);

        DLLIMPORT char *GetStatusAsm(int *error_code);
        DLLIMPORT bool QueryAssembleStatus(int *error_code);
        DLLIMPORT unsigned char GetPrecentageCompleteAsm();

        DLLIMPORT void ConvertEndian(void* input, void* output, int input_length);
        DLLIMPORT void CheckEndianA(void* input, int input_length);
    #endif

        #ifndef BUILDING_DLL

                #ifndef NDS

                    typedef bool (*lpHandlePacket)(pcap_pkthdr *header, u_char *pkt_data, int length, char *argv[], pcap_t *fp, bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir, bool *Has_AVS);
                    typedef void (*lpInitAsm)(SuccessCallback callback, bool debug);
                    typedef void (*lpResetAsm)(void);
                    typedef void (*lpExitAsm)(void);
                    typedef char* (*lpCaptureAsmReset)(int *code);
                    typedef char* (*lpGetStatusAsm)(int *error_code);
                    typedef char* (*lpQueryAssembleStatus)(int *error_code);
                    typedef unsigned char (*lpGetPrecentageCompleteAsm)(void);

                    typedef void (*lpConvertEndian)(void* input, void* output, int input_length);
                    typedef void (*lpCheckEndianA)(void* input, int input_length);

                        #ifdef DLLMAIN
                            lpHandlePacket HandlePacket=NULL;
                            lpInitAsm InitAsm=NULL;
                            lpResetAsm ResetAsm=NULL;
                            lpExitAsm ExitAsm=NULL;
                            lpCaptureAsmReset CaptureAsmReset=NULL;
                            lpGetStatusAsm GetStatusAsm=NULL;
                            lpQueryAssembleStatus QueryAssembleStatus=NULL;
                            lpGetPrecentageCompleteAsm GetPrecentageCompleteAsm=NULL;

                            lpConvertEndian ConvertEndian=NULL;
                            lpCheckEndianA CheckEndianA=NULL;
                        #endif

                        #ifndef DLLMAIN
                            extern lpHandlePacket HandlePacket;//So source code outside of the one that loads the module, can use the module's functions.
                            extern lpInitAsm InitAsm;
                            extern lpResetAsm ResetAsm;
                            extern lpExitAsm ExitAsm;
                            extern lpCaptureAsmReset CaptureAsmReset;
                            extern lpGetStatusAsm GetStatusAsm;
                            extern lpQueryAssembleStatus QueryAssembleStatus;
                            extern lpGetPrecentageCompleteAsm GetPrecentageCompleteAsm;

                            extern lpConvertEndian ConvertEndian;
                            extern lpCheckEndianA CheckEndianA;
                        #endif

                #endif

        #endif

//These structs are from Juglak's WMB Host. Others are from libnds, while one is mine
struct iee80211_framehead2 {//<----------This is the struct actually used in the program. The other is a backup.
	unsigned short frame_control;
	unsigned short duration_id;
	unsigned char mac1[6];//dest
	unsigned char mac2[6];//src
	unsigned char mac3[6];//bssid
	unsigned short sequence_control;
};

struct iee80211_framehead {
	unsigned short frame_control;
	unsigned short duration_id;
	unsigned char mac1[6];
	unsigned char mac2[6];
	unsigned char mac3[6];
	unsigned short sequence_control;
	unsigned char mac4[6];
};

struct ds_advert {
	unsigned short icon_pallete[16];
	unsigned char icon[512];
	unsigned char unk220;
	unsigned char hostname_len;
	unsigned short hostname[10];
	unsigned char max_players;
	unsigned char unk237;
	unsigned short game_name[48];
	unsigned short game_description[96];
	//unsigned char unk358[64];
} __attribute__ ((__packed__));

typedef struct SNDSBanner {
  unsigned short version;
  unsigned short crc;
  unsigned char reserved[28];
  unsigned char icon[512];
  unsigned short palette[16];
  unsigned short titles[6][128];
} __attribute__ ((__packed__)) TNDSBanner;

struct beacon {
	unsigned char unk0[4];
	unsigned char destmac[6];
	unsigned char srcmac[6];
	unsigned char bssmac[6];
	unsigned short seq;
	unsigned int ts1; // ??? some timestamp
	unsigned int ts2; // ???
	unsigned short interval;
	unsigned short capability;
	unsigned char tagparms[4];
	unsigned char dsparms[3];
	unsigned char tidm[7];
	unsigned char vendor[2];
} __attribute__ ((__packed__));

struct ds_element {
	unsigned char manufacturer[3];
	unsigned char unk03;
	unsigned char unk04[4];
	unsigned char unk08[4];
	unsigned char unk0c[4];
	unsigned short streamcode;
	//unsigned char unk12[2];
	unsigned char payload_length;
	unsigned char beacon_type;//0x0b for WMB usually
    unsigned char unk14[4];
	unsigned char unk18[4];
	unsigned char non_advert;
	unsigned char gameID;
	unsigned char connected_clients;
	unsigned char sequence_number;
	unsigned short checksum;
	unsigned char advert_sequence_number;
	unsigned char advert_len; // in beacons
	unsigned short data_size;
	unsigned char data[98];
} __attribute__ ((__packed__));

struct nds_rsaframe {
	unsigned int arm9exeaddr;
	unsigned int arm7exeaddr;
	unsigned int unk08; // 0
	unsigned int headerdest; // 0x027FFE00
	unsigned int headerdest2; // 0x027FFE00
	unsigned int headersize; // 0x160
	unsigned int unk18; // 0
	unsigned int arm9dest; //
	unsigned int arm9dest2; //
	unsigned int arm9size; //
	unsigned int unk28; // 0
	unsigned int unk2C; // 0x022C0000
	unsigned int arm7dest; //
	unsigned int arm7size; //
	unsigned char unk38[2]; // 1
	unsigned char signature[136];
	unsigned char unkC4[36]; // 0's
} __attribute__ ((__packed__));



#define AVS_magic    0x08021100//If you think about it, this magic string looks like 0 802.11 00
#define AVS_revision 0x01//1/01
#define AVS_PHY_type 4//67108864//4//802.11 type I guess... (DSSS 802.11b in this case)
#define AVS_DATA_RATE 20//335544320//14
#define AVS_SSI_TYPE 1//16777216//1
#define AVS_PREAMBLE 1//-805175296//1
#define AVS_ENCODING 1//-1//1

struct AVS_header//AVS WLAN Capture header
{
       unsigned int magic_revision;//Conatins both a magic number and the revision in one.
       //Use if((avs->magic_revision & AVS_magic) && (((unsigned char*)&avs->magic_revision)[3] & AVS_revision)) to check if it's correct
       unsigned int header_length;//The length of the rest of of the AVS header
       double MAC_timestamp;//I have no idea about this two timestamps...
       double host_timestamp;
       unsigned int PHY_type;//Should be AVS_PHY_type
       unsigned int channel;
       unsigned int data_rate;//AKA transfer rate. I.e 2.0 mbps...
       unsigned int antenna;
       unsigned int priority;
       unsigned int SSI_type;
       unsigned int RSSI_signal;
       unsigned char unk[4];//Some 32-bit field that Wireshark passed over...
       unsigned int preamble;
       unsigned int encoding_type;

} __attribute__((__packed__));

typedef struct SNDSHeader {
  char gameTitle[12];
  char gameCode[4];
  char makercode[2];
  unsigned char unitCode;
  unsigned char deviceType;           // type of device in the game card
  unsigned char deviceSize;           // device capacity (1<<n Mbit)
  unsigned char reserved1[9];
  unsigned char romversion;
  unsigned char flags;                // auto-boot flag

  unsigned int arm9romSource;
  unsigned int arm9executeAddress;
  unsigned int arm9destination;
  unsigned int arm9binarySize;

  unsigned int arm7romSource;
  unsigned int arm7executeAddress;
  unsigned int arm7destination;
  unsigned int arm7binarySize;

  unsigned int filenameSource;
  unsigned int filenameSize;
  unsigned int fatSource;
  unsigned int fatSize;

  unsigned int arm9overlaySource;
  unsigned int arm9overlaySize;
  unsigned int arm7overlaySource;
  unsigned int arm7overlaySize;

  unsigned int cardControl13;  // used in modes 1 and 3
  unsigned int cardControlBF;  // used in mode 2
  unsigned int bannerOffset;

  unsigned short secureCRC16;

  unsigned short readTimeout;

  unsigned int unknownRAM1;
  unsigned int unknownRAM2;

  unsigned int bfPrime1;
  unsigned int bfPrime2;
  unsigned int romSize;

  unsigned int headerSize;
  unsigned int zeros88[14];
  unsigned char gbaLogo[156];
  unsigned short logoCRC16;
  unsigned short headerCRC16;

  unsigned char zero[160];
} __attribute__ ((__packed__)) TNDSHeader;

//Beacon code is based on masscat's WMB client code for beacons.
struct Nds_data
{
       int found_beacon[10*15];//At least one if we have seen a beacon. Access it like so: nds_data.found_beacon((gameID*10)+advert_sequence_number)
       unsigned char beacon_data[980*15];
       unsigned short beacon_checksum[10];
       ds_advert advert;
       ds_advert oldadvert;
       nds_rsaframe rsa;
       TNDSHeader header;
       bool data_init;
       int arm7s, arm7e;
       int arm7s_seq, arm7e_seq;
       int cur_pos;
       int last_dat_seq;
       int total_binaries_size;
       unsigned char *saved_data;//The data for Arm7/9
       int *data_sizes;//Size of each seq block
       int pkt_size;//The size of most of the data packets
       bool pkt_fixed;
       bool finished_first_assembly;//Set to true when we finished assembling the first transfer in the capture
       int beacon_thing;
       unsigned char prev_nonadvert;
       bool handledIDs[256];
       unsigned char gameID;
       bool gotID;
       bool multipleIDs;
       bool FoundAllBeacons;
       unsigned char FirstBeaconID;
       bool foundIDs[15];
       bool skipseq;
       bool FoundGameIDs;
       unsigned char clientID;
};

extern Nds_data nds_data;

    #ifdef BUILDING_DLL
        DLLIMPORT void ConvertAVSEndian(AVS_header *avs);
    #endif

            #ifdef NDS
                #undef BUILDING_DLL
            #endif

    #ifndef BUILDING_DLL

        #ifndef NDS

            typedef void (*lpConvertAVSEndian)(AVS_header *avs);

            #ifdef DLLMAIN
                lpConvertAVSEndian ConvertAVSEndian=NULL;
            #endif

            #ifndef DLLMAIN
                extern lpConvertAVSEndian ConvertAVSEndian;
            #endif

            typedef char *(*lpGetModuleVersionStr)();
            typedef int (*lpGetModuleVersionInt)(int);

            #ifdef DLLMAIN
                lpGetModuleVersionStr GetModuleVersionStr=NULL;
                lpGetModuleVersionInt GetModuleVersionInt=NULL;
            #endif

            #ifndef DLLMAIN
                extern lpGetModuleVersionStr GetModuleVersionStr;
                extern lpGetModuleVersionInt GetModuleVersionInt;
            #endif

        #endif

    #endif

                        #ifdef NDS
                            #ifdef BUILDING_DLL
                                #undef BUILDING_DLL
                            #endif
                        #endif

#include "dll_load.h"

    #ifdef NDS
        #ifndef BUILDING_DLL
            #define BUILDING_DLL 1
        #endif
    #endif

#endif
