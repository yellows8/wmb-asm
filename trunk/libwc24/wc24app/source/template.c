/*
libwc24 is licensed under the MIT license:
Copyright (c) 2009 and 2010 yellowstar6

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the Software), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>
#include <wc24.h>
#include <fat.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void DoStuff(char *url)
{
	nwc24dl_record rec;
	nwc24dl_entry ent;
	s32 retval;
	u64 *titleid = (u64*)memalign(32, 8);
	int which, i;	
	char mailurl[256];
	memset(mailurl, 0, 256);
	strncpy(mailurl, url, 255);
	strncat(mailurl, "mail", 255);

	if(titleid==NULL)
	{
		printf("Failed to alloc mem.\n");
		return;
	}
	printf("Getting titleid...\n");
	/*retval = ES_GetTitleID(titleid);
	if(retval<0)
	{
		free(titleid);
		printf("ES_GetTitleID returned %d\n", retval);
		return;
	}*/
	*titleid = 0x000100014a4f4449LL;
	
	printf("Identify as HBC?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	printf("Initalizing WC24...\n");
	retval = WC24_Init(which);
	if(retval<0)
	{
		free(titleid);
		printf("WC24_Init returned %d\n", retval);
		return;
	}
	
	printf("Create wc24dl.vff?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		printf("Creating wc24dl.vff...\n");
		retval = WC24_CreateWC24DlVFF(128 * 1024);
		if(retval<0)
		{
			free(titleid);
			printf("WC24_CreateWC24DlVFF returned %d\n", retval);
			return;
		}
	}

	printf("Overwrite JODI WC24 entries+records on NAND with the nwc24dlbak.bin entries from SD?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		FILE *f = fopen("/nwc24dlbak.bin", "rb");
		retval = WC24_FindEntry(0x4a4f4449, url, &ent);
		if(retval<0)
		{
			printf("Failed to find WC24 title data entry.\n");
		}
		else
		{
			fseek(f, 0x800 + (retval * sizeof(nwc24dl_entry)), SEEK_SET);
			fread(&ent, 1, sizeof(nwc24dl_entry), f);
			fseek(f, 0x80 + (retval * sizeof(nwc24dl_record)), SEEK_SET);
			fread(&rec, 1, sizeof(nwc24dl_record), f);
			
			WC24_WriteEntry((u32)retval, &ent);
			WC24_WriteRecord((u32)retval, &rec);
		}

		retval = WC24_FindEntry(0x4a4f4449, mailurl, &ent);
		if(retval<0)
		{
			printf("Failed to find WC24 mail entry.\n");
		}
		else
		{
			fseek(f, 0x800 + (retval * sizeof(nwc24dl_entry)), SEEK_SET);
			fread(&ent, 1, sizeof(nwc24dl_entry), f);
			fseek(f, 0x80 + (retval * sizeof(nwc24dl_record)), SEEK_SET);
			fread(&rec, 1, sizeof(nwc24dl_record), f);
			
			WC24_WriteEntry((u32)retval, &ent);
			WC24_WriteRecord((u32)retval, &rec);
		}
		fclose(f);
	}

	printf("Delete JODI WC24 records+entries?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		while((retval=WC24_FindRecord(0x4a4f4449, &rec))!=LIBWC24_ENOENT)//Delete all HBC records+entries.
		{
			printf("deleting %x %d\n", retval, retval);
			WC24_DeleteRecord((u32)retval);
		}
	}

	printf("Install WC24 title data dl record+entry?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		printf("creating record\n");
		retval = WC24_CreateRecord(&rec, &ent, (u32)*titleid, *titleid, 0x4842, WC24_TYPE_TITLEDATA, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, 0xf, 0x5a0, url, "wc24test");//Set the dl_freq fields to download hourly and daily.0x3c
		if(retval<0)
		{
			free(titleid);
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}
		
		if(usb_isgeckoalive(1))usb_sendbuffer_safe(1, &ent, sizeof(nwc24dl_entry));
	}

	printf("Install WC24 msg board e-mail dl record+entry?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		printf("creating record\n");
		retval = WC24_CreateRecord(&rec, &ent, (u32)*titleid, *titleid, 0x4842, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, 0xf, 0x5a0, mailurl, NULL);//Set the dl_freq fields to download hourly and daily.0x3c
		if(retval<0)
		{
			free(titleid);
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}
		
		if(usb_isgeckoalive(1))usb_sendbuffer_safe(1, &ent, sizeof(nwc24dl_entry));
	}

	printf("Download entries immediately?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		retval = KD_Open();
		if(retval<0)
		{
			printf("KD_Open failed: %d\n", retval);
		}
		else
		{
			retval = WC24_FindEntry(0x4a4f4449, url, &ent);
			if(retval<0)
			{
				printf("Failed to find WC24 title data entry.\n");
			}
			else
			{
				printf("Downloading title data...\n");
				retval = KD_Download(BIT(1), (u16)retval, 0x0);
				printf("KD_Download returned %d\n", retval);
			}

			retval = WC24_FindEntry(0x4a4f4449, mailurl, &ent);
			if(retval<0)
			{
				printf("Failed to find WC24 mail entry.\n");
			}
			else
			{
				printf("Downloading mail...\n");
				retval = KD_Download(BIT(1), (u16)retval, 0x0);
				printf("KD_Download returned %d\n", retval);
			}
			KD_Close();
		}
	}

	printf("Check WC24 entry for WC24 error, and dump timestamps?(A = yes, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		struct tm *time;
		time_t dltime;
		fat_filectx *fdl;
		char *dlbuf;
		retval = WC24_FindEntry(0x4a4f4449, url, &ent);
		if(retval<0)
		{
			printf("Failed to find WC24 title data entry.\n");
		}
		else
		{
			dltime = WC24_TimestampToSeconds(ent.dl_timestamp);
			time = localtime(&dltime);
			printf("error_code %d total_errors %d ", ent.error_code, ent.total_errors);
			printf("dl_timestamp %s ", asctime(time));
			if(ent.error_code==0 || ent.error_code==WC24_EHTTP304)
			{
				printf("Reading VFF since download was successful.\n");
				printf("Mounting...\n");
				if((retval = WC24_MountWC24DlVFF())<0)
				{
					printf("failed %d\n", retval);
				}
				else
				{
					printf("Opening wc24test file in VFF...\n");
					fdl = VFF_Open("WC24TEST");
					if(fdl==NULL)
					{
						printf("Failed to open wc24test in VFF.\n");
					}
					else
					{
						printf("Filesize: %x cluster %x\n", fdl->filesize, fdl->cluster);
						dlbuf = (char*)malloc(fdl->filesize);
						if(dlbuf)
						{
							VFF_Read(fdl, (u8*)dlbuf, fdl->filesize);
							printf("Content:\n");
							for(i=0; i<fdl->filesize; i++)printf("%c", dlbuf[i]);
							free(dlbuf);
							printf("\n");
						}
						else
						{
							printf("Failed to allocate buffer for content.\n");
						}
						printf("Closing file...\n");
						VFF_Close(fdl);
					}

					printf("Unmounting VFF...\n");
					VFF_Unmount();
					printf("VFF reading done.\n");
				}
			}
		}

		retval = WC24_FindEntry(0x4a4f4449, mailurl, &ent);
		if(retval<0)
		{
			printf("Failed to find WC24 mail entry.\n");
		}
		else
		{
			printf("error_code %d total_errors %d ", ent.error_code, ent.total_errors);
		}
	}

	free(titleid);
	printf("Shutting down WC24...\n");
	retval = WC24_Shutdown();
	if(retval<0)
	{
		printf("WC24_Shutdown returned %d\n", retval);
		return;
	}
	printf("Done.\n");
}

void IOSReload_SelectMenu()
{
	s32 retval;
	u32 numtitles = 0;
	u32 numios = 0;
	u64 *titles, *ios;
	int i, iosi, noreload;
	retval = ES_GetNumTitles(&numtitles);
	if(retval<0)
	{
		printf("ES_GetNumTitles returned %d\n", retval);
		return;
	}
	titles = (u64*)memalign(32, numtitles * 8);
	memset(titles, 0, numtitles * 8);
	retval = ES_GetTitles(titles, numtitles);
	if(retval<0)
	{
		printf("ES_GetTitles returned %d\n", retval);
		free(titles);
		return;
	}

	for(i=0; i<numtitles; i++)
	{
		if((titles[i]>>32)==1)
		{
			if(((u32)titles[i])>0x02 && ((u32)titles[i])<0x100)
			{
				numios++;
			}
		}
	}
	ios = (u64*)memalign(32, numios * 8);
	memset(ios, 0, numios * 8);

	iosi = 0;
	for(i=0; i<numtitles; i++)
	{
		if((titles[i]>>32)==1)
		{
			if(((u32)titles[i])>0x02 && ((u32)titles[i])<0x100)
			{
				ios[iosi] = titles[i];
				iosi++;
			}
		}
	}

	iosi = 0;
	noreload = 0;
	printf("\x1b[2J");//Clear screen, so that gecko output is displayed correctly.
	printf("\x1b[1;0HSelect a IOS to reload. Press B to skip IOS reload.\n");
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		printf("\x1b[2;0HIOS     ");
		printf("\x1b[2;0HIOS%d", (int)ios[iosi]);
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)break;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)
		{
			noreload = 1;
			break;
		}
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_RIGHT)iosi++;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_LEFT)iosi--;
		if(iosi>=(int)numios)iosi = 0;
		if(iosi<0)iosi = numios-1;
		VIDEO_WaitVSync();
	}	

	printf("\n");
	if(!noreload)
	{
		printf("Reloading IOS%d...\n", (int)ios[iosi]);
		WPAD_Shutdown();
		IOS_ReloadIOS((u32)ios[iosi]);
		WPAD_Init();
	}
	free(titles);
	free(ios);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	char *url = (char*)"http://members.iglide.net/ticeandsons/yellowstar/wc24test";
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
	//DEBUG_Init(GDBSTUB_DEVICE_USB, 1);
	if(usb_isgeckoalive(1))CON_EnableGecko(1, 1);

	IOSReload_SelectMenu();

	printf("\nUse a Internet server URL(Button A) or a LAN server URL?(Button B)\n");
	WPAD_ScanPads();	
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)break;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)
		{
			url = (char*)"http://yellzone.en/wc24test";//This is a LAN only server, this domain name isn't registered.
			break;
		}
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)return 0;
		VIDEO_WaitVSync();
	}

	fatInitDefault();
	DoStuff(url);

	WPAD_ScanPads();
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME )return 0;

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
