//This program, DSPacketCapture, is based on wifi_lib_test, which is written by sgstair.
/****************************************************************************** 
DSPacketCapture is licenced under the MIT open source licence:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

//Almost everything in this block is from wifi_lib_test, written by sgstair. The ones marked with "by yellowstar" are written by yellowstar.
//Block start
#include <nds.h>
#include <stdio.h>
#include <time.h>

#include <dswifi9.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fat.h>

#include "extra.c"//These three preprocessor directives are written by yellowstar.
#include "defs.h"
#include "utils.c"

#define MAX_PACKET_SIZE 3192
#define MAX_NUM_PACKETS 128
int packetsizes[MAX_NUM_PACKETS];
unsigned char * capture_data;
int firstpacket, lastpacket;
int pktnum;
//Block end

int capture_enabled=0;
pcapDumpPacketHeader pkthdr;

int *captureDataFrontSizes;
int *captureDataBackSizes;
unsigned char *captureDataFront;
unsigned char *captureDataBack;
bool captureDataLocked;
int capturedPackets;
int capFirst, capLast;

long TheTimer = 0;
double MAC_stamp = 0;//352106476
double Host_stamp = 0;//6253182424706

//Used for channel scanning, but that has been disabled.
int channel_inter=0;//Increments every time ChannelScanner is triggered. When it hits 10, it advances to the next channel.
int channel_chan = 1;//The channel we are scanning.
bool channel_halt = 0;//Set to one if we went through all the channels and didn't find anything.
bool channel_found=0;//Set to one if we found the correct channel.

bool found_it = 0;//Set to true when we find the correct channel. Set to true all the time later since channel scanning is disabled.

FILE *fcap = NULL;//FILE* for the capture

FILE *Log = NULL;//Text file for debugging

void ConvertAVSEndian(AVS_header *avs);

typedef struct tCommandControl
{
bool shutdown;//If true, shutdown/turn off/poweroff the DS.
} CommandControl;
#define commandControl ((CommandControl*)((uint32)(IPC) + sizeof(TransferRegion)))

//These three functions are from wifi_lib_test, written by sgstair.
//---------------------------------------------------------------------------------
void Timer_50ms(void) {
//---------------------------------------------------------------------------------
   Wifi_Timer(50);
}

//---------------------------------------------------------------------------------
void arm9_synctoarm7() { // send fifo message
//---------------------------------------------------------------------------------
   REG_IPC_FIFO_TX=0x87654321;
}

//---------------------------------------------------------------------------------
void arm9_fifo() { // check incoming fifo messages
//---------------------------------------------------------------------------------
   u32 value = REG_IPC_FIFO_RX;
   if(value == 0x87654321) Wifi_Sync();
}

/*
//This scans channels for WMB beacons, but it has been disabled.
void ChannelScanner()
{
channel_inter++;
if(channel_inter>=50)
{
//CheckPackets();
channel_inter=0;

if(channel_chan==13)channel_chan = 14;
if(channel_chan==7)channel_chan = 13;
if(channel_chan==1)channel_chan = 7;

Wifi_SetChannel(channel_chan);
if(channel_chan>13)channel_halt=1;
}

}*/

void GetTime(pcapDumpPacketHeader *pkthdr)
{
//Gets the current time
pkthdr->tv_sec = time(NULL);//Gets unix time, which libpcap captures uses.
pkthdr->tv_usec = (long)TIMER0_DATA / 33;//Gets the microseconds.
}

unsigned int WriteAVS(unsigned char *data, unsigned int length, FILE *fp)
{
//Writes the AVS header to the capture.
AVS_header avs;
memset(&avs,0,sizeof(AVS_header));
unsigned int mgc_number = 0x80211001;
unsigned int len;
len = length;
len+=64;

//Set all of the fields to what they usually are in WMB captures.
avs.magic_revision = mgc_number;
avs.header_length = 64;
avs.MAC_timestamp = MAC_stamp;//No clue what these are, or how to calculate them, so I'll just fake the timestamps.
avs.host_timestamp = Host_stamp;
MAC_stamp++;
Host_stamp++;
avs.PHY_type = AVS_PHY_type;
avs.channel = channel_chan;
avs.data_rate = AVS_DATA_RATE;
avs.antenna = 0;
avs.priority = 0;
avs.SSI_type = AVS_SSI_TYPE;
avs.RSSI_signal = 750;
memset(avs.unk,0,4);
avs.preamble = AVS_PREAMBLE;
avs.encoding_type = AVS_ENCODING;

//Don't know why this is needed, but we need to flip the endians.(DS is little-endian, I think, and AVS is little-endian)
ConvertEndian(&avs.magic_revision,&avs.magic_revision,4);
ConvertAVSEndian(&avs);

fwrite(&avs.magic_revision,1,sizeof(unsigned int),fp);
fwrite(&avs.header_length,1,sizeof(unsigned int),fp);
fwrite(&avs.MAC_timestamp,1,sizeof(double),fp);
fwrite(&avs.host_timestamp,1,sizeof(double),fp);
fwrite(&avs.PHY_type,1,sizeof(unsigned int),fp);
fwrite(&avs.channel,1,sizeof(unsigned int),fp);
fwrite(&avs.data_rate,1,sizeof(unsigned int),fp);
fwrite(&avs.antenna,1,sizeof(unsigned int),fp);
fwrite(&avs.priority,1,sizeof(unsigned int),fp);
fwrite(&avs.SSI_type,1,sizeof(unsigned int),fp);
fwrite(&avs.RSSI_signal,1,sizeof(unsigned int),fp);
fwrite(&avs.unk,1,4,fp);
fwrite(&avs.preamble,1,sizeof(unsigned int),fp);
fwrite(&avs.encoding_type,1,sizeof(unsigned int),fp);

return len;

}

void WritePacket(unsigned char *data, int length, FILE *fp)
{
//Writes the packets found to the capture

if(length==0)return;

unsigned int len=0;
len = (unsigned int)length;
fprintf(Log,"BEFORE WRITING\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");

len+=64;
memset(&pkthdr,0,sizeof(pcapDumpPacketHeader));
GetTime(&pkthdr);//Get the time
pkthdr.packetLength = (unsigned int)len;//Set the packet length fields in the packet header
pkthdr.captureLength = (unsigned int)len;

fprintf(Log,"a\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");
fwrite(&pkthdr.tv_sec,1,sizeof(long),fp);//Write the packet header
fwrite(&pkthdr.tv_usec,1,sizeof(long),fp);
fwrite(&pkthdr.packetLength,1,sizeof(unsigned int),fp);

fprintf(Log,"b\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");
fwrite(&pkthdr.captureLength,1,sizeof(unsigned int),fp);
fprintf(Log,"c\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");

WriteAVS(data,(unsigned int)length,fp);//Write the AVS header

fprintf(Log,"d\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");

fwrite(data,1,length,fp);//Write the actual data
fprintf(Log,"e\n");
fclose(fp);
fp = fopen("/capture.cap","ab");//Normally, I'd use fflush, but that wouldn't work right...

/*fprintf(Log,"LEN1 %d LEN2 %u C1 %u C2 %u\n",length,len,pkthdr.packetLength,pkthdr.captureLength);
fclose(Log);
Log = fopen("/log.txt","a");*/

fprintf(Log,"PACKET WRITE END\n");
fclose(Log);
Log = fopen("/capture_log.txt","a");

}

//This function is based on the function with the same name from masscat's WMB client
/*
 * Determine if the frame is a nintendo WMB beacon and returns
 * a pointer to the Nintendo IE if it is.
 */
 #define NINTENDO_IE_TAG 0xDD

#define WMB_BEACON_SIZE 188
unsigned char *nintendoWMBBeacon( unsigned char *frame, int frame_size) {
  unsigned char *nin_ie = NULL;
  int i=0x24;
  bool not_found=1;
  int length=0;

  if ( frame[0] == 0x80) {

         while(i<frame_size)
         {
            //Keep going through the tags until we reach the end of the data
                         if(frame[i]==NINTENDO_IE_TAG)//We found the Nintendo tag!
                         {
                                                      nin_ie=&frame[i+2];
                                                       //if ( !nin_ie[NIN_WMB_IE_NON_ADVERT_FLAG])
                                                       //{



                                                        length=((int)frame[i+1]);
                         //fprintfdebug(log,"TAG IE %d LEN %d\n",(int)frame[i],length);
                                                      not_found=0;
                                                      break;
                         }
                         length=((int)frame[i+1]);//Grab the tag's length
                         //fprintfdebug(log,"TAG IE %d LEN %d\n",(int)frame[i],length);

                             i+= length+2;
                             //Skip to the next tag by reading the current tag's length,
                             //and by jumping ahead by that amount.

                         if(!not_found)break;
          }

      }

  //fprintfdebug(log,"NIN IE %d\n",i);

  return nin_ie;
}

//The same as the same function from wifi_lib_test by sgstair, minus the comments.
void PacketCaptureHandler(int packetID, int packetlength) {
	//Called every time a packet is captured, by dswifi
	int i;
	if(!capture_enabled) return;//Don't capture any packets until we're ready
	i=lastpacket+1;
	if(i==MAX_NUM_PACKETS) i=0;
	if(firstpacket==i) {
		pktnum++;
		if(firstpacket+1==MAX_NUM_PACKETS) firstpacket=0; else firstpacket++;
	}
	if(packetlength>MAX_PACKET_SIZE) packetlength=MAX_PACKET_SIZE;
	Wifi_RxRawReadPacket(packetID,packetlength,(unsigned short *)(capture_data+MAX_PACKET_SIZE*i));
	packetsizes[i]=packetlength;
	lastpacket=i;
	
	/*if(!found_it)
	{
		//This is used in channel scanning, but that's disabled.
		if(nintendoWMBBeacon( (unsigned char*)(capture_data+(MAX_PACKET_SIZE*i)), packetlength)!=NULL)
			channel_found=1;
	}*/
	
	//if(found_it)
	//{
	 //while(captureDataLocked);
	 
	 /*captureDataBackSizes[capturedPackets] = packetlength;
	 memcpy(&captureDataBack[capturedPackets*MAX_PACKET_SIZE],&capture_data[MAX_PACKET_SIZE*i],packetlength);
	 capturedPackets++;*/
	//}
	
}

//This function is based on the same function from wifi_lib_test, written by sgstair. Code was added, written by yellowstar,
//for saving the captured packets to FAT.
void Do_Play_PacketCapture() {
	firstpacket=0; lastpacket=0;
	int capchan,capnum;
	int sel,tsel;
	int i,j,k,n,z;
	int scrolltop;
	char buf[64];
	int basetouchy,basetouchscroll,basetouchnum;
	
	unsigned char *packetdata=NULL;
	basetouchscroll=-2;
	basetouchnum=1;
	basetouchy=96;
	pcapDumpHeader caphdr;
	memset(&caphdr,0,sizeof(pcapDumpHeader));
	memset(&pkthdr,0,sizeof(pcapDumpPacketHeader));
	
	fcap = fopen("/capture.cap","wb");
	
	if(fcap==NULL)
	{
	 while(1)
	 {
	 WaitVbl();
	 ClsBtm();
	 
     
	 printbtm(0,0,"Failed to open capture.cap for writing in the root of the card!");
	 }
	}

	capchan=1;
	pktnum=0;
	
	channel_inter= 0;//Init channel scanning stuff. Some of these are still used by some code.
	channel_chan = 1;
	channel_halt = 0;
	channel_found =0;
	
	capture_data = (unsigned char *) malloc(MAX_PACKET_SIZE*MAX_NUM_PACKETS);//Buffer which contains the actual packets
	memset(capture_data,0,MAX_PACKET_SIZE*MAX_NUM_PACKETS);
	//captureDataFront = (unsigned char *) malloc(MAX_PACKET_SIZE*MAX_NUM_PACKETS);
	//captureDataBack = (unsigned char *) malloc(MAX_PACKET_SIZE*MAX_NUM_PACKETS);
	//memset(captureDataFront,0,MAX_PACKET_SIZE*MAX_NUM_PACKETS);
	//memset(captureDataBack,0,MAX_PACKET_SIZE*MAX_NUM_PACKETS);
	
	//captureDataFrontSizes = (int*)malloc(MAX_NUM_PACKETS*sizeof(int));
	//captureDataBackSizes = (int*)malloc(MAX_NUM_PACKETS*sizeof(int));
	//memset(captureDataFrontSizes,0,MAX_NUM_PACKETS*sizeof(int));
	//memset(captureDataBackSizes,0,MAX_NUM_PACKETS*sizeof(int));
	
	captureDataLocked=0;
	capturedPackets=0;
	capFirst=0;capLast=0;//Has to do with writing packets.

	Wifi_EnableWifi();//Init Wifi for Arm9.
	Wifi_SetChannel(capchan);
	Wifi_RawSetPacketHandler(PacketCaptureHandler);
	capture_enabled=1;
	Wifi_SetPromiscuousMode(1);
	
	/*
	//For channel scanning, but that's disabled.
	TIMER2_DATA = (unsigned short) TIMER_FREQ_64(8);
	TIMER2_CR = TIMER_DIV_64 | TIMER_ENABLE | TIMER_IRQ_REQ;
	irqEnable(IRQ_TIMER2);
	irqSet(IRQ_TIMER2, ChannelScanner);*/
	
	caphdr.magic = 0xa1b2c3d4;//A program reading a capture would check this field when reading. If it's value, due to endians, is not the same as it's version of this value, it flips the endians in the rest of the capture header, and the packet headers.(So they can be read correctly)
	caphdr.majorVersion = 2;
	caphdr.minorVersion = 4;
	caphdr.timeZoneOffset = 0;
	caphdr.timeStampAccuracy = 0;
	caphdr.snapshotLength = 65535;//Max length of each packet
	caphdr.linkLayerType = 163;
	REG_IME=0;
	fwrite(&caphdr,1,sizeof(pcapDumpHeader),fcap);
	fclose(fcap);
	fcap = fopen("/capture.cap","ab");
	REG_IME=1;
	
	scrolltop=0;
	sel=-1;
	
	Log = fopen("/capture_log.txt","w");//Open the Log text file for debugging
	if(Log==NULL)
	{
	 while(1)
	 {
	 WaitVbl();
	 ClsBtm();
	 
     
	 printbtm(0,0,"Failed to open capture_log.txt for writing in the root of the card!");
	 }
	}
	
	// config screen:
	irqDisable(IRQ_TIMER3);
	/*
	//This was used in channel scanning. Commented out for reasons mentioned before.(written by yellowstar)
	while(1) {
	    Wifi_Update();
		WaitVbl();
		ClsBtm();
		
		//time_t unixTime = time(NULL);
		//struct tm* GMtime = gmtime((const time_t *)&the_time);

		printbtm(0,0,"Scanning for WMB traffic on all channels");
		sprintf(buf,"Channel %i",capchan);
		btm_drawbutton(2,2,29,4,0,buf);
		
		//sprintf(buf,"time_t %d",(int)unixTime);
		printbtm(0,5,buf);
		
		capnum=lastpacket-firstpacket;
	  if(capnum<0) capnum+=MAX_NUM_PACKETS;
	  tsel=sel;
	  if(tsel==-1) tsel=capnum+pktnum;
	  if(sel==-1) scrolltop=capnum-7;
		
		if(capnum>0) {
		  j=tsel%MAX_NUM_PACKETS;
		  packetdata = &capture_data[j*MAX_PACKET_SIZE];
		  //WritePacket(packetdata,packetsizes[j],fp);
	  }
		
		if(capchan!=channel_chan)capchan = channel_chan;
		
		if(channel_halt)
		{
		
			while(1)
			{
			WaitVbl();
			ClsBtm();
			
			printbtm(0,2,"Failed to find any WMB traffic! Stop!");
			}
		}
		
		if(channel_found)
		{
		irqDisable(IRQ_TIMER2);
		break;
		}

	}*/

	capFirst=lastpacket;
	capLast=lastpacket;
	found_it=1;
	
	SwitchTopDisplay(0);
	//irqDisable(IRQ_TIMER2);
	while(1) {
		Wifi_Update();
      WaitVbl();
      ClsBtm();
	  ClsTopAlt(0);
	  // process input
	  //scanKeys();
	  capnum=lastpacket-firstpacket;
	  if(capnum<0) capnum+=MAX_NUM_PACKETS;
	  tsel=sel;
	  if(tsel==-1) tsel=capnum+pktnum;
	  if(sel==-1) scrolltop=capnum-7;

if(keysDown() & KEY_SELECT)//Close opened files, write a file entitled dump.bin in the FS root, and shutdown the DS.
	  {
	  fclose(fcap);
   
   Wifi_DisableWifi();
	capture_enabled=0;
	Wifi_RawSetPacketHandler(0);
	Wifi_SetPromiscuousMode(0);
	
	FILE *dump = fopen("/dump.bin","wb");
	  fwrite(capture_data,MAX_PACKET_SIZE,MAX_NUM_PACKETS,dump);
	  fclose(dump);
	
	commandControl->shutdown=1;
	
	while(1);
	  }
	  
	  if(keysDown() & KEY_START)//Create a dump file
	  {
	  FILE *dump = fopen("/dump.bin","wb");
	  fwrite(capture_data,MAX_PACKET_SIZE,MAX_NUM_PACKETS,dump);
	  fclose(dump);
	  }
	  
	  if(keysDown() & KEY_R)//Increase the current channel variable by one, and switch to it.
	  {
	  capchan++;
	  if(capchan>13)capchan=1;
	  if(capchan<11)capchan=13;
	  Wifi_SetChannel(capchan);
	  }
	  if(keysDown() & KEY_L)//Decrease the current channel variable by one, and switch to it.
	  {
	  capchan--;
	  if(capchan>13)capchan=1;
	  if(capchan<11)capchan=13;
	  Wifi_SetChannel(capchan);
	  }
	  
	  
	  if(keysDown()&(KEY_UP)) {
		  scrolltop--;
	  }
	  if(keysDown()&(KEY_DOWN)) {
		  scrolltop++;
	  }
	  if(keysDown()&(KEY_TOUCH)) {
		  basetouchscroll=-2;
		  if(touchXY.px>230) {
			  if(touchXY.py<16) {
				  scrolltop--;
			  } else if(touchXY.py>176) {
				  scrolltop++;
			  } else {
				  basetouchy=touchXY.py;
				  basetouchscroll=scrolltop;
				  basetouchnum=capnum;
				  if(basetouchnum==0) basetouchnum=1;
			  }
		  }
	  }
	  if(keysHeld()&KEY_TOUCH && basetouchscroll!=-2) {
		  scrolltop=basetouchscroll+(((touchXY.py-basetouchy)*basetouchnum)/160);
	  }

	  if(scrolltop>capnum-7) scrolltop=capnum-7;
	  if(scrolltop<0) scrolltop=0;

	  sprintf(buf,"Chan%i - List%i - Total%i",capchan,capnum,capnum+pktnum );
	  printbtm(0,0,buf);
	  printbtm( 0,23,"[UP/DN] Scroll [B] Exit" );
	  printbtm( 0, 1,"\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4" );
	  printbtm( 0, 22,"\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4" );

	  if(capnum<=7) {
		  j=4; k=44;
	  } else {
		  j=(scrolltop*40)/capnum +4;
		  k=((scrolltop+7)*40)/capnum +4;
	  }
	  printbtm( 30, 0,"\xb3\x18");
	  printbtm( 30, 1,"\xc5\xc4");
	  printbtm( 30,22,"\xc5\xc4");
	  printbtm( 30,23,"\xb3\x19");
	  for(i=2;i<22;i++) {
		  if(i*2+1==j) { // top single piece
			  printbtm( 30, i,"\xb3\xdc");
		  } else if(i*2==k) { // bottom single piece
			  printbtm( 30, i,"\xb3\xdf");
		  } else 	if(i*2>=j && i*2<=k) {
			  printbtm( 30, i,"\xb3\xdb");
		  } else {
			  printbtm( 30, i,"\xb3 ");
		  }
	  }

	  if(scrolltop>capnum-7) scrolltop=capnum-7;
	  if(scrolltop<0) scrolltop=0;

	  sprintf(buf,"Chan%i - List%i - Total%i",capchan,capnum,capnum+pktnum );
	  printbtm(0,0,buf);
	  printbtm( 0,23,"[UP/DN] Scroll [B] Exit" ); 
	  printbtm( 0, 1,"\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4" ); 
	  printbtm( 0, 22,"\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4" ); 

	  if(capnum<=7) {
		  j=4; k=44;
	  } else {
		  j=(scrolltop*40)/capnum +4;
		  k=((scrolltop+7)*40)/capnum +4;
	  }
	  printbtm( 30, 0,"\xb3\x18");
	  printbtm( 30, 1,"\xc5\xc4");
	  printbtm( 30,22,"\xc5\xc4");
	  printbtm( 30,23,"\xb3\x19");
	  for(i=2;i<22;i++) {
		  if(i*2+1==j) { // top single piece
			  printbtm( 30, i,"\xb3\xdc");
		  } else if(i*2==k) { // bottom single piece
			  printbtm( 30, i,"\xb3\xdf");
		  } else 	if(i*2>=j && i*2<=k) {
			  printbtm( 30, i,"\xb3\xdb");
		  } else {
			  printbtm( 30, i,"\xb3 ");
		  }
	  }

	  if(capnum>0) {
		  j=tsel%MAX_NUM_PACKETS;
		  packetdata = &capture_data[j*MAX_PACKET_SIZE];
		  
		  for(i=0;i<packetsizes[j];i+=8) {
			  if(i>23*8) break;
			  strcpy(buf,"                                ");
			  for(k=0;k<8;k++) {
				 if(k+i>=packetsizes[j]) break;
				 n=capture_data[j*MAX_PACKET_SIZE+i+k];
				 z=n>>4;
				 if(z>10) buf[k*3] = 'A'+z-10; else buf[k*3]='0'+z;
				 z=n&15;
				 if(z>10) buf[k*3+1] = 'A'+z-10; else buf[k*3+1]='0'+z;
				 if(n<128 && n>31) buf[24+k]=n; else buf[24+k]='.';
			  }
			  printtopalt(0,0,i>>3,buf);
		  }
		  
		  j=tsel%MAX_NUM_PACKETS;
		  
	  }

	  for(i=0;i<7;i++) {
		  if(i+scrolltop>=capnum) break;
		  j=((i+scrolltop+pktnum)%MAX_NUM_PACKETS);
		  n=j*MAX_PACKET_SIZE;
		  sprintf(buf,"%c #%i From:%02X%02X%02X%02X%02X%02X",(i+scrolltop+pktnum)==tsel?'*':' ',i+scrolltop+pktnum,
			  capture_data[n+10],capture_data[n+11],capture_data[n+12],capture_data[n+13],capture_data[n+14],capture_data[n+15]);
		  printbtm( 0, 2+3*i,buf ); 

		  sprintf(buf,"%c To:%02X%02X%02X%02X%02X%02X Len:%i",(i+scrolltop+pktnum)==tsel?'*':' ',
			  capture_data[n+4],capture_data[n+5],capture_data[n+6],capture_data[n+7],capture_data[n+8],capture_data[n+9],packetsizes[j]);
		  printbtm( 0, 3+3*i,buf ); 
		  printbtm( 0, 4+3*i,"\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4\xc4" ); 

	  }
		
		capLast=lastpacket;
		capturedPackets=capLast-capFirst;
		if(capturedPackets<0)capturedPackets=capFirst-capLast;

	   if(capturedPackets>0)//If any more packets were captured since capFist was set last, write the new packets
	   {
			int pos=capLast;
			//REG_IME=0;
			//captureDataLocked=1;
			
			/*unsigned char *temp = captureDataBack;
			int *temp2 = captureDataBackSizes;
			captureDataBack = captureDataFront;
			captureDataFront = temp;
			
			temp2 = captureDataBackSizes;
			captureDataBackSizes = captureDataFrontSizes;
			captureDataFrontSizes = temp2;*/
			
			
			//memset(captureDataBack,0,MAX_PACKET_SIZE*MAX_NUM_PACKETS);
			//memset(captureDataBackSizes,0,MAX_NUM_PACKETS*sizeof(int));
			
				for(i=0; i<capturedPackets; i++)//Go through all of the newly captured packets, then write them to the capture.
				{
					if(pos>=MAX_NUM_PACKETS)pos=0;
					
					fprintf(Log,"POS %d A\n",pos);
					fclose(Log);
					Log = fopen("/capture_log.txt","a");
					WritePacket(&capture_data[MAX_PACKET_SIZE*(pos)],packetsizes[pos],fcap);
					pos++;
					
					if(pos>=MAX_NUM_PACKETS)pos=0;
					
					fprintf(Log,"POS %d B\n",pos);
					fclose(Log);
					Log = fopen("/capture_log.txt","a");
				}
	   
	   //capturedPackets=0;
	   //captureDataLocked=0;
	   //REG_IME=1;
	   }
	   
	   fprintf(Log,"CAPTUREDPKTS %d FIRST %d LAST %d LASTPKT %d\n",capturedPackets,capFirst,capLast,lastpacket);
	   fclose(Log);
	   Log = fopen("/capture_log.txt","a");
	   
	   capFirst=capLast;

   }
   
   

}


//This whole function is almost the same as the main in wifi_lib_test by sgstair, minus the fatInitDefault code.
//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------
	//int sekrit_counter;

	powerON(POWER_ALL_2D | POWER_SWAP_LCDS); 
	psych_mode=0;
	*((volatile u16 *)0x0400010E) = 0;
	irqInit();
	irqEnable(IRQ_VBLANK);
	*((volatile u16 *)0x0400010C) = -6553; // 6553.1 * 256 cycles = ~50ms;
	*((volatile u16 *)0x0400010E) = 0x00C2; // enable, irq, 1/256 clock
	
	initDisplay();

	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;

	

	irqSet(IRQ_TIMER3, Timer_50ms);
	irqEnable(IRQ_TIMER3);

	irqSet(IRQ_FIFO_NOT_EMPTY, arm9_fifo);
	irqEnable(IRQ_FIFO_NOT_EMPTY);

	// Start timer
    TIMER0_DATA = 0xFFFF - 32640;//32768
    TIMER0_CR = TIMER_ENABLE | TIMER_DIV_1;
	

	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ;
	Wifi_SetSyncHandler(arm9_synctoarm7);

	u32 Wifi_pass= Wifi_Init(WIFIINIT_OPTION_USELED);
	REG_IPC_FIFO_TX=0x12345678;
	REG_IPC_FIFO_TX=Wifi_pass;
	
    if(!fatInitDefault())
	{
		while(1)
		{
			WaitVbl();
			ClsBtm();
			
			
			printbtm(0,0,"Failed to initialize FAT!");
			printbtm(0,1,"Did you DLDI patch this .nds for your card?");
			printbtm(0,3,"Press A to shutdown.\n");
			
				if(keysDown() & KEY_A)
				{
					commandControl->shutdown=1;
						
						while(1)swiWaitForVBlank();
				}
		}
	}

	printtop("Waiting for ARM7 to init..");
	while(Wifi_CheckInit()==0) WaitVbl();
	printtop("ARM7 Init Confirmed.");

	int state, touchstate;
	state=0;
	touchstate=-1;
	psych_mode=1;
	
	Do_Play_PacketCapture();

	return 0;
}
