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

#ifndef _H_WMB_ASM_SDK
#define _H_WMB_ASM_SDK

#define ASM_SDK_VERSION_MAJOR 1
#define ASM_SDK_VERSION_MINOR 0
#define ASM_SDK_VERSION_RELEASE 0
#define ASM_SDK_VERSION_BUILD 0
#define ASM_SDK_VERSION_STR "1.0b"

#define ASM_MODULE_VERSION_MAJOR 2//These defines are the Wmb Asm Module version this version of the SDK was built with.
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>

    #ifndef NDS
    #ifdef WIN32
        #include <io.h>
    #endif

            #ifdef __cplusplus
                #include <cstdio>
            #endif
    #endif

#ifndef ARM9
        #ifndef __cplusplus
                typedef enum {false = 0, true = 1} bool;
        #endif
#endif

#include "pcap.h"
#include "dirscan.h"

    #ifdef WIN32
        #include <windows.h>
    #endif

            #ifdef WIN32
                    #ifdef __cplusplus
                        #include <cstdlib>
                        #include <iostream>
                    #endif
                #include "win32dirent.h"
            #endif

                    #ifdef unix
                        //#include <cstdlib>
                        //#include <iostream>
                        #include <dirent.h>
                        #include <dlfcn.h>
			#include <sys/stat.h>
			#include <sys/types.h>
                    #endif

    #ifdef NDS
        #include <sys/dirent.h>
    #endif

            #ifndef NDS
                #ifdef __cplusplus
                    using namespace std;
                #endif
            #endif

#ifdef BUILDING_SDK
	extern bool PCAP_CheckAVS;
#endif

//*****************DEBUG*******************************

#define fprintfdebug fprintf
#define fflushdebug(f) fflush(f);
#define fopendebug fopen
#define fclosedebug fclose

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

#define SDK_STAGE_START 0
#define SDK_STAGE_BEACON 1
#define SDK_STAGE_AUTH 2
#define SDK_STAGE_RSA 3
#define SDK_STAGE_HEADER 4
#define SDK_STAGE_DATA 5
#define SDK_STAGE_ASSEMBLE 6
#define SDK_STAGE_END 7

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

int GetFileLength(FILE* _pfile);

//struct sAsmSDK_Config;

typedef void (*SuccessCallback)(void);
void ConvertEndian(void* input, void* output, int input_length);
void CheckEndianA(void* input, int input_length);

typedef struct _sAsmSDK_Params
{
    spcap_pkthdr *header;
    u_char *pkt_data;
    int length;
    char **argv;
    pcap_t *fp;
    bool checkrsa;
    char *outdir;
    bool run;
    char *copydir;
    bool use_copydir;
    bool has_avs;
}sAsmSDK_Params;

typedef void (*lpAsmGetStatusCallback)(char *str);

typedef int (*lpGetPacketNumber)();

#ifndef ASM_SDK_MODULE
typedef struct _PacketModule
{
    unsigned char filler[64];//The plugins are not supposed to access these directly, only through exported Wmb Asm Module functions.
} PacketModule;
#endif

//These structs are from Juglak's WMB Host. Others are from libnds, while one is mine
typedef struct _iee80211_framehead2 {//<----------This is the struct actually used in the program. The other is a backup.
	unsigned short frame_control;
	unsigned short duration_id;
	unsigned char mac1[6];
	unsigned char mac2[6];
	unsigned char mac3[6];
	unsigned short sequence_control;
} iee80211_framehead2;

typedef struct _ds_advert {
	volatile unsigned short icon_pallete[16];
	volatile unsigned char icon[512];
	volatile unsigned char unk220;
	volatile unsigned char hostname_len;
	volatile unsigned short hostname[10];
	volatile unsigned char max_players;
	volatile unsigned char unk237;
	volatile unsigned short game_name[48];
	volatile unsigned short game_description[96];
} __attribute__ ((__packed__)) ds_advert;

typedef struct _TNDSBanner {
  unsigned short version;
  unsigned short crc;
  unsigned char reserved[28];
  unsigned char icon[512];
  unsigned short palette[16];
  unsigned short titles[6][128];
} __attribute__ ((__packed__)) TNDSBanner;

typedef struct _beacon {
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
} __attribute__ ((__packed__)) beacon;

typedef struct _ds_element {
	unsigned char manufacturer[3];
	unsigned char unk03;
	unsigned char unk04[4];
	unsigned char unk08[4];
	unsigned char unk0c[4];
	unsigned short streamcode;
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
} __attribute__ ((__packed__)) ds_element;

typedef struct nds_rsaframe {
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
} __attribute__ ((__packed__)) nds_rsaframe;



#define AVS_magic    0x08021100
#define AVS_revision 0x01//1/01
#define AVS_PHY_type 4//67108864//4//802.11 type I guess... (DSSS 802.11b in this case)
#define AVS_DATA_RATE 20//335544320//14
#define AVS_SSI_TYPE 1//16777216//1
#define AVS_PREAMBLE 1//-805175296//1
#define AVS_ENCODING 1//-1//1

typedef struct _AVS_header//AVS WLAN Capture header
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

} __attribute__((__packed__)) AVS_header;

typedef struct _TNDSHeader {
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
typedef struct _Nds_data
{
       volatile int found_beacon[10*15];//At least one if we have seen a beacon. Access it like so: nds_data.found_beacon((gameID*10)+advert_sequence_number)
       volatile unsigned char beacon_data[980*15];
       volatile unsigned short beacon_checksum[10];
       volatile ds_advert advert;
       volatile ds_advert oldadvert;
       volatile ds_advert adverts[15];
       volatile nds_rsaframe rsa;
       volatile TNDSHeader header;
       volatile TNDSBanner *banner;//When null, the Wmb Asm Module will take care of the banner, but when not null, the packet module plugins can put data in here, and the Wmb Asm Module will just take care of writing it to the .nds.
       volatile bool data_init;
       volatile int arm7s, arm7e;
       volatile int arm7s_seq, arm7e_seq;
       volatile int cur_pos;
       volatile int last_dat_seq;
       volatile int total_binaries_size;
       volatile unsigned char *saved_data;//The data for Arm7/9
       volatile int *data_sizes;//Size of each seq block
       volatile int pkt_size;//The size of most of the data packets
       volatile bool pkt_fixed;
       volatile bool finished_first_assembly;//Set to true when we finished assembling the first transfer in the capture
       volatile int beacon_thing;
       volatile unsigned char prev_nonadvert;
       volatile bool handledIDs[256];
       volatile unsigned char gameID;
       volatile bool gotID;
       volatile bool multipleIDs;
       volatile bool FoundAllBeacons;
       volatile unsigned char FirstBeaconID;
       volatile bool foundIDs[15];
       volatile bool skipseq;
       volatile bool FoundGameIDs;
       volatile unsigned char clientID;

       volatile bool trigger_assembly;//Set by 1 by the packet module plugins when it's time to assembly and write
       //the .nds. This is reset when assembly is done.
       volatile bool use_advert;//This is only used when multipleIDs is true. When this variable is 0, advert will be used directly for assembly, with beacon_data being copyed into it first, however when 1, the adverts array will be used during assembly, instead of copying in data from the beacon_data array.
       int build_raw;//When doing assembly, when this is <=0, continue as normal. However, when this is larger than one, the Wmb Asm Module will just calculate the filename of the .nds, and directly write the contents of the saved_data buffer. The size of the .nds to write, from the buffer, is the value of this build_raw variable.
       int PacketModIndex;//The index of the packet module, for this plugin.
}Nds_data;

	typedef struct _sAsmSDK_Config
        {
            volatile Nds_data **nds_data;
            bool *DEBUG;
            FILE **Log;
	    lpGetPacketNumber getpacketnumber;
        } sAsmSDK_Config;

	#ifdef ASM_SDK_CLIENT
	//#ifdef NDS
        //In this case with building dlls, DLLIMPORT really means to export the function.
	bool HandlePacket(sAsmSDK_Params *params);

        bool InitAsm(SuccessCallback callback, bool debug, sAsmSDK_Config *config);
        void ExitAsm();
        void CaptureAsmReset(int *code, lpAsmGetStatusCallback callback);

        char *GetStatusAsm(int *error_code);
        bool QueryAssembleStatus(int *error_code);
        unsigned char GetPrecentageCompleteAsm();

        int SwitchMode(int mode);
        int SelectPacketModule(int index);
        PacketModule *GetPacketModules();
        int GetTotalPacketModules();
        int GetPacketModuleID(int index);
        char *GetPacketModuleIDStr(int index);
	//#endif
    #endif

    /*#ifdef NDS
	   #ifdef ASM_SDK_PLUGIN
		unsigned char GetPrecentageCompleteAsm();

		extern lpGetPacketNumber GetPacketNum;
	   #endif

	   #ifdef ASM_SDK_CLIENT
		#ifdef DLLMAIN
			lpGetPacketNumber GetPacketNum = NULL;
		#endif
	   #endif

	#endif*/

                //#ifndef NDS

                    /*typedef bool (*lpHandlePacket)( sAsmSDK_Params *params);
                    typedef bool (*lpInitAsm)(SuccessCallback callback, bool debug, struct sAsmSDK_Config *config);
                    typedef void (*lpExitAsm)(void);
                    typedef void (*lpCaptureAsmReset)(int *code, lpAsmGetStatusCallback callback);
                    typedef char* (*lpGetStatusAsm)(int *error_code);
                    typedef char* (*lpQueryAssembleStatus)(int *error_code);
                    typedef unsigned char (*lpGetPrecentageCompleteAsm)(void);

                    typedef char *(*lpGetModuleVersionStr)();
                    typedef int (*lpGetModuleVersionInt)(int);

                    typedef int (*lpSwitchMode)(int mode);
                    typedef int (*lpSelectPacketModule)(int index);
                    typedef struct PacketModule *(*lpGetPacketModules)();
                    typedef int (*lpGetTotalPacketModules)();
                    typedef int (*lpGetPacketModuleID)(int index);
                    typedef char *(*lpGetPacketModuleIDStr)(int index);*/

                        #ifndef BUILDING_SDK

                            #ifndef ASM_SDK_MODULE

                                #ifdef DLLMAIN
                                    /*lpHandlePacket HandlePacket=NULL;
                                    lpInitAsm InitAsm=NULL;
                                    lpExitAsm ExitAsm=NULL;
                                    lpCaptureAsmReset CaptureAsmReset=NULL;
                                    lpGetStatusAsm GetStatusAsm=NULL;
                                    lpQueryAssembleStatus QueryAssembleStatus=NULL;
                                    lpGetPrecentageCompleteAsm GetPrecentageCompleteAsm=NULL;

                                    lpGetModuleVersionStr GetModuleVersionStr=NULL;
                                    lpGetModuleVersionInt GetModuleVersionInt=NULL;*/

                                    lpGetPacketNumber GetPacketNum = NULL;

                                    /*lpSwitchMode SwitchMode = NULL;
                                    lpSelectPacketModule SelectPacketModule = NULL;
                                    lpGetPacketModules GetPacketModules = NULL;
                                    lpGetTotalPacketModules GetTotalPacketModules = NULL;
                                    lpGetPacketModuleID GetPacketModuleID = NULL;
                                    lpGetPacketModuleIDStr GetPacketModuleIDStr = NULL;*/

                                #endif

                                #ifndef DLLMAIN
                                    /*extern lpHandlePacket HandlePacket;//So source code outside of the one that loads the module, can use the module's functions.
                                    extern lpInitAsm InitAsm;
                                    extern lpExitAsm ExitAsm;
                                    extern lpCaptureAsmReset CaptureAsmReset;
                                    extern lpGetStatusAsm GetStatusAsm;
                                    extern lpQueryAssembleStatus QueryAssembleStatus;
                                    extern lpGetPrecentageCompleteAsm GetPrecentageCompleteAsm;

                                    extern lpGetModuleVersionStr GetModuleVersionStr;
                                    extern lpGetModuleVersionInt GetModuleVersionInt;*/

                                    extern lpGetPacketNumber GetPacketNum;

                                    /*extern lpSwitchMode SwitchMode;
                                    extern lpSelectPacketModule SelectPacketModule;
                                    extern lpGetPacketModules GetPacketModules;
                                    extern lpGetTotalPacketModules GetTotalPacketModules;
                                    extern lpGetPacketModuleID GetPacketModuleID;
                                    extern lpGetPacketModuleIDStr GetPacketModuleIDStr;*/

                                #endif

                            #endif

                        #endif
                    //#endif

	//#ifdef ASM_SDK_PLUGIN
	//#ifdef NDS
		void ResetAsm(volatile Nds_data *nds_data);
	//#endif
	//#endif

    /*#ifndef NDS
        #ifdef USING_DLL
            void ResetAsm(volatile Nds_data *nds_data);
        #endif

        typedef void (*lpResetAsm)(volatile Nds_data *nds_data);

        #ifndef BUILDING_SDK

            #ifndef ASM_SDK_MODULE

                #ifdef DLLMAIN
                    lpResetAsm ResetAsm=NULL;
                #endif

                #ifndef DLLMAIN
                     extern lpResetAsm ResetAsm;
                #endif

            #endif

        #endif

    #endif*/

        #ifdef ASM_SDK_CLIENT

                #ifdef DLLMAIN

                    int getpacketnumber()
                    {
                        return GetPacketNumber();
                    }

                    //LoadAsmDLL loads the Asm module, and puts addresses/function-pointers of
                    //the module functions into the sAsmSDK_Config struct passed to it.
                    //This function copys the function addresses from the config struct into to an easily accessed and called
                    //function pointers.
                    void InitDLLFunctions(sAsmSDK_Config *config)
                    {
                        /*#ifndef NDS
						HandlePacket = config->HandlePacket;
                        InitAsm = config->InitAsm;
                        ResetAsm = config->ResetAsm;
                        ExitAsm = config->ExitAsm;
                        CaptureAsmReset = config->CaptureAsmReset;
                        GetStatusAsm = config->GetStatusAsm;
                        QueryAssembleStatus = config->QueryAssembleStatus;
                        GetPrecentageCompleteAsm = config->GetPrecentageCompleteAsm;

                        GetModuleVersionStr = config->GetModuleVersionStr;
                        GetModuleVersionInt = config->GetModuleVersionInt;

                        SwitchMode = config->SwitchMode;
                        SelectPacketModule = config->SelectPacketModule;
                        GetPacketModules = config->GetPacketModules;
                        GetTotalPacketModules = config->GetTotalPacketModules;
                        GetPacketModuleID = config->GetPacketModuleID;
                        GetPacketModuleIDStr = config->GetPacketModuleIDStr;
                        #endif*/

                        config->getpacketnumber = &getpacketnumber;
			GetPacketNum = config->getpacketnumber;
                    }
                #endif

        #endif

        #define MODE_ASM 0
        #define MODE_CLIENT 1
        #define MODE_HOST 2

        #ifndef BUILDING_SDK
        #ifdef ASM_SDK_PLUGIN

            #define ASMPLUG_PRI_LOW -1//Plugin priorities. The plugins with the highest priority are dealt with first, then the ones with the lower priority, and so on. The plugins with the highest pritority with have the Handle802_11 function called first, then that function will be called after the current/highest, for the lower priority, and so on.
            #define ASMPLUG_PRI_NORMAL 0//You can use priorites higher or lower than the ones defined here in the SDK, it's not restricted to only these defines. But you'll need to either make a new define, or just return the prioritiy directly from AsmPlug_GetPriority.
            #define ASMPLUG_PRI_HIGH 1

            bool CheckFrame(unsigned char *data, unsigned char *host_mac, unsigned char command, unsigned short *size, unsigned char *pos);
            bool CheckFlow(unsigned char *mac,unsigned char flow);
            unsigned int CalcCRC32(unsigned char *data, unsigned int length);
            unsigned short CalcCRC16(unsigned char *data, unsigned int length);
            unsigned char *nintendoWMBBeacon( unsigned char *frame, int frame_size);
            bool CheckDataPackets(int seq);
            int ReadSeq(unsigned short *seq);
            bool CheckFrameControl(iee80211_framehead2 *framehead, int type, int subtype);
            unsigned short computeBeaconChecksum(unsigned short *data, int length);
            unsigned char GetGameID(unsigned char *data);
            bool CompareMAC(unsigned char *a, unsigned char *b);
	    void InitConfig(sAsmSDK_Config *config);

            void AsmPlugin_Init(sAsmSDK_Config *config, volatile Nds_data **dat);
	    void AsmPlugin_DeInit(volatile Nds_data **dat);

            void UpdateAvert(volatile Nds_data *dat);

        #endif
        #endif

            #ifdef NDS
                #undef BUILDING_DLL
            #endif

    #ifdef BUILDING_SDK

        #include "wmb_asm_sdk_internal.h"
    #endif

#endif
