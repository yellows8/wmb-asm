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

    #ifndef NDS
    
    #ifndef NDS
//extern "C" {
#endif
    
                    LPDLL AsmDLL=0;

                    bool LoadDLL(LPDLL *lpdll, const char *filename, char *error_buffer)
                    {
                        
                        char modname[256];
                        memset(modname,0,256);
                        strcpy(modname,filename);
                        
                        bool found=0;
                        int pos=strlen(filename);
                        int i;
                        
                        for(i=2; i<(int)strlen(filename); i++)
                        {
                            if(filename[i]=='.')
                            {
                                found=1;
                                pos=i;
                            }
                        }
                                    #ifdef WIN32
                                        strcpy(&modname[pos],".dll");
                                    #endif
                                    
                                            #ifdef unix
                                                strcpy(&modname[pos],".so");
                                            #endif
    
                            #ifdef WIN32
                                *lpdll = (LPDLL)LoadLibrary(modname);
    
                                if(error_buffer!=NULL && *lpdll==NULL)sprintf(error_buffer,"Failed to open module %s\n",modname);
                            #endif
    
                                    #ifdef unix
                                        lpdll = dlopen(modname,RTLD_NOW);
                                        
                                        if(error_buffer!=NULL && dllHandle==NULL)sprintf(error_buffer,"Failed to open shared library %s\n",modname);
                                    #endif
    
                                            #ifndef WIN32
                                                #ifndef unix
                                                    if(error_buffer!=NULL)strcpy(error_buffer,"DLL loading not supported on this platform. Stop.");
                                                #endif
                                            #endif
    
	                                                   #ifndef NDS
                                                            if(*lpdll==NULL)return 0;
                                                        #endif
	
	#ifdef NDS
	return 0;
	#endif
	
    return 1;
}

bool CloseDLL(LPDLL *lpdll, char *error_buffer)
{
    
        #ifdef WIN32
            if(!FreeLibrary(*lpdll))
            {
                if(error_buffer!=NULL)sprintf(error_buffer,"Failed to close the module.\n");
                return 0;
            }
        #endif
    
                    #ifdef unix
                        if(dlclose(lpdll)!=0)
                        {
                            if(error_buffer!=NULL)sprintf(error_buffer,"Failed to close the shared library.\n");
                            return 0;
                        }
                    #endif
    
    return 1;
}

void* LoadFunctionDLL(LPDLL *lpdll, const char *func_name, const char *func_name2, char *error_buffer)
{
    void* func_addr=NULL;
    char *f_name = NULL;
    
    if(func_addr==NULL && func_name!=NULL)
    {
        f_name = (char*)func_name;
        
        #ifdef WIN32
            func_addr = (void*)GetProcAddress((HINSTANCE)*lpdll,f_name);
        #endif
    
                #ifdef unix
                    func_addr = dlsym(lpdll,f_name);
                #endif
                
                
    }
    
    if(func_addr==NULL && func_name2!=NULL)
    {
        f_name = (char*)func_name2;
        #ifdef WIN32
            func_addr = (void*)GetProcAddress((HINSTANCE)*lpdll,f_name);
        #endif

                #ifdef unix
                    func_addr = dlsym(lpdll,f_name);
                #endif
    }
    
    if(error_buffer!=NULL && func_addr==NULL)
    {
        sprintf(error_buffer ,"Failed to load function %s in the module.\n",f_name);
    }
    
    return func_addr;
}

#endif

bool LoadAsmDLL(const char *filename, struct sAsmSDK_Config *config, char *error_buffer)
{
    if(filename==NULL || config==NULL)return 0;
    
	#ifndef NDS
	
    AsmDLL = 0;
    
    if(!LoadDLL(&AsmDLL, filename, error_buffer))return 0;
    
	   
	        
	        
            
            config->HandlePacket=(lpHandlePacket)LoadFunctionDLL(&AsmDLL, "HandlePacket", "_Z12HandlePacketP12spcap_pkthdrPhiPPcP7spcap_tbS2_bS2_b", error_buffer);if((config->HandlePacket)==NULL) return 0;
            config->InitAsm=(lpInitAsm)LoadFunctionDLL(&AsmDLL, "InitAsm", "_Z7InitAsmPFvvEb", error_buffer);if((config->InitAsm)==NULL)return 0;
            config->ResetAsm=(lpResetAsm)LoadFunctionDLL(&AsmDLL, "ResetAsm", "_Z8ResetAsmv", error_buffer);if((config->ResetAsm)==NULL)return 0;
            config->ExitAsm=(lpExitAsm)LoadFunctionDLL(&AsmDLL, "ExitAsm", "_Z7ExitAsmv", error_buffer);if((config->ExitAsm)==NULL)return 0;
            config->CaptureAsmReset=(lpCaptureAsmReset)LoadFunctionDLL(&AsmDLL, "CaptureAsmReset", "_Z15CaptureAsmResetPi", error_buffer);if((config->CaptureAsmReset)==NULL)return 0;
            config->GetStatusAsm=(lpGetStatusAsm)LoadFunctionDLL(&AsmDLL, "GetStatusAsm", "_Z12GetStatusAsmPi", error_buffer);if((config->GetStatusAsm)==NULL)return 0;
            config->QueryAssembleStatus=(lpQueryAssembleStatus)LoadFunctionDLL(&AsmDLL, "QueryAssembleStatus", "_Z19QueryAssembleStatusPi", error_buffer);if((config->QueryAssembleStatus)==NULL)return 0;
            config->GetPrecentageCompleteAsm=(lpGetPrecentageCompleteAsm)LoadFunctionDLL(&AsmDLL, "GetPrecentageCompleteAsm", "_Z24GetPrecentageCompleteAsmv", error_buffer);if((config->GetPrecentageCompleteAsm)==NULL)return 0;
            config->GetModuleVersionStr=(lpGetModuleVersionStr)LoadFunctionDLL(&AsmDLL, "GetModuleVersionStr", "_Z19GetModuleVersionStrv", error_buffer);if((config->GetModuleVersionStr)==NULL)return 0;
            config->GetModuleVersionInt=(lpGetModuleVersionInt)LoadFunctionDLL(&AsmDLL, "GetModuleVersionInt", "_Z19GetModuleVersionInti", error_buffer);if((config->GetModuleVersionInt)==NULL)return 0;
            config->SwitchMode=(lpSwitchMode)LoadFunctionDLL(&AsmDLL, "SwitchMode", NULL, error_buffer);if((config->SwitchMode)==NULL)return 0;
            config->SelectPacketModule=(lpSelectPacketModule)LoadFunctionDLL(&AsmDLL, "SelectPacketModule", NULL, error_buffer);if((config->SelectPacketModule)==NULL)return 0;
            config->GetPacketModules=(lpGetPacketModules)LoadFunctionDLL(&AsmDLL, "GetPacketModules", NULL, error_buffer);if((config->GetPacketModules)==NULL)return 0;
            config->GetTotalPacketModules=(lpGetTotalPacketModules)LoadFunctionDLL(&AsmDLL, "GetTotalPacketModules", NULL, error_buffer);if((config->GetTotalPacketModules)==NULL)return 0;
            config->GetPacketModuleID=(lpGetPacketModuleID)LoadFunctionDLL(&AsmDLL, "GetPacketModuleID", NULL, error_buffer);if((config->GetPacketModuleID)==NULL)return 0;
            config->GetPacketModuleIDStr=(lpGetPacketModuleIDStr)LoadFunctionDLL(&AsmDLL, "GetPacketModuleIDStr", NULL, error_buffer);if((config->GetPacketModuleIDStr)==NULL)return 0;
            
            
    #endif
        
        ND_DAT = (struct Nds_data*)malloc(sizeof(struct Nds_data));        
        config->nds_data = (volatile struct Nds_data**)&ND_DAT;
    memset((void*)*config->nds_data, 0, sizeof(struct Nds_data));

    config->DEBUG = (bool*)malloc(sizeof(bool));

        *config->DEBUG = 0;

        config->Log = (FILE**)malloc(sizeof(FILE*));
        
        sdk_nds_data = config->nds_data;
        SDK_DEBUG = config->DEBUG;
        SDK_Log = config->Log;
        SDK_CONFIG = config;
        
            //printf("A CONFIG DAT %p CONFIG %p\n", config->nds_data, sdk_nds_data);
            //printf("CONFIG %p  SDKC %p\n",config, SDK_CONFIG);
        
    return 1;
}

#ifndef NDS

bool CloseAsmDLL(char *error_buffer)
{
    return CloseDLL(&AsmDLL, error_buffer);
}

#ifndef NDS
//}
#endif

#endif
