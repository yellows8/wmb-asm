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

unsigned char BroadcastMAC[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

int WMBProcessBeacons(unsigned char *data, int length);
int WMBProcessAuth(unsigned char *data, int length);
int WMBProcessRSA(unsigned char *data, int length);
int WMBProcessHeader(unsigned char *data, int length);
int WMBProcessData(unsigned char *data, int length);

bool CheckFrame(unsigned char *data, unsigned char command, unsigned short *size, unsigned char *pos);

int stage=SDK_STAGE_BEACON;

unsigned char host_mac[6];
unsigned char client_mac[6];

int last_seq=0;//Last seq in the data transfer

unsigned char *Nin_ie = NULL;

sAsmSDK_Config *WMBCONFIG = NULL;
bool *WMBDEBUG = NULL;
FILE **WMBLog = NULL;
FILE *wlog;

void Init();

#ifdef __cplusplus
  extern "C" {
#endif

#ifdef DLLIMPORT
	#ifdef NDS
		#undef DLLIMPORT
		#define DLLIMPORT
	#endif
#endif

//Change the names of the functions on-the-fly when compiling for DS, since everything is in one binary.
#ifdef NDS
	#define AsmPlug_GetID WMB_AsmPlug_GetID
	#define AsmPlug_GetIDStr WMB_AsmPlug_GetIDStr
	#define AsmPlug_GetPriority WMB_AsmPlug_GetPriority
	#define AsmPlug_GetStatus WMB_AsmPlug_GetStatus
	#define AsmPlug_QueryFailure WMB_AsmPlug_QueryFailure
	#define AsmPlug_Handle802_11 WMB_AsmPlug_Handle802_11
	#define AsmPlug_Init WMB_AsmPlug_Init
	#define AsmPlug_DeInit WMB_AsmPlug_DeInit
	#define AsmPlug_GetNdsData WMB_AsmPlug_GetNdsData
	#define AsmPlug_Reset WMB_AsmPlug_Reset
#endif

DLLIMPORT int AsmPlug_GetID()
{
    return 0;
}

DLLIMPORT char *AsmPlug_GetIDStr()
{
    return (char*)"WMB";
}

DLLIMPORT int AsmPlug_GetPriority()
{
    return ASMPLUG_PRI_LOW;//Why the lowest priority of all the default Wmb Asm plugins? The DS Download Station plugin needs to check some WMB packets. If the priority of this WMB plugin was normal, the DS Download Station plugin most likely wouldn't work correctly, if at all.
}

DLLIMPORT char *AsmPlug_GetStatus(int *error_code)
{
    
    if(stage==SDK_STAGE_AUTH && !nds_data->finished_first_assembly)
    {
        *error_code=SDK_STAGE_AUTH;
        return (char*)"02: Failed to find the Authentication packet; Maybe the receiving DS never attempted to download?\n";
    }
    if(stage==SDK_STAGE_RSA)
    {
        *error_code=SDK_STAGE_RSA;
        return (char*)"03: Failed to find the RSA frame, try doing the capture again?\n";
    }
    if(stage==SDK_STAGE_HEADER)
    {
        *error_code=SDK_STAGE_HEADER;
        return (char*)"04: Failed to find the header. Major bugged capture! Try capturing again.\n";
    }
    if(stage==SDK_STAGE_DATA)
    {
        char *str;
        int missed=0;
        int found=0;
        str=(char*)malloc(256);
        memset(str, 0, 256);
        *error_code=SDK_STAGE_DATA;

            if(nds_data!=NULL)
            {
                if(nds_data->data_sizes != NULL)
                {
                    for(int i=0; i<nds_data->arm7e_seq; i++)
                    {
                        if(nds_data->data_sizes[i]==0)
                        {
                            missed++;
                        }
                        else
                        {
                            found++;
                        }
                    }
                }
                
                sprintf(str,"05: Failed to find all the necessary data. Missed %d packets, got %d out of %d packets. %d percent done.\n",missed,found,nds_data->arm7e_seq,(int)GetPrecentageCompleteAsm());
            }

        return str;
    }

	*error_code=-1;
	return NULL;
}

DLLIMPORT int AsmPlug_QueryFailure()
{
    if(stage>=SDK_STAGE_DATA)return 2;
    if(stage==SDK_STAGE_HEADER)return 1;
    if(stage<SDK_STAGE_HEADER && stage>SDK_STAGE_BEACON)return 3;
    
    return 0;
}

DLLIMPORT int AsmPlug_Handle802_11(unsigned char *data, int length)
{
     if(stage==SDK_STAGE_BEACON)return WMBProcessBeacons(data,length);
     if(stage==SDK_STAGE_AUTH)return WMBProcessAuth(data,length);
     if(stage==SDK_STAGE_RSA)return WMBProcessRSA(data,length);
     if(stage==SDK_STAGE_HEADER)return WMBProcessHeader(data,length);
     if(stage==SDK_STAGE_DATA)return WMBProcessData(data,length);
     
     return 1;
}

DLLIMPORT bool AsmPlug_Init(sAsmSDK_Config *config)
{
    AsmPlugin_Init(config, &nds_data);//Allocates memory for the nds_data struct
    memset((void*)nds_data, 0, sizeof(Nds_data));
    
	#ifndef NDS
    ResetAsm = config->ResetAsm;
    #endif
	//DEBUG = config->DEBUG;
    //Log = config->Log;
    WMBDEBUG = (bool*)malloc(1);
    *WMBDEBUG = 1;
    WMBLog = &wlog;
    wlog = fopen("wmb_log.txt","w");
    WMBCONFIG = config;
    
    return 1;
}

DLLIMPORT bool AsmPlug_DeInit()
{
    AsmPlugin_DeInit(&nds_data);
    
    free(WMBDEBUG);
    fclose(wlog);
    
    return 1;
}

DLLIMPORT volatile Nds_data *AsmPlug_GetNdsData()
{
    return nds_data;
}

DLLIMPORT void AsmPlug_Reset()
{   
    stage=SDK_STAGE_BEACON;
    last_seq=0;
    
    memset(host_mac,0,6);
	memset(client_mac,0,6);
}

#ifdef __cplusplus
  }
#endif

void WMBBeaconGrab(unsigned char *data)
{
     ds_element *ds = (ds_element*)Nin_ie;
     //Block start. This block is based on code from masscat's WMB client.
     int *ptr = (int*)&nds_data->found_beacon[(int)ds->advert_sequence_number + ((int)ds->gameID * 10)];

     *ptr+=1;

		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"PROCESSING BEACON DSSEQ %d\n",(int)ds->advert_sequence_number);
			fflushdebug(*WMBLog);
			
			//printf("A\n");
		}
     
     if(*ptr<=2)
     {
                        int pos=0;
                        for(int i=0; i<(int)ds->advert_sequence_number; i++)
                        {
                            if(i!=8)pos+=98;
                            if(i==8)pos+=98;
                        }
                        
                        memcpy((void*)&nds_data->beacon_data[(980*(int)ds->gameID)+pos],ds->data,(size_t)ds->data_size);
                        //memcpy((void*)((int)&nds_data->advert + pos), ds->data, (size_t)ds->data_size);
     }

    //Block end

    //printf("B\n");

     bool got_all=1;
     
     if(!nds_data->multipleIDs)
     {
        //This code is based on code from masscat's WMB client.
        for(int i=0; i<9; i++)
        {
             if(!nds_data->found_beacon[i])got_all=0;
        }
     }
     else
     {
            if(nds_data->FoundAllBeacons)
            {
                got_all=1;
            }
            else
            {
                got_all=0;
            }
     }

    //printf("C\n");

    got_all = 1;

     if(got_all)
     {
            
            //printf("a\n");
            
            int pos, dsize;

                if(!nds_data->multipleIDs)
                {
                    //printf("b\n");
                    for(int i=0; i<9; i++)
                    {
                        pos=i*98;
                        if(i!=8)dsize=98;
                        if(i==8)dsize=72;
                        
                        memcpy((void*)&((unsigned char*)&nds_data->advert)[pos],(void*)&nds_data->beacon_data[(980*(int)nds_data->FirstBeaconID)+pos],dsize);
                    }
                    
                    //printf("c\n");
                }
                
                //printf("D\n");
            
            
            
            if(nds_data->finished_first_assembly)
            {
                
              if(!nds_data->multipleIDs)
              {
                    //printf("E\n");
                if(memcmp((void*)nds_data->oldadvert.icon_pallete, (void*)nds_data->advert.icon_pallete,32)==0)
                {
                    
                    ResetAsm((Nds_data*)nds_data);
                    return;
                }
                else
                {
                    memcpy((void*)&nds_data->oldadvert,(void*)&nds_data->advert,sizeof(ds_advert));
                    //memset(&nds_data->advert,0,sizeof(ds_advert));
                }
                //printf("F\n");
              }
	                         
            }
            else
            {
            //printf("G\n");
            if(!nds_data->multipleIDs)
            memcpy((void*)&nds_data->oldadvert,(void*)&nds_data->advert,sizeof(ds_advert));
            
            //printf("H\n");
            }
            
            FILE *fdump = fopen("advertW.bin","wb");
            fwrite((void*)&nds_data->advert, 1, sizeof(ds_advert), fdump);
            fclose(fdump);
            
                           stage=SDK_STAGE_AUTH;
								if(*WMBDEBUG)
								{
									fprintfdebug(*WMBLog,"FOUND ALL BEACONS!\n");
									fprintfdebug(*WMBLog,"ENTERING ASSOC STAGE\n");
									fprintfdebug(*WMBLog,"HOST MAC ");
											for(int i=0; i<5; i++)
												fprintfdebug(*WMBLog,"%x:",host_mac[i]);
									fprintfdebug(*WMBLog,"%x\n",host_mac[5]);
									fflushdebug(*WMBLog);
								}

        //printf("I\n");


     }
}

int WMBProcessAuth(unsigned char *data, int length)
{
     iee80211_framehead *fh = (iee80211_framehead*)data;

     if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 11)) && CompareMAC(host_mac, fh->mac3))
     {
     unsigned char ID = GetGameID(data);//Eh, not really. The real gameID is grabbed in the header handling code.
     nds_data->clientID = ID;//This is some kind of ID stored in the duration field in the 80211 ieee header
     
     memcpy(client_mac,fh->mac2,6);
	 
	 if(*WMBDEBUG)
	 {
        
        fprintfdebug(*WMBLog,"FOUND AUTH\n");
        fprintfdebug(*WMBLog,"CLIENT MAC ");
        
            for(int i=0; i<5; i++)
                fprintfdebug(*WMBLog,"%x:",client_mac[i]);
        
        fprintfdebug(*WMBLog,"%x\n",client_mac[5]);
        
		fprintfdebug(*WMBLog,"ENTERING RSA STAGE\n");
		fflushdebug(*WMBLog);
		
	 }

                           stage=SDK_STAGE_RSA;
                           
                           nds_data->gotID=0;

                           return 1;
     }

     return 0;
}

int WMBProcessRSA(unsigned char *data, int length)
{
     unsigned char *dat = NULL;
     unsigned char pos=0;
     unsigned short size=0;
     nds_rsaframe rsa;
     memset(&rsa,0,sizeof(nds_rsaframe));

     if(CheckFrame(data, host_mac, 0x03, &size, &pos))
     {
            
            size=230;

            unsigned char ID = GetGameID(data);
            
                if(nds_data->clientID!=ID)
                {
					if(*WMBDEBUG)
					{
                        fprintfdebug(*WMBLog,"RSA FAILED ID %d MEM %d\n",(int)ID,(int)nds_data->clientID);
                        fflushdebug(*WMBLog);
                    }
					
					return -1;
                }
    
        dat=&data[(int)pos];
        memcpy(&rsa,dat,(size_t)size);
     
                            if(nds_data->finished_first_assembly)
                            {
                                if(memcmp((void*)&nds_data->rsa,(void*)&rsa,sizeof(nds_rsaframe))==0)
                                {
                                        ResetAsm((Nds_data*)nds_data);
                                        return -1;
                                }
                            }
     
        memcpy((void*)&nds_data->rsa,(void*)&rsa,sizeof(nds_rsaframe));
        
		  if(*WMBDEBUG)
		  {
                fprintfdebug(*WMBLog,"FOUND RSA %d %d\n",(int)size,(int)pos);
                fprintfdebug(*WMBLog,"ENTERING HEADER STAGE\n");
		        fflushdebug(*WMBLog);
          }
		
		stage=SDK_STAGE_HEADER;
        
        return 1;
     }

     return 0;
}

//Bunch of data the DS Download Station's don't send in their WMB header, and possiblely other demos/WMB transfers too.
unsigned char header_filler[]={
    0x8A, 0xC0, 0x13, 0x72, 0xA7, 0xFC, 0x9F, 0x84, 0x4D, 0x73, 0xA3, 0xCA, 0x9A, 0x61,
    0x58, 0x97, 0xA3, 0x27, 0xFC, 0x03, 0x98, 0x76, 0x23, 0x1D, 0xC7, 0x61, 0x03, 0x04,
    0xAE, 0x56, 0xBF, 0x38, 0x84, 0x00, 0x40, 0xA7, 0x0E, 0xFD, 0xFF, 0x52, 0xFE, 0x03,
    0x6F, 0x95, 0x30, 0xF1, 0x97, 0xFB, 0xC0, 0x85, 0x60, 0xD6, 0x80, 0x25, 0xA9, 0x63,
    0xBE, 0x03, 0x01, 0x4E, 0x38, 0xE2, 0xF9, 0xA2, 0x34, 0xFF, 0xBB, 0x3E, 0x03, 0x44,
    0x78, 0x00, 0x90, 0xCB, 0x88, 0x11, 0x3A, 0x94, 0x65, 0xC0, 0x7C, 0x63, 0x87, 0xF0,
    0x3C, 0xAF, 0xD6, 0x25, 0xE4, 0x8B, 0x38, 0x0A, 0xAC, 0x72, 0x21, 0xD4, 0xF8, 0x07,
    0x56, 0xCF, 0x39, 0x5F
    };

int WMBProcessHeader(unsigned char *data, int length)
{
     unsigned char *dat = NULL;
     unsigned char pos=0;
     unsigned short size=0;
     unsigned short seq=0;
     unsigned short *Seq=&seq;

	TNDSHeader temp_header;
	memset(&temp_header,0,sizeof(TNDSHeader));

     if(CheckFrame(data, host_mac, 0x04, &size, &pos))
     {
        
        
        if(size!=352 && size!=250)return 1;

        dat=&data[(int)pos-2];
        
        unsigned char ID = *dat;
        nds_data->gameID = ID;
        
            if(nds_data->handledIDs[(int)ID] && nds_data->multipleIDs)
            {
				if(*WMBDEBUG)
				{
					fprintfdebug(*WMBLog,"FOUND HEADER WITH BAD ID %d ALREADY FOUND IDS ",(int)ID);
						for(int i=0; i<15; i++)
							if(nds_data->handledIDs[i])fprintfdebug(*WMBLog,"%d ",i);
					
					fprintfdebug(*WMBLog,"\n");
					fflushdebug(*WMBLog);
                }
                
                stage=SDK_STAGE_AUTH;//If we wouldn't go back to the Auth or RSA stage, with some multi-game captures, we'd get the wrong RSA-signature.
                
                return -1;
            }
     
     dat++;

     if(*dat!=0)return 0;

     dat++;
     Seq = (unsigned short*)dat;
     seq=*Seq;
     ConvertEndian(&seq,&seq,2);
     if(seq!=0)return -1;//Ignore non-header packets.
     dat+=2;



     iee80211_framehead *fh = (iee80211_framehead*)data;
	int fh_seq=ReadSeq(&fh->sequence_control);
	if(last_seq==0)
	{
                   last_seq=fh_seq;
    }
    else
    {
        if(fh_seq-2!=last_seq)return 0;
        last_seq=fh_seq;
    }


     if(size==352)
     memcpy(&temp_header,dat,(size_t)size);

     if(size==250)
     {
            memcpy(&temp_header,dat,250);
            memcpy(&temp_header.gbaLogo[58],header_filler,sizeof(header_filler));
            //DS Download Station's, and maybe other things, don't send their whole header.
     }
    
    if(nds_data->multipleIDs)
     printf("Found game with ID %d, assembling now.\n",(int)ID);
     
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"HEADER SET GAMEID TO %d\n",ID);
			fflushdebug(*WMBLog);
        }

    if(nds_data->finished_first_assembly)
    {
        
        //fprintfdebug(*Log,"HIT HEADER TITLE MEM %s CUR %s\n",nds_data.header.gameTitle,temp_header.gameTitle);
        
        if(memcmp((void*)&nds_data->header, (void*)&temp_header,sizeof(TNDSHeader))==0)
        {
            
            ResetAsm((Nds_data*)nds_data);
            return -1;
        }
    }
    
      memcpy((void*)&nds_data->header,(void*)&temp_header,sizeof(TNDSHeader));

     int arm7s_seq, arm7e_seq;

     nds_data->pkt_fixed = 0;

     //This code here is based on Juglak's code. This needs to be done, otherwise there would be problems with entering the final stage, assembly, too soon.
     arm7s_seq = ((int)nds_data->header.arm9binarySize-((int)nds_data->header.arm9binarySize%490))/490;
	if (((int)nds_data->header.arm9binarySize%490) != 0) arm7s_seq++;

	arm7e_seq = ((int)nds_data->header.arm7binarySize-((int)nds_data->header.arm7binarySize%490))/490;

	arm7e_seq += arm7s_seq;

     nds_data->arm7s_seq = arm7s_seq;
     nds_data->arm7e_seq = arm7e_seq;
	
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"ARM7S %d ARM7E %d\n",nds_data->arm7s_seq,nds_data->arm7e_seq);
        }

     stage=SDK_STAGE_DATA;
	 
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"ENTERING DATA STAGE\n");
		}

     char str[20];
     strncpy(str,(char*)nds_data->header.gameTitle,12);
     str[12]=0;
     
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"FOUND HEADER SZ %d GAME NAME %s\n",size,str);
			fflushdebug(*WMBLog);
        }
     
     printf("FOUND GAME NAME: %s\n\n",str);

     return 1;
     
     }

     return 0;
}

int WMBProcessData(unsigned char *data, int length)
{
     //unsigned short arm7s,arm7e;
     unsigned short size = 0;
     unsigned char pos = 0;
     unsigned short *Seq, seq;
     unsigned char *dat=NULL;
     //unsigned short ts=0;
     //int k=0;
     Seq=NULL;
     seq=0;

	bool prev_init=nds_data->data_init;
	Init();

	iee80211_framehead *fh = (iee80211_framehead*)data;

	if(CheckFrame(data, host_mac, 0x04, &size, &pos))
	{

    dat=&data[(int)pos-2];
    //unsigned char ID = GetGameID(data);
    unsigned char ID = *dat;
        if(ID!=nds_data->gameID)
        {
                if(*WMBDEBUG)
                {
                    fprintfdebug(*WMBLog,"DATA FAILED ID %d MEM %d---------------------------------\n",(int)ID,(int)nds_data->gameID);
			        fflushdebug(*WMBLog);
                }
        return -1;
        }
     dat++;

     if(*dat!=0)return -1;

     dat++;
     Seq = (unsigned short*)dat;
     seq=*Seq;
     //ConvertEndian(&seq,&seq,2);//Ummm... NdsTech Wiki says this is litte-endian... And PCs are big-endian...
     //And yet, the seq is wrong with this call, correct without this call... Strange...
     if(seq==0)return -1;//Official WMB host sends the header many times, ignore the duplicates
     dat+=2;

    int fh_seq=ReadSeq(&fh->sequence_control);
	if(last_seq==0)
	{
                   last_seq=fh_seq;
    }
    else
    {
        if(fh_seq-2!=last_seq && prev_init==0)
        {
                                if(*WMBDEBUG)
                                {
                                    fprintfdebug(*WMBLog,"DROPPED DATA PKT SEQ %d PREV SEQ %d\n",fh_seq,last_seq);
                                    fflushdebug(*WMBLog);
                                }
                              return 0;
        }
        last_seq=fh_seq;
    }
    
     if(size==102)nds_data->pkt_size=250;
     if(nds_data->pkt_size==250)seq--;
     if(size==102 && seq==0)return -1;

     if(seq==1)nds_data->pkt_size = size;

     if(size==102)
     nds_data->data_sizes[(int)seq-1]=1;

            if(nds_data->data_sizes[(int)seq-1]!=0)
            {
            //printf("KA NA\n");
            //printf("CONFIG %p NDS %p\n", CONFIG, nds_data);
            //if(!CheckDataPackets((int)seq))return 1;
            
            if(nds_data->arm7e==0)return -1;
            }

     int temp=0;
     for(int i=0; i<(int)seq-1; i++)
     {
     temp+=nds_data->data_sizes[i];
     
     if(nds_data->data_sizes[i]==0)temp+=nds_data->pkt_size;
     }

     nds_data->cur_pos = temp;

     nds_data->data_sizes[(int)seq-1] = size;
     
     memcpy((void*)&nds_data->saved_data[nds_data->cur_pos],dat,(size_t)(size));
     
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"FOUND THE DATA SIZE %d SEQ %d DZ %d",(int)size,(int)*Seq,(int)seq-1);
				#ifdef _H_CUSTOM_PCAP
					fprintfdebug(*WMBLog," PKTNUM %d",GetPacketNum());
				#endif
			fprintfdebug(*WMBLog,"\n");
			fflushdebug(*WMBLog);
        }

    nds_data->cur_pos+=size;

    

     if(nds_data->cur_pos>=(int)nds_data->header.arm9binarySize && nds_data->arm7s == 0)
     {

            if(nds_data->cur_pos!=(int)nds_data->header.arm9binarySize)
            nds_data->cur_pos=(int)nds_data->header.arm9binarySize;

            nds_data->arm7s = nds_data->cur_pos;
            nds_data->arm7s_seq = (int)seq-1;
            
                 if(*WMBDEBUG)
                 {
                    fprintfdebug(*WMBLog,"ARM7S SET TO %d SEQ %d\n",nds_data->arm7s,nds_data->arm7s_seq);
                    //printf("ARM7S SET TO %d SEQ %d PKTNUM %d\n",nds_data.arm7s,nds_data.arm7s_seq,GetPacketNumber());
			        fflushdebug(*WMBLog);
                 }
     }

    

     if(nds_data->cur_pos>=nds_data->total_binaries_size && nds_data->arm7e==0)
     {

            if(nds_data->cur_pos!=nds_data->total_binaries_size)
            nds_data->cur_pos=nds_data->total_binaries_size;

            //nds_data->cur_pos-= (nds_data->pkt_size*3);
            nds_data->arm7e = nds_data->cur_pos;
            nds_data->arm7e_seq = (int)seq-1;
            
            if(*WMBDEBUG)
            {
                fprintfdebug(*WMBLog,"ARM7E SET TO %d SEQ %d\n",nds_data->arm7e,nds_data->arm7e_seq);
                //printf("ARM7E SET TO %d SEQ %d PKTNUM %d\n",nds_data.arm7e,nds_data.arm7e_seq,GetPacketNumber());
			    fflushdebug(*WMBLog);
            }
     }
     
     
     
     if(nds_data->arm7e_seq!=0)
     {
     
     //Make sure we found every packet. Only thing left to do in this function is the goto assembly
     //stage code, so it's safe to do this here.
     for(int i=0; i<nds_data->arm7e_seq; i++)
     {
            if(nds_data->data_sizes[i]==0)
            {
                return 0;//If we missed any packets, don't begin assembly
            }
     }

     nds_data->arm7e = nds_data->total_binaries_size;//Sometimes there problems if we don't set this here...
	 
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"FOUND ALL DATA PACKETS\n");
			fprintfdebug(*WMBLog,"ENTERING ASSEMBLE STAGE\n");
			fflushdebug(*WMBLog);
        }
	 	
	 //if(nds_data->multipleIDs)
        UpdateAvert();
     
     nds_data->trigger_assembly = 1;
     
     }

     return 1;

    }

     return 0;
}

int WMBProcessBeacons(unsigned char *data, int length)
{
     iee80211_framehead2 *framehead = (iee80211_framehead2*)data;
     beacon *Beacon = (beacon*)data;
     ds_element *ds = (ds_element*)&data[52];
     //unsigned short interval = BEACON_INTERVAL;
     unsigned short cap = BEACON_CAPABILITY;
     unsigned char tagparms[4];
     tagparms[0]=0x01;tagparms[1]=0x02;
     tagparms[2]=0x82;tagparms[3]=0x04;
     //BEACON_TAGPARAMS;
     unsigned char tagparms2[4];
     tagparms2[0]=0x01;tagparms2[1]=0x08;
     tagparms2[2]=0x82;tagparms2[3]=0x84;
     //BEACON_TAGPARAMS2;
     unsigned char tagparms3[4];
     tagparms3[0]=0x01;tagparms3[1]=0x02;
     tagparms3[2]=0x82;tagparms3[3]=0x84;
     //BEACON_TAGPARM3
     unsigned char *nin_ie=NULL;

     if(CheckFrameControl(framehead,0,8))//frame control type/subtype for beacons
     {
                      if(memcmp(Beacon->destmac,BroadcastMAC,6)==0)
                      {
                      //If it's a broadcast...
                      
                       if(memcmp(Beacon->srcmac,Beacon->bssmac,6)==0)
                       {
                       //If the src MAC and BSSID are the same...
                       //It passed the Beacon frame test, time to test the Managment frame!
                       //ConvertEndian(&Beacon->interval,&Beacon->interval,2);
                       //printf("A\n");
                       unsigned char *temp = (unsigned char*)&Beacon->capability;
                       //unsigned char *temp2 = (unsigned char*)&cap;
                       temp[1]=0;

                       //unsigned char *Temp = (unsigned char*)Beacon->tagparms;
                       //unsigned char *Temp2 = (unsigned char*)tagparms;


                            //if(memcmp(&Beacon->interval,&interval,2)==0)
                            //{
                                if(memcmp(&Beacon->capability,&cap,2)==0)
                                {
                                    
                                    if(memcmp(Beacon->tagparms,&tagparms,4)==0 ||
                                    memcmp(Beacon->tagparms,&tagparms2,4)==0 ||
                                    memcmp(Beacon->tagparms,&tagparms3,4)==0)
                                    {
                                        //printf("B\n");
                                         nin_ie = nintendoWMBBeacon(data,length);
                                         if(nin_ie==NULL)return 0;
                                         //printf("C\n");
                                         ds = (ds_element*)nin_ie;
                                         Nin_ie=nin_ie;

                                         unsigned short checksum = computeBeaconChecksum((unsigned short *) (unsigned int)(((unsigned int)&ds->data[0])-4), (ds->data_size+4)/2);
                                       
                                         if(checksum!=ds->checksum)
                                         {
											if(*WMBDEBUG)
											{
												fprintfdebug(*WMBLog,"BEACON SEQ %d FAILED CRC16 CHECK!\n",(int)ds->advert_sequence_number);
												fflushdebug(*WMBLog);
                                            }
                                         return 0;
                                         }
                                        //printf("D\n");
                                         if(ds->data_size==0x01)return -1;//This beacon isn't part of the advert          
                                         //if(ds->gameID!=5)return -1;//Fake beacon with gameID 5, and screwed data through the beacon, sent by Nintendo Spot.
                                         
                                         
                                         nds_data->foundIDs[(int)ds->gameID]=1;
                                         if(!nds_data->gotID && ds->sequence_number!=8)
                                         {
                                                nds_data->gotID=1;
                                                nds_data->FirstBeaconID = ds->gameID;
                                                
													if(*WMBDEBUG)
													{
														fprintfdebug(*WMBLog,"BEACON SET ID TO %d\n",(int)nds_data->FirstBeaconID);
														fflushdebug(*WMBLog);
                                                    }
                                         }
                                         
                                         //printf("E\n");
                                         
                                         if(ds->sequence_number<8 && ds->sequence_number>0)nds_data->prev_nonadvert = ds->non_advert;
                                         if(ds->sequence_number==0 && nds_data->prev_nonadvert!=ds->non_advert)nds_data->multipleIDs=1;
                                         
                                         //printf("F\n");

                                         if(ds->sequence_number==8 && nds_data->multipleIDs && nds_data->FirstBeaconID==ds->gameID)
                                         {
                                                if(nds_data->beacon_thing==1)
                                                {
                                                    
                                                    bool got_it=1;
                                                    
                                                        for(int i=0; i<9; i++)
                                                        {
                                         
                                                            if(nds_data->found_beacon[((int)ds->gameID*10)+i])
                                                                got_it=0;
                                                        }
                                         
                                                    nds_data->FoundAllBeacons=1;
                                                    
                                                        if(got_it && !nds_data->FoundGameIDs)
                                                        {
                                                            int total=0;
                                                            
                                                                for(int i=0; i<15; i++)
                                                                {
                                                                    if(nds_data->foundIDs[i])total++;
                                                                }
                                                            
                                                            nds_data->FoundGameIDs=1;
                                                            printf("Found %d demos broadcasted in capture.\n",total);
                                                        }
                                         
                                                }
                                                else
                                                {
                                                    nds_data->beacon_thing=1;
                                                }
                                         }
                                         
                                         //printf("G\n");
                                       
                                        memcpy(host_mac,Beacon->srcmac,6);
                                        
                                        nds_data->beacon_checksum[(int)ds->advert_sequence_number] = checksum;
                                        
                                        //printf("H\n");
                                        
											if(*WMBDEBUG)
											{
												fprintfdebug(*WMBLog,"FOUND BEACON DSSEQ %d AD %d BC %d CON %d LEN %d DS %d BC %d LEN %d ID %d NONAD %d PKTNUM %d\n",
												(int)ds->sequence_number,(int)ds->advert_sequence_number,
												ReadSeq(&Beacon->seq), (int)ds->connected_clients, (int)ds->advert_len,
												(int)ds->data_size,(int)ds->advert_len,(int)ds->advert_len,(int)ds->gameID,(int)ds->non_advert, 1);
												fflushdebug(*WMBLog);
                                            }

                                        WMBBeaconGrab(data);


                                        return 1;

                                        }
                                }
                       }
                      }
     }

     return 0;
}

void Init()
{
     if(!nds_data->data_init)
	{
    nds_data->data_init=1;

    nds_data->total_binaries_size = ((int)nds_data->header.arm9binarySize + (int)nds_data->header.arm7binarySize);

		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"ARM9SZ %d ARM7SZ %d\n",(int)nds_data->header.arm9binarySize,(int)nds_data->header.arm7binarySize);
			
			fprintfdebug(*WMBLog,"BINARIES SIZES %d\n",nds_data->total_binaries_size);
			fflushdebug(*WMBLog);
        }
    nds_data->saved_data = (unsigned char*)malloc((size_t)nds_data->total_binaries_size);
    nds_data->data_sizes = (int*)malloc((sizeof(int)*(7980*4)));
    nds_data->arm7s = 0;nds_data->arm7e = 0;
    if(nds_data->saved_data==NULL || nds_data->data_sizes==NULL)
    {
    printf("FATAL ERROR: Failed to allocate memory for binaries data.\n");
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"FATAL ERROR: Failed to allocate memory for binaries data.\n");
			fflushdebug(*WMBLog);
        }
		
	system("PAUSE");
	
	    if(*WMBDEBUG)
	    {
            exit(1);
            return;
        }
    }
    memset((void*)nds_data->saved_data,0,(size_t)nds_data->total_binaries_size);
    memset((void*)nds_data->data_sizes,0,(sizeof(int)*(7980*4)));
    nds_data->last_dat_seq=1;
		if(*WMBDEBUG)
		{
			fprintfdebug(*WMBLog,"FINISHED INIT\n");
			fflushdebug(*WMBLog);
        }
		
    }
}
