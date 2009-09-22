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

int putc (
	int chr,	/* A character to be output */
	FIL* fil	/* Ponter to the file object */
);
int getc (
	FIL* fil	/* Ponter to the file object */
);
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

char country_codes[172][3] = {
{"JP"}, {"AI"}, {"AG"}, {"AR"}, {"AW"}, {"BS"}, {"BB"}, {"BZ"}, {"BO"}, {"BR"}, {"VG"},
{"CA"}, {"KY"}, {"CL"}, {"CO"}, {"CR"}, {"DM"}, {"DO"}, {"EC"}, {"SV"}, {"GF"}, {"GD"},
{"GP"}, {"GT"}, {"GY"}, {"HT"}, {"HN"}, {"JM"}, {"MQ"}, {"MX"}, {"MS"}, {"AN"}, {"NI"},
{"PA"}, {"PY"}, {"PE"}, {"KN"}, {"LC"}, {"VC"}, {"SR"}, {"TT"}, {"TC"}, {"US"}, {"UY"},
{"VI"}, {"VE"}, {"AL"}, {"AU"}, {"AT"}, {"BE"}, {"BA"}, {"BW"}, {"BG"}, {"HR"}, {"CY"},
{"CZ"}, {"DK"}, {"EE"}, {"FI"}, {"FR"}, {"DE"}, {"GR"}, {"HU"}, {"IS"}, {"IE"}, {"IT"},
{"LV"}, {"LS"}, {"LI"}, {"LT"}, {"LU"}, {"MK"}, {"MT"}, {"ME"}, {"MZ"}, {"NA"}, {"NL"},
{"NZ"}, {"NO"}, {"PL"}, {"PT"}, {"RO"}, {"RU"}, {"RS"}, {"SK"}, {"SI"}, {"ZA"}, {"ES"},
{"SZ"}, {"SE"}, {"CH"}, {"TR"}, {"GB"}, {"ZM"}, {"ZW"}, {"TW"}, {"KR"}, {"HK"}, {"MO"},
{"ID"}, {"SG"}, {"TH"}, {"PH"}, {"MY"}, {"CN"}, {"AE"}, {"IN"}, {"EG"}, {"OM"}, {"QA"},
{"KW"}, {"SA"}, {"SY"}, {"BH"}, {"JO"} };

char language_codes[7][3] = {{"ja"}, {"en"}, {"de"}, {"fr"}, {"es"}, {"it"}, {"nl"}};

typedef struct _DLlist_timestamp
{
    u16 year;
    u8 month;//Zero based.
    u8 day_of_month;
} __attribute((packed)) DLlist_timestamp;

typedef struct _DLlist_rating_entry//v3
{
	u8 ratingID;
	u8 boardID;//? For US, 2.
	u8 age;//Required years of age.
	u16 title[11];//Title/text of the rating.
} __attribute((packed)) DLlist_rating_entry;

typedef struct _DLlist_rating_entry_v4//v4
{
	u8 ratingID;
	u8 unk;
	u8 age;//Required years of age.
	u8 unk2;
	u32 JFIF_offset;//.jpg pic for this rating.
	u32 unk3;
	u16 title[11];//Title/text of the rating.
} __attribute((packed)) DLlist_rating_entry_v4;

typedef struct _DLlist_title_type_entry//v3
{
	u8 typeID;
	u8 groupID;
	u16 title[31];//Name of type.
} __attribute((packed)) DLlist_title_type_entry;

typedef struct _DLlist_title_type_entry_v4//v4
{
	u8 typeID;
	char console_model[3];//console model code.
	u16 title[31];//Name of type.
	u8 groupID;
	u8 unk;//usually matches the rating list "header" last byte.
} __attribute((packed)) DLlist_title_type_entry_v4;

typedef struct _DLlist_company_entry//v3
{
	u32 ID;//?
	u16 devtitle[31];
	u16 pubtitle[31];
} __attribute((packed)) DLlist_company_entry;

typedef struct _DLlist_title_entry//v3
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
} __attribute((packed)) DLlist_title_entry;

typedef struct _DLlist_title_entry_v4//v4
{
	u32 ID;
	u32 titleID;//Or DS Game code.
	u8 title_type;
	u8 unk;
	u32 unk2;
	u16 company_offset;
	u32 unk3;//Release date timestamp?
	u8 ratingID;
	u8 unk4[0x1d];
	u16 title[62];
	u16 subtitle[31];
} __attribute((packed)) DLlist_title_entry_v4;

typedef struct _DLlist_video_entry//v3
{
	u32 ID;//Decimal ID for URL filename.
	u16 unk;
	u32 titleid;//The assocaited title entry's ID.
	u8 unk2[0x5];
	u16 title[51];
} __attribute((packed)) DLlist_video_entry;

typedef struct _DLlist_video_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 time_length;//Length of video measured in seconds.
	u32 titleid;//The assocaited title entry's ID.
	u8 unk2[0x10];
	u8 ratingID;
	u8 unk3;
	u8 new_tag;
	u8 video_index;
	u8 unk4[2];
	u16 title[123];
} __attribute((packed)) DLlist_video_entry_v4;

typedef struct _DLlist_video2_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 unk;//time_length?
	u32 titleid;//The assocaited title entry's ID.
	u8 unk2[0x14];
	u16 title[102];
} __attribute((packed)) DLlist_video2_entry_v4;

typedef struct _DLlist_demo_entry//v3
{
	u32 ID;//Decimal ID for URL filename.
	u16 title[31];
	u16 subtitle[31];//Optional
	u32 titleid;//ID of title entry, not title entry titleid.
	u32 company_offset;
	u32 removal_timestamp;//0xffffffff when there is no end of distrubition date, timestamp otherwise.
	u8 unk[2];
} __attribute((packed)) DLlist_demo_entry;

typedef struct _DLlist_demo_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 title[31];
	u16 subtitle[31];//Optional
	u32 titleid;//ID of title entry, not title entry titleid.
	u32 company_offset;
	u32 removal_timestamp;//0xffffffff when there is no end of distrubition date, timestamp otherwise.
	u32 unk1;
	u8 ratingID;
	u8 new_tag;//When non-zero, NinCh displays the "NEW" tag?
	u8 new_tag_index;//Might be the index for the order of demos with the "NEW" tag?
	u8 unk2[0xcd];
} __attribute((packed)) DLlist_demo_entry_v4;

typedef struct _DLlist_detailed_rating_entry_v4
{
    u8 rating_group;
    u8 ratingID;
    u16 title[102];
} __attribute((packed)) DLlist_detailed_rating_entry_v4;

typedef struct _DLlist_header//v3
{
	u16 unk0;
	u8 version;
	u8 unk_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u32 filesize;//filesize of dl list.
	u8 unk5[7];
	u32 country_code;
	u32 language_code;
	u8 unk6[0x17];
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
} __attribute((packed)) DLlist_header;

typedef struct _DLlist_header_wrapper//v3
{
	u16 unk0;
	u8 version;
	u8 unk_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u32 filesize;//filesize of dl list.
	u8 unk5[7];
	u32 country_code;
	u32 language_code;
	u8 unk6[0x17];
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
} __attribute((packed)) DLlist_header_wrapper;

typedef struct _DLlist_header_v4
{
	u16 unk0;
	u8 version;
	u8 unkb_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u32 filesize;//filesize of dl list.
	u32 unk8;
	u32 DlListID;
	u32 unk_region;
	u32 country_code;
	u8 unk18[0x2];
	u8 language_code;
	u8 unk6[0x9];
	u32 ratings_total;
	u32 ratings_offset;
	u32 total_title_types;
	u32 title_types_offset;
	u32 companies_total;
	u32 companies_offset;
	u32 total_titles;
	u32 titles_offset;
	u32 unk[2];
	u32 videos0_total;
	u32 videos0_offset;
	u32 unk1[2];
	u32 demos_total;
	u32 demos_offset;
	u8 unk20[0x20];
	u32 videos1_total;//Unknown what this list is for exactly.
	u32 videos1_offset;
	u32 total_detailed_ratings;
	u32 detailed_ratings_offset;
} __attribute((packed)) DLlist_header_v4;

typedef struct _DLlist_header_wrapper_v4
{
	u16 unk0;
	u8 version;
	u8 unkb_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u32 filesize;//filesize of dl list.
	u32 unk8;
	u32 DlListID;
	u32 unk_region;
	u32 country_code;
	u32 language_code;
	u8 unk6[0x9];
	u32 ratings_total;
	DLlist_rating_entry_v4 *ratings;
	u32 total_title_types;
	DLlist_title_type_entry_v4 *title_types;
	u32 companies_total;
	DLlist_company_entry *companies;
	u32 total_titles;
	DLlist_title_entry_v4 *titles;
	u32 unk[2];
	u32 videos0_total;
	DLlist_video_entry_v4 *videos0;//All 60 videos.
	u32 unk1[2];
	u32 demos_total;
	DLlist_demo_entry_v4 *demos;
	u8 unk20[0x20];
	u32 videos1_total;
	DLlist_video2_entry_v4 *videos1;//Old NinCh v3 videos.
	u32 total_detailed_ratings;
	DLlist_detailed_rating_entry_v4 *detailed_ratings;
} __attribute((packed)) DLlist_header_wrapper_v4;

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

inline u16 be16(u16 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

inline u16 le16(u16 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

DLlist_title_entry *LookupTitle(u32 ID, DLlist_header_wrapper *header)
{
    u32 i;
    u32 numtitles = ((u32)header->titles - (u32)header->videos) / sizeof(DLlist_title_entry);
    for(i = 0; i<numtitles; i++)
    {
        if(header->titles[i].ID==ID)
        {
            return &header->titles[i];
        }
    }
    return NULL;
}

DLlist_title_entry_v4 *LookupTitleV4(u32 ID, DLlist_header_wrapper_v4 *header)
{
    u32 i;
    u32 numtitles = header->total_titles;
    for(i = 0; i<numtitles; i++)
    {
        if(header->titles[i].ID==ID)
        {
            return &header->titles[i];
        }
    }
    return NULL;
}

char *GetCountryCode(u32 code)
{
    if(code==1)
    {
        return (char*)country_codes[0];
    }
    else if(code > 7 && code < 53)
    {
        return (char*)country_codes[code - 7];
    }
    else if(code > 63 && code < 113)
    {
        return (char*)country_codes[code - 18];
    }
    else if(code > 147 && code < 158)
    {
        return (char*)country_codes[code - 52];
    }
    else if(code > 167 && code < 178)
    {
        return (char*)country_codes[code - 62];
    }
    else
    {
        return NULL;
    }
}

char GetRegionCode(u32 code)
{
    if(code==1)
    {
        return 'j';
    }
    else if(code > 7 && code < 53)
    {
        return 'u';
    }
    else if(code > 63 && code < 113)
    {
        return 'e';
    }
    return 'e';
}

void GetTimestamp(u32 input, DLlist_timestamp *timestamp)
{
    memcpy(timestamp, &input, 4);
    timestamp->year = be16(timestamp->year);
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
	int is_vff = 1;
	int version = 3;
	#ifndef WII_MINI_APP
	FILE *fil;
	#endif
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

    int fat_init = fat_mount();

    FILINFO idfinfo;
    char *buf;
    u32 temp;
	if(f_stat(str, &idfinfo)==0)
	{
	    if(f_open(&fil, "/bootmii/bootmii.ini", FA_READ)==0)
	    {
	    buf = (char*)malloc(idfinfo.fsize);
	    f_read(&fil, buf, idfinfo.fsize, &temp);
	    f_close(&fil);
        if(strstr(buf, "NTSC"))vmode = VIDEO_640X480_NTSCi_YUV16;
        if(strstr(buf, "PAL50"))vmode = VIDEO_640X480_PAL50_YUV16;
        if(strstr(buf, "PAL60"))vmode = VIDEO_640X480_PAL60_YUV16;
        if(strstr(buf, "PROGRESSIVE"))vmode = VIDEO_640X480_NTSCp_YUV16;
        free(buf);
	    }
	}

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
	if(fat_init!=0)
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

    printf("ninchdl-listext v1.0 by yellowstar.\n");
    #ifdef HW_RVL
    printf("Mounting FAT...\n");
    if(!fatInitialize())
    {
        printf("Failed to mount FAT.\n");
    }
    #else
    if(argc!=2 && argc!=5)
    {
        printf("Usage:\n");
        printf("ninchdl-listext <wc24dl.vff>\n");
        printf("Input can be a compressed dl list as well.\n");
        printf("Alternate usage:\n");
        printf("ninchdl-listext <country code> <language code>\n<region char> <version> <wc24pubk.mod>\n");
        printf("See either source code or google code wmb-asm NintendoChannel wiki page for list of country and language codes.\n");
        printf("region char must be either u, e, or j.\n");
        printf("wc24pubk.mod is the filename for the NinCh WC24 keys.(Can also be the 16 byte\nAES key.) The default is wc24pubk.mod if this is ommitted.\n");
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
	    if(buffer[0]!=0x10)
		{
            print_str_noscroll(112, 194, "Invalid VFF, and input file is not raw compressed data.");
            return -4;
		}
		else
		{
		    is_vff = 0;
		}
	}
    #else
        #ifndef HW_RVL
        struct stat fstats;
        if(strstr(vff_dumpfn, ".vff"))
        {
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
            if(buffer[0]!=0x10)
            {
                #ifdef WII_MINI_APP
                print_str_noscroll(112, 194, "Invalid VFF, and input file is not raw compressed data.\n");
                #else
                printf("Invalid VFF, and input file is not raw compressed data.\n");
                #endif
                return -4;
            }
            else
            {
                is_vff = 0;
                return -4;
            }
        }
        }
        else
        {
            is_vff = 0;
        }
        #endif
    #endif

    #ifdef WII_MINI_APP
    if(is_vff)
    {
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
    }
    #endif

    if(is_vff)
    {
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
    }

    u32 DlListID = 0;
    #ifndef WII_MINI_APP
    if(!is_vff && strstr(argv[1], ".LZ")==NULL)
    {
        memset(pstr, 0, 256);
        memset(str, 0, 256);
        sscanf(argv[4], "%d", &version);
        if(version<4)strcpy(str, "wc24data.LZ");
        if(version>=4)strcpy(str, "csdata.LZ");
        sprintf(pstr, "http://ent%c.wapp.wii.com/%d/%s/%s/%s", argv[3][0], version, argv[1], argv[2], str);
        memset(str, 0, 256);
        sprintf(str, "wc24decrypt %s ", pstr);
        memset(pstr, 0, 256);
        if(version<4)strcpy(pstr, "WC24Data.bin");
        if(version>=4)strcpy(pstr, "CSData.bin");
        strcat(str, pstr);
        strcat(str, " ");
        if(argc!=6)
        {
            strcat(str, "wc24pubk.mod");
        }
        else
        {
            strcat(str, argv[5]);
        }
        printf("%s\n", str);
        system(str);

        if(version>=4)
        {
            FILE *f;
            memset(argv_str, 0, 256);
            sprintf(argv_str, "gbalzss%cd%c%s%c%s", 0, 0, pstr, 0, "decom.bin");
            unsigned int Argv_[4];
            Argv_[0] = (unsigned int)&argv_str[0];
            Argv_[1] = (unsigned int)&argv_str[8];
            Argv_[2] = (unsigned int)&argv_str[10];
            Argv_[3] = (unsigned int)&argv_str[11 + strlen(pstr)];
            gbalzss_main(4, (char**)Argv_);
            memset(argv_str, 0, 256);
            memset(str, 0, 256);

            f = fopen("decom.bin", "rb");
            if(f==NULL)return -3;
            fseek(f, 0xc, SEEK_SET);
            fread(&DlListID, 1, 4, f);
            fclose(f);
            DlListID = be32(DlListID);
            sprintf(str, "curl -k --output %u.LZ https://ent%cs.wapp.wii.com/%s/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/list/%s/%s/%u.LZ", DlListID, argv[3][0], argv[4], argv[1], argv[2], DlListID);
            printf("%s\n", str);
            system(str);
        }
    }
    #endif

    if(is_vff)
    {
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
        free(buffer);
    }
    else
    {
        memset(str, 0, 256);
        if(strstr(argv[1], ".LZ")==NULL && strstr(argv[1], ".")==NULL)
        {
            if(version<4)strcpy(str, "WC24Data.bin");
            if(version>=4)sprintf(str, "%u.LZ", DlListID);
        }
        else if(strstr(argv[1], "."))
        {
            strcpy(str, argv[1]);
        }
    }

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
		#ifdef WII_MINI_APP
		fat_umount();
		print_str_noscroll(112, 258, "Done.");
		#else
		printf("Done.\n");
		#endif
		return 0;
	}


    DLlist_header_wrapper *header = NULL;
    DLlist_header_wrapper_v4 *header_v4 = NULL;

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

	buffer = (unsigned char*)malloc(dfinfo.fsize);
	f_read(&fil, buffer, dfinfo.fsize, &tempsz);
	f_close(&fil);
	char *country_code;
   	char *language_code;
   	char region_code;
	if(buffer[2]<4)header = (DLlist_header_wrapper*)buffer;
	if(buffer[2]>=4)header_v4 = (DLlist_header_wrapper_v4*)buffer;

	if(buffer[2]<4)
	{
        header->country_code = be32(header->country_code);
        header->language_code = be32(header->language_code);
	}
	else
	{
	    header_v4->country_code = be32(header_v4->country_code);
        header_v4->language_code = be32(header_v4->language_code);
	}

   	if(buffer[2]<4)
   	{
        country_code = GetCountryCode(header->country_code);
        language_code = (char*)language_codes[header->language_code];
   	}
   	else
   	{
        country_code = GetCountryCode(header_v4->country_code);
        language_code = (char*)language_codes[header_v4->language_code];
   	}

	if(country_code==NULL)
    	{
        	free(buffer);
        	sprintf(pstr, "Unknown country code: %d\n", (int)header->country_code);
        	#ifdef WII_MINI_APP
        	fat_umount();
        	print_str_noscroll(112, 274, pstr);
        	#else
        	printf(pstr);
        	#endif
        	return -4;
    	}

    if(buffer[2]<4)region_code = GetRegionCode(header->country_code);
    if(buffer[2]>=4)region_code = GetRegionCode(header_v4->country_code);
	memset(str, 0, 256);
	sprintf(str, "/dump%c.txt", region_code);
	if(f_open(&fil, str, FA_WRITE | FA_CREATE_ALWAYS)!=0)
	{
		free(buffer);
		fat_umount();
		sprintf(str, "Failed to open %s", "/dump.txt");
		print_str_noscroll(112, 274, str);
		return -3;
	}
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

        if(buffer[2]<4)header = (DLlist_header_wrapper*)buffer;
        if(buffer[2]>=4)header_v4 = (DLlist_header_wrapper_v4*)buffer;
        char *country_code;
   	char *language_code;
   	char region_code;

	if(buffer[2]<4)
	{
        header->country_code = be32(header->country_code);
        header->language_code = be32(header->language_code);
	}
	else
	{
	    header_v4->country_code = be32(header_v4->country_code);
        header_v4->language_code = be32(header_v4->language_code);
	}

   	if(buffer[2]<4)
   	{
        country_code = GetCountryCode(header->country_code);
        language_code = (char*)language_codes[header->language_code];
   	}
   	else
   	{
        country_code = GetCountryCode(header_v4->country_code);
        language_code = (char*)language_codes[header_v4->language_code];
   	}

	if(country_code==NULL)
    	{
        	free(buffer);
        	sprintf(pstr, "Unknown country code: %d\n", (int)header->country_code);
        	#ifdef WII_MINI_APP
        	fat_umount();
        	print_str_noscroll(112, 274, pstr);
        	#else
        	printf(pstr);
        	#endif
        	return -4;
    	}

        if(buffer[2]<4)region_code = GetRegionCode(header->country_code);
        if(buffer[2]>=4)region_code = GetRegionCode(header_v4->country_code);
        memset(str, 0, 256);
        #ifdef HW_RVL
        sprintf(str, "/dump%c.txt", region_code);
        #else
        sprintf(str, "dump%c.txt", region_code);
        #endif
        fil = fopen(str, "wb");
        if(fil==NULL)
        {
		sprintf(pstr, "Failed to open %s", str);
		printf(pstr);
		return -3;
        }
    #endif

    if(buffer[2]>4)
    {
        sprintf(pstr, "Unsupported dl list version for parser: %d\n", header->version);
        free(buffer);

        #ifdef WII_MINI_APP
        print_str_noscroll(112, 274, pstr);
        f_close(&fil);
        fat_umount();
        #else
        printf(pstr);
        fclose(fil);
        #endif
        return 0;
    }

    u16 utf_temp;
    u32 i, texti, ratingi;
    u32 total_demos, total_videos;

    if(buffer[2]<4)
    {
        header->ratings = (DLlist_rating_entry*)(be32((u32)header->ratings) + (u32)buffer);
        header->main_title_types = (DLlist_title_type_entry*)(be32((u32)header->main_title_types) + (u32)buffer);
        header->companies = (DLlist_company_entry*)(be32((u32)header->companies) + (u32)buffer);
        header->sub_title_types = (DLlist_title_entry*)(be32((u32)header->sub_title_types) + (u32)buffer);
        header->titles = (DLlist_title_entry*)(be32((u32)header->titles) + (u32)buffer);
        header->unk_data = (u32*)(be32((u32)header->unk_data) + (u32)buffer);
        header->videos = (DLlist_video_entry*)(be32((u32)header->videos) + (u32)buffer);
        header->demos = (DLlist_demo_entry*)(be32((u32)header->demos) + (u32)buffer);
        header->wc24msg_opt = (u16*)(be32((u32)header->wc24msg_opt) + (u32)buffer);
    }
    else
    {
        header_v4->ratings = (DLlist_rating_entry_v4*)(be32((u32)header_v4->ratings) + (u32)buffer);
        header_v4->title_types = (DLlist_title_type_entry_v4*)(be32((u32)header_v4->title_types) + (u32)buffer);
        header_v4->companies = (DLlist_company_entry*)(be32((u32)header_v4->companies) + (u32)buffer);
        header_v4->titles = (DLlist_title_entry_v4*)(be32((u32)header_v4->titles) + (u32)buffer);
        header_v4->demos = (DLlist_demo_entry_v4*)(be32((u32)header_v4->demos) + (u32)buffer);
        header_v4->videos0 = (DLlist_video_entry_v4*)(be32((u32)header_v4->videos0) + (u32)buffer);
        header_v4->videos1 = (DLlist_video2_entry_v4*)(be32((u32)header_v4->videos1) + (u32)buffer);
        header_v4->detailed_ratings = (DLlist_detailed_rating_entry_v4*)(be32((u32)header_v4->detailed_ratings) + (u32)buffer);
    }

    if(buffer[2]<4)
    {
        header->ratings_total = be32(header->ratings_total);
        header->total_title_types = be32(header->total_title_types);
        header->companies_total = be32(header->companies_total);
        header->unk_title = be32(header->unk_title);
        header->videos_total = be32(header->videos_total);
        header->demos_total = be32(header->demos_total);
        total_demos = header->demos_total;
        total_videos = header->videos_total;
    }
    else
    {
        header_v4->ratings_total = be32(header_v4->ratings_total);
        header_v4->total_title_types = be32(header_v4->total_title_types);
        header_v4->companies_total = be32(header_v4->companies_total);
        header_v4->demos_total = be32(header_v4->demos_total);
        header_v4->videos0_total = be32(header_v4->videos0_total);
        header_v4->videos1_total = be32(header_v4->videos1_total);
        header_v4->total_detailed_ratings = be32(header_v4->total_detailed_ratings);
        total_demos = header_v4->demos_total;
        total_videos = header_v4->videos0_total;
    }

    #ifdef USING_LIBFF
    f_puts("Demos:\n", &fil);
    #else
    fputs("Demos:\r\n", fil);
    #endif

    #ifdef USING_LIBFF
    sprintf(str, "Number of demos: %d\n\n", total_demos);
    f_puts(str, &fil);
    #else
    sprintf(str, "Number of demos: %d\r\n\r\n", total_demos);
    fputs(str, fil);
    #endif

    DLlist_title_entry *title_ptr;
    DLlist_title_entry_v4 *title_ptr_v4;
    for(i=0; i<total_demos; i++)
    {
        for(texti=0; texti<31; texti++)
        {
            if(buffer[2]<4)utf_temp = header->demos[i].title[texti];
            if(buffer[2]>=4)utf_temp = header_v4->demos[i].title[texti];
            if(utf_temp==0)break;
            utf_temp = be16(utf_temp);
            #ifdef USING_LIBFF
            putc((u8)utf_temp, &fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
            #else
            putc((u8)utf_temp, fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
            #endif
        }
        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif
        for(texti=0; texti<31; texti++)
        {
            if(buffer[2]<4)utf_temp = header->demos[i].subtitle[texti];
            if(buffer[2]>=4)utf_temp = header_v4->demos[i].subtitle[texti];
            if(utf_temp==0)break;
            utf_temp = be16(utf_temp);
            #ifdef USING_LIBFF
            putc((u8)utf_temp, &fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
            #else
            putc((u8)utf_temp, fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
            #endif
        }
        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif

        if(buffer[2]>=4)
        {
            if(header_v4->demos[i].new_tag)
            {
                #ifdef USING_LIBFF
                sprintf(str, "NEW (index %d)\n", header_v4->demos[i].new_tag_index);
                f_puts(str, &fil);
                #else
                sprintf(str, "NEW (index %d)\r\n", header_v4->demos[i].new_tag_index);
                fputs(str, fil);
                #endif
            }
        }

        u32 demo_ID = 0;
        if(buffer[2]<4)demo_ID = be32(header->demos[i].ID);
        if(buffer[2]>=4)demo_ID = be32(header_v4->demos[i].ID);
        #ifdef USING_LIBFF
        sprintf(str, "ID: %u\n", demo_ID);
        f_puts(str, &fil);
        #else
        sprintf(str, "ID: %u\r\n", demo_ID);
        fputs(str, fil);
        #endif

        sprintf(str, "Rating: ");
        #ifdef USING_LIBFF
        f_puts(str, &fil);
        #else
        fputs(str, fil);
        #endif

        u32 ratingID = 0;
        if(buffer[2]<4)
        {
            title_ptr = LookupTitle(header->demos[i].titleid, header);
            ratingID = title_ptr->ratingID;
        }
        else
        {
            title_ptr_v4 = LookupTitleV4(header_v4->demos[i].titleid, header_v4);
            ratingID = header_v4->demos[i].ratingID;
        }
        if(buffer[2]<4)
        {
            for(ratingi=0; ratingi<header->ratings_total; ratingi++)
            {
                if(header->ratings[ratingi].ratingID==ratingID)
                {
                    for(texti=0; texti<11; texti++)
                    {
                        utf_temp = header->ratings[ratingi].title[texti];
                        if(utf_temp==0)break;
                        utf_temp = be16(utf_temp);
                        #ifdef USING_LIBFF
                        putc((u8)utf_temp, &fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                        #else
                        putc((u8)utf_temp, fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                        #endif
                    }
                }
            }
        }
        else
        {
            for(ratingi=0; ratingi<header_v4->ratings_total; ratingi++)
            {
                if(header_v4->ratings[ratingi].ratingID==ratingID)
                {
                    for(texti=0; texti<11; texti++)
                    {
                        utf_temp = header_v4->ratings[ratingi].title[texti];
                        if(utf_temp==0)break;
                        utf_temp = be16(utf_temp);
                        #ifdef USING_LIBFF
                        putc((u8)utf_temp, &fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                        #else
                        putc((u8)utf_temp, fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                        #endif
                    }
                }
            }
        }

        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif

        DLlist_company_entry *comp;
        if(buffer[2]<4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr->company_offset));
        if(buffer[2]>=4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr_v4->company_offset));
        for(texti=0; texti<31; texti++)
        {
                utf_temp = comp->devtitle[texti];
                if(utf_temp==0)break;
                utf_temp = be16(utf_temp);
                #ifdef USING_LIBFF
                putc((u8)utf_temp, &fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                #else
                putc((u8)utf_temp, fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                #endif
        }
        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif
        if(memcmp(comp->devtitle, comp->pubtitle, 31 * 2)!=0)
        {
        for(texti=0; texti<31; texti++)
        {
                utf_temp = comp->pubtitle[texti];
                if(utf_temp==0)break;
                utf_temp = be16(utf_temp);
                #ifdef USING_LIBFF
                putc((u8)utf_temp, &fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                #else
                putc((u8)utf_temp, fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                #endif
        }
        }

        DLlist_timestamp timestamp;
        u32 timestamp_u32;
        if(buffer[2]<4)timestamp_u32 = header->demos[i].removal_timestamp;
        if(buffer[2]>=4)timestamp_u32 = header_v4->demos[i].removal_timestamp;

        if(timestamp_u32==0xffffffff)
        {
            #ifdef USING_LIBFF
            f_puts("\nNo removal date.", &fil);
            #else
            fputs("\r\nNo removal date.", fil);
            #endif
        }
        else
        {
            GetTimestamp(timestamp_u32, &timestamp);
            #ifdef USING_LIBFF
            sprintf(str, "\nRemoval date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
            f_puts(str, &fil);
            #else
            sprintf(str, "\r\nRemoval date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
            fputs(str, fil);
            #endif
        }

        #ifdef USING_LIBFF
        sprintf(str, "\nURL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/dstrial/%s/%s/%u.bin\n\n", region_code, (int)buffer[2], country_code, language_code, demo_ID);
        f_puts(str, &fil);
        f_sync(&fil);
        #else
        sprintf(str, "\r\nURL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/dstrial/%s/%s/%u.bin\r\n\r\n", region_code, (int)buffer[2], country_code, language_code, demo_ID);
        fputs(str, fil);
        fflush(fil);
        #endif
    }

    #ifdef USING_LIBFF
    f_puts("Videos:\n", &fil);
    #else
    fputs("Videos:\r\n", fil);
    #endif

    #ifdef USING_LIBFF
    sprintf(str, "Number of videos: %d\n\n", total_videos);
    f_puts(str, &fil);
    #else
    sprintf(str, "Number of videos: %d\r\n\r\n", total_videos);
    fputs(str, fil);
    #endif
    for(i=0; i<total_videos; i++)
    {
        u32 txt_len = 0;
        if(buffer[2]<4)txt_len = 51;
        if(buffer[2]>=4)txt_len = 123;
        for(texti=0; texti<txt_len; texti++)
        {
            if(buffer[2]<4)utf_temp = header->videos[i].title[texti];
            if(buffer[2]>=4)utf_temp = header_v4->videos0[i].title[texti];
            if(utf_temp==0)break;
            utf_temp = be16(utf_temp);
            #ifdef USING_LIBFF
            if(((u8)utf_temp)==0x0A)putc(0x0D, &fil);
            putc((u8)utf_temp, &fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
            #else
            if(((u8)utf_temp)==0x0A)putc(0x0D, fil);
            putc((u8)utf_temp, fil);
            if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
            #endif
        }
        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif

        if(buffer[2]>=4)
        {
            if(header_v4->videos0[i].new_tag)
            {
                #ifdef USING_LIBFF
                sprintf(str, "NEW\n");
                f_puts(str, &fil);
                #else
                sprintf(str, "NEW\r\n");
                fputs(str, fil);
                #endif
            }
            u16 time_length = be16(header_v4->videos0[i].time_length);
            #ifdef USING_LIBFF
            sprintf(str, "Time length %02d:%02d\n", time_length / 60, time_length % 60);
            f_puts(str, &fil);
            #else
            sprintf(str, "Time length %02d:%02d\r\n", time_length / 60, time_length % 60);
            fputs(str, fil);
            #endif
        }

        u32 video_ID, video_titleid, video1_ID = 0;
        if(buffer[2]<4)video_ID = be32(header->videos[i].ID);
        if(buffer[2]>=4)
        {
            video_ID = be32(header_v4->videos0[i].ID);
            video1_ID = be32(header_v4->videos1[i].ID);
        }
        #ifdef USING_LIBFF
        if(buffer[2]<4)
        {
            sprintf(str, "ID: %u\n", video_ID);
            f_puts(str, &fil);
        }
        else
        {
            sprintf(str, "ID: %u\n", video_ID);
            f_puts(str, &fil);
        }
        #else
        if(buffer[2]<4)
        {
            sprintf(str, "ID: %u\r\n", video_ID);
            fputs(str, fil);
        }
        else
        {
            sprintf(str, "ID: %u\r\n", video_ID);
            fputs(str, fil);
        }
        #endif

        if(buffer[2]<4)video_titleid = header->videos[i].titleid;
        if(buffer[2]>=4)video_titleid = header_v4->videos0[i].titleid;
        if(video_titleid!=0)
        {
        sprintf(str, "Rating: ");
        #ifdef USING_LIBFF
        f_puts(str, &fil);
        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
        #else
        fputs(str, fil);
        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
        #endif

        u32 ratingID = 0;
        if(buffer[2]<4)
        {
            title_ptr = LookupTitle(header->videos[i].titleid, header);
            ratingID = title_ptr->ratingID;
        }
        else
        {
            title_ptr_v4 = LookupTitleV4(header_v4->videos0[i].titleid, header_v4);
            ratingID = header_v4->videos0[i].ratingID;
        }
        if(buffer[2]<4)
        {
            for(ratingi=0; ratingi<header->ratings_total; ratingi++)
            {
                if(header->ratings[ratingi].ratingID==ratingID)
                {
                    for(texti=0; texti<11; texti++)
                    {
                        utf_temp = header->ratings[ratingi].title[texti];
                        if(utf_temp==0)break;
                        utf_temp = be16(utf_temp);
                        #ifdef USING_LIBFF
                        putc((u8)utf_temp, &fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                        #else
                        putc((u8)utf_temp, fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                        #endif
                    }
                }
            }
        }
        else
        {
            for(ratingi=0; ratingi<header_v4->ratings_total; ratingi++)
            {
                if(header_v4->ratings[ratingi].ratingID==ratingID)
                {
                    for(texti=0; texti<11; texti++)
                    {
                        utf_temp = header_v4->ratings[ratingi].title[texti];
                        if(utf_temp==0)break;
                        utf_temp = be16(utf_temp);
                        #ifdef USING_LIBFF
                        putc((u8)utf_temp, &fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                        #else
                        putc((u8)utf_temp, fil);
                        if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                        #endif
                    }
                }
            }
        }

        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif

        DLlist_company_entry *comp;
        if(buffer[2]<4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr->company_offset));
        if(buffer[2]>=4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr_v4->company_offset));
        for(texti=0; texti<31; texti++)
        {
                utf_temp = comp->devtitle[texti];
                if(utf_temp==0)break;
                utf_temp = be16(utf_temp);
                #ifdef USING_LIBFF
                putc((u8)utf_temp, &fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                #else
                putc((u8)utf_temp, fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                #endif
        }
        #ifdef USING_LIBFF
        f_puts("\n", &fil);
        #else
        fputs("\r\n", fil);
        #endif
        if(memcmp(comp->devtitle, comp->pubtitle, 31 * 2)!=0)
        {
        for(texti=0; texti<31; texti++)
        {
                utf_temp = comp->pubtitle[texti];
                if(utf_temp==0)break;
                utf_temp = be16(utf_temp);
                #ifdef USING_LIBFF
                putc((u8)utf_temp, &fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), &fil);
                #else
                putc((u8)utf_temp, fil);
                if((utf_temp >> 8))putc((u8)(utf_temp >> 8), fil);
                #endif
        }
        }
        }

        #ifdef USING_LIBFF
        if(buffer[2]<4)
        {
            sprintf(str, "\nURL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u.3gp\n\n", GetRegionCode(header->country_code), (int)header->version, country_code, language_code, video_ID);
            f_puts(str, &fil);
        }
        else
        {
            sprintf(str, "\nLow Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-l.mo", region_code, buffer[2], country_code, language_code, video_ID);
            f_puts(str, &fil);
            sprintf(str, "\nHigh Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-h.mo\n\n", region_code, buffer[2], country_code, language_code, video_ID);
            f_puts(str, &fil);
        }
        f_sync(&fil);
        #else
        if(buffer[2]<4)
        {
            sprintf(str, "\r\nURL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u.3gp\r\n\r\n", region_code, buffer[2], country_code, language_code, video_ID);
        }
        else
        {
            sprintf(str, "\r\nLow Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-l.mo", region_code, buffer[2], country_code, language_code, video_ID);
            fputs(str, fil);
            sprintf(str, "\r\nHigh Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-h.mo\r\n\r\n", region_code, buffer[2], country_code, language_code, video_ID);
            fputs(str, fil);
        }
        fflush(fil);
        #endif
    }

    free(buffer);
    #ifdef WII_MINI_APP
    f_close(&fil);
	fat_umount();
	print_str_noscroll(112, 274, "Done.");
	#else
	fclose(fil);
    printf("Done.\n");
	#endif
	return 0;
}

