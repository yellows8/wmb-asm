/*
binconv is licensed under the MIT license:
Copyright (c) 2008 - 2010 yellowstar6

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

unsigned short calccrc16(unsigned char *data, unsigned int length);
void ConvertEndian(void* input, void* output, int input_length);
unsigned char *ConvertBinBuff(unsigned char *data, int length);
typedef enum{false = 0, true = 1} bool;

typedef struct _NCBin_Header
{
    unsigned char mgc_num[5];//Should be 00 00 02 00 00
    unsigned char unk1[7];
    unsigned int unk_int1;
    unsigned int unk_int2;
    unsigned char unknown[8];
    unsigned short game_title[49];//The name of the binary displayed on the client DS.
    unsigned short game_description[97];//The description of the binary. game_title and game_description need combined, with a newline (0A 00) character in between both of these, for the actual desciption fields in the .nds banner - the .nds banner description field from a Nintendo Channel demo .bin contains data that appears to be used for purposes other than the normal purpose of the banner description, it's not description data.
    unsigned short game_name[61];//The demo name displayed on the Wii when broadcasting/transferring the demo to the DS
    unsigned char padding;
    unsigned char unk[12];
} __attribute__ ((__packed__)) NCBin_Header;

//v4 header notes: u32 at offset 0x4 is filesize. u32 at 0x14 is size of header. u32 at offset 0x18 is size of the .nds.(bin size minus header size.) u32 at 0x140 is removal date timestamp. Next u32 is company ID. Next u32 is titleid.

#define ENDIAN_LITTLE 0
#define ENDIAN_BIG 1
bool machine_endian=ENDIAN_LITTLE;

typedef struct _TNDSBanner {
  unsigned short version;
  unsigned short crc;
  unsigned char reserved[28];
  unsigned char icon[512];
  unsigned short palette[16];
  unsigned short titles[6][128];
} __attribute__ ((__packed__)) TNDSBanner;

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

int GetFileLength(FILE* _pfile)
{
	int l_iSavPos, l_iEnd;

	l_iSavPos = ftell(_pfile);
	fseek(_pfile, 0, SEEK_END);
	l_iEnd = ftell(_pfile);
	fseek(_pfile, l_iSavPos, SEEK_SET);

	return l_iEnd;
}

void ConvertBin(char *filename)
{
    printf("Converting %s...\n",filename);

    FILE *fbin, *fnds;
    unsigned char *input_buffer, *output_buffer;
    int length;
    unsigned int romsize;
    char str[256];
    memset(str, 0, 256);

    fbin = fopen(filename, "rb");

    if(fbin==NULL)
    {
        printf("Failed to open file %s\n",filename);
        return;
    }

    strncpy(str, filename, strlen(filename) - 4);
    strcat(str, ".nds");

    length = GetFileLength(fbin);//Nintendo Channel .bin files for demos, have a special header 0x1C8 bytes long. Remove that, and you got an .nds.
    input_buffer = (unsigned char*)malloc(length);
    memset(input_buffer, 0, length);
    fread(input_buffer, 1, length, fbin);

    fclose(fbin);

    output_buffer = ConvertBinBuff(input_buffer, length);
    if(output_buffer==NULL)
    {
        printf("Failed to convert the file.\n");
        return;
    }

    fnds = fopen(str, "wb");
    if(fnds==NULL)
    {
        printf("Failed to open file %s\n", str);
        return;
    }

    if(input_buffer[2]<4)
    {
        memcpy(&romsize, &input_buffer[0x1c8 + 0x80], 4);
        fwrite(output_buffer, 1, romsize + 136, fnds);
    }
    if(input_buffer[2]>=4)
    {
        memcpy(&romsize, &input_buffer[0x14c + 0x80], 4);
        fwrite(output_buffer, 1, romsize + 136, fnds);
    }

    fclose(fnds);

    printf("Successfully converted the file to %s!\n",str);
}

unsigned char *ConvertBinBuff(unsigned char *data, int length)
{
    unsigned char *nds;
    NCBin_Header *header = (NCBin_Header*)data;
    unsigned int romsize = 0;

    if(data[2]>6)
    {
        printf("Unsupported .bin header version.\n");
        return NULL;
    }

    unsigned int hdrsz = 0x14c;
    if(data[2]<4)hdrsz = 0x1c8;
    nds = (unsigned char*)malloc(length - hdrsz);

    memset(nds, 0, length - hdrsz);
    memcpy(&romsize, &data[hdrsz + 0x80], 4);
    memcpy(nds, &data[hdrsz], romsize + 136);

    TNDSHeader *nds_header = (TNDSHeader*)nds;
    unsigned int *bannerOffset = &nds_header->bannerOffset;
    TNDSBanner *banner = (TNDSBanner*)&nds[*bannerOffset];
    unsigned short description[128];
    memset(description, 0, 128 * 2);
    //int end = 0;
    int offset = 0;
    int i;

    if(data[2]<4)//For bin versions less than 4, fix the banner with the info from the .bin header.
    {
    	    memset(banner->titles, 0, 1536);
	    for(i=0; i<49; i++)
	    {
	        if(header->game_title[i]==0)break;//If we found the null terminating character, break, write the newline character, then handle the description.
	
	        description[offset] = header->game_title[i];
	
	        ConvertEndian(&description[offset], &description[offset], 2);//The bytes in the UTF characters are byte swapped, this fixes that.
	        offset++;
	    }

	    description[offset] = 0x000A;
	    offset++;
	
	    for(i=0; i<97; i++)
	    {
	        if(header->game_description[i]==0)break;//If we found the null terminating character, break, write the newline character, then handle the description.
	
	        description[offset] = header->game_description[i];
	
	        ConvertEndian(&description[offset], &description[offset], 2);
	
	        offset++;
	    }
	
	    /*description[offset] = 0xFFFF;
	    offset++;
	
	    for(int i=0; i<97; i++)
	    {
	        if(header->game_name[i]==0)break;//If we found the null terminating character, break, write the newline character, then handle the description.
	
	        description[offset] = header->game_name[i];
	
	        offset++;
	    }*/
	
	    memcpy(&banner->titles[0][0], description, 256);
	    memcpy(&banner->titles[1][0], description, 256);
	    memcpy(&banner->titles[2][0], description, 256);
	    memcpy(&banner->titles[3][0], description, 256);
	    memcpy(&banner->titles[4][0], description, 256);
	    memcpy(&banner->titles[5][0], description, 256);
	
	    banner->crc = calccrc16(banner->icon, 2080);
    }

    return nds;
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
unsigned short calccrc16(unsigned char *data, unsigned int length)
{
	unsigned short crc = 0xFFFF;
	unsigned int i;
	for (i=0; i<length; i++)
	{
		crc = (crc >> 8) ^ crc16tab[(crc ^ data[i]) & 0xFF];
	}

	return crc;
}

void CheckEndianA(void* input, int input_length)
{
     if(input_length!=4)return;

     int wanted_value=64;
     int *int_input=(int*)input;

     if(wanted_value==*int_input)machine_endian=ENDIAN_BIG;
     if(wanted_value!=*int_input)machine_endian=ENDIAN_LITTLE;
}

void ConvertEndian(void* input, void* output, int input_length)
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
     int i;
     for(i=1; i<=input_length; i++)
     {
             out[I-1]=in[i-1];
             I--;
     }

     memcpy(output,out,(size_t)input_length);

     free(out);
     free(in);
}

int main(int argc, char *argv[])
{
    int i;
    if(argc==1)
    {
        printf("Binconv v1.0 by yellowstar6 08/24/08\n");
        printf("Convert Nintendo Channel DS Demo .bin files to an .nds.\n");
        printf("Usage:\n");
        printf("binconv <list of input demo.bin>\n");
        printf("The output .nds will have the same filename as the input,\nexcept the extension will be .nds.\n");
    }
    else
    {
        for(i=1; i<=argc-1; i++)
            ConvertBin(argv[i]);
    }

    return 0;
}
