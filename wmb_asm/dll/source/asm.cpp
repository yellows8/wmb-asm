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

#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#define DLLMAIN

#include "..\..\SDK\include\wmb_asm_sdk_module.h"
#include "..\include\network.h"

#define MAX_PKT_MODULES 255

#undef DLLIMPORT
#define DLLIMPORT __declspec (dllexport)

//#define DUMPARMBINARIES//If this this uncommented and defined, the Arm7/9 binaries will be dumped to arm7.bin and arm9.bin during assembly. This will not dump these binaries for the DLStation plugin, as the data transferred is already an .nds.

SuccessCallback AssemblySuccessCallback = NULL;

#ifdef __cplusplus
  extern "C" {
#endif

void CheckEndianA(void* input, int input_length);//This one must be called first, with AVS header length as input
void ConvertEndian(void* input, void* output, int input_length);//Only call this after both of the above are called.
//This converts the Endian of the input to the Endian of the current machine
void ConvertAVSEndian(AVS_header *avs);

void ChangeFilename(char *in, char *out, char *filename);
unsigned char* IsValidAVS(u_char *pkt_data);

unsigned int CalcCRC32(unsigned char *data, unsigned int length);
unsigned short CalcCRC16(unsigned char *data, unsigned int length);

bool Handle802_11(unsigned char *data, int length);
bool AssembleNds(char *output);

#ifdef DLLIMPORT
	#ifdef NDS
		#undef DLLIMPORT
		#define DLLIMPORT
	#endif
#endif

DLLIMPORT unsigned char GetPrecentageCompleteAsm();

//DLLIMPORT void CaptureAsmReset(lpAsmGetStatusCallback callback);

DLLIMPORT bool HandlePacket(sAsmSDK_Params *params);

DLLIMPORT char *GetStatusAsm(int *error_code);
void GetStatusAsmA(lpAsmGetStatusCallback callback, int index);

#ifdef WIN32
void ExecuteApp(char *appname, char *cmdline);
#endif

#ifdef __cplusplus
  }
#endif

bool debug=0;

FILE *funusedpkt = NULL;
bool fpkt_error=0;
bool save_unused_packets=1;

spcap_pkthdr *glob_header;
char **glob_argv;
bool glob_checkrsa;
char *glob_outdir;
bool glob_run;
char *glob_copydir;
bool glob_use_copydir;

FILE **Log = NULL;
bool *DEBUG = NULL;

sAsmSDK_Config *CONFIG = NULL;

volatile Nds_data *module_nds_data;

int asm_mode = MODE_ASM;

typedef bool (*lpInit)(sAsmSDK_Config *config);
typedef bool (*lpDeInit)();
typedef int (*lpGetID)();
typedef char *(*lpGetIDStr)();
typedef int (*lpGetPriority)();
typedef volatile Nds_data *(*lpGetNdsData)();
typedef int (*lpHandle802_11)(unsigned char *data, int length);
typedef void (*lpReset)();
typedef char *(*lpGetStatus)(int *error_code);
typedef int (*lpGetModeStatus)(int status);
typedef int (*lpQueryFailure)();
typedef int (*lpSwitchMode)(int mode);

struct PacketModule
{
    lpInit Init;
    lpDeInit DeInit;
    lpGetNdsData GetNdsData;
    lpHandle802_11 handle802_11;
    lpReset reset;
    lpGetStatus get_status;
    lpSwitchMode SwitchMode;
    lpGetModeStatus GetModeStatus;
    lpQueryFailure query_failure;
    lpGetID GetID;
    lpGetIDStr GetIDStr;
    lpGetPriority GetPriority;
	#ifndef NDS
    LPDLL lpdll;//The handle of the module if this one isn't a built-in packet handler.
	#endif
	
	int ID;
	char *IDStr;
	int Priority;
};
PacketModule packetModules[MAX_PKT_MODULES];
int totalPacketModules = 0;
int currentPacketModule = -1;

bool PktModReorder();

void PktModClose()
{
	for(int i=0; i<totalPacketModules; i++)
	{
		if(packetModules[i].DeInit!=NULL)
			packetModules[i].DeInit();
		
		#ifndef NDS
			CloseDLL(&packetModules[i].lpdll, NULL);
		#endif
	}
}

bool PktModInit()
{
	#ifndef NDS
	if(!PktModReorder())return 0;
	#endif
	
		for(int i=0; i<totalPacketModules; i++)
		{
			if(packetModules[i].Init!=NULL)
			{
				if(!packetModules[i].Init(CONFIG))
				    return 0;
            }
		}
	
	return 1;
}

//This function reorders the plugins in the packetMoudles array, according to the priorties. In this way, priority code is only needed in the initialization function, not across several plugin function calling code, also.
bool PktModReorder()
{
	#ifdef NDS
		return 1;
	#endif

	#ifndef NDS
    PacketModule *list = (PacketModule*)malloc(sizeof(PacketModule) * 256);
    int pri = 0;
    int high_pri = 0;//Highest priority found so far.
    int lowest_pri = 0;//Lowest priority found so far.
    bool found = 0;//Did we find a plugin's priority yet?
    int cur = 0;//Current index in the priorities array.
    int plug_cur = 0;//Current index in the packetModules array.
    int pri_cur = 0;//Plugin index for the re-ordering code.
    if(list==NULL)
    {
        printf("ERROR: Failed to allocate memory\n");
        return 0;
    }
    memset(list, 0, sizeof(PacketModule) * MAX_PKT_MODULES);
    
    while(packetModules[plug_cur].lpdll!=NULL)
    {
        pri = packetModules[plug_cur].Priority;
        
            if(!found)
            {
                high_pri = pri;
                lowest_pri = pri;
                found = 1;
            }
            else
            {   
                if(pri > high_pri)
                    high_pri = pri;
                
                if(pri < lowest_pri)
                    lowest_pri = pri;
                
            }
            
        plug_cur++;
    }
    plug_cur = 0;
    found = 0;
    
    cur = high_pri;
    pri_cur = 0;
    while(cur>=lowest_pri)
    {
        
            while(packetModules[plug_cur].lpdll!=NULL)
            {
                pri = packetModules[plug_cur].Priority;
                
                    if(pri==cur)
                    {
                        memcpy(&list[pri_cur], &packetModules[plug_cur], sizeof(PacketModule));
                        pri_cur++;
                    }
                
                plug_cur++;
            }
        plug_cur = 0;
        cur--;
    }
    
    memcpy(packetModules, list, sizeof(PacketModule) * MAX_PKT_MODULES);
    free(list);
    
    return 1;
	
	#endif
}

void PktModReset()
{
	for(int i=0; i<totalPacketModules; i++)
	{	
		if(packetModules[i].reset!=NULL)
			packetModules[i].reset();
	}
}

void PktModSwitchMode(int mode)
{
	for(int i=0; i<totalPacketModules; i++)
	{
		if(packetModules[i].SwitchMode!=NULL)
			packetModules[i].SwitchMode(mode);
	}
}

bool PktModHandle802_11(unsigned char *data, int length)
{
    int ret = 0;
    
    if(currentPacketModule == -1)
    {
			for(int ii=0; ii<totalPacketModules; ii++)
			{
				if(packetModules[ii].handle802_11!=NULL)
				{
                    ret = packetModules[ii].handle802_11(data, length);
					
					if(ret!=0)
					{
                    currentPacketModule = ii;
                    return 1;
					}
				}
			}
    }
    else
    {
        ret = packetModules[currentPacketModule].handle802_11(data, length);
        
        if(ret==0)
        {
			for(int ii=0; ii<totalPacketModules; ii++)
			{
				if(packetModules[ii].handle802_11!=NULL && ii!=currentPacketModule)
				{
                    ret = packetModules[ii].handle802_11(data, length);

					if(ret)
					{
                    currentPacketModule = ii;
                    return 1;
					}
				}
			}
        }
    }
    
    return 0;
}

#ifndef NDS
int LoadPacketModule(char *filename, char *error_buffer, char *destr, LPDLL *lpdll);
#endif
FILE *modlog = NULL;

#ifdef __cplusplus
  extern "C" {
#endif

int WMB_AsmPlug_GetID();
char *WMB_AsmPlug_GetIDStr();
int WMB_AsmPlug_GetPriority();
char *WMB_AsmPlug_GetStatus(int *error_code);
int WMB_AsmPlug_QueryFailure();
int WMB_AsmPlug_Handle802_11(unsigned char *data, int length);
bool WMB_AsmPlug_Init(sAsmSDK_Config *config);
bool WMB_AsmPlug_DeInit();
volatile Nds_data *WMB_AsmPlug_GetNdsData();
void WMB_AsmPlug_Reset();

int DLSTATION_AsmPlug_GetID();
char *DLSTATION_AsmPlug_GetIDStr();
int DLSTATION_AsmPlug_GetPriority();
char *DLSTATION_AsmPlug_GetStatus(int *error_code);
int DLSTATION_AsmPlug_QueryFailure();
int DLSTATION_AsmPlug_Handle802_11(unsigned char *data, int length);
bool DLSTATION_AsmPlug_Init(sAsmSDK_Config *config);
bool DLSTATION_AsmPlug_DeInit();
volatile Nds_data *DLSTATION_AsmPlug_GetNdsData();
void DLSTATION_AsmPlug_Reset();

int NINCH_AsmPlug_GetID();
char *NINCH_AsmPlug_GetIDStr();
int NINCH_AsmPlug_GetPriority();
char *NINCH_AsmPlug_GetStatus(int *error_code);
int NINCH_AsmPlug_QueryFailure();
int NINCH_AsmPlug_Handle802_11(unsigned char *data, int length);
bool NINCH_AsmPlug_Init(sAsmSDK_Config *config);
bool NINCH_AsmPlug_DeInit();
volatile Nds_data *NINCH_AsmPlug_GetNdsData();
void NINCH_AsmPlug_Reset();

#ifdef __cplusplus
  }
#endif

bool InitPktModules()
{
	#ifndef NDS
    LPDLL lpdll;
    char filename[256];
    memset(filename, 0, 256);
	FILE_LIST *files_list = NULL;
    FILE_LIST *cur_file = files_list;
    //memset(files_list,0,sizeof(FILE_LIST));
    bool open_failed = 0;

    char *error_buffer = (char*)malloc(256);
	char *destr = (char*)malloc(256);
    memset(error_buffer, 0, 256);
    memset(destr, 0, 256);
	#endif
	
    memset(packetModules, 0, sizeof(PacketModule) * MAX_PKT_MODULES);
    totalPacketModules = 0;
    currentPacketModule = -1;

	#ifdef NDS
		
		//Ordering of packet modules in the array when compiling for DS, must be done manually, for ordering and priority ordering.
		packetModules[totalPacketModules].Init = &NINCH_AsmPlug_Init;
		packetModules[totalPacketModules].DeInit = &NINCH_AsmPlug_DeInit;
		packetModules[totalPacketModules].GetNdsData = &NINCH_AsmPlug_GetNdsData;
		packetModules[totalPacketModules].handle802_11 = &NINCH_AsmPlug_Handle802_11;
		packetModules[totalPacketModules].reset = &NINCH_AsmPlug_Reset;
		packetModules[totalPacketModules].get_status = &NINCH_AsmPlug_GetStatus;
		packetModules[totalPacketModules].query_failure = &NINCH_AsmPlug_QueryFailure;
		packetModules[totalPacketModules].GetID = &NINCH_AsmPlug_GetID;
		packetModules[totalPacketModules].GetIDStr = &NINCH_AsmPlug_GetIDStr;
		packetModules[totalPacketModules].GetPriority = &NINCH_AsmPlug_GetPriority;
		totalPacketModules++;
		
		packetModules[totalPacketModules].Init = &DLSTATION_AsmPlug_Init;
		packetModules[totalPacketModules].DeInit = &DLSTATION_AsmPlug_DeInit;
		packetModules[totalPacketModules].GetNdsData = &DLSTATION_AsmPlug_GetNdsData;
		packetModules[totalPacketModules].handle802_11 = &DLSTATION_AsmPlug_Handle802_11;
		packetModules[totalPacketModules].reset = &DLSTATION_AsmPlug_Reset;
		packetModules[totalPacketModules].get_status = &DLSTATION_AsmPlug_GetStatus;
		packetModules[totalPacketModules].query_failure = &DLSTATION_AsmPlug_QueryFailure;
		packetModules[totalPacketModules].GetID = &DLSTATION_AsmPlug_GetID;
		packetModules[totalPacketModules].GetIDStr = &DLSTATION_AsmPlug_GetIDStr;
		packetModules[totalPacketModules].GetPriority = &DLSTATION_AsmPlug_GetPriority;
		totalPacketModules++;
		
		packetModules[totalPacketModules].Init = &WMB_AsmPlug_Init;
		packetModules[totalPacketModules].DeInit = &WMB_AsmPlug_DeInit;
		packetModules[totalPacketModules].GetNdsData = &WMB_AsmPlug_GetNdsData;
		packetModules[totalPacketModules].handle802_11 = &WMB_AsmPlug_Handle802_11;
		packetModules[totalPacketModules].reset = &WMB_AsmPlug_Reset;
		packetModules[totalPacketModules].get_status = &WMB_AsmPlug_GetStatus;
		packetModules[totalPacketModules].query_failure = &WMB_AsmPlug_QueryFailure;
		packetModules[totalPacketModules].GetID = &WMB_AsmPlug_GetID;
		packetModules[totalPacketModules].GetIDStr = &WMB_AsmPlug_GetIDStr;
		packetModules[totalPacketModules].GetPriority = &WMB_AsmPlug_GetPriority;
		totalPacketModules++;
		
	#endif

    #ifndef NDS
    
        files_list = (FILE_LIST*)malloc(sizeof(FILE_LIST));
        memset(files_list, 0, sizeof(FILE_LIST));
        ScanDirectory(files_list,(char*)".",(char*)".dll");
            
            if(files_list!=NULL)
            {

            
            cur_file = files_list;

                while(cur_file!=NULL)
                {
                    if(cur_file->filename==NULL)break;
                    if(strcmp(cur_file->filename,"")==0)break;
                    
                    if(strstr(cur_file->filename, ".dll")!=0 || strstr(cur_file->filename, ".so")!=0)
                    {
                    
                    if(strstr(cur_file->filename, "wmb_asm.dll")==0 && strstr(cur_file->filename, "wmb_asm.so")==0)//Don't try to load ourself, the Wmb Asm Module.
                    {

                            if(modlog==NULL)
                            {
                                modlog = fopen("module_log.txt","w");
                                    if(modlog==NULL)break;
                            }
                            
                            //printf("Loading %s\n",cur_file->filename);
                            
                            int ret = LoadPacketModule(cur_file->filename, error_buffer, destr, &lpdll);
							if(ret==0)return 0;
                            if(ret==2)
                            {
                                open_failed = 1;
                                continue;
                            }
                            
                            //printf("Loaded %s\n",cur_file->filename);
                    }
                    
                    }
                    
                    cur_file=cur_file->next;
                }
            }
            else
            {
                printf("Failed to open file or directory %s\n",(char*)".");
            }
        
        if(modlog!=NULL)
        fclose(modlog);

        FreeDirectory(files_list);

        if(totalPacketModules==0)
        {
            printf("ERROR: Failed to find any Packet Module Plugins! Wmb Asm Module and the\nCommand-line can't work without the plugins! Stop!\n");
            system("PAUSE");
            exit(1);
        }
        
    if(open_failed)return 0;

    remove("module_log.txt");//Since we made it this far, there was no errors, so we can safely remove this file without saved errors being deleted.

    printf("Done.\n");

    #endif
    
    if(!PktModInit())
        return 0;
    
    PktModReset();
    
    return 1;
}

#ifndef NDS
int LoadPacketModule(char *filename, char *error_buffer, char *destr, LPDLL *lpdll)
{
    
    if(!LoadDLL(lpdll, filename, error_buffer))
                                    {
                                        sprintf(destr, "ERROR: %s", error_buffer);

                                        if(modlog!=NULL)
                                        {
                                            fprintf(modlog, "%s", destr);
                                            fflush(modlog);
                                        }
                                        
                                        printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                        return 2;
                                    }
                                    else
                                    {

                                        if((packetModules[totalPacketModules].Init=(lpInit)LoadFunctionDLL(lpdll, "AsmPlug_Init", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].DeInit=(lpDeInit)LoadFunctionDLL(lpdll, "AsmPlug_DeInit", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].reset=(lpReset)LoadFunctionDLL(lpdll, "AsmPlug_Reset", "_Z8AsmPlug_Resetv", error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }

                                        if((packetModules[totalPacketModules].handle802_11=(lpHandle802_11)LoadFunctionDLL(lpdll, "AsmPlug_Handle802_11", "_Z15AsmPlug_Handle802_11", error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }

                                        if((packetModules[totalPacketModules].get_status=(lpGetStatus)LoadFunctionDLL(lpdll, "AsmPlug_GetStatus", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }

                                        if((packetModules[totalPacketModules].query_failure=(lpQueryFailure)LoadFunctionDLL(lpdll, "AsmPlug_QueryFailure", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].GetID=(lpGetID)LoadFunctionDLL(lpdll, "AsmPlug_GetID", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].GetIDStr=(lpGetIDStr)LoadFunctionDLL(lpdll, "AsmPlug_GetIDStr", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);
                                            
                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].GetPriority=(lpGetPriority)LoadFunctionDLL(lpdll, "AsmPlug_GetPriority", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].GetNdsData=(lpGetNdsData)LoadFunctionDLL(lpdll, "AsmPlug_GetNdsData", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].GetModeStatus=(lpGetModeStatus)LoadFunctionDLL(lpdll, "AsmPlug_GetModeStatus", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }
                                        
                                        if((packetModules[totalPacketModules].SwitchMode=(lpSwitchMode)LoadFunctionDLL(lpdll, "AsmPlug_SwitchMode", NULL, error_buffer))==NULL)
                                        {
                                            sprintf(destr, "ERROR: In module %s: %s", filename, error_buffer);

                                                if(modlog!=NULL)
                                                {
                                                    fprintf(modlog, "%s", destr);
                                                    fflush(modlog);
                                                }

                                            CloseDLL(lpdll, NULL);

                                            printf("An error occured while loading the packet module plugin(s). The error was written to module_log.txt\n%s\n",destr);

                                            return 0;
                                        }

                                        packetModules[totalPacketModules].ID = packetModules[totalPacketModules].GetID();
                                        packetModules[totalPacketModules].IDStr = packetModules[totalPacketModules].GetIDStr();
                                        packetModules[totalPacketModules].Priority = packetModules[totalPacketModules].GetPriority();

                                        packetModules[totalPacketModules].lpdll = *lpdll;
                                        totalPacketModules++;
                                    }
                                    
                                    return 1;
}
#endif

//***************************CHECK FCS*************************************************
inline bool CheckFCS(unsigned char *data, int length)
{
    unsigned fcs_mgc[3] = {0x00, 0x02, 0x00};//All packets that have an FCS at the end have these numbers before the FCS

	return 1;

    if(memcmp(&data[length-7],fcs_mgc,3)==0)
    {
        unsigned int checksum = CalcCRC32(data,length-4);
        unsigned int *fcheck = (unsigned int*)&data[length-4];
        if(checksum!=*fcheck)
        {
			if(*DEBUG)
			{
				fprintf(*Log,"CHECKSUM %d FILE %d\n",(int)checksum,(int)*fcheck);
				fflushdebug(*Log);
            }

            return 0;
        }

        return 1;
    }
    else
    {
        return 1;//Since this packet doesn't have an FCS, return 1 so the program still processes it
    }

    return 0;
}

#ifdef __cplusplus
  extern "C" {
#endif

//************VERSION************************************

DLLIMPORT char *GetModuleVersionStr()
{
    return (char*)ASM_MODULE_VERSION_STR;
}

DLLIMPORT int GetModuleVersionInt(int which_number)
{
    if(which_number <= 0)return ASM_MODULE_VERSION_MAJOR;
    if(which_number == 1)return ASM_MODULE_VERSION_MINOR;
    if(which_number == 2)return ASM_MODULE_VERSION_RELEASE;
    if(which_number >= 3)return ASM_MODULE_VERSION_BUILD;

    return 0;
}

DLLIMPORT bool InitAsm(SuccessCallback callback, bool debug, sAsmSDK_Config *config)
{
    
    DEBUG = config->DEBUG;
    Log = config->Log;
    module_nds_data = *config->nds_data;
    //*config = *config;
    *config->Log = *config->Log;
    if(config->DEBUG==NULL)printf("ACK!\n");
    
    *DEBUG = debug;
    
    CONFIG = config;
    
    if(*DEBUG)
		{
			*Log = fopendebug("log.txt","w");
        }
    
	memset((void*)module_nds_data,0,sizeof(Nds_data));
	save_unused_packets=1;
	funusedpkt=NULL;

    AssemblySuccessCallback = callback;

    if(!InitPktModules())return 0;
    
    return 1;
}

DLLIMPORT void ResetAsm(volatile Nds_data *dat)
{
                        if(dat!=NULL)module_nds_data = dat;
                        if(dat==NULL)
                        {
							volatile Nds_data *data = NULL;
		                      for(int i=0; i<totalPacketModules; i++)
		                      {
			                         if(packetModules[i].GetNdsData!=NULL)
			                         {
				                        data = packetModules[i].GetNdsData();
				                        if(data==NULL)return;//Should never happen; But if it would, Wmb Asm would get into a infinite loop, and possibly crash eventually.
				                        
				                        ResetAsm(data);
                                     }
		                      }
                            
                            return;
                        }
    
                        if(module_nds_data->saved_data!=NULL)free((void*)module_nds_data->saved_data);
                        if(module_nds_data->data_sizes!=NULL)free((void*)module_nds_data->data_sizes);
                        module_nds_data->saved_data=NULL;
                        module_nds_data->data_sizes=NULL;

                                if(*DEBUG)
                                    fflushdebug(*Log);
                                
	                           TNDSHeader temp_header;
	                           nds_rsaframe temp_rsa;
	                           bool first=module_nds_data->finished_first_assembly;
                               unsigned short temp_checksums[10];
                               ds_advert temp_advert1;
                               //bool beacon_thing=nds_data.beacon_thing;
                               bool multipleIDs = module_nds_data->multipleIDs;
                               bool handledIDs[256];
                               bool FoundGameIDs = 0;

	                           if(module_nds_data->finished_first_assembly)
	                           {
                                    memcpy((void*)&temp_header, (void*)&module_nds_data->header,sizeof(TNDSHeader));
									memcpy((void*)&temp_rsa, (void*)&module_nds_data->rsa,sizeof(nds_rsaframe));
									memcpy((void*)temp_checksums, (void*)module_nds_data->beacon_checksum,20);
									memcpy((void*)&temp_advert1, (void*)&module_nds_data->oldadvert,sizeof(ds_advert));
									//memcpy(&temp_advert2,&module_nds_data->advert,sizeof(ds_advert));
									//module_nds_data->beacon_thing=beacon_thing;
									module_nds_data->multipleIDs = multipleIDs;
									memcpy((void*)handledIDs,(void*)module_nds_data->handledIDs,256);
									FoundGameIDs = module_nds_data->FoundGameIDs;
                               }

	                           memset((void*)module_nds_data,0,sizeof(Nds_data));
	                           module_nds_data->finished_first_assembly=first;
	                           if(module_nds_data->finished_first_assembly)
	                           {
                                    memcpy((void*)&module_nds_data->header, (void*)&temp_header,sizeof(TNDSHeader));
									memcpy((void*)&module_nds_data->rsa, (void*)&temp_rsa,sizeof(nds_rsaframe));
									memcpy((void*)&module_nds_data->beacon_checksum, (void*)temp_checksums,20);
									memcpy((void*)&module_nds_data->oldadvert, (void*)&temp_advert1,sizeof(ds_advert));
									//memcpy(&module_nds_data->advert,&temp_advert2,sizeof(ds_advert));
									//beacon_thing=module_nds_data->beacon_thing;
									multipleIDs = module_nds_data->multipleIDs;
									memcpy((void*)module_nds_data->handledIDs, (void*)handledIDs,256);
									module_nds_data->FoundGameIDs = FoundGameIDs;
                               }
                               
                               currentPacketModule = -1;
                               PktModReset();
}

DLLIMPORT void ExitAsm()
{   
    ResetAsm(NULL);
    PktModClose();
    
    deinit_network();
    
    if(funusedpkt!=NULL)
    {
    fclose(funusedpkt);
    funusedpkt=NULL;
    }

		#ifndef NDS
			remove("unused_packets.bin");
		#endif

		if(DEBUG)
        {
                if(*DEBUG)
			    {
					fclosedebug(*Log);
                }
            
            free(DEBUG);
        }
}

DLLIMPORT int SwitchMode(int mode)
{
    if(mode < 0 || mode > MODE_HOST)return 0;
    
    int ret = init_network();
    if(ret!=0)return ret;
    
    ResetAsm(NULL);
    PktModReset();
    
    asm_mode = mode;
    
    PktModSwitchMode(mode);
    
    return 1;
}

DLLIMPORT int SelectPacketModule(int index)
{
    if(index < 0 || index > totalPacketModules)return 0;
    
    int ret = packetModules[index].GetModeStatus(asm_mode);
    if(ret!=1)return ret;
    
    currentPacketModule = index;
    
    return 1;
}

#ifdef __cplusplus
  }
#endif

int total_assembled=0;
int prev_total=0;
void CaptureBacktrack()
{
    //Used mainly with multi-game captures. This is done because some packets that the program will not find, may have already been transmitted & captured previously in the capture.
    total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(module_nds_data->handledIDs[i])total_assembled++;
        }
    prev_total=total_assembled;
    
            if(funusedpkt!=NULL)
            fclose(funusedpkt);

            funusedpkt = fopen("unused_packets.bin","rb");
            int length=0;
            unsigned char *buffer = (unsigned char*)malloc((size_t)GetSnapshotLength());
            memset(buffer,0,(size_t)GetSnapshotLength());
            save_unused_packets=0;//HandlePacket would write the packets we send to it, to this file we are reading, if this wasn't set to zero. Which would obviously cause problems.
            sAsmSDK_Params *params = NULL;

                if(funusedpkt!=NULL)
                {
							if(*DEBUG)
							{
								fprintfdebug(*Log,"Backtracking through unused packets...\n");
								fflushdebug(*Log);
							}

                    params = (sAsmSDK_Params*)malloc(sizeof(sAsmSDK_Params));
                    memset(params, 0, sizeof(sAsmSDK_Params));

                    params->header = glob_header;
                    params->pkt_data = buffer;
                    params->argv = glob_argv;
                    params->fp = NULL;
                    params->checkrsa = glob_checkrsa;
                    params->outdir = glob_outdir;
                    params->run = glob_run;
                    params->copydir = glob_copydir;
                    params->use_copydir = glob_use_copydir;
                    params->has_avs = GetPacketHasAVS();

                        while(feof(funusedpkt)==0)
                        {
                            if(fread(&length,1,4,funusedpkt)!=4)break;
                            if(fread(buffer,1,(size_t)length,funusedpkt)!=(size_t)length)break;

                            params->length = length;

                            HandlePacket(params);
                        }

                    fclose(funusedpkt);
                    free(params);
                    funusedpkt=NULL;
                    save_unused_packets=1;
                }

            free(buffer);


        total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(module_nds_data->handledIDs[i])total_assembled++;
        }


        if(total_assembled==prev_total)return;

        CaptureBacktrack();
}

#ifdef __cplusplus
  extern "C" {
#endif

int PKTINDX = 0;

void CaptureAsmResetA(volatile Nds_data *dat, lpAsmGetStatusCallback callback)
{
    module_nds_data = dat;
    
    if(funusedpkt!=NULL && module_nds_data->multipleIDs)
    {
        fclose(funusedpkt);
        funusedpkt=NULL;

        CaptureBacktrack();

        total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(module_nds_data->handledIDs[i])total_assembled++;
        }

        bool got_all=1;
        int total_asm=0, total=0;
        for(int i=0; i<15; i++)
        {
            if(!module_nds_data->handledIDs[i] && module_nds_data->foundIDs[i])got_all=0;
            if(module_nds_data->foundIDs[i])total++;
            if(module_nds_data->handledIDs[i])total_asm++;
        }

        if(got_all)
        {
            printf("Successfully assembled all %d demos in the capture.\n",total);

                if(*DEBUG)
                {
			         fprintfdebug(*Log,"Successfully assembled all %d demos in the capture.\n",total);
			    }
        }
        else
        {
            //if(stage<=STAGE_HEADER)
            if(packetModules[currentPacketModule].query_failure() == 1)
            printf("Capture is incomplete.\n");

            printf("Failed to assemble all %d demos in the capture; %d out of %d demos in the capture assembled.\n",total,total_asm,total);

				if(*DEBUG)
					fprintfdebug(*Log,"Failed to assemble all %d demos in the capture; %d out of %d demos in the capture assembled.\n",total,total_asm,total);
        }

				if(*DEBUG)
					fflushdebug(*Log);

			#ifndef NDS
				funusedpkt = fopen("unused_packets.bin","wb");
			#endif
    }

    memset((void*)module_nds_data->handledIDs,0,256);
    memset((void*)module_nds_data->found_beacon,0,(10*15) * sizeof(int));
    module_nds_data->FoundGameIDs=0;
    
    GetStatusAsmA(callback, PKTINDX);

    ResetAsm((Nds_data*)module_nds_data);
}

DLLIMPORT void CaptureAsmReset(int *code, lpAsmGetStatusCallback callback)//Needs to be called after reading the whole capture.
{
        #ifndef NDS
		volatile Nds_data *data = NULL;
		char *str = NULL;
		
		    for(int i=0; i<totalPacketModules; i++)
		    {
			    if(packetModules[i].lpdll==NULL)break;

			         if(packetModules[i].GetNdsData!=NULL)
			         {
				        data = packetModules[i].GetNdsData();
				        if(data==NULL)return;//Should never happen; But if it would, Wmb Asm would get into a infinite loop, and possibly crash eventually.
                        
                        PKTINDX = i;
                        
                        if(currentPacketModule != -1 && i==currentPacketModule)
                        {
				            CaptureAsmResetA(data, callback);
                        }
                        else
                        {
                            CaptureAsmResetA(data, callback);
                        }
                     }
		    }
	    #endif
}

DLLIMPORT char *GetStatusAsm(int *error_code)
{
    if(currentPacketModule != -1)
    {
        return packetModules[currentPacketModule].get_status(error_code);
    }
    
    return NULL;
}

void GetStatusAsmA(lpAsmGetStatusCallback callback, int index)
{
    int *error = (int*)malloc(sizeof(int));
    *error = 0;
    
    if(packetModules[index].query_failure() != 3)//If these capture/transfer actually has packets for this plugin's protocol, get any errors, then trigger the callback with a pointer to the error's string.
    {
        char *str = packetModules[index].get_status(error);
        callback( str );
    }
    
    free(error);
}

DLLIMPORT bool QueryAssembleStatus(int *error_code)
{
    GetStatusAsm(error_code);
    return module_nds_data->finished_first_assembly;
}

DLLIMPORT unsigned char GetPrecentageCompleteAsm()
{
    unsigned char percent=0;

    int temp=0,temp2=0;
    int devisor=0;
    int size=490;
    int i=0;

    if(currentPacketModule == -1)return 0;//Prevent access violations and other problems.
    
        if(packetModules[currentPacketModule].query_failure() == 2)
        {

                for(int I=0; I<(int)module_nds_data->total_binaries_size; I+=size)
                {
                    temp+=module_nds_data->data_sizes[i];
                    size=module_nds_data->data_sizes[i];

                    if(size==0)size=module_nds_data->pkt_size;
                    i++;
                }

            if(temp == 0)return 0;

            devisor = module_nds_data->total_binaries_size/100;
            temp2 = temp / devisor;
            percent = (unsigned char)ceil((double)temp2);
        }

    return percent;
}

int TheTime=0;
DLLIMPORT bool HandlePacket(sAsmSDK_Params *params)
{
     //THE main assenbly function. All the host program of this module needs to do is load this module, call some functions at certain times, and call this function for each packet it reads/captures, and the assembler takes care of the rest.

     unsigned char *data;

     glob_header = params->header;
     glob_argv = params->argv;
     glob_checkrsa = params->checkrsa;
     glob_outdir = params->outdir;
     glob_run = params->run;
     glob_copydir = params->copydir;
     glob_use_copydir = params->use_copydir;

    SetPacketHasAVS(params->has_avs);

     //Is the AVS WLAN Capture header valid?
     data=IsValidAVS(params->pkt_data);
     if(data)
     {
            if(data==NULL)data = params->pkt_data;
            if(params->has_avs)params->length-=64;
            
             Handle802_11(data,params->length);

                    #ifndef NDS
					if(save_unused_packets && module_nds_data->multipleIDs)
                    {
                        if(funusedpkt==NULL)
                        {
                            funusedpkt = fopen("unused_packets.bin","wb");
	                       fpkt_error=0;
	                       if(funusedpkt!=NULL)fpkt_error=1;
                        }

                        if(funusedpkt!=NULL)
                        {
                            fwrite(&params->length,1,4,funusedpkt);
                            fwrite(params->pkt_data,1,(size_t)params->length,funusedpkt);
                        }
                    }
					#endif

     }

    volatile Nds_data *dat = NULL;

     for(int ii=0; ii<totalPacketModules; ii++)
	 {
       
	   if(packetModules[ii].GetNdsData!=NULL)
	   {
            dat = packetModules[ii].GetNdsData();
            module_nds_data = dat;

            //printf("DAT %p\n",dat);

     if(module_nds_data->trigger_assembly)
     {
        

                            char out[256];
                            char output[256];
                            int pos=0;
                            int I=0;
                            bool found=0;
	                        memset(out,0,256);
	                        memset(output,0,256);
                            strncpy(out,(char*)module_nds_data->header.gameTitle,12);
                            for(int i=0; i<12; i++)
                            {
                                if(out[I+i]=='\\' || out[I+i]=='/' || out[I+i]==':' || out[I+i]=='*' || out[I+i]=='?' || out[I+i]=='"' || out[I+i]=='<' || out[I+i]=='>' || out[I+i]=='|')
                                out[I+i] = ' ';//Remove any illegal chars, that will cause file opening to fail

                                if(out[I+i]==0)found=1;

                                if(!found)
                                pos++;
                            }

                            //strcpy(&out[pos]," ");
                            out[pos] = '_';
                            pos+=1;

                            found=0;
                            I=pos;
                            strncpy((char*)&out[pos],(char*)module_nds_data->header.gameCode,4);
                            for(int i=0; i<4; i++)
                            {
                                if(out[I+i]=='\\' || out[I+i]=='/' || out[I+i]==':' || out[I+i]=='*' || out[I+i]=='?' || out[I+i]=='"' || out[I+i]=='<' || out[I+i]=='>' || out[I+i]=='|')
                                out[I+i] = ' ';//Remove any illegal chars, that will cause file opening to fail

                                pos++;
                            }

                            //strcpy(&out[pos]," ");
                            out[pos] = '_';
                            pos+=1;
                            I=pos;
                            found=0;

                            strncpy((char*)&out[pos],(char*)module_nds_data->header.makercode,2);
                            for(int i=0; i<2; i++)
                            {
                                if(out[I+i]=='\\' || out[I+i]=='/' || out[I+i]==':' || out[I+i]=='*' || out[I+i]=='?' || out[I+i]=='"' || out[I+i]=='<' || out[I+i]=='>' || out[I+i]=='|')
                                out[I+i] = ' ';//Remove any illegal chars, that will cause file opening to fail

                                pos++;
                            }
                            out[pos] = '_';
                            pos+=1;
                            I=pos;
                            sprintf(out,"%s_%d",out,rand() % 1000);

                            sprintf(out,"%s%s",out,".nds");
                            //out[pos+5]=0;
                            strcpy(output,out);
                            //printf("OUT %s\n",out);
                            sprintf(output,"%s%s",params->outdir,out);
                            //printf("OUTPUT %s OUTDIR %s OUT %s\n",output,outdir,out);


                             char *Str = new char[256];
                             strcpy(Str,output);

                             if(!AssembleNds(Str))
                             {
                             delete[]Str;
                             pcap_close(params->fp);

                                if(*DEBUG)
                                {
	                               fclosedebug(*Log);
                                   *Log=NULL;
                                   free(DEBUG);
                                }

                             printf("Failure.\n");
                             module_nds_data->trigger_assembly = 0;
	                         return 0;
                             }

                            if(AssemblySuccessCallback!=NULL)
	                           AssemblySuccessCallback();

                             module_nds_data->finished_first_assembly=1;
                             memcpy((void*)&module_nds_data->oldadvert, (void*)&module_nds_data->advert,sizeof(ds_advert));
                             //memset(&nds_data->advert,0,sizeof(ds_advert));
                             memcpy((void*)&module_nds_data->oldadvert, (void*)&module_nds_data->advert,sizeof(ds_advert));
                             ResetAsm((Nds_data*)module_nds_data);



	                           #ifdef WIN32
	                           if(params->checkrsa)
	                           {
	                           char *rsa_cmdline = (char*)malloc(256);
	                           memset(rsa_cmdline, 0, 256);
	                           int rsai=0;
	                           strcpy(&rsa_cmdline[rsai],"verify nintendo");
	                           rsai+=strlen("verify nintendo");
	                           rsa_cmdline[rsai]=' ';
	                           rsai++;
	                           rsa_cmdline[rsai] = '\"';
	                           rsai++;
	                           strcpy(&rsa_cmdline[rsai],Str);
	                           rsai+=strlen(Str);
	                           rsa_cmdline[rsai] = '\"';
	                           rsai++;
	                           rsa_cmdline[rsai]=0;
	                           printf("Executing ndsrsa.exe %s\n",rsa_cmdline);
	                           ExecuteApp("ndsrsa.exe",rsa_cmdline);
	                           printf("\n");
	                           free(rsa_cmdline);
                               }
                               #endif

                               if(params->copydir!=NULL && params->use_copydir)
                               {
                               char str[256];
                               unsigned char *buffer=NULL;
                               int filesize = 0;
                               sprintf(str,"%s%s",params->copydir,out);
                               FILE *f = fopen(Str,"rb");
                               if(f!=NULL)
                               {
                               filesize = GetFileLength(f);
                               buffer = (unsigned char*)malloc((size_t)filesize);
                               fread(buffer,1,filesize,f);
                               fclose(f);
                               }
                               else
                               {
                                    printf("Failed to open file %s for reading, to copy it elsewhere.\n",Str);
                               }

                                    if(buffer!=NULL)
                                    {
                                        f = fopen(str,"wb");
                                        if(f!=NULL)
                                        {
                                            fwrite(buffer,1,(size_t)filesize,f);
                                            fclose(f);
                                        }
                                        else
                                        {
                                            printf("Failed to copy file %s to %s\n",Str,str);
                                        }
                                    }
                               }

                               #ifdef WIN32
                               if(params->run)
                               {
                                    char cline[256];
                                    sprintf(cline,"\"%s\"",output);
                                    printf("Executing %s\n",output);
                                    int err=0;
                                    err = (int)ShellExecute(NULL,NULL,output,NULL,NULL,SW_SHOW);
                               }
	                           #endif

	                           delete []Str;

                               module_nds_data->trigger_assembly = 0;

        
	}
	
     }
    }

     return 1;

}

#ifdef __cplusplus
  }
#endif

bool Handle802_11(unsigned char *data, int length)
{

     CheckFCS(data,length);

     return PktModHandle802_11(data, length);
}

//Data for AssembleNds
unsigned char reserved[160];
int TheTime1=0;
bool FoundIT=0;
bool AssembleNds(char *output)
{
     int temp=0;
     ds_advert *ad = (ds_advert*)&module_nds_data->advert;
     //unsigned char *Ad = (unsigned char*)ad;
     TNDSHeader ndshdr;
     TNDSBanner banner;
     FILE *nds=NULL;
     //size_t length=98;
     unsigned char *Temp=NULL;
     size_t sz=0;
     char *download_play = (char*)malloc(35);
     strcpy(download_play,"DS DOWNLOAD PLAY----------------");

     char gdtemp[128*10];
     memset(gdtemp,0,128*10);

     if(!FoundIT)
     {
     FoundIT=1;
     }

     memset(&ndshdr,0,sizeof(TNDSHeader));
     memset(&banner,0,sizeof(TNDSBanner));
     memcpy((void*)&ndshdr,(void*)&module_nds_data->header,sizeof(TNDSHeader));
     
     unsigned char *ptr;
     ptr = (unsigned char*)&module_nds_data->advert;
     
     if(module_nds_data->use_advert)
     {
            memcpy((void*)&module_nds_data->advert, (void*)&module_nds_data->adverts[(int)module_nds_data->gameID], sizeof(ds_advert));
     }

     FILE *fdump = fopen("advertM.bin","wb");
     fwrite((void*)&module_nds_data->advert, 1, sizeof(ds_advert), fdump);
     fclose(fdump);
     
     if(module_nds_data->banner==NULL)
     {
        
        memcpy((void*)banner.palette, (void*)ad->icon_pallete,32);
        memcpy((void*)banner.icon, (void*)ad->icon,512);
        memset((void*)banner.titles,0,6*(128*2));
        
        int tempi=0;
        for(int i=0; i<48; i++)
        {
            if(module_nds_data->advert.game_name[i]==0)
            {
            break;
            }
            
            tempi+=2;
        }
     
        memcpy((void*)&gdtemp[0], (void*)&module_nds_data->advert.game_name[0],(size_t)tempi);//Copy the game name into the banner discription, add a newline character after that, then copy in the actual discription.
        gdtemp[tempi] = '\n';
        tempi+=2;
        
        
        memcpy((void*)&gdtemp[tempi], (void*)&module_nds_data->advert.game_description[0],96*2);
        
        for(int i=0; i<96; i++)
        {
            if(module_nds_data->advert.game_description[i]==0)
            {
            break;
            }
            
            tempi+=2;
        }
        
        if(tempi>0 && tempi<256)
        memset(&gdtemp[tempi],0,256-tempi);//One capture had a discription from another demo, after it's own discription. This gets rid of the extra one.
        
        memcpy(&banner.titles[0][0],gdtemp,192*2);
        memcpy(&banner.titles[1][0],gdtemp,192*2);
        memcpy(&banner.titles[2][0],gdtemp,192*2);
        memcpy(&banner.titles[3][0],gdtemp,192*2);
        memcpy(&banner.titles[4][0],gdtemp,192*2);
        memcpy(&banner.titles[5][0],gdtemp,192*2);
        
        banner.version = 1;
        banner.crc = CalcCRC16(banner.icon, 2080);
        
     }
     else
     {
        memcpy(&banner, (void*)&module_nds_data->banner, sizeof(TNDSBanner));
     }
        
     nds=fopen(output,"w+b");
     if(nds==NULL)
     {
            printf("Failed to open output file for writing: %s\n",output);
            return 0;
     }

     if(module_nds_data->build_raw>0)
     {
        fwrite((void*)module_nds_data->saved_data, 1, (size_t)module_nds_data->build_raw, nds);
        fclose(nds);
        
        module_nds_data->beacon_thing=0;
        module_nds_data->multipleIDs=0;
        module_nds_data->handledIDs[(int)module_nds_data->gameID] = 1;
        prev_total=0;
        
        return 1;
     }

     unsigned int temp1, temp2;
     unsigned short temp3, temp4;
     temp1 = module_nds_data->header.bannerOffset;
     temp2 = module_nds_data->header.romSize;
     temp3 = module_nds_data->header.logoCRC16;
     temp4 = module_nds_data->header.headerCRC16;

            if(module_nds_data->header.bannerOffset==0)//Some demos don't set this fields properly. Fix these for the first header, then use the original header for the second header, so RSA isn't invalidated.
            {
                module_nds_data->header.bannerOffset = (unsigned int)((int)(((int)module_nds_data->header.arm7romSource) + ((int)module_nds_data->header.arm7binarySize)) + 660);
            }
            if(module_nds_data->header.romSize!=module_nds_data->header.bannerOffset+2112)
            {
                module_nds_data->header.romSize=(unsigned int)((int)module_nds_data->header.bannerOffset+2112);
            }

            memcpy((void*)&ndshdr, (void*)&module_nds_data->header,sizeof(TNDSHeader));

     int rpos = 0;
     memset(reserved,0,160);
     for(int i=0; i<16; i++)
     reserved[i] = 0x2D;
     rpos+=16;

     memcpy((void*)&reserved[rpos], (void*)module_nds_data->advert.hostname,(size_t)(module_nds_data->advert.hostname_len*2));
     rpos+=((int)module_nds_data->advert.hostname_len*2);
     for(int i=0; i<32-(rpos-16); i++)
     reserved[rpos + i] = 0x00;
     rpos+= 32-(rpos-16);

     for(int i=0; i<16; i++)
     reserved[rpos + i] = 0x2D;

     ndshdr.logoCRC16 = CalcCRC16(ndshdr.gbaLogo,156);
     ndshdr.headerCRC16 = CalcCRC16((unsigned char*)&ndshdr,0x15E);
     fwrite(&ndshdr,1,sizeof(TNDSHeader),nds);
     fwrite(download_play,1,32,nds);

     //memcpy(&ndshdr,&nds_data.header,sizeof(TNDSHeader));
     ndshdr.bannerOffset = temp1;
     ndshdr.romSize = temp2;
     ndshdr.logoCRC16 = CalcCRC16(ndshdr.gbaLogo,156);
     ndshdr.headerCRC16 = CalcCRC16((unsigned char*)&ndshdr,0x15E);
     memcpy(ndshdr.zero,reserved,160);
     fwrite(&ndshdr,1,sizeof(TNDSHeader),nds);
     fflush(nds);
     free(download_play);

     sz=((((int)module_nds_data->header.arm9romSource)-((int)ftell(nds))));
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;

     temp=0;
     int writtenBytes=0;

     fwrite((void*)&module_nds_data->saved_data[0],1,module_nds_data->arm7s,nds);

     #ifdef DUMPARMBINARIES
     fdump = fopen("arm9.bin", "wb");
     fwrite((void*)&module_nds_data->saved_data[0],1,module_nds_data->arm7s,fdump);
     fclose(fdump);
     #endif

     sz=((int)module_nds_data->header.arm7romSource-(int)ftell(nds));
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;

     writtenBytes=0;
     if(module_nds_data->arm7e > module_nds_data->arm7s)
     {
        fwrite((void*)&module_nds_data->saved_data[module_nds_data->arm7s],1,module_nds_data->arm7e - module_nds_data->arm7s,nds);
     
        #ifdef DUMPARMBINARIES
        fdump = fopen("arm7.bin", "wb");
        fwrite((void*)&module_nds_data->saved_data[module_nds_data->arm7s],1,module_nds_data->arm7e - module_nds_data->arm7s,fdump);
        fclose(fdump);
        #endif
     }

            if(module_nds_data->header.bannerOffset==0)
            {
                printf("ERROR! BANNER OFFSET WRONG!\n");
                return 0;
            }

     sz=((int)module_nds_data->header.bannerOffset-((int)ftell(nds)));

     if(sz>0)
     {

        Temp = new unsigned char[sz];

        memset(Temp,0,sz);

        //This following block is based on code from the sachen program.
        if(module_nds_data->pkt_size!=250)
        {
            if (( ((module_nds_data->header.bannerOffset - 0x200) > (module_nds_data->header.arm7romSource + module_nds_data->header.arm7binarySize)) ))
            {
                //No clue what this is, but Firefly adds this, so...
                unsigned char unknownData[] = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };
                memcpy(&Temp[sz-0x200],unknownData,7);
            }
        }

     fwrite(Temp,1,sz,nds);

     delete[] Temp;
     }

     fwrite(&banner,1,sizeof(TNDSBanner),nds);

     if(module_nds_data->header.romSize!=module_nds_data->header.bannerOffset+2112)
            {
                printf("ERROR! ROM SIZE WRONG!\n");
                return 0;
            }

     if((module_nds_data->header.bannerOffset+sizeof(TNDSBanner)) < module_nds_data->header.romSize)
     {
     sz=((module_nds_data->header.romSize)-(module_nds_data->header.bannerOffset+sizeof(TNDSBanner)));
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;
     }

     fwrite((void*)&module_nds_data->rsa.signature,1,136,nds);

     fflush(nds);

     //Calculate the Secure CRC16 in the header, and write the checksum there
     int buffer_size = 0x8000 - 0x4000;
     unsigned short checksum16 = 0;
     unsigned char *buffer = (unsigned char*)malloc(buffer_size);
     memset(buffer,0,buffer_size);
     fseek(nds,0x4000,SEEK_SET);
     fread(buffer,1,buffer_size,nds);
     checksum16 = CalcCRC16(buffer,buffer_size);
     fseek(nds,0x06C,SEEK_SET);
     fwrite(&checksum16,1,sizeof(unsigned short),nds);
     free(buffer);
     fflush(nds);

     //Read the header
     fseek(nds,0x00,SEEK_SET);
     buffer_size = 350;
     buffer = (unsigned char*)malloc(buffer_size);
     memset(buffer,0,350);
     fread(buffer,1,350,nds);

     //Redo the header CRC16
     fseek(nds,0x015E,SEEK_SET);
     checksum16 = CalcCRC16(buffer,buffer_size);
     fwrite(&checksum16,1,sizeof(unsigned short),nds);

     free(buffer);
     fclose(nds);

     module_nds_data->beacon_thing=0;
     module_nds_data->multipleIDs=0;
     module_nds_data->handledIDs[(int)module_nds_data->gameID] = 1;
     prev_total=0;

     if(funusedpkt!=NULL && module_nds_data->multipleIDs)
     {
            fclose(funusedpkt);
            funusedpkt=NULL;
            remove("unused_packets.bin");
     }

		if(*DEBUG)
		{
			fprintfdebug(*Log,"ASSEMBLY SUCCESSFUL\n");
			fflushdebug(*Log);
		}

     return 1;
}
