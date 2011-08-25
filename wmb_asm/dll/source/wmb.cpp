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

unsigned char BroadcastMAC[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
unsigned char host_client_mgc[4] = {0x06,0x01,0x02,0x00};

bool CompareMAC(unsigned char *a, unsigned char *b);

bool WMBProcessBeacons(unsigned char *data, int length);
bool WMBProcessAuth(unsigned char *data, int length);
bool WMBProcessRSA(unsigned char *data, int length);
bool WMBProcessHeader(unsigned char *data, int length);
bool WMBProcessData(unsigned char *data, int length);

unsigned int CalcCRC32(unsigned char *data, unsigned int length);
unsigned short CalcCRC16(unsigned char *data, unsigned int length);
bool CheckFrame(unsigned char *data, unsigned char command, unsigned short *size, unsigned char *pos);
unsigned char *nintendoWMBBeacon( unsigned char *frame, int frame_size);
bool CheckDataPackets(int seq);
int ReadSeq(unsigned short *seq);
bool CheckFrameControl(iee80211_framehead2 *framehead, int type, int subtype);
unsigned short computeBeaconChecksum(unsigned short *data, int length);

bool CheckFlow(unsigned char *mac,unsigned char flow);

unsigned char GetGameID(unsigned char *data);

int stage=STAGE_BEACON;

unsigned char host_mac[6];
unsigned char client_mac[6];

int last_seq=0;//Last seq in the data transfer

unsigned char *Nin_ie = NULL;

void Init();

DLLIMPORT bool WMBHandle802_11(unsigned char *data, int length)
{
     if(stage==STAGE_BEACON)return WMBProcessBeacons(data,length);
     if(stage==STAGE_AUTH)return WMBProcessAuth(data,length);
     if(stage==STAGE_RSA)return WMBProcessRSA(data,length);
     if(stage==STAGE_HEADER)return WMBProcessHeader(data,length);
     if(stage==STAGE_DATA)return WMBProcessData(data,length);
     
     return 0;
}

DLLIMPORT void WMBReset()
{
    last_seq=0;
}

void WMBBeaconGrab(unsigned char *data)
{
     ds_element *ds = (ds_element*)Nin_ie;
     //Block start. This block is based on code from masscat's WMB client.
     int *ptr = &nds_data.found_beacon[(int)ds->advert_sequence_number + ((int)ds->gameID * 10)];

     *ptr+=1;

		if(DEBUG)
		{
			fprintfdebug(Log,"PROCESSING BEACON DSSEQ %d\n",(int)ds->advert_sequence_number);
			fflushdebug(Log);
		}
     
     if(*ptr<=2)
     {
                        int pos=0;
                        for(int i=0; i<(int)ds->advert_sequence_number; i++)
                        {
                            if(i!=8)pos+=98;
                            if(i==8)pos+=98;
                        }
                        
                        memcpy(&nds_data.beacon_data[(980*(int)ds->gameID)+pos],ds->data,(size_t)ds->data_size);
    }

    //Block end

     bool got_all=1;
     
     if(!nds_data.multipleIDs)
     {
        //This code is based on code from masscat's WMB client.
        for(int i=0; i<9; i++)
        {
             if(!nds_data.found_beacon[i])got_all=0;
        }
     }
     else
     {
            if(nds_data.FoundAllBeacons)
            {
                got_all=1;
            }
            else
            {
                got_all=0;
            }
     }

     if(got_all)
     {
            
            int pos, dsize;

                if(!nds_data.multipleIDs)
                {
                    for(int i=0; i<9; i++)
                    {
                        pos=i*98;
                        if(i!=8)dsize=98;
                        if(i==8)dsize=72;
                        
                        memcpy(&((unsigned char*)&nds_data.advert)[pos],&nds_data.beacon_data[(980*(int)nds_data.FirstBeaconID)+pos],dsize);
                    }
                }
            
            if(nds_data.finished_first_assembly)
            {
                
              if(!nds_data.multipleIDs)
              {
                if(memcmp(nds_data.oldadvert.icon_pallete,nds_data.advert.icon_pallete,32)==0)
                {
                    
                    ResetAsm();
                    return;
                }
                else
                {
                    memcpy(&nds_data.oldadvert,&nds_data.advert,sizeof(ds_advert));
                    //memset(&nds_data.advert,0,sizeof(ds_advert));
                }
              }
	                         
            }
            else
            {
            if(!nds_data.multipleIDs)
            memcpy(&nds_data.oldadvert,&nds_data.advert,sizeof(ds_advert));
            }
            
                           stage=STAGE_AUTH;
								if(DEBUG)
								{
									fprintfdebug(Log,"FOUND ALL BEACONS!\n");
									fprintfdebug(Log,"ENTERING ASSOC STAGE\n");
									fprintfdebug(Log,"HOST MAC ");
											for(int i=0; i<5; i++)
												fprintfdebug(Log,"%x:",host_mac[i]);
									fprintfdebug(Log,"%x\n",host_mac[5]);
									fflushdebug(Log);
								}


     }
}

bool WMBProcessAuth(unsigned char *data, int length)
{
     iee80211_framehead *fh = (iee80211_framehead*)data;

     if (((FH_FC_TYPE(fh->frame_control) == 0) && (FH_FC_SUBTYPE(fh->frame_control) == 11)) && CompareMAC(host_mac, fh->mac3))
     {
     unsigned char ID = GetGameID(data);//Eh, not really. The real gameID is grabbed in the header handling code.
     nds_data.clientID = ID;//This is some kind of ID stored in the duration field in the 80211 ieee header
     
     memcpy(client_mac,fh->mac2,6);
	 
	 if(DEBUG)
	 {
        
        fprintfdebug(Log,"FOUND AUTH\n");
        fprintfdebug(Log,"CLIENT MAC ");
        
            for(int i=0; i<5; i++)
                fprintfdebug(Log,"%x:",client_mac[i]);
        
        fprintfdebug(Log,"%x\n",client_mac[5]);
        
		fprintfdebug(Log,"ENTERING RSA STAGE\n");
		fflushdebug(Log);
		
	 }

                           stage=STAGE_RSA;
                           
                           nds_data.gotID=0;

                           return 1;
     }

     return 0;
}

bool WMBProcessRSA(unsigned char *data, int length)
{
     unsigned char *dat = NULL;
     unsigned char pos=0;
     unsigned short size=0;
     nds_rsaframe rsa;
     memset(&rsa,0,sizeof(nds_rsaframe));

     if(CheckFrame(data,0x03,&size,&pos))
     {
            
            size=230;

            unsigned char ID = GetGameID(data);
            
                if(nds_data.clientID!=ID)
                {
					if(DEBUG)
					{
                        fprintfdebug(Log,"RSA FAILED ID %d MEM %d\n",(int)ID,(int)nds_data.clientID);
                        fflushdebug(Log);
                    }
					
					return 0;
                }

                    if(nds_data.finished_first_assembly)
                    {
                    
                    //fprintfdebug(Log,"HIT RSA TITLE MEM %s\n",nds_data.header.gameTitle);
                    }
    
        dat=&data[(int)pos];
        memcpy(&rsa,dat,(size_t)size);
     
                            if(nds_data.finished_first_assembly)
                            {
                                if(memcmp(&nds_data.rsa,&rsa,sizeof(nds_rsaframe))==0)
                                {
                                        ResetAsm();
                                        return 1;
                                }
                            }
     
        memcpy(&nds_data.rsa,&rsa,sizeof(nds_rsaframe));
        
		  if(DEBUG)
		  {
                fprintfdebug(Log,"FOUND RSA %d %d\n",(int)size,(int)pos);
                fprintfdebug(Log,"ENTERING HEADER STAGE\n");
		        fflushdebug(Log);
          }
		
		stage=STAGE_HEADER;
        
        return 1;
     }

     return 0;
}

//Buch of data the DS Download Station's don't send in their WMB header, and possiblely other demos/WMB transfers too.
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

bool WMBProcessHeader(unsigned char *data, int length)
{
     unsigned char *dat = NULL;
     unsigned char pos=0;
     unsigned short size=0;
     unsigned short seq=0;
     unsigned short *Seq=&seq;

	TNDSHeader temp_header;
	memset(&temp_header,0,sizeof(TNDSHeader));

     if(CheckFrame(data,0x04,&size,&pos))
     {
        
        
        if(size!=352 && size!=250)return 1;

        dat=&data[(int)pos-2];
        
        unsigned char ID = *dat;
        nds_data.gameID = ID;
        
            if(nds_data.handledIDs[(int)ID] && nds_data.multipleIDs)
            {
				if(DEBUG)
				{
					fprintfdebug(Log,"FOUND HEADER WITH BAD ID %d ALREADY FOUND IDS ",(int)ID);
						for(int i=0; i<15; i++)
							if(nds_data.handledIDs[i])fprintfdebug(Log,"%d ",i);
					
					fprintfdebug(Log,"\n");
					fflushdebug(Log);
                }
                
                stage=STAGE_AUTH;//If we wouldn't go back to the Auth or RSA stage, with some multi-game captures, we'd get the wrong RSA-signature.
                
                return 1;
            }
     
     dat++;

     if(*dat!=0)return 1;

     dat++;
     Seq = (unsigned short*)dat;
     seq=*Seq;
     ConvertEndian(&seq,&seq,2);
     if(seq!=0)return 1;//Ignore non-header packets.
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
    
    if(nds_data.multipleIDs)
     printf("Found game with ID %d, assembling now.\n",(int)ID);
     
		if(DEBUG)
		{
			fprintfdebug(Log,"HEADER SET GAMEID TO %d\n",ID);
			fflushdebug(Log);
        }

    if(nds_data.finished_first_assembly)
    {
        
        //fprintfdebug(Log,"HIT HEADER TITLE MEM %s CUR %s\n",nds_data.header.gameTitle,temp_header.gameTitle);
        
        if(memcmp(&nds_data.header,&temp_header,sizeof(TNDSHeader))==0)
        {
            
            ResetAsm();
            return 1;
        }
    }
    
      memcpy(&nds_data.header,&temp_header,sizeof(TNDSHeader));

     int arm7s_seq, arm7e_seq;

     nds_data.pkt_fixed = 0;

     //This code here is based on Juglak's code. This needs to be done, otherwise there would be problems with entering the final stage, assembly, too soon.
     arm7s_seq = ((int)nds_data.header.arm9binarySize-((int)nds_data.header.arm9binarySize%490))/490;
	if (((int)nds_data.header.arm9binarySize%490) != 0) arm7s_seq++;

	arm7e_seq = ((int)nds_data.header.arm7binarySize-((int)nds_data.header.arm7binarySize%490))/490;

	arm7e_seq += arm7s_seq;

     nds_data.arm7s_seq = arm7s_seq;
     nds_data.arm7e_seq = arm7e_seq;
	
		if(DEBUG)
		{
			fprintfdebug(Log,"ARM7S %d ARM7E %d\n",nds_data.arm7s_seq,nds_data.arm7e_seq);
        }

     stage=STAGE_DATA;
	 
		if(DEBUG)
		{
			fprintfdebug(Log,"ENTERING DATA STAGE\n");
		}

     char str[20];
     strncpy(str,nds_data.header.gameTitle,12);
     str[12]=0;
     
		if(DEBUG)
		{
			fprintfdebug(Log,"FOUND HEADER SZ %d GAME NAME %s\n",size,str);
			fflushdebug(Log);
        }
     
     printf("FOUND GAME NAME: %s\n\n",str);

     return 1;
     
     }

     return 0;
}

bool WMBProcessData(unsigned char *data, int length)
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

	bool prev_init=nds_data.data_init;
	Init();

	iee80211_framehead *fh = (iee80211_framehead*)data;

	if(CheckFrame(data,0x04,&size,&pos))
	{

    dat=&data[(int)pos-2];
    //unsigned char ID = GetGameID(data);
    unsigned char ID = *dat;
        if(ID!=nds_data.gameID)
        {
                if(DEBUG)
                {
                    fprintfdebug(Log,"DATA FAILED ID %d MEM %d---------------------------------\n",(int)ID,(int)nds_data.gameID);
			        fflushdebug(Log);
                }
        return 0;
        }
     dat++;

     if(*dat!=0)return 1;

     dat++;
     Seq = (unsigned short*)dat;
     seq=*Seq;
     //ConvertEndian(&seq,&seq,2);//Ummm... NdsTech Wiki says this is litte-endian... And PCs are big-endian...
     //And yet, the seq is wrong with this call, correct without this call... Strange...
     if(seq==0)return 1;//Official WMB host sends the header many times, ignore the duplicates
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
                                if(DEBUG)
                                {
                                    fprintfdebug(Log,"DROPPED DATA PKT SEQ %d PREV SEQ %d\n",fh_seq,last_seq);
                                    fflushdebug(Log);
                                }
                              return 0;
        }
        last_seq=fh_seq;
    }
    
     if(size==102)nds_data.pkt_size=250;
     if(nds_data.pkt_size==250)seq--;
     if(size==102 && seq==0)return 1;

     if(seq==1)nds_data.pkt_size = size;

     if(size==102)
     nds_data.data_sizes[(int)seq-1]=1;

            if(nds_data.data_sizes[(int)seq-1]!=0)
            {
            if(!CheckDataPackets((int)seq))return 1;
            
            if(nds_data.arm7e==0)return 1;
            }

     int temp=0;
     for(int i=0; i<(int)seq-1; i++)
     {
     temp+=nds_data.data_sizes[i];
     
     if(nds_data.data_sizes[i]==0)temp+=nds_data.pkt_size;
     }

     nds_data.cur_pos = temp;

     nds_data.data_sizes[(int)seq-1] = size;
          
     memcpy(&nds_data.saved_data[nds_data.cur_pos],dat,(size_t)(size));
     
		if(DEBUG)
		{
			fprintfdebug(Log,"FOUND THE DATA SIZE %d SEQ %d DZ %d",(int)size,(int)*Seq,(int)seq-1);
				#ifdef _H_CUSTOM_PCAP
					fprintfdebug(Log," PKTNUM %d",GetPacketNumber());
				#endif
			fprintfdebug(Log,"\n");
			fflushdebug(Log);
        }

    nds_data.cur_pos+=size;

     if(nds_data.cur_pos>=(int)nds_data.header.arm9binarySize && nds_data.arm7s == 0)
     {

            if(nds_data.cur_pos!=(int)nds_data.header.arm9binarySize)
            nds_data.cur_pos=(int)nds_data.header.arm9binarySize;

            nds_data.arm7s = nds_data.cur_pos;
            nds_data.arm7s_seq = (int)seq-1;
            
                 if(DEBUG)
                 {
                    fprintfdebug(Log,"ARM7S SET TO %d SEQ %d\n",nds_data.arm7s,nds_data.arm7s_seq);
                    //printf("ARM7S SET TO %d SEQ %d PKTNUM %d\n",nds_data.arm7s,nds_data.arm7s_seq,GetPacketNumber());
			        fflushdebug(Log);
                 }
     }

     if(nds_data.cur_pos>=nds_data.total_binaries_size && nds_data.arm7e==0)
     {

            if(nds_data.cur_pos!=nds_data.total_binaries_size)
            nds_data.cur_pos=nds_data.total_binaries_size;

            //nds_data.cur_pos-= (nds_data.pkt_size*3);
            nds_data.arm7e = nds_data.cur_pos;
            nds_data.arm7e_seq = (int)seq-1;
            
            if(DEBUG)
            {
                fprintfdebug(Log,"ARM7E SET TO %d SEQ %d\n",nds_data.arm7e,nds_data.arm7e_seq);
                //printf("ARM7E SET TO %d SEQ %d PKTNUM %d\n",nds_data.arm7e,nds_data.arm7e_seq,GetPacketNumber());
			    fflushdebug(Log);
            }
     }
     
     if(nds_data.arm7e_seq!=0)
     {
     
     //Make sure we found every packet. Only thing left to do in this function is the goto assembly
     //stage code, so it's safe to do this here.
     for(int i=0; i<nds_data.arm7e_seq; i++)
     {
            if(nds_data.data_sizes[i]==0)
            {
                return 0;//If we missed any packets, don't begin assembly
            }
     }

     nds_data.arm7e = nds_data.total_binaries_size;//Sometimes there problems if we don't set this here...
	 
		if(DEBUG)
		{
			fprintfdebug(Log,"FOUND ALL DATA PACKETS\n");
			fprintfdebug(Log,"ENTERING ASSEMBLE STAGE\n");
			fflushdebug(Log);
        }
		
     stage=STAGE_ASSEMBLE;
     return 1;
     
     }

     return 1;

    }

     return 0;
}

bool WMBProcessBeacons(unsigned char *data, int length)
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

                                         nin_ie = nintendoWMBBeacon(data,length);
                                         if(nin_ie==NULL)return 0;

                                         ds = (ds_element*)nin_ie;
                                         Nin_ie=nin_ie;

                                         unsigned short checksum = computeBeaconChecksum((unsigned short *) (unsigned int)(((unsigned int)&ds->data[0])-4), (ds->data_size+4)/2);
                                       
                                         if(checksum!=ds->checksum)
                                         {
											if(DEBUG)
											{
												fprintfdebug(Log,"BEACON SEQ %d FAILED CRC16 CHECK!\n",(int)ds->advert_sequence_number);
												fflushdebug(Log);
                                            }
                                         //return 1;
                                         }
                                        
                                         if(ds->data_size==0x01)return 1;//This beacon isn't part of the advert
                                       
                                         //if(ds->connected_clients!=1)return 0;
                                       
                                         nds_data.foundIDs[(int)ds->gameID]=1;
                                         if(!nds_data.gotID && ds->sequence_number!=8)
                                         {
                                                nds_data.gotID=1;
                                                nds_data.FirstBeaconID = ds->gameID;
                                                
													if(DEBUG)
													{
														fprintfdebug(Log,"BEACON SET ID TO %d\n",(int)nds_data.FirstBeaconID);
														fflushdebug(Log);
                                                    }
                                         }
                                         
                                         if(ds->sequence_number<8 && ds->sequence_number>0)nds_data.prev_nonadvert = ds->non_advert;
                                         if(ds->sequence_number==0 && nds_data.prev_nonadvert!=ds->non_advert)nds_data.multipleIDs=1;
                                         
                                         
                                         if(ds->sequence_number==8 && nds_data.multipleIDs && nds_data.FirstBeaconID==ds->gameID)
                                         {
                                                if(nds_data.beacon_thing==1)
                                                {
                                                    
                                                    bool got_it=1;
                                                    
                                                        for(int i=0; i<9; i++)
                                                        {
                                         
                                                            if(nds_data.found_beacon[((int)ds->gameID*10)+i])
                                                                got_it=0;
                                                        }
                                         
                                                    nds_data.FoundAllBeacons=1;
                                                    
                                                        if(got_it && !nds_data.FoundGameIDs)
                                                        {
                                                            int total=0;
                                                            
                                                                for(int i=0; i<15; i++)
                                                                {
                                                                    if(nds_data.foundIDs[i])total++;
                                                                }
                                                            
                                                            nds_data.FoundGameIDs=1;
                                                            printf("Found %d demos broadcasted in capture.\n",total);
                                                        }
                                         
                                                }
                                                else
                                                {
                                                    nds_data.beacon_thing=1;
                                                }
                                         }
                                       
                                        memcpy(host_mac,Beacon->srcmac,6);
                                        
                                        nds_data.beacon_checksum[(int)ds->advert_sequence_number] = checksum;
                                        
                                        
											if(DEBUG)
											{
												fprintfdebug(Log,"FOUND BEACON DSSEQ %d AD %d BC %d CON %d LEN %d DS %d BC %d LEN %d ID %d NONAD %d PKTNUM %d\n",
												(int)ds->sequence_number,(int)ds->advert_sequence_number,
												ReadSeq(&Beacon->seq), (int)ds->connected_clients, (int)ds->advert_len,
												(int)ds->data_size,(int)ds->advert_len,(int)ds->advert_len,(int)ds->gameID,(int)ds->non_advert, (int)GetPacketNumber());
												fflushdebug(Log);
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
     if(!nds_data.data_init)
	{
    nds_data.data_init=1;

    nds_data.total_binaries_size = ((int)nds_data.header.arm9binarySize + (int)nds_data.header.arm7binarySize);

		if(DEBUG)
		{
			fprintfdebug(Log,"ARM9SZ %d ARM7SZ %d\n",(int)nds_data.header.arm9binarySize,(int)nds_data.header.arm7binarySize);
			
			fprintfdebug(Log,"BINARIES SIZES %d\n",nds_data.total_binaries_size);
			fflushdebug(Log);
        }
    nds_data.saved_data = (unsigned char*)malloc((size_t)nds_data.total_binaries_size);
    nds_data.data_sizes = (int*)malloc((sizeof(int)*(7980*4)));
    nds_data.arm7s = 0;nds_data.arm7e = 0;
    if(nds_data.saved_data==NULL || nds_data.data_sizes==NULL)
    {
    printf("FATAL ERROR: Failed to allocate memory for binaries data.\n");
		if(DEBUG)
		{
			fprintfdebug(Log,"FATAL ERROR: Failed to allocate memory for binaries data.\n");
			fflushdebug(Log);
        }
		
	system("PAUSE");
	
	    if(DEBUG)
	    {
            exit(1);
            return;
        }
    }
    memset(nds_data.saved_data,0,(size_t)nds_data.total_binaries_size);
    memset(nds_data.data_sizes,0,(sizeof(int)*(7980*4)));
    nds_data.last_dat_seq=1;
		if(DEBUG)
		{
			fprintfdebug(Log,"FINISHED INIT\n");
			fflushdebug(Log);
        }
		
    }
}
