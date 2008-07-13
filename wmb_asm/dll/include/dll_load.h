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

    #ifndef NDS
        
        #ifdef WIN32
            typedef HMODULE LPDLL;
        #endif

        #ifdef unix
            typedef void* LPDLL;
        #endif

        #ifdef DLLMAIN

                #ifndef BUILDING_DLL
                    LPDLL AsmDLL=0;
                #endif

                    bool LoadDLL(LPDLL *lpdll, const char *filename, char *error_buffer = NULL)
                    {
                        //if(error_buffer!=NULL)
                        //strcpy(error_buffer,"");
                        
                        char modname[256];
                        memset(modname,0,256);
                        strcpy(modname,filename);
                        
                        bool found=0;
                        int pos=strlen(filename);
                        
                        for(int i=2; i<(int)strlen(filename); i++)
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

bool CloseDLL(LPDLL *lpdll, char *error_buffer = NULL)
{
    //if(error_buffer!=NULL)
    //strcpy(error_buffer,"");
    
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

void* LoadFunctionDLL(LPDLL *lpdll, const char *func_name, const char *func_name2, char *error_buffer = NULL)
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

#ifndef BUILDING_DLL
bool LoadAsmDLL(const char *filename ,char *error_buffer = NULL)
{
    if(filename==NULL)return 0;
    
    AsmDLL = 0;
    
    if(!LoadDLL(&AsmDLL, filename, error_buffer))return 0;
    
	   #ifndef NDS
	           //Why the strange function names you ask? I'm even sure why my compiler/IDE does this... Maybe because a def file isn't created?
	           //If you get errors with "failed to load function [...] in module", when you compiled it yourself, try undecorating these names.
            HandlePacket=(lpHandlePacket)LoadFunctionDLL(&AsmDLL, "HandlePacket", "_Z12HandlePacketP12spcap_pkthdrPhiPPcP7spcap_tbS2_bS2_b", error_buffer);if((HandlePacket)==NULL) return 0;
            InitAsm=(lpInitAsm)LoadFunctionDLL(&AsmDLL, "InitAsm", "_Z7InitAsmPFvvEb", error_buffer);if((InitAsm)==NULL)return 0;
            ResetAsm=(lpResetAsm)LoadFunctionDLL(&AsmDLL, "ResetAsm", "_Z8ResetAsmv", error_buffer);if((ResetAsm)==NULL)return 0;
            ExitAsm=(lpExitAsm)LoadFunctionDLL(&AsmDLL, "ExitAsm", "_Z7ExitAsmv", error_buffer);if((ExitAsm)==NULL)return 0;
            CaptureAsmReset=(lpCaptureAsmReset)LoadFunctionDLL(&AsmDLL, "CaptureAsmReset", "_Z15CaptureAsmResetPi", error_buffer);if((CaptureAsmReset)==NULL)return 0;
            GetStatusAsm=(lpGetStatusAsm)LoadFunctionDLL(&AsmDLL, "GetStatusAsm", "_Z12GetStatusAsmPi", error_buffer);if((GetStatusAsm)==NULL)return 0;
            QueryAssembleStatus=(lpQueryAssembleStatus)LoadFunctionDLL(&AsmDLL, "QueryAssembleStatus", "_Z19QueryAssembleStatusPi", error_buffer);if((QueryAssembleStatus)==NULL)return 0;
            GetPrecentageCompleteAsm=(lpGetPrecentageCompleteAsm)LoadFunctionDLL(&AsmDLL, "GetPrecentageCompleteAsm", "_Z24GetPrecentageCompleteAsmv", error_buffer);if((GetPrecentageCompleteAsm)==NULL)return 0;
            ConvertEndian=(lpConvertEndian)LoadFunctionDLL(&AsmDLL, "ConvertEndian", "_Z13ConvertEndianPvS_i", error_buffer);if((ConvertEndian)==NULL)return 0;
            CheckEndianA=(lpCheckEndianA)LoadFunctionDLL(&AsmDLL, "CheckEndianA", "_Z12CheckEndianAPvi", error_buffer);if((CheckEndianA)==NULL)return 0;
            GetModuleVersionStr=(lpGetModuleVersionStr)LoadFunctionDLL(&AsmDLL, "GetModuleVersionStr", "_Z19GetModuleVersionStrv", error_buffer);if((GetModuleVersionStr)==NULL)return 0;
            GetModuleVersionInt=(lpGetModuleVersionInt)LoadFunctionDLL(&AsmDLL, "GetModuleVersionInt", "_Z19GetModuleVersionInti", error_buffer);if((GetModuleVersionInt)==NULL)return 0;
            
            pcap_open_offline=(lppcap_open_offline)LoadFunctionDLL(&AsmDLL, "pcap_open_offline", "_Z17pcap_open_offlinePKcPc", error_buffer);if((pcap_open_offline)==NULL)return 0;
            pcap_next_ex=(lppcap_next_ex)LoadFunctionDLL(&AsmDLL, "pcap_next_ex", "_Z12pcap_next_exP7spcap_tPP12spcap_pkthdrPPKh", error_buffer);if((pcap_next_ex)==NULL)return 0;
            pcap_close=(lppcap_close)LoadFunctionDLL(&AsmDLL, "pcap_close", "_Z10pcap_closeP7spcap_t", error_buffer);if((pcap_close)==NULL)return 0;
            pcap_geterr=(lppcap_geterr)LoadFunctionDLL(&AsmDLL, "pcap_geterr", "_Z11pcap_geterrP7spcap_t", error_buffer);if((pcap_geterr)==NULL)return 0;
            GetPacketNumber=(lpGetPacketNumber)LoadFunctionDLL(&AsmDLL, "GetPacketNumber", "_Z15GetPacketNumberv", error_buffer);if((GetPacketNumber)==NULL)return 0;
            GetSnapshotLength=(lpGetSnapshotLength)LoadFunctionDLL(&AsmDLL, "GetSnapshotLength", "_Z17GetSnapshotLengthv", error_buffer);if((GetSnapshotLength)==NULL)return 0;
            
            opendir=(lpopendir)LoadFunctionDLL(&AsmDLL, "opendir", NULL, error_buffer);if((opendir)==NULL)return 0;
            closedir=(lpclosedir)LoadFunctionDLL(&AsmDLL, "closedir", NULL, error_buffer);if((closedir)==NULL)return 0;
            readdir=(lpreaddir)LoadFunctionDLL(&AsmDLL, "readdir", NULL, error_buffer);if((readdir)==NULL)return 0;
            rewinddir=(lprewinddir)LoadFunctionDLL(&AsmDLL, "rewinddir", NULL, error_buffer);if((rewinddir)==NULL)return 0;
            
            ScanDirectory=(lpScanDirectory)LoadFunctionDLL(&AsmDLL, "ScanDirectory", "_Z13ScanDirectoryP10TFILE_LISTPcS1_", error_buffer);if((ScanDirectory)==NULL)return 0;
            FreeDirectory=(lpFreeDirectory)LoadFunctionDLL(&AsmDLL, "FreeDirectory", "_Z13FreeDirectoryP10TFILE_LIST", error_buffer);if((FreeDirectory)==NULL)return 0;
        #endif
	
	           #ifdef NDS
	               return 0;
	           #endif
	
    return 1;
}

bool CloseAsmDLL(char *error_buffer = NULL)
{
    return CloseDLL(&AsmDLL, error_buffer);
}

        #endif
        
        #endif

        #ifndef DLLMAIN
            bool LoadDLL(LPDLL *lpdll, const char *filename, char *error_buffer);
            bool CloseDLL(LPDLL *lpdll, char *error_buffer);
            void* LoadFunctionDLL(LPDLL *lpdll, const char *func_name, char *error_buffer);
        #endif
    
#endif
