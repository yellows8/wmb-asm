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

#include "bootmii_ppc.h"
#include "string.h"
#include "ipc.h"
#include "mini_ipc.h"
#include "nandfs.h"
#include "fat.h"
#include "malloc.h"
#include "diskio.h"
#include "printf.h"
#include "video_low.h"
#include "input.h"
#include "console.h"

#define MINIMUM_MINI_VERSION 0x00010001

int gbalzss_main(int argc, char *argv[]);
char argv_str[256];

inline u32 be32(u32 x)//From update_download by SquidMan.
{
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

int main(void)
{
	int vmode = -1;
	struct nandfs_fp nfs_fil;
	char str[256];
	char pstr[256];
	char vff_dumpfn[256];
	unsigned char *buffer;
	int is_csdata = 0;
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

    memset(pstr, 0, 256);
    //print_str_noscroll(112, 112, "ohai, world!\n");

	//testOTP();

	//print_str_noscroll(113, 113, "bye, world!\n");

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

	memset(str, 0, 256);
	memset(vff_dumpfn, 0, 256);
	print_str_noscroll(112, 128, "Choose a region via GC/GPIO: A/Pwr for E, B/Rst for P, Y/Ejt for J.");
	char region = 'a';
	while(1)
	{
		u16 button = input_wait();
		if(button & PAD_A)region = 'E';
		if(button & PAD_B)region = 'P';
		if(button & PAD_Y)region = 'J';

		button = gpio_read();
		if(button & GPIO_POWER)region = 'E';
		if(button & GPIO_RESET)region = 'P';
		if(button & GPIO_EJECT)region = 'J';

		if(region!='a')break;
	}

    sprintf(vff_dumpfn, "/HAT%c_wc24dl.vff", region);
	sprintf(str, "/title/00010001/484154%02x/data/wc24dl.vff", region);
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

	print_str_noscroll(112, 194, "Writing to SD...");
	if(f_open(&fil, vff_dumpfn, FA_WRITE | FA_CREATE_ALWAYS)!=0)
	{
		print_str_noscroll(112, 110, "Failed to open /wc24data.vff");
		return -3;
	}

	f_write(&fil, buffer, nfs_fil.size, &tempsz);

	f_close(&fil);
    print_str_noscroll(112, 210, "Done.");

    if(memcmp(&buffer[0x480], "WC24DATABIN", 11)==0)
    {
        print_str_noscroll(112, 110, "Found WC24Data.bin in VFF.");
    }
    else if(memcmp(&buffer[0x480], "CSDATA  BIN", 11)==0)
	{
        print_str_noscroll(112, 110, "Found CSData.bin in VFF; only decompression is supported.");
        is_csdata = 1;
	}
	else
	{
        print_str_noscroll(112, 110, "WC24 dl list not found in VFF.");
		return -3;
	}

    sprintf(str, "Extracting %s...", is_csdata==0?"WC24Data.bin":"CSData.bin");
	print_str_noscroll(112, 226, str);

    u32 datasize;
    if(is_csdata)memcpy(&datasize, &buffer[0x49c], 4);
    if(!is_csdata)memcpy(&datasize, &buffer[0x209c], 4);
    datasize = be32(datasize);
    sprintf(str, "/HAT%c_%s", region, is_csdata==0?"WC24Data.bin":"CSData.bin");
    if(f_open(&fil, str, FA_WRITE | FA_CREATE_ALWAYS)!=0)
	{
	    sprintf(pstr, "Failed to open %s", str);
		print_str_noscroll(112, 242, pstr);
		return -3;
	}

    f_write(&fil, &buffer[is_csdata==0?0x3220:0x1620], datasize, &tempsz);

    f_close(&fil);

    print_str_noscroll(112, 242, "Decompressing...");
    memset(argv_str, 0, 256);
    sprintf(pstr, "/HAT%cdecom_%s", region, is_csdata==0?"WC24Data.bin":"CSData.bin");
    sprintf(argv_str, "gbalzss%xd%x%s%x%s", 0, 0, str, 0, str);
    gbalzss_main(4, argv_str);

	free(buffer);
	fat_umount();
	print_str_noscroll(112, 258, "Done.");
	return 0;
}

