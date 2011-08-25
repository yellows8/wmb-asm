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

unsigned char normal_mac[5] = {0x03,0x09,0xBF,0x00,0x00};

//************VERSION
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

//**********ENDIANS*******************************

#define ENDIAN_LITTE 0
#define ENDIAN_BIG 1
bool machine_endian=ENDIAN_LITTE;

DLLIMPORT void CheckEndianA(void* input, int input_length)
{
     if(input_length!=4)return;

     int wanted_value=64;
     int *int_input=(int*)input;

     if(wanted_value==*int_input)machine_endian=ENDIAN_BIG;
     if(wanted_value!=*int_input)machine_endian=ENDIAN_LITTE;
}

DLLIMPORT void ConvertEndian(void* input, void* output, int input_length)
{
     if(machine_endian==ENDIAN_BIG)return;

     unsigned char *in, *out;
     in=(unsigned char*)malloc((size_t)input_length);
     out=(unsigned char*)malloc((size_t)input_length);
     if(in==NULL || out==NULL)
     {
            printf("FATAL ERROR: FAILED TO ALLOCATE MEMORY FOR CONVERTENDIAN.\n");
            system("PAUSE");
            #ifndef NDS
            exit(1);
            #endif
     }

     memset(out,0,(size_t)input_length);
     memset(in,0,(size_t)input_length);
     memcpy(in,input,(size_t)input_length);

     int I=input_length;
     for(int i=1; i<=input_length; i++)
     {
             out[I-1]=in[i-1];
             I--;

             //if(debug)printf("IN %d %d\n",i,(int)in[i-1]);
     }

     memcpy(output,out,(size_t)input_length);

     free(out);
     free(in);
}

//**************ChangeFilename**********************************
//This changes the the filename in the in var, with filename
//So if in equals "C:/Captures/capture.cap", capture.cap would be replaced by, say, NAMCO MUESUM.nds
void ChangeFilename(char *in, char *out, char *filename)
{
     if(in==NULL || out==NULL)return;

     int pos=strlen(in);
     bool found=1;

     while(pos<=0 || found)
     {
                  if(pos==0){found=0;break;}



                  if(in[pos]=='/' || in[pos]=='\\')
                  {
                        pos++;
                        found=0;
                        break;
                  }

                  pos--;
     }

     strncpy(out,in,(size_t)pos);//Copy the filename of input to outout string, upto the extension point.
     strcpy(&out[pos],filename);//Append the extension to output.
}

//*********************READ SEQ**************************************
int ReadSeq(unsigned short *seq)
{
    int Seq=0;

    for(int i=0; i<12; i++)
    {
            if(*seq & BIT(i))
            Seq |= BIT(i);
    }

    return Seq>>4;
}

//*******************COMAPATE MAC*************************************************
bool CompareMAC(unsigned char *a, unsigned char *b)
{
     if(memcmp(a,b,6)==0)return 1;

     return 0;
}

//******************CHECK FRAME CONTROL***************************************************************
bool CheckFrameControl(iee80211_framehead2 *framehead, int type, int subtype)
{
     if(FH_FC_TYPE(framehead->frame_control) == type &&
        FH_FC_SUBTYPE(framehead->frame_control) == subtype)
        {
        unsigned char temp[2];
        memcpy((void*)temp,&framehead->frame_control,2);

              if(temp[1]==0x00)
              {
                            if(framehead->duration_id==0x0000)
                            return 1;
              }
        }

        return 0;
}

//***************CHECK FLOW************************************************************
bool CheckFlow(unsigned char *mac, unsigned char flow)
{
     if(memcmp(mac,normal_mac,5)!=0)return 0;

     if(mac[5]==flow)return 1;

     return 0;
}

//******************CHECK FRAME*********************************************************
bool CheckFrame(unsigned char *data, unsigned char command, unsigned short *size, unsigned char *pos)
{
     iee80211_framehead *fh = (iee80211_framehead*)data;
     unsigned char *dat = &data[24];
     unsigned char rsize=0;
     unsigned short Size=0;

     if (((FH_FC_TYPE(fh->frame_control) == 2) && (FH_FC_SUBTYPE(fh->frame_control) == 2)) && CompareMAC(host_mac, fh->mac2))
     {
          if(CheckFlow(fh->mac1,0))
          {
                                   if(memcmp(dat,host_client_mgc,4)==0)
                                   {
                                   dat+=4;

                                   rsize=*dat;
                                   Size = ((unsigned short)(rsize<<1));

                                   dat++;

                                     if(*dat==0x11)//Command packet, ignore non-command packets
                                     {
                                     dat++;
                                       if(*dat==command)
                                       {
                                       //if(command==0x03)
                                       //Size+=2;
                                       if(command==0x04 || command==0x00)
                                       Size-=6;

                                       dat++;

                                       if(pos)
                                       *pos=(unsigned char)((int)dat - (int)data)+2;
                                       if(size)*size=Size;

                                       return 1;
                                       }
                                     }
                                   }
          }
     }

     return 0;
}

//**********************NINTENDO WMB BEACON********************************************
//Based on masscat's function in his WMB client
/*
 * Determine if the frame is a nintendo WMB beacon and returns
 * a pointer to the Nintendo IE if it is.
 */
unsigned char *nintendoWMBBeacon( unsigned char *frame, int frame_size) {
  unsigned char *nin_ie = NULL;
  int i=0x24;
  bool not_found=1;
  int length=0;

  if ( frame[0] == 0x80) {
    /* FIXME: can the beacon size vary? *///yellowstar: Indeed, it can vary
    //if ( frame_size == WMB_BEACON_SIZE) {

         while(i<frame_size)
         {
            //Keep going through the tags until we reach the end of the data
                         if(frame[i]==NINTENDO_IE_TAG)//We found the Nintendo tag!
                         {
                                                      nin_ie=&frame[i+2];
                                                        length=((int)frame[i+1]);
                                                      not_found=0;
                                                      break;
                         }
                         length=((int)frame[i+1]);//Grab the tag's length
                         //fprintfdebug(Log,"FRAME IE %d LEN %d\n",(int)frame[i],length);

                             i+= length+2;
                             //Skip to the next tag by reading the current tag's length,
                             //and by jumping ahead by that amount.

                         if(!not_found)break;
          }

      //}
    }

  return nin_ie;
}

//*********************AVS*******************************************************
unsigned char *IsValidAVS(u_char *pkt_data)
{
     AVS_header *avs = (AVS_header*)pkt_data;

     if((avs->magic_revision & AVS_magic) && (((unsigned char*)&avs->magic_revision)[3] & AVS_revision))
     {
     ConvertAVSEndian(avs);

     //fprintf(log,"LENGTH %u ",avs->header_length);
         if(avs->PHY_type==AVS_PHY_type)
         {
         //avs->channel-=218103795;
         //printf("Channel: %u\n",avs->channel);

              //if(avs->data_rate<=AVS_DATA_RATE)
              //{

                   if(avs->SSI_type==AVS_SSI_TYPE)
                   {

                        if(avs->preamble==AVS_PREAMBLE && avs->encoding_type==AVS_ENCODING)
                        {
                        return pkt_data + ((int)sizeof(AVS_header));
                        }
                        else
                        {
							#ifdef DEBUG
								fprintfdebug(Log,"PD BECAUSE PREAMBLE! PRE %u ENC %u\n",avs->preamble,avs->encoding_type);
								fflushdebug(Log);
							#endif
						}


                   }
                   else
                   {
						#ifdef DEBUG
							fprintfdebug(Log,"SSI TYPE FAILED: %u\n",avs->SSI_type);
							fflushdebug(Log);
						#endif
                   }
         }
         else
         {
			 #ifdef DEBUG
				fprintfdebug(Log,"PHY FAILED: %u\n",avs->PHY_type);
				fflushdebug(Log);
			 #endif
         }
     }

	 #ifdef DEBUG
		fprintfdebug(Log,"PACKET DROPPED!\n");
		fflushdebug(Log);
	 #endif

     return NULL;
}

//**********************AVS ENDIAN*******************************************************
DLLIMPORT void ConvertAVSEndian(AVS_header *avs)
{
     CheckEndianA(&avs->header_length,4);

     ConvertEndian(&avs->header_length,&avs->header_length,4);
     ConvertEndian(&avs->MAC_timestamp,&avs->MAC_timestamp,8);
     ConvertEndian(&avs->host_timestamp,&avs->host_timestamp,8);
     ConvertEndian(&avs->PHY_type,&avs->PHY_type,4);
     ConvertEndian(&avs->channel,&avs->channel,4);
     ConvertEndian(&avs->data_rate,&avs->data_rate,4);
     ConvertEndian(&avs->antenna,&avs->antenna,4);
     ConvertEndian(&avs->priority,&avs->priority,4);
     ConvertEndian(&avs->SSI_type,&avs->SSI_type,4);
     ConvertEndian(&avs->RSSI_signal,&avs->RSSI_signal,4);
     ConvertEndian(&avs->preamble,&avs->preamble,4);
     ConvertEndian(&avs->encoding_type,&avs->encoding_type,4);
}

//********************************CHECK DATA PACKETS************************************
bool CheckDataPackets(int seq)
{
for(int i=0; i<(int)seq; i++)
     {
            if(nds_data.data_sizes[i]==0)
            {
					#ifdef DEBUG
						fprintfdebug(Log,"_____DATA SIZES FAILED %d\n",i);
						fflushdebug(Log);
					#endif

                return 0;//If we missed any packets, don't begin assembly
            }
     }

     return 1;
}

unsigned char GetGameID(unsigned char *data)
{
    iee80211_framehead2 *framehead = (iee80211_framehead2*)data;
    unsigned Byte1, Byte2, gameID;
    unsigned char *bytes;
    Byte1=0;Byte2=0;gameID=0;
    bytes = (unsigned char*)&framehead->duration_id;
    Byte1 = bytes[0];
    Byte2 = bytes[1];

    if(!nds_data.multipleIDs)return 0;

    if(Byte1==0x00)
    {
        Byte2--;
    }
    else
    {
        if(Byte1 % 8 != 25 && Byte1!=0xFA)//Don't do anything if the remainder from this devision operation is 25
        {
            Byte2++;
        }
    }

    //Byte2--;//I thought this value was 1-based, but apparently it's not...
    gameID = Byte2;

    return gameID;
}

//********************************CRC 16**************************************************
//CRC code written by Frz. CRC32 code at least. CRC16 code is based on the CRC32 code.

const unsigned short crc16tab[] = /* CRC lookup table */
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

void crc16_init(unsigned short *uCrc16)
{
	*uCrc16 = 0xFFFF;
}

void crc16_update(unsigned short *uCrc16, const unsigned char *pBuffer, unsigned long uBufSize)
{
	unsigned long i = 0;

	for(i = 0; i < uBufSize; i++)
		*uCrc16 = (*uCrc16 >> 8) ^ crc16tab[(*uCrc16 ^ *pBuffer++) & 0xFF];

}

void crc16_final(unsigned short *uCrc16)
{
	*uCrc16 = ~(*uCrc16);
}

/*
 * CalcCRC
 */
unsigned short CalcCRC16(unsigned char *data, unsigned int length)
{
	unsigned short crc = 0xFFFF;
	unsigned int i;
	for (i=0; i<length; i++)
	{
		crc = (crc >> 8) ^ crc16tab[(crc ^ data[i]) & 0xFF];
	}

	return crc;
}

//************************************CRC32*************************************
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;

#define UPDC32(ch, crc) crc = (( crc >> 8 ) ^ crc_32_tab[(( crc ^ ch ) & 0xFF )]);
u32 crc32buf(char *buf, size_t len);

static u32 crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
 0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

u32 updateCRC32(unsigned char ch, u32 crc)
{
      return (( crc >> 8 ) ^ crc_32_tab[(( crc ^ ch ) & 0xFF )]);;
}

u32 crc32buf(char *buf, size_t len)
{
      register u32 oldcrc32;

      oldcrc32 = 0xFFFFFFFF;

      for ( ; len; --len, ++buf)
      {
			UPDC32(*buf,oldcrc32);
      }

      return oldcrc32 ^ 0xffffffffUL;

}

unsigned int CalcCRC32(unsigned char *data, unsigned int length)
{
    return crc32buf((char*)data, length);
}

//**************************computeBeaconChecksum****************************************
//From Juglak's WMB Host
// got this from some wiki somewhere... dabu for that...
unsigned short computeBeaconChecksum(unsigned short *data, int length) {
  unsigned int sum = 0;
  for (int j = 0; j < length; j++) sum += *data++;
  sum = (-((sum >> 16) + (sum & 0xFFFF) + 1)) & 0xFFFF;
  return sum;
}

int GetFileSize(FILE* _pfile)
{
	int l_iSavPos, l_iEnd;

	l_iSavPos = ftell(_pfile);
	fseek(_pfile, 0, SEEK_END);
	l_iEnd = ftell(_pfile);
	fseek(_pfile, l_iSavPos, SEEK_SET);

	return l_iEnd;
}


//******************************ExecuteApp***********************************************
#ifdef WIN32
void ExecuteApp(char *appname, char *cmdline)
{
    STARTUPINFO si;
     PROCESS_INFORMATION pi;

     char *acstr = (char*)malloc(256);
     memset(acstr, 0, 256);

     ZeroMemory(&si,sizeof(si));
     si.cb=sizeof(si);
     ZeroMemory(&pi,sizeof(pi));

     //si.dwFlags = STARTF_USESTDHANDLES;

     wsprintf(acstr,"%s %s",appname,cmdline);

     if(!CreateProcess(
     NULL,//appname,
     acstr,//cmdline,
     NULL,
     NULL,
     false,
     0,
     NULL,
     NULL,
     &si,
     &pi))
     {
            #ifdef DEBUG
                sprintf(acstr,"CreateProcess Failed: %d",GetLastError());
                fprintfdebug(Log,"%s\n",acstr);
            #endif

            if(GetLastError() == ERROR_FILE_NOT_FOUND)
            printf("Failed to execute application ndsrsa, because the file was not found.\n");

          free(acstr);

          return;
     }

     WaitForSingleObject(pi.hProcess,INFINITE);
     CloseHandle(pi.hProcess);
     CloseHandle(pi.hThread);

     free(acstr);
}
#endif
