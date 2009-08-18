/*
        BootMii - a Free Software replacement for the Nintendo/BroadOn bootloader.
        Requires mini.

Copyright (C) 2008, 2009        Haxx Enterprises <bushing@gmail.com>
Copyright (C) 2009              Andre Heider "dhewg" <dhewg@wiibrew.org>
Copyright (C) 2008, 2009        Hector Martin "marcan" <marcan@marcansoft.com>
Copyright (C) 2008, 2009        Sven Peter <svenpeter@gmail.com>
Copyright (C) 2009              John Kelley <wiidev@kelley.ca>

# This code is licensed to you under the terms of the GNU GPL, version 2;
# see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
*/

//main.c modified by yellows8.

#ifdef WII_MINI_APP

#include "../bootmii_ppc.h"
#include "../string.h"
#include "../ipc.h"
#include "../mini_ipc.h"
#include "../nandfs.h"
#include "../fat.h"
#include "../malloc.h"
#include "../diskio.h"
#include "../printf.h"
#include "../video_low.h"
#include "../input.h"
#include "../console.h"

#define MINIMUM_MINI_VERSION 0x00010001

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#define BYTE_ORDER BIG_ENDIAN
#endif

#ifndef WII_MINI_APP
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#endif

#ifdef HW_RVL
#include <gccore.h>
#include <wiiuse/wpad.h>
#endif

#ifndef WII_MINI_APP
#ifndef HW_RVL
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#define BYTE_ORDER LITTLE_ENDIAN

#endif
#endif

int gbalzss_main(int argc, char *argv[]);
char argv_str[256];

#ifdef HW_RVL
static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
#endif

typedef struct _DLlist_rating_entry
{
	u8 ratingID;
	u8 boardID;//? For US, 2.
	u8 age;//Required years of age.
	u16 title[11];//Title/text of the rating.
} DLlist_rating_entry;

typedef struct _DLlist_title_type_entry
{
	u8 typeID;
	u8 groupID;
	u16 title[31];//Name of type.
} DLlist_title_type_entry;

typedef struct _DLlist_company_entry
{
	u32 ID;//?
	u16 devtitle[31];
	u16 pubtitle[31];
} DLlist_company_entry;

typedef struct _DLlist_title_entry
{
	u32 ID;
	u32 titleID;//Or DS Game code.
	u8 title_type;
	u8 unk;
	u32 unk2;
	u16 company_offset;
	u32 unk3;
	u8 ratingID;
	u8 unk4[0x13];
	u16 title[62];
	u16 subtitle[31];
} DLlist_title_entry;

typedef struct _DLlist_video_entry
{
	u32 ID;//Decimal ID for URL filename.
	u8 unk1[0xb];
	u16 title[36];
	u8 unk2[0x1e];
} DLlist_video_entry;

typedef struct _DLlist_demo_entry
{
	u32 ID;//Decimal ID for URL filename.
	u16 title[31];
	u16 subtitle[31];//Optional
	u32 titleid;//ID of title entry, not title entry titleid.
	u8 unk[0xa];
} DLlist_demo_entry;

typedef struct _DLlist_header
{
	u16 unk0;
	u8 version;
	u8 ver1;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u8 unk5[0x2b];
	u32 ratings_total;
	u32 ratings_offset;
	u32 total_title_types;
	u32 main_title_types_offset;
	u32 companies_total;
	u32 companies_offset;
	u32 sub_title_types_offset;
	u32 titles_offset;
	u32 unk_title;
	u32 unk_offset;
	u32 videos_total;
	u32 videos_offset;
	u32 demos_total;
	u32 demos_offset;
	u32 wc24msg_opt_offset;
} DLlist_header;

typedef struct _DLlist_header_wrapper
{
	u16 unk0;
	u8 version;
	u8 ver1;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u8 unk5[0x2b];
	u32 ratings_total;
	DLlist_rating_entry *ratings;
	u32 total_title_types;
	DLlist_title_type_entry *main_title_types;
	u32 companies_total;
	DLlist_company_entry *companies;
	DLlist_title_entry *sub_title_types;
	DLlist_title_entry *titles;
	u32 unk_title;
	u32 *unk_data;
	u32 videos_total;
	DLlist_video_entry *videos;
	u32 demos_total;
	DLlist_demo_entry *demos;
	u16 *wc24msg_opt;
} DLlist_header_wrapper;

inline u32 be32(u32 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

inline u32 le32(u32 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

#ifdef WII_MINI_APP
static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}
#endif

#ifdef WII_MINI_APP
int main(void)
#else
int main(int argc, char **argv)
#endif
{
    #ifdef WII_MINI_APP
	int vmode = -1;
	struct nandfs_fp nfs_fil;
	#endif
	char str[256];
	char pstr[256];
	char vff_dumpfn[256];
	unsigned char *buffer;
	int is_csdata = 0;
	#ifdef WII_MINI_APP
	FIL fil;
	u32 tempsz;
	exception_init();
	dsp_reset();

	// clear interrupt mask
	write32(0x0c003004, 0);

	ipc_initialize();
	ipc_slowping();

	gecko_init();
    input_init();
	init_fb(vmode);

	VIDEO_Init(vmode);
	VIDEO_SetFrameBuffer(get_xfb());
	VISetupEncoder();

	u32 version = ipc_getvers();
	u16 mini_version_major = version >> 16 & 0xFFFF;
	u16 mini_version_minor = version & 0xFFFF;
	printf("Mini version: %d.%0d\n", mini_version_major, mini_version_minor);

	if (version < MINIMUM_MINI_VERSION) {
		printf("Sorry, this version of MINI (armboot.bin)\n"
			"is too old, please update to at least %d.%0d.\n",
			(MINIMUM_MINI_VERSION >> 16), (MINIMUM_MINI_VERSION & 0xFFFF));
		for (;;)
			; // better ideas welcome!
	}
    #endif

    #ifdef HW_RVL
    // Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	printf("\x1b[5;5H");
    #endif

    memset(pstr, 0, 256);

    #ifdef WII_MINI_APP
    print_str_noscroll(112, 96, "ninchdl-listext v1.0 by yellows8.");
	print_str_noscroll(112, 112, "Mounting FAT and initalizing nandfs...");
	if(fat_mount()!=0)
	{
		print_str_noscroll(112, 128, "fat_mount failed.");
		return -1;
	}

	if(nandfs_initialize()!=0)
	{
		print_str_noscroll(112, 128, "nandfs_initialize failed.");
		return -1;
	}
    #else

    printf("ninchdl-listext v1.0 by yellows8.\n");
    #ifdef HW_RVL
    printf("Mounting FAT...\n");
    if(!fatInitialize())
    {
        printf("Failed to mount FAT.\n");
    }
    #else
    if(argc!=4)
    {
        printf("Usage:\n");
        printf("ninchdl-listext <wc24dl.vff> <region code> <language code>\n");
        return 0;
    }
    #endif

    #endif

	memset(str, 0, 256);
	memset(vff_dumpfn, 0, 256);
	#ifdef WII_MINI_APP
	print_str_noscroll(112, 128, "Choose a region via GC/GPIO: A/Pwr for E, B/Rst for P, Y/Ejt for J.");
	#endif
	#ifdef HW_RVL
	WPAD_ScanPads();
	#endif
	char region = 'a';
	#ifdef WII_MINI_APP
	while(1)
	{
    #endif

    #ifdef HW_RVL
	while(1)
	{
    #endif

	    #ifdef HW_RVL
	    WPAD_ScanPads();
	    u32 button = WPAD_ButtonsDown(0);
	    if(button & WPAD_BUTTON_A)region = 'E';
	    if(button & WPAD_BUTTON_1)region = 'P';
	    if(button & WPAD_BUTTON_2)region = 'J';
	    #endif

	    #ifdef WII_MINI_APP
		u32 button = (u32)input_wait();
		if(button & PAD_A)region = 'E';
		if(button & PAD_B)region = 'P';
		if(button & PAD_Y)region = 'J';
        #endif

        #ifdef WII_MINI_APP
		button = gpio_read();
		if(button & GPIO_POWER)region = 'E';
		if(button & GPIO_RESET)region = 'P';
		if(button & GPIO_EJECT)region = 'J';
        #endif

    #ifdef WII_MINI_APP
		if(region!='a')break;
	}
	#endif
	#ifdef HW_RVL
        if(region!='a')break;
	}
	#endif

    #ifdef WII_MINI_APP
    sprintf(vff_dumpfn, "/wc24dl.vff");
	#else
        #ifdef HW_RVL
        sprintf(vff_dumpfn, "/wc24dl.vff");
        #else
        strcpy(vff_dumpfn, argv[1]);
        #endif
	#endif
	sprintf(str, "/title/00010001/484154%02x/data/wc24dl.vff", region);
	#ifdef WII_MINI_APP
	if(nandfs_open(&nfs_fil, str)!=0)
	{
		sprintf(pstr, "Failed to open %s", str);
		print_str_noscroll(112, 144, pstr);
		return -3;
	}
	buffer = (unsigned char*)malloc(nfs_fil.size);
	if(buffer==NULL)
	{
		sprintf(pstr, "Failed to allocate 0x%x bytes.", nfs_fil.size);
		print_str_noscroll(112, 160, pstr);
		return -2;
	}

	sprintf(pstr, "Reading %s...", str);
	print_str_noscroll(112, 178, pstr);
	tempsz = nandfs_read(buffer, 1, nfs_fil.size, &nfs_fil);
	if(tempsz!=nfs_fil.size)
	{
		sprintf(pstr, "Read invalid num bytes %x(%d)", tempsz, tempsz);
		print_str_noscroll(112, 194, pstr);
		return -4;
	}

	if(strncmp((char*)buffer, "VFF ", 4)!=0)
	{
		print_str_noscroll(112, 194, "Invalid VFF.");
		return -4;
	}
    #else
        #ifndef HW_RVL
        struct stat fstats;
        FILE *fil = fopen(vff_dumpfn, "rb");
        if(fil==NULL)
        {
            sprintf(pstr, "Failed to open %s\n", str);
            printf(pstr);
            return -3;
        }

        stat(vff_dumpfn, &fstats);
        buffer = (unsigned char*)malloc(fstats.st_size);
        if(buffer==NULL)
        {
            sprintf(pstr, "Failed to allocate 0x%x bytes.\n", (int)fstats.st_size);
            printf(pstr);
            return -2;
        }
        fread(buffer, 1, fstats.st_size, fil);
        fclose(fil);

        if(strncmp((char*)buffer, "VFF ", 4)!=0)
        {
            #ifdef WII_MINI_APP
            print_str_noscroll(112, 194, "Invalid VFF.");
            #else
            printf("Invalid VFF.");
            #endif
            return -4;
        }
        #endif
    #endif

    #ifdef WII_MINI_APP
	print_str_noscroll(112, 194, "Writing to SD...");
	if(f_open(&fil, vff_dumpfn, FA_WRITE | FA_CREATE_ALWAYS)!=0)
	{
		sprintf(str, "Failed to open %s", vff_dumpfn);
		print_str_noscroll(112, 110, str);
		return -3;
	}

	f_write(&fil, buffer, nfs_fil.size, &tempsz);

	f_close(&fil);
    print_str_noscroll(112, 210, "Done.");
    #endif

    if(memcmp(&buffer[0x2080], "WC24DATABIN", 11)==0)
    {
        #ifdef WII_MINI_APP
        print_str_noscroll(112, 110, "Found WC24Data.bin in VFF.");
        #else
        printf("Found WC24Data.bin in VFF.\n");
        #endif
    }
    else if(memcmp(&buffer[0x480], "CSDATA  BIN", 11)==0)
	{
	    #ifdef WII_MINI_APP
        print_str_noscroll(112, 110, "Found CSData.bin in VFF; only decompression is supported.");
        #else
        printf("Found CSData.bin in VFF; only decompression is supported.\n");
        #endif
        is_csdata = 1;
	}
	else
	{
	    #ifdef WII_MINI_APP
        print_str_noscroll(112, 110, "WC24 dl list not found in VFF.");
        #else
        printf("WC24 dl list not found in VFF.\n");
        #endif
		return -3;
	}

    sprintf(str, "Extracting %s...\n", is_csdata==0?"WC24Data.bin":"CSData.bin");
	#ifdef WII_MINI_APP
	print_str_noscroll(112, 226, str);
    #else
    printf(str);
    #endif

    u32 datasize;
    if(is_csdata)memcpy(&datasize, &buffer[0x49c], 4);
    if(!is_csdata)memcpy(&datasize, &buffer[0x209c], 4);
    datasize = le32(datasize);
    memset(str, 0, 256);
    #ifdef WII_MINI_APP
    sprintf(str, "/%s", is_csdata==0?"WC24Data.bin":"CSData.bin");
    #else
        #ifdef HW_RVL
        sprintf(str, "/%s", is_csdata==0?"WC24Data.bin":"CSData.bin");
        #else
            sprintf(str, "%s", is_csdata==0?"WC24Data.bin":"CSData.bin");
        #endif
    #endif

    #ifdef USING_LIBFF
    if(f_open(&fil, str, FA_WRITE | FA_CREATE_ALWAYS)!=0)
	{
	    sprintf(pstr, "Failed to open %s", str);
		print_str_noscroll(112, 242, pstr);
		return -3;
	}
    #else
    fil = fopen(str, "wb");
    if(fil==NULL)
    {
	    sprintf(pstr, "Failed to open %s\n", str);
		printf(pstr);
		return -3;
	}
    #endif

    #ifdef USING_LIBFF
    f_write(&fil, &buffer[is_csdata==0?0x3220:0x1620], datasize, &tempsz);
    #else
    fwrite(&buffer[is_csdata==0?0x3220:0x1620], 1, datasize, fil);
    #endif

    #ifdef USING_LIBFF
    f_close(&fil);
    #else
    fclose(fil);
    #endif
    #ifdef WII_MINI_APP
    print_str_noscroll(112, 242, "Decompressing...");
    #else
    printf("Decompressing...\n");
    #endif
    memset(argv_str, 0, 256);
    memset(pstr, 0, 256);
    #ifdef WII_MINI_APP
    sprintf(pstr, "/decom.bin");
    #else
        #ifdef HW_RVL
            sprintf(pstr, "/decom.bin");
        #else
        sprintf(pstr, "decom.bin");
        #endif
    #endif
    sprintf(argv_str, "gbalzss%cd%c%s%c%s", 0, 0, str, 0, pstr);
    unsigned int Argv[4];
    Argv[0] = (unsigned int)&argv_str[0];
    Argv[1] = (unsigned int)&argv_str[8];
    Argv[2] = (unsigned int)&argv_str[10];
    Argv[3] = (unsigned int)&argv_str[11 + strlen(str)];
    gbalzss_main(4, (char**)Argv);

	if(is_csdata)
	{
		free(buffer);
		#ifdef WII_MINI_APP
		fat_umount();
		print_str_noscroll(112, 258, "Done.");
		#else
		printf("Done.\n");
		#endif
		return 0;
	}

	free(buffer);

    #ifdef WII_MINI_APP
    sprintf(str, "/decom.bin");
    #else
        #ifdef HW_RVL
        sprintf(str, "/decom.bin");
        #else
        sprintf(str, "decom.bin");
        #endif
    #endif

    #ifdef WII_MINI_APP
	FILINFO dfinfo;
	print_str_noscroll(112, 258, "Parsing...");
	if(f_stat(str, &dfinfo)!=0)
	{
		print_str_noscroll(112, 274, "f_stat decom failed.");
		fat_umount();
		return 0;
	}
	if(f_open(&fil, str, FA_READ)!=0)
	{
		print_str_noscroll(112, 274, "f_open decom failed.");
		fat_umount();
		return 0;
	}

	f_read(&fil, buffer, dfinfo.fsize, &tempsz);
	f_close(&fil);
    #else
        printf("Parsing...\n");
        fil = fopen(str, "rb");
        if(fil==NULL)
        {
            sprintf(pstr, "Failed to open %s\n", str);
            printf(pstr);
            return -3;
        }

        stat(str, &fstats);
        buffer = (unsigned char*)malloc(fstats.st_size);
        if(buffer==NULL)
        {
            sprintf(pstr, "Failed to allocate 0x%x bytes.\n", (int)fstats.st_size);
            printf(pstr);
            return -2;
        }
        fread(buffer, 1, fstats.st_size, fil);
        fclose(fil);
    #endif

    free(buffer);
    #ifdef WII_MINI_APP
	fat_umount();
	print_str_noscroll(112, 274, "Done.");
	#else
    printf("Done.\n");
	#endif
	return 0;
}

