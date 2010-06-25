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

extern otp_t otp;
seeprom_t seeprom;

char commands[3][4] = {"KEYS", "READ", "WRTE"};
unsigned char pagebuf[0x840];

int gecko_recvbuffer(void *buffer, u32 size);
int gecko_sendbuffer(const void *buffer, u32 size);

static void dsp_reset(void)
{
	write16(0x0c00500a, read16(0x0c00500a) & ~0x01f8);
	write16(0x0c00500a, read16(0x0c00500a) | 0x0010);
	write16(0x0c005036, 0);
}

static char ascii(char s) {
  if(s < 0x20) return '.';
  if(s > 0x7E) return '.';
  return s;
}

void hexdump(void *d, int len) {
  /*u8 *data;
  int i, off;
  data = (u8*)d;
  for (off=0; off<len; off += 16) {
    printf("%08x  ",off);
    for(i=0; i<16; i++)
      if((i+off)>=len) printf("   ");
      else printf("%02x ",data[off+i]);

    printf(" ");
    for(i=0; i<16; i++)
      if((i+off)>=len) printf(" ");
      else printf("%c",ascii(data[off+i]));
    printf("\n");
  }*/
}
	
/*void testOTP(void)
{
	//print_str_noscroll(114, 114, "reading OTP...\n");
	getotp(&otp);
	print_str_noscroll(115, 115, "read OTP!");
	//print_str_noscroll(116, 116, "OTP:\n");
	hexdump(&otp, sizeof(otp));

	//print_str_noscroll(117, 117, "reading SEEPROM...\n");
	getseeprom(&seeprom);
	//print_str_noscroll(118, 118, "read SEEPROM!\n");
	//print_str_noscroll(119, 119, "SEEPROM:\n");
	hexdump(&seeprom, sizeof(seeprom));
}*/

int main(void)
{
	int vmode = -1;
	int len = 4;
	int i;
	int val;
	unsigned char temp[4];
	unsigned int *tempu32 = (unsigned int*)temp;
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
    print_str_noscroll(112, 90, "hey there!\n");
    //getotp(&OTP);
    print_str_noscroll(112, 112, "ohai, world!\n");
	nandfs_initialize();

	while(1)
	{
		val = gecko_recvbuffer_safe(&temp[4 - len], len);
		len-= len-val;
		if(val!=0)continue;

		print_str_noscroll(112, 115, "Got command\n");
		if(strncmp(temp, "STOP", 4)==0)
		{
			print_str_noscroll(112, 130, "Recieved stop command.\n");
			break;
		}
		for(i=0; i<3; i++)
		{
			if(strncmp(temp, commands[i], 4)==0)
			{
				switch(i)
				{
					case 0: //KEYS
						print_str_noscroll(112, 120, "KEYS\n");
						len = 4;
						val = 1;
						while(val!=0)
						{
							val = gecko_sendbuffer_safe(&temp[4 - len], len);
							len-= len-val;
						}
						len = 16;
						val = 1;
						while(val!=0)
						{
							val = gecko_sendbuffer_safe(&otp.nand_key[16 - len], len);
							len-= len-val;
						}
						len = 20;
						val = 1;
						while(val!=0)
						{
							val = gecko_sendbuffer_safe(&otp.nand_hmac[20 - len], len);
							len-= len-val;
						}
					break;

					case 1://READ
						print_str_noscroll(112, 130, "READ\n");
						len = 4;
						val = 1;
						while(val!=0)
						{
							val = gecko_recvbuffer_safe(&temp[4 - len], len);
							len-= len-val;
						}
						nand_read(*tempu32, pagebuf, &pagebuf[0x800]);
						len = 0x840;
						val = 1;
						while(val!=0)
						{
							val = gecko_sendbuffer_safe(&pagebuf[0x840 - len], len);
							len-= len-val;
						}
					break;

					case 2://WRITE
						len = 4;
						val = 1;
						print_str_noscroll(112, 130, "WRTE\n");
						while(val!=0)
						{
							val = gecko_recvbuffer_safe(&temp[4 - len], len);
							len-= len-val;
						}
						len = 0x840;
						val = 1;
						while(val!=0)
						{
							val = gecko_recvbuffer_safe(&pagebuf[0x840 - len], len);
							len-= len-val;
						}
						nand_write(*tempu32, pagebuf, &pagebuf[0x800]);
					break;
				}
			}
		}
		len = 4;
	}

	return 0;
}

