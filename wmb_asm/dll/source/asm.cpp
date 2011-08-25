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
#define BUILDING_DLL
#endif
#include "..\include\wmb_asm.h"

#define MAX_PKT_MODULES 255

SuccessCallback AssemblySuccessCallback = NULL;

void CheckEndianA(void* input, int input_length);//This one must be called first, with AVS header length as input
void ConvertEndian(void* input, void* output, int input_length);//Only call this after both of the above are called.
//This converts the Endian of the input to the Endian of the current machine
void ConvertAVSEndian(AVS_header *avs);

void ChangeFilename(char *in, char *out, char *filename);
bool AssembleNds(char *output);
unsigned char* IsValidAVS(u_char *pkt_data);
bool Handle802_11(unsigned char *data, int length);

unsigned int CalcCRC32(unsigned char *data, unsigned int length);
unsigned short CalcCRC16(unsigned char *data, unsigned int length);

void WMBReset();
bool WMBHandle802_11(unsigned char *data, int length);

int GetFileSize(FILE* _pfile);

void CaptureAsmReset();

bool HandlePacket(pcap_pkthdr *header, u_char *pkt_data, int length, char *argv[], pcap_t *fp, bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir, bool *Has_AVS);

char *GetStatusAsm(int *error_code);

#ifdef WIN32
void ExecuteApp(char *appname, char *cmdline);
#endif

FILE *Log = stdout;
bool debug=0;

Nds_data nds_data;

FILE *funusedpkt = NULL;
bool fpkt_error=0;
bool save_unused_packets=1;

bool *HAS_AVS;
pcap_pkthdr *glob_header;
char **glob_argv;
bool glob_checkrsa;
char *glob_outdir;
bool glob_run;
char *glob_copydir;
bool glob_use_copydir;

bool DEBUG = 0;

typedef bool (*lpHandle802_11)(unsigned char *data, int length);
typedef void (*lpReset)();

struct PacketModule
{
    lpHandle802_11 handle802_11;
    lpReset reset;
	#ifndef NDS
    LPDLL lpdll;//The handle of the module if this one isn't a built-in packet handler.
	#endif
};
PacketModule packetModules[MAX_PKT_MODULES];
int totalPacketModules = 0;
int currentPacketModule = -1;

void PktModClose()
{
	#ifndef NDS
		for(int i=1; i<totalPacketModules+1; i++)
		{
			if(packetModules[i].lpdll==NULL)break;

			CloseDLL(&packetModules[i].lpdll, NULL);
		}
	#endif
}

void PktModReset()
{
    for(int i=0; i<1; i++)
    {
        if(packetModules[i].reset!=NULL)
            packetModules[i].reset();
    }

	#ifndef NDS
		for(int i=1; i<totalPacketModules+1; i++)
		{
			if(packetModules[i].lpdll==NULL)break;

			if(packetModules[i].reset!=NULL)
				packetModules[i].reset();
		}
	#endif
}

bool PktModHandle802_11(unsigned char *data, int length)
{
    if(currentPacketModule == -1)
    {
        bool ret = 0;

        for(int i=0; i<1; i++)
        {
            if(packetModules[i].handle802_11!=NULL)
            {
                ret = packetModules[i].handle802_11(data, length);

                if(ret)
                {
                    currentPacketModule = i;
                    return 1;
                }
            }
        }

		#ifndef NDS
			for(int i=1; i<totalPacketModules+1; i++)
			{
				if(packetModules[i].lpdll==NULL)break;

				if(packetModules[i].handle802_11!=NULL)
				{
					ret = packetModules[i].handle802_11(data, length);

					if(ret)
					{
                    currentPacketModule = i;
                    return 1;
					}
				}
			}
		#endif

    }
    else
    {
        return packetModules[currentPacketModule].handle802_11(data, length);
    }

    return 0;
}

void InitPktModules()
{
    /*DIR *dir;
    dirent *ent;
    int skip=0;
    LPDLL lpdll;
	FILE *modlog = NULL;
    char error_buffer[256];
	char destr[256];*/
    char filename[256];
    memset(filename, 0, 256);
	/*#ifndef NDS
    memset(error_buffer, 0, 256);
    memset(destr, 0, 256);
	#endif*/

    memset(packetModules, 0, sizeof(PacketModule) * MAX_PKT_MODULES);
    totalPacketModules = 0;
    currentPacketModule = -1;

    packetModules[0].reset = &WMBReset;
    packetModules[0].handle802_11 = &WMBHandle802_11;

    PktModReset();
}

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
			if(DEBUG)
			{
				fprintf(Log,"CHECKSUM %d FILE %d\n",(int)checksum,(int)*fcheck);
				fflushdebug(Log);
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

DLLIMPORT void InitAsm(SuccessCallback callback, bool debug)
{
    DEBUG = debug;

		if(DEBUG)
		{
			Log = fopendebug("log.txt","w");
        }

    stage=STAGE_BEACON;
	memset(host_mac,0,6);
	memset(client_mac,0,6);
	memset(&nds_data,0,sizeof(Nds_data));
	save_unused_packets=1;
	funusedpkt=NULL;

    InitPktModules();

	AssemblySuccessCallback = callback;
}

DLLIMPORT void ResetAsm()
{
                        if(nds_data.saved_data!=NULL)free(nds_data.saved_data);
                        if(nds_data.data_sizes!=NULL)free(nds_data.data_sizes);
                        nds_data.saved_data=NULL;
                        nds_data.data_sizes=NULL;

                               stage=STAGE_BEACON;
	                           memset(host_mac,0,6);
	                           memset(client_mac,0,6);

                                if(DEBUG)
                                    fflushdebug(Log);

	                           TNDSHeader temp_header;
	                           nds_rsaframe temp_rsa;
	                           bool first=nds_data.finished_first_assembly;
                               unsigned short temp_checksums[10];
                               ds_advert temp_advert1;
                               //bool beacon_thing=nds_data.beacon_thing;
                               bool multipleIDs = nds_data.multipleIDs;
                               bool handledIDs[256];
                               bool FoundGameIDs = 0;

	                           if(nds_data.finished_first_assembly)
	                           {
                                    memcpy(&temp_header,&nds_data.header,sizeof(TNDSHeader));
									memcpy(&temp_rsa,&nds_data.rsa,sizeof(nds_rsaframe));
									memcpy(temp_checksums,nds_data.beacon_checksum,20);
									memcpy(&temp_advert1,&nds_data.oldadvert,sizeof(ds_advert));
									//memcpy(&temp_advert2,&nds_data.advert,sizeof(ds_advert));
									//nds_data.beacon_thing=beacon_thing;
									nds_data.multipleIDs = multipleIDs;
									memcpy(handledIDs,nds_data.handledIDs,256);
									FoundGameIDs = nds_data.FoundGameIDs;
                               }

	                           memset(&nds_data,0,sizeof(Nds_data));
	                           nds_data.finished_first_assembly=first;
	                           if(nds_data.finished_first_assembly)
	                           {
                                    memcpy(&nds_data.header,&temp_header,sizeof(TNDSHeader));
									memcpy(&nds_data.rsa,&temp_rsa,sizeof(nds_rsaframe));
									memcpy(nds_data.beacon_checksum,temp_checksums,20);
									memcpy(&nds_data.oldadvert,&temp_advert1,sizeof(ds_advert));
									//memcpy(&nds_data.advert,&temp_advert2,sizeof(ds_advert));
									//beacon_thing=nds_data.beacon_thing;
									multipleIDs = nds_data.multipleIDs;
									memcpy(nds_data.handledIDs,handledIDs,256);
									nds_data.FoundGameIDs = FoundGameIDs;
                               }

                               currentPacketModule = -1;
	                           PktModReset();
}

DLLIMPORT void ExitAsm()
{
    if(funusedpkt!=NULL)
    {
    fclose(funusedpkt);
    funusedpkt=NULL;
    }

		#ifndef NDS
			remove("unused_packets.bin");
		#endif

				if(DEBUG)
					fclosedebug(Log);

    ResetAsm();

    PktModClose();
}

int captureStage=0;
int total_assembled=0;
int prev_total=0;
void CaptureBacktrack()
{
    //Used mainly with multi-game captures. This is done because some packets that the program will not find, may have already been transmitted & captured previously in the capture.

    //if(captureStage==stage)return;

    captureStage=stage;
    total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(nds_data.handledIDs[i])total_assembled++;
        }
    prev_total=total_assembled;

    //if(stage>STAGE_BEACON && stage<STAGE_ASSEMBLE)
        //{
            if(funusedpkt!=NULL)
            fclose(funusedpkt);

            funusedpkt = fopen("unused_packets.bin","rb");
            int length=0;
            unsigned char *buffer = (unsigned char*)malloc((size_t)GetSnapshotLength());
            memset(buffer,0,(size_t)GetSnapshotLength());
            save_unused_packets=0;//HandlePacket would write the packets we send to it, to this file we are reading, if this wasn't set to zero. Which would obviously cause problems.

                if(funusedpkt!=NULL)
                {
                    bool allocate = 0;
                    if(HAS_AVS==NULL)//Should never happen, but just in case, prevent crashes due to access violations.
                    {
                        allocate = 1;
                        HAS_AVS = (bool*)malloc(sizeof(bool));
                        *HAS_AVS = 1;
                    }
							if(DEBUG)
							{
								fprintfdebug(Log,"Backtracking through unused packets...\n");
								fflushdebug(Log);
							}

                        while(feof(funusedpkt)==0)
                        {
                            if(fread(&length,1,4,funusedpkt)!=4)break;
                            if(fread(buffer,1,(size_t)length,funusedpkt)!=(size_t)length)break;

                            HandlePacket(glob_header,buffer,length,glob_argv,NULL,glob_checkrsa,glob_outdir,glob_run,glob_copydir,glob_use_copydir, HAS_AVS);
                        }

                    fclose(funusedpkt);
                    funusedpkt=NULL;
                    save_unused_packets=1;

                    if(allocate)free(HAS_AVS);
                }

            free(buffer);


        total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(nds_data.handledIDs[i])total_assembled++;
        }


        if(total_assembled==prev_total && captureStage==stage)return;

        CaptureBacktrack();
}

DLLIMPORT char *CaptureAsmReset(int *code)//Needs to be called after reading the whole capture.
{
    char *str = NULL;
    if(funusedpkt!=NULL && nds_data.multipleIDs)
    {
        fclose(funusedpkt);
        funusedpkt=NULL;

        captureStage=0;
        CaptureBacktrack();

        total_assembled=0;
        for(int i=0; i<15; i++)
        {
            if(nds_data.handledIDs[i])total_assembled++;
        }

        bool got_all=1;
        int total_asm=0, total=0;
        for(int i=0; i<15; i++)
        {
            if(!nds_data.handledIDs[i] && nds_data.foundIDs[i])got_all=0;
            if(nds_data.foundIDs[i])total++;
            if(nds_data.handledIDs[i])total_asm++;
        }

        if(got_all)
        {
            printf("Successfully assembled all %d demos in the capture.\n",total);

                if(DEBUG)
                {
			         fprintfdebug(Log,"Successfully assembled all %d demos in the capture.\n",total);
			    }
        }
        else
        {
            if(stage<=STAGE_HEADER)
            printf("Capture is incomplete.\n");

            printf("Failed to assemble all %d demos in the capture; %d out of %d demos in the capture assembled.\n",total,total_asm,total);

				if(DEBUG)
					fprintfdebug(Log,"Failed to assemble all %d demos in the capture; %d out of %d demos in the capture assembled.\n",total,total_asm,total);
        }

				if(DEBUG)
					fflushdebug(Log);

			#ifndef NDS
				funusedpkt = fopen("unused_packets.bin","wb");
			#endif
    }

    memset(nds_data.handledIDs,0,256);
    memset(nds_data.found_beacon,0,(10*15) * sizeof(int));
    nds_data.FoundGameIDs=0;

    str = GetStatusAsm(code);

    ResetAsm();

    return str;
}

DLLIMPORT char *GetStatusAsm(int *error_code)
{
    /*if(stage==STAGE_BEACON)
    {
        *error_code=STAGE_BEACON;
        return (char*)"01: Failed to finish the Beacon stage\n";
    }*/
    if(stage==STAGE_AUTH && !nds_data.finished_first_assembly)
    {
        *error_code=STAGE_AUTH;
        return (char*)"02: Failed to find the Authentication packet; Maybe the receiving DS never attempted to download?\n";
    }
    if(stage==STAGE_RSA)
    {
        *error_code=STAGE_RSA;
        return (char*)"03: Failed to find the RSA frame, try doing the capture again?\n";
    }
    if(stage==STAGE_HEADER)
    {
        *error_code=STAGE_HEADER;
        return (char*)"04: Failed to find the header. Major bugged capture! Try capturing again.\n";
    }
    if(stage==STAGE_DATA)
    {
        char *str;
        int missed=0;
        int found=0;
        str=(char*)malloc(256);
        *error_code=STAGE_DATA;

        if(nds_data.data_sizes != NULL)
        {
            for(int i=0; i<nds_data.arm7e_seq; i++)
            {
                if(nds_data.data_sizes[i]==0)
                {
                    missed++;
                }
                else
                {
                    found++;
                }
            }
        }

        sprintf(str,"05: Failed to find all the necessary data. Missed %d packets, got %d out of %d packets. %d percent done.\n",missed,found,nds_data.arm7e_seq,(int)GetPrecentageCompleteAsm());

        return str;
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT bool QueryAssembleStatus(int *error_code)
{
	*error_code = stage;
    return nds_data.finished_first_assembly;
}

DLLIMPORT unsigned char GetPrecentageCompleteAsm()
{
    unsigned char percent=0;

    int temp=0,temp2=0;
    int devisor=0;
    int size=490;
    int i=0;

        if(stage>=STAGE_DATA)
        {

                for(int I=0; I<(int)nds_data.total_binaries_size; I+=size)
                {
                    temp+=nds_data.data_sizes[i];
                    size=nds_data.data_sizes[i];

                    if(size==0)size=nds_data.pkt_size;
                    i++;
                }


            if(temp == 0)return 0;

            devisor = nds_data.total_binaries_size/100;
            temp2 = temp / devisor;
            percent = (unsigned char)ceil((double)temp2);
        }

    return percent;
}

int TheTime=0;
DLLIMPORT bool HandlePacket(pcap_pkthdr *header, u_char *pkt_data, int length, char *argv[], pcap_t *fp, bool checkrsa, char *outdir, bool run, char *copydir, bool use_copydir, bool *Has_AVS)
{
     //THE main assenbly function. All the host program of this module needs to do is load this module, call some functions at certain times, and call this function for each packet it reads/captures, and the assembler takes care of the rest.

     unsigned char *data = NULL;

     glob_header = header;
     glob_argv = argv;
     glob_checkrsa = checkrsa;
     glob_outdir = outdir;
     glob_run = run;
     glob_copydir = copydir;
     glob_use_copydir = use_copydir;

     //Is the AVS WLAN Capture header valid?
     HAS_AVS = Has_AVS;
     if(*HAS_AVS==1)
     data=IsValidAVS(pkt_data);

     if(*HAS_AVS==0)
     data = pkt_data;

     if(data)
     {
             Handle802_11(data,length-64);

                    #ifndef NDS
					if(save_unused_packets && nds_data.multipleIDs)
                    {
                        if(funusedpkt==NULL)
                        {
                            funusedpkt = fopen("unused_packets.bin","wb");
	                       fpkt_error=0;
	                       if(funusedpkt!=NULL)fpkt_error=1;
                        }

                        if(funusedpkt!=NULL)
                        {
                            fwrite(&length,1,4,funusedpkt);
                            fwrite(pkt_data,1,(size_t)length,funusedpkt);
                        }
                    }
					#endif

     }

     if(stage==STAGE_ASSEMBLE)
     {

                            char out[256];
                            char output[256];
                            int pos=0;
                            int I=0;
                            bool found=0;
	                        memset(out,0,256);
	                        memset(output,0,256);
                            strncpy(out,nds_data.header.gameTitle,12);
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
                            strncpy(&out[pos],nds_data.header.gameCode,4);
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

                            strncpy(&out[pos],nds_data.header.makercode,2);
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
                            sprintf(output,"%s%s",outdir,out);
                            //printf("OUTPUT %s OUTDIR %s OUT %s\n",output,outdir,out);


                             char *Str = new char[256];
                             strcpy(Str,output);

                             if(!AssembleNds(Str))
                             {
                             delete[]Str;
                             pcap_close(fp);

                                if(DEBUG)
                                {
	                               fclosedebug(Log);
                                   Log=NULL;
                                }

                             printf("Failure.\n");
	                         return 0;
                             }

                            if(AssemblySuccessCallback!=NULL)
	                           AssemblySuccessCallback();

                             nds_data.finished_first_assembly=1;
                             memcpy(&nds_data.oldadvert,&nds_data.advert,sizeof(ds_advert));
                             //memset(&nds_data.advert,0,sizeof(ds_advert));
                             memcpy(&nds_data.oldadvert,&nds_data.advert,sizeof(ds_advert));
                             ResetAsm();



	                           #ifdef WIN32
	                           if(checkrsa)
	                           {
	                           char rsa_cmdline[256];
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
                               }
                               #endif

                               if(copydir!=NULL && use_copydir)
                               {
                               char str[256];
                               unsigned char *buffer=NULL;
                               int filesize = 0;
                               sprintf(str,"%s%s",copydir,out);
                               FILE *f = fopen(Str,"rb");
                               if(f!=NULL)
                               {
                               filesize = GetFileSize(f);
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
                               if(run)
                               {
                                    char cline[256];
                                    sprintf(cline,"\"%s\"",output);
                                    printf("Executing %s\n",output);
                                    int err=0;
                                    err = (int)ShellExecute(NULL,NULL,output,NULL,NULL,SW_SHOW);
                               }
	                           #endif

	                           delete []Str;

	}

     return 1;

}

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
     ds_advert *ad = &nds_data.advert;
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
     memcpy(&ndshdr,&nds_data.header,sizeof(TNDSHeader));

     unsigned char *ptr;
     ptr = (unsigned char*)&nds_data.advert;

     int pos, dsize;

     for(int i=0; i<9; i++)
     {
     pos=i*98;
     if(i!=8)dsize=98;
     if(i==8)dsize=72;
     memcpy(&((unsigned char*)&nds_data.advert)[pos],&nds_data.beacon_data[(980*(int)nds_data.gameID)+pos],dsize);
     }

     memcpy(banner.palette,ad->icon_pallete,32);
     memcpy(banner.icon,ad->icon,512);
     memset(banner.titles,0,6*(128*2));
     int tempi=0;
     for(int i=0; i<48; i++)
     {
            if(nds_data.advert.game_name[i]==0)
            {
            break;
            }

            tempi+=2;
     }
     memcpy(&gdtemp[0],&nds_data.advert.game_name[0],(size_t)tempi);//Copy the game name into the banner discription, add a newline character after that, then copy in the actual discription.
     //tempi+=2;
     gdtemp[tempi] = '\n';
     tempi+=2;


     memcpy(&gdtemp[tempi],&nds_data.advert.game_description[0],96*2);
     //tempi+=96*2+2;

     for(int i=0; i<96; i++)
     {
            if(nds_data.advert.game_description[i]==0)
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

     nds=fopen(output,"w+b");
     if(nds==NULL)
     {
            printf("Failed to open output file for writing: %s\n",output);
            return 0;
     }

     unsigned int temp1, temp2;
     unsigned short temp3, temp4;
     temp1 = nds_data.header.bannerOffset;
     temp2 = nds_data.header.romSize;
     temp3 = nds_data.header.logoCRC16;
     temp4 = nds_data.header.headerCRC16;

            if(nds_data.header.bannerOffset==0)//Some demos don't set this fields properly. Fix these for the first header, then use the original header for the second header, so RSA isn't invalidated.
            {
                nds_data.header.bannerOffset = (unsigned int)((int)(((int)nds_data.header.arm7romSource) + ((int)nds_data.header.arm7binarySize)) + 660);
            }
            if(nds_data.header.romSize!=nds_data.header.bannerOffset+2112)
            {
                nds_data.header.romSize=(unsigned int)((int)nds_data.header.bannerOffset+2112);
            }

            memcpy(&ndshdr,&nds_data.header,sizeof(TNDSHeader));

     int rpos = 0;
     memset(reserved,0,160);
     for(int i=0; i<16; i++)
     reserved[i] = 0x2D;
     rpos+=16;

     memcpy(&reserved[rpos],(void*)nds_data.advert.hostname,(size_t)(nds_data.advert.hostname_len*2));
     rpos+=((int)nds_data.advert.hostname_len*2);
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

     sz=((((int)nds_data.header.arm9romSource)-((int)ftell(nds))));//72
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;

     temp=0;
     int writtenBytes=0;

     fwrite(&nds_data.saved_data[0],1,nds_data.arm7s,nds);

     sz=((int)nds_data.header.arm7romSource-(int)ftell(nds));
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;

     writtenBytes=0;
     if(nds_data.arm7e>nds_data.arm7s)
     {
        fwrite(&nds_data.saved_data[nds_data.arm7s],1,nds_data.arm7e - nds_data.arm7s,nds);
     }

            if(nds_data.header.bannerOffset==0)
            {
                printf("ERROR! BANNER OFFSET WRONG!\n");
                return 0;
            }



     sz=((int)nds_data.header.bannerOffset-((int)ftell(nds)));//(int)(nds_data.header.arm7romSource+nds_data.header.arm7binarySize)

     if(sz>0)
     {

        Temp = new unsigned char[sz];

        memset(Temp,0,sz);

        //This following block is based on code from the sachen program.
        if(nds_data.pkt_size!=250)
        {
            if (( ((nds_data.header.bannerOffset - 0x200) > (nds_data.header.arm7romSource + nds_data.header.arm7binarySize)) ))
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

     if(nds_data.header.romSize!=nds_data.header.bannerOffset+2112)
            {
                printf("ERROR! ROM SIZE WRONG!\n");
                return 0;
            }

     if((nds_data.header.bannerOffset+sizeof(TNDSBanner))<nds_data.header.romSize)
     {
     sz=((nds_data.header.romSize)-(nds_data.header.bannerOffset+sizeof(TNDSBanner)));
     Temp = new unsigned char[sz];
     memset(Temp,0,sz);
     fwrite(Temp,1,sz,nds);
     delete[] Temp;
     //system("PAUSE");
     //printf("SZ %d\n",sz);
     }

     fwrite(&nds_data.rsa.signature,1,136,nds);

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

     nds_data.beacon_thing=0;
     nds_data.multipleIDs=0;
     nds_data.handledIDs[(int)nds_data.gameID] = 1;
     prev_total=0;

     if(funusedpkt!=NULL && nds_data.multipleIDs)
     {
            fclose(funusedpkt);
            funusedpkt=NULL;
            remove("unused_packets.bin");
     }

		if(DEBUG)
		{
			fprintfdebug(Log,"ASSEMBLY SUCCESSFUL\n");
			fflushdebug(Log);
		}

     return 1;
}


