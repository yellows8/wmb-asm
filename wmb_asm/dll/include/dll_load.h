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

#ifndef BUILDING_DLL

    #ifndef NDS
        
        #ifdef DLLMAIN
            
            #ifdef WIN32
                
                HINSTANCE dllHandle;
                HMODULE hmodule;
            
            #endif

                #ifdef unix
                void* dllHandle;
                #endif

                    bool LoadDLL(const char *filename, char *error_buffer = NULL)
                    {
                        if(error_buffer!=NULL)
                        strcpy(error_buffer,"");
                        
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
                                dllHandle = LoadLibrary(modname);
                                
                                if(dllHandle!=NULL)
                                hmodule = (HMODULE)dllHandle;
    
                                if(error_buffer!=NULL && dllHandle==NULL)sprintf(error_buffer,"Failed to open module %s\n",modname);
                            #endif
    
                                    #ifdef unix
                                        dllHandle = dlopen(modname,RTLD_NOW);
                                        
                                        if(error_buffer!=NULL && dllHandle==NULL)sprintf(error_buffer,"Failed to open shared library %s\n",modname);
                                    #endif
    
                                            #ifndef WIN32
                                                #ifndef unix
                                                    if(error_buffer!=NULL)strcpy(error_buffer,"DLL loading not supported on this platform. Stop.");
                                                #endif
                                            #endif
    
	                                                   #ifndef NDS
                                                            if(dllHandle==NULL)return 0;
                                                        #endif
	
	#ifdef NDS
	return 0;
	#endif
	
    return 1;
}

bool CloseDLL(char *error_buffer = NULL)
{
    if(error_buffer!=NULL)
    strcpy(error_buffer,"");
    
        #ifdef WIN32
            if(!FreeLibrary(hmodule))
            {
                if(error_buffer!=NULL)sprintf(error_buffer,"Failed to close the module.\n");
                return 0;
            }
        #endif
    
                    #ifdef unix
                        if(dlclose(dllHandle)!=0)
                        {
                            if(error_buffer!=NULL)sprintf(error_buffer,"Failed to close the shared library.\n");
                            return 0;
                        }
                    #endif
    
    return 1;
}

void* LoadFunctionDLL(const char *func_name, char *error_buffer = NULL)
{
    void* func_addr=NULL;
    
    if(error_buffer!=NULL)
    strcpy(error_buffer,"");
    
        #ifdef WIN32
            func_addr = (void*)GetProcAddress(dllHandle,func_name);
        #endif
    
                #ifdef unix
                    func_addr = dlsym(dllHandle,func_name);
                #endif
    
    if(error_buffer!=NULL && func_addr==NULL)sprintf(error_buffer,"Failed to load function %s in the module.\n",func_name);
    
    return func_addr;
}

bool LoadAsmDLL(const char *filename ,char *error_buffer = NULL)
{
    if(filename==NULL)return 0;
    
    if(!LoadDLL(filename, error_buffer))return 0;
    
	   #ifndef NDS
	           //Why the strange function names you ask? I'm even sure why my compiler/IDE does this... Maybe because a def file isn't created?
	           //If you get errors with "failed to load function [...] in module", when you compiled it yourself, try undecorating these names.
            if((HandlePacket=(lpHandlePacket)LoadFunctionDLL("_Z12HandlePacketP12spcap_pkthdrPhiPPcP7spcap_tbS2_bS2_b", error_buffer))==NULL);
            if((InitAsm=(lpInitAsm)LoadFunctionDLL("_Z7InitAsmPFvvEb", error_buffer))==NULL)return 0;
            if((ResetAsm=(lpResetAsm)LoadFunctionDLL("_Z8ResetAsmv", error_buffer))==NULL)return 0;
            if((ExitAsm=(lpExitAsm)LoadFunctionDLL("_Z7ExitAsmv", error_buffer))==NULL)return 0;
            if((CaptureAsmReset=(lpCaptureAsmReset)LoadFunctionDLL("_Z15CaptureAsmResetPi", error_buffer))==NULL)return 0;
            if((GetStatusAsm=(lpGetStatusAsm)LoadFunctionDLL("_Z12GetStatusAsmPi", error_buffer))==NULL)return 0;
            if((QueryAssembleStatus=(lpQueryAssembleStatus)LoadFunctionDLL("_Z19QueryAssembleStatusPi", error_buffer))==NULL)return 0;
            if((GetPrecentageCompleteAsm=(lpGetPrecentageCompleteAsm)LoadFunctionDLL("_Z24GetPrecentageCompleteAsmv", error_buffer))==NULL)return 0;
            if((ConvertEndian=(lpConvertEndian)LoadFunctionDLL("_Z13ConvertEndianPvS_i", error_buffer))==NULL)return 0;
            if((CheckEndianA=(lpCheckEndianA)LoadFunctionDLL("_Z12CheckEndianAPvi", error_buffer))==NULL)return 0;
            if((GetModuleVersionStr=(lpGetModuleVersionStr)LoadFunctionDLL("_Z19GetModuleVersionStrv", error_buffer))==NULL)return 0;
            if((GetModuleVersionInt=(lpGetModuleVersionInt)LoadFunctionDLL("_Z19GetModuleVersionInti", error_buffer))==NULL)return 0;
            
            if((pcap_open_offline=(lppcap_open_offline)LoadFunctionDLL("_Z17pcap_open_offlinePKcPc", error_buffer))==NULL)return 0;
            if((pcap_next_ex=(lppcap_next_ex)LoadFunctionDLL("_Z12pcap_next_exP7spcap_tPP12spcap_pkthdrPPKh", error_buffer))==NULL)return 0;
            if((pcap_close=(lppcap_close)LoadFunctionDLL("_Z10pcap_closeP7spcap_t", error_buffer))==NULL)return 0;
            if((pcap_geterr=(lppcap_geterr)LoadFunctionDLL("_Z11pcap_geterrP7spcap_t", error_buffer))==NULL)return 0;
            if((GetPacketNumber=(lpGetPacketNumber)LoadFunctionDLL("_Z15GetPacketNumberv", error_buffer))==NULL)return 0;
            if((GetSnapshotLength=(lpGetSnapshotLength)LoadFunctionDLL("_Z17GetSnapshotLengthv", error_buffer))==NULL)return 0;
            
            if((opendir=(lpopendir)LoadFunctionDLL("opendir", error_buffer))==NULL)return 0;
            if((closedir=(lpclosedir)LoadFunctionDLL("closedir", error_buffer))==NULL)return 0;
            if((readdir=(lpreaddir)LoadFunctionDLL("readdir", error_buffer))==NULL)return 0;
            if((rewinddir=(lprewinddir)LoadFunctionDLL("rewinddir", error_buffer))==NULL)return 0;
            
            if((ScanDirectory=(lpScanDirectory)LoadFunctionDLL("_Z13ScanDirectoryP10TFILE_LISTPcS1_", error_buffer))==NULL)return 0;
            if((FreeDirectory=(lpFreeDirectory)LoadFunctionDLL("_Z13FreeDirectoryP10TFILE_LIST", error_buffer))==NULL)return 0;
        #endif
	
	           #ifdef NDS
	               return 0;
	           #endif
	
    return 1;
}

bool CloseAsmDLL(char *error_buffer = NULL)
{
    return CloseDLL(error_buffer);
}
        #endif

    #endif

#endif
