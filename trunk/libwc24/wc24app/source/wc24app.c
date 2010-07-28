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
#include <wc24/wc24.h>
#include <fat.h>
#include <sys/stat.h>
#include <dirent.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
nwc24dl_record myrec;
nwc24dl_entry myent;
char mailurl[256];

char hackmii_url[256];
char wiibrewnews_url[256];
char wiibrewreleases_url[256];

void IOSReload_SelectMenu();

typedef struct {//From libogc.
	u32 checksum;
	u8 flags;
	u8 type;
	u8 discstate;
	u8 returnto;
	u32 unknown[6];
} StateFlags;

#define TYPE_RETURN 3//From libogc.
#define TYPE_NANDBOOT 4
#define TYPE_SHUTDOWNSYSTEM 5
#define RETURN_TO_MENU 0
#define RETURN_TO_SETTINGS 1
#define RETURN_TO_ARGS 2

#define LANSRVR_ADDR "192.168.1.200"

static u32 __CalcChecksum(u32 *buf, int len)//Based on function from libogc.
{
	u32 sum = 0;
	int i;
	len = (len/4);

	for(i=0; i<len; i++)
		sum += buf[i];

	return sum;
}

void DoStuff(char *url)
{
	s32 retval;
	int which, i; //, enableboot;
	u32 triggers[2];
	u64 titleid;
	u64 homebrewtitleid = 0x0001000848424D4CLL;//TitleID for wiibrew+hackmii mail: 00010008-HBML. This is only an ID used for WC24, it's not a real NAND title.

	memset(hackmii_url, 0, 256);
	memset(wiibrewnews_url, 0, 256);
	memset(wiibrewreleases_url, 0, 256);	
	snprintf(hackmii_url, 255, "http://%s/hackmii/index.php", LANSRVR_ADDR);
	snprintf(wiibrewnews_url, 255, "http://%s/wiibrew/releases/index.php", LANSRVR_ADDR);
	snprintf(wiibrewreleases_url, 255, "http://%s/wiibrew/news/index.php", LANSRVR_ADDR);
	memset(mailurl, 0, 256);
	strncpy(mailurl, url, 255);

	printf("Use normal mail URL, or boot mail URL? Boot mail will wake up the Wii from idle mode to boot HBC, if you use(d) the option to enable WC24 title booting. Once the entry is installed with WC24 title booting enabled, you must either shutdown with the wiimote when \"Done\" is displayed, or with official software, in order for title booting to work.(A = normal mail, B = boot mail)\n");
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

	if(which)strncat(mailurl, "mail", 255);
	if(!which)strncat(mailurl, "boot", 255);

	fatInitDefault();

	printf("Initializing WC24...\n");
	retval = WC24_Init();
	if(retval<0)
	{
		printf("WC24_Init returned %d\n", retval);
		return;
	}
	titleid = WC24_GetTitleID();

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
			printf("WC24_CreateWC24DlVFF returned %d\n", retval);
			return;
		}
	}

	printf("Overwrite WC24 entries+records on NAND for the current title with the nwc24dlbak.bin entries from SD?(A = yes, B = no)\n");
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
		retval = WC24_FindEntry((u32)titleid, url, &myent);
		if(retval<0)
		{
			printf("Failed to find WC24 title data entry.\n");
		}
		else
		{
			fseek(f, 0x800 + (retval * sizeof(nwc24dl_entry)), SEEK_SET);
			fread(&myent, 1, sizeof(nwc24dl_entry), f);
			fseek(f, 0x80 + (retval * sizeof(nwc24dl_record)), SEEK_SET);
			fread(&myrec, 1, sizeof(nwc24dl_record), f);
			
			WC24_WriteEntry((u32)retval, &myent);
			WC24_WriteRecord((u32)retval, &myrec);
		}

		retval = WC24_FindEntry((u32)titleid, mailurl, &myent);
		if(retval<0)
		{
			printf("Failed to find WC24 mail entry.\n");
		}
		else
		{
			fseek(f, 0x800 + (retval * sizeof(nwc24dl_entry)), SEEK_SET);
			fread(&myent, 1, sizeof(nwc24dl_entry), f);
			fseek(f, 0x80 + (retval * sizeof(nwc24dl_record)), SEEK_SET);
			fread(&myrec, 1, sizeof(nwc24dl_record), f);
			
			WC24_WriteEntry((u32)retval, &myent);
			WC24_WriteRecord((u32)retval, &myrec);
		}
		fclose(f);
	}

	printf("Delete WC24 records+entries for the current title?(A = yes, B = no)\n");
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
		while((retval=WC24_FindRecord((u32)titleid, &myrec))!=LIBWC24_ENOENT)//Delete all HBC records+entries.
		{
			WC24_DeleteRecord((u32)retval);
		}
	}

	printf("Delete WC24 msg board mail records+entries for wiibrew+hackmii news/releases feed mail? This is LAN testing only.(A = yes, B = no)\n");
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
		while((retval=WC24_FindRecord((u32)homebrewtitleid, &myrec))!=LIBWC24_ENOENT)
		{
			WC24_DeleteRecord((u32)retval);
		}
	}

	printf("Install WC24 test title data dl record+entry?(A = yes, B = no)\n");
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
		printf("Creating record+entry...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_TITLEDATA, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, 0x3c, 0x5a0, url, "wc24test");//Set the dl_freq fields to download hourly and daily.
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}
	}

	printf("Install WC24 test msg board e-mail dl record+entry?(A = yes dl hourly, B = no, 1 = dl 30 every minutes, 2 = dl every 15 minutes, + = dl every 5 minutes)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 60;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_1)which = 30;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_2)which = 15;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_PLUS)which = 5;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		printf("Creating record+entry...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, which, 0x5a0, mailurl, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}
	}

	printf("Install WC24 msg board mail records+entries for wiibrew+hackmii news/releases feed mail? This is LAN testing only.(A = yes download hourly, 1 = yes download daily, B = no)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 0x3c;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_1)which = 0x5a0;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		printf("Creating record+entry(hackmii)...\n");
		retval = WC24_CreateRecord(&myrec, &myent, (u32)homebrewtitleid, homebrewtitleid, 0x4842, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, which, 0x5a0, hackmii_url, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}

		printf("Creating record+entry(wiibrew releases)...\n");
		retval = WC24_CreateRecord(&myrec, &myent, (u32)homebrewtitleid, homebrewtitleid, 0x4842, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, which, 0x5a0, wiibrewnews_url, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}

		printf("Creating record+entry(wiibrew news)...\n");
		retval = WC24_CreateRecord(&myrec, &myent, (u32)homebrewtitleid, homebrewtitleid, 0x4842, WC24_TYPE_MSGBOARD, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, which, 0x5a0, wiibrewreleases_url, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return;
		}
	}

	printf("Set the flag which enables WC24 title booting?(A = yes, B = no, 1 = clear flag)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 1;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_1)which = 2;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		if(which==1)wc24mail_nwc24msgcfg->wc24titleboot_enableflag = 1;
		if(which==2)wc24mail_nwc24msgcfg->wc24titleboot_enableflag = 0;
		retval = WC24Mail_CfgUpdate();
		if(retval<0)printf("WC24Mail_Update returned %d\n", retval);
	}

	/*printf("Write a test WC24 NANDBOOTINFO to NAND?(A = yes, B = no)\n");
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
		//u8 *nandinfobuf;
		//FILE *finfo;
		//printf("Opening NANDBOOTINFO in NAND...\n");
		s32 infofd = ISFS_Open("/shared2/sys/NANDBOOTINFO", ISFS_OPEN_RW);
		if(infofd<0)
		{
			printf("Failed to open NANDBOOTINFO in NAND.\n");
		}
		else
		{
			printf("Opening /NANDBOOTINFO on SD...\n");
			finfo = fopen("/NANDBOOTINFO", "r");
			if(finfo==NULL)
			{
				printf("Failed to open /NANDBOOTINFO on SD.\n");
			}
			else
			{
				printf("Allocating buffer...\n");
				nandinfobuf = (u8*)memalign(32, 0x1020);
				if(nandinfobuf)
				{
					printf("Reading from SD...\n");
					fread(nandinfobuf, 1, 0x1020, finfo);
					printf("Writing to NAND...\n");
					ISFS_Write(infofd, nandinfobuf, 0x1020);
					free(nandinfobuf);
				}
				else
				{
					printf("Failed to allocate buffer.\n");
				}
				fclose(finfo);
				
			}
			ISFS_Close(infofd);
		}
		retval = WII_LaunchTitleWithArgsWC24(0x0001000148415445LL, 0, 0);
		if(retval<0)printf("WII_LaunchTitleWithArgsWC24 returned %d\n", retval);
	}*/

	printf("Get time triggers?(A = yes, B = no)\n");
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
		memset(triggers, 0, 8);
		retval = KD_GetTimeTriggers(triggers);
		if(retval<0)printf("KD_GetTimeTriggers returned %d\n", retval);
		printf("KD_Download trigger: %d KD_CheckMail: %d\n", triggers[0], triggers[1]);
	}

	printf("Download entries immediately?(A = yes all, B = no, 1 = only test entries, 2 = only hackmii+wiibrew mail)\n");
	which = -1;
	WPAD_ScanPads();
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)which = 0;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)which = 3;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_1)which = BIT(0);
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_2)which = BIT(1);
		if(which>-1)break;
		VIDEO_WaitVSync();
	}

	if(which)
	{
		if(which & BIT(0))
		{
			retval = WC24_FindEntry((u32)titleid, url, &myent);
			if(retval<0)
			{
				printf("Failed to find WC24 title data entry.\n");
			}
			else
			{
				printf("Downloading title data...\n");
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				printf("KD_Download returned %d\n", retval);
			}

			retval = WC24_FindEntry((u32)titleid, mailurl, &myent);
			if(retval<0)
			{
				printf("Failed to find WC24 mail entry.\n");
			}
			else
			{
				printf("Downloading mail...\n");
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download returned %d\n", retval);
				retval = KD_SaveMail();//This is called so that the unsaved dlcnt.bin mail content isn't overwritten when following mail is downloaded immediately. This should always be called after downloading mail immediately.
				if(retval<0)printf("KD_SaveMail returned %d\n", retval);
			}
		}

		if(which & BIT(1))
		{
			retval = WC24_FindEntry((u32)homebrewtitleid, hackmii_url, &myent);
			if(retval>=0)
			{
				printf("Downloading hackmii mail...\n");
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download returned %d\n", retval);
				retval = KD_SaveMail();
				if(retval<0)printf("KD_SaveMail returned %d\n", retval);
			}

			retval = WC24_FindEntry((u32)homebrewtitleid, wiibrewnews_url, &myent);
			if(retval>=0)
			{
				printf("Downloading wiibrew news mail...\n");
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download returned %d\n", retval);
				retval = KD_SaveMail();
				if(retval<0)printf("KD_SaveMail returned %d\n", retval);
			}

			retval = WC24_FindEntry((u32)homebrewtitleid, wiibrewreleases_url, &myent);
			if(retval>=0)
			{
				printf("Downloading wiibrew releases mail...\n");
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download returned %d\n", retval);
				retval = KD_SaveMail();
				if(retval<0)printf("KD_SaveMail returned %d\n", retval);
			}
		}

		/*retval = WC24_FindEntry(0x524d4345, "https://mariokartwii.race.gs.nintendowifi.net/raceservice/messagedl_us_en.ashx", &myent);//This was used to rip the raw mail content from MK, from /shared2/wc24/mobx/dlcnt.bin.
		if(retval<0)
		{
			printf("Failed to find MK WC24 mail entry.\n");
		}
		else
		{
			printf("Downloading MK mail...\n");
			retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
			printf("KD_Download returned %d\n", retval);
		}*/
	}

	printf("Check WC24 entries for WC24 error, and dump timestamps?(A = yes, B = no)\n");
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
		char *dlbuf = NULL;
		retval = WC24_FindEntry((u32)titleid, url, &myent);
		if(retval<0)
		{
			printf("Failed to find WC24 title data entry.\n");
		}
		else
		{
			dltime = WC24_TimestampToSeconds(myent.dl_timestamp);
			time = localtime(&dltime);
			printf("error_code %d total_errors %d ", myent.error_code, myent.total_errors);
			if(myent.dl_timestamp!=0)
			{
				printf("dl_timestamp %s ", asctime(time));
			}
			else
			{
				if(myent.error_code!=0 || myent.error_code!=WC24_EHTTP304)printf("This entry was never downloaded since dl_timestamp is zero.\n");
			}
			if((myent.error_code==0 || myent.error_code==WC24_EHTTP304) && myent.dl_timestamp!=0)
			{
				printf("Reading VFF since download was successful.\n");
				printf("Mounting...\n");
				if((retval = WC24_MountWC24DlVFF())<0)
				{
					printf("failed %d\n", retval);
				}
				else
				{
					FILE *fdlfile = NULL;
					struct stat filestats;
					printf("Opening wc24test file in VFF...\n");
					fdlfile = fopen("vff:/wc24test", "r");
					if(fdlfile==NULL)
					{
						printf("Failed to open wc24test in VFF.\n");
					}
					else
					{
						stat("vff:/wc24test", &filestats);
						printf("Filesize: %x\n", (u32)filestats.st_size);
						if(filestats.st_size)dlbuf = (char*)malloc((u32)filestats.st_size);
						if(dlbuf)
						{
							memset(dlbuf, 0, filestats.st_size);
							retval = fread(dlbuf, 1, filestats.st_size, fdlfile);
							printf("Content:\n");
							for(i=0; i<(u32)filestats.st_size; i++)
							{
								if(dlbuf[i])printf("%c", dlbuf[i]);//Don't print null bytes.
							}
							free(dlbuf);
							printf("\n");
						}
						else
						{
							printf("Failed to allocate buffer for content, or filesize is zero.\n");
						}
						printf("Closing file...\n");
						fclose(fdlfile);
					}

					printf("List the VFF root directory entries?(A = yes, B = no)\n");
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
						DIR *dir = opendir("vff:/");
						struct dirent *dent;
						if(dir==NULL)
						{
							printf("opendir failed.\n");
						}
						else
						{
							printf("opened dir\n");
							while((dent = readdir(dir))!=NULL)
							{
								printf("Found dirent: %s\n", dent->d_name);
							}
							closedir(dir);
							printf("closed dir\n");
						}
					}

					printf("Unmounting VFF...\n");
					VFF_Unmount();
					printf("VFF reading done.\n");
				}
			}
		}

		retval = WC24_FindEntry((u32)titleid, mailurl, &myent);
		if(retval<0)
		{
			printf("Failed to find WC24 mail entry.\n");
		}
		else
		{
			printf("error_code %d total_errors %d ", myent.error_code, myent.total_errors);
		}
	}

	printf("Shutting down WC24...\n");
	retval = WC24_Shutdown();
	if(retval<0)
	{
		printf("WC24_Shutdown returned %d\n", retval);
		return;
	}
	printf("Done.\n");
}

int shutdown = 0;
void shutdown_callback(u32 chan)
{
	shutdown = 1;
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	char url[256];
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

	WPAD_SetPowerButtonCallback((WPADShutdownCallback)&shutdown_callback);

	memset(url, 0, 256);
	strncpy(url, "http://members.iglide.net/ticeandsons/yellowstar/wc24test", 255);
	printf("\nUse a Internet server URL(Button A) or a LAN server URL?(Button B)\n");
	WPAD_ScanPads();	
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)break;
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_B)
		{
			snprintf(url, 255, "http://%s/wc24test", LANSRVR_ADDR);
			break;
		}
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)return 0;
		VIDEO_WaitVSync();
	}

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
		if(shutdown)
		{
			WPAD_Shutdown();
			#define WIILAUNCHMOD
			#ifndef WIILAUNCHMOD
			StateFlags *state = memalign(32, sizeof(StateFlags));
			s32 fd = ISFS_Open("/title/00000001/00000002/data/state.dat", ISFS_OPEN_RW);
			if(fd<0)
			{
				printf("Failed to open sysmenu state.dat.\n");
			}
			else
			{
				ISFS_Read(fd, state, sizeof(StateFlags));
				ISFS_Seek(fd, 0, SEEK_SET);
				state->type = TYPE_NANDBOOT;
				state->checksum = __CalcChecksum((void*)((int)state + 4), sizeof(StateFlags));
				ISFS_Write(fd, state, sizeof(StateFlags));
				ISFS_Close(fd);
			}
			SYS_ResetSystem(SYS_POWEROFF, 0, 0);
			#else
			WII_Shutdown();
			#endif
		}

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
