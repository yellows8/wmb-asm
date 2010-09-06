#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>
#include <ogc/machine/processor.h>
#include <network.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <fat.h>
#include <yellhttp.h>
#include <wc24/wc24.h>
#include <di/di.h>
#include <iso9660.h>

#include "loader_bin.h"
#include "tinyload_dol.h"

//Use WIILOADAPPDEBUG=1 for make input param to test wc24boottitle via wiiload directly. Without that option realmode is enabled for title installation, that doesn't work when attempting to boot via HBC.
//Uncomment these defines to test wc24boottitle via wiiload, without modifying NANDBOOTINFO.
//#define WIILOADTEST_BOOTHB "/apps/wc24app/boot.dol"//Uncomment this to test booting homebrew, by default when enabled this boots /apps/wc24app/boot.dol from SD.
//#define WIILOADTEST_BOOTDISC//Uncomment this to test booting discs.

#define WC24BOOTTITLE_VERSION "Stable: v1.0.0 0"
#ifdef LAN
#define SRVR_BASEURL "http://192.168.1.200/"
#else
#define SRVR_BASEURL "http://iwconfig.net/~yellows8/wc24boottitle/"
#endif

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

u32 launchcode = 0;
u64 curtitleid = 0;
nwc24dl_record myrec;
nwc24dl_entry myent;

char errstr[256];
char url[256];
char url_id[256];

char *log_buffer;
YellHttp_MIMEFormEntry formentries[1];
YellHttp_MIMEFormEntryField formfields[2] = {{"name", "file"}, {"filename", ""}};

char log_name[256];
char creds[256];
char log_line[256];

char localip[16];
char gateway[16];
char netmask[16];

void FlushLog();

void ProcessArgs(int argc, char **argv, int boothbdirect)
{
	int i;
	s32 retval;
	u32 bootcode = launchcode & ~(0xff<<24);//Mask out the high 8-bits of launchcode, since that's used for options etc.
	char *path = (char*)0x900FFF00;
	void (*entry)() = (void*)0x80001800;
	u64 nandboot_titleid;
	YellHttp_Ctx *ctx;
	int use_wc24http = 0;
	u32 index;
	FILE *fdol;
	struct stat dolstats;

	if(!fatInitDefault())printf("FAT init failed.\n");
	printf("Processing args...\n");
	#ifndef WIILOADAPPDEBUG
	if(argc && !boothbdirect)
	{
		sscanf(argv[0], "%016llx", &nandboot_titleid);
		if(curtitleid!=nandboot_titleid)
		{
			printf("Current titleID and titleID from NANDBOOTINFO don't match: %016llx %s\n", curtitleid, argv[0]);
			argc = 0;
		}
	}
	#endif

	if(argc)
	{
		if(boothbdirect)launchcode = 1;
		if(launchcode & BIT(24))use_wc24http = 1;

		switch(bootcode)
		{
			case 1://Boot homebrew
				if(argc<2)break;
				if(!boothbdirect)printf("Booting homebrew from: %s\n", argv[1]);
				memcpy((void*)0x80001800, loader_bin, loader_bin_size);
				memset(path, 0, 256);
				if(!boothbdirect)
				{
					if(strncmp(argv[1], "http", 4)==0)
					{
						if(!use_wc24http)
						{
							memset(localip, 0, 16);
							memset(netmask, 0, 16);
							memset(gateway, 0, 16);
							printf("Initializing network...\n");
							retval = if_config (localip, netmask, gateway, true);
							if(retval<0)
							{
								printf("Network init failed: %d\n", retval);
								break;
							}
							ctx = YellHttp_InitCtx();
							if(ctx==NULL)
							{
								printf("Failed to init/alloc http ctx.\n");
								break;
							}

							printf("Downloading %s...\n", argv[1]);
							retval = YellHttp_ExecRequest(ctx, argv[1]);
							YellHttp_FreeCtx(ctx);

							if(retval<0)
							{
								memset(errstr, 0, 256);
								YellHttp_GetErrorStr(retval, errstr, 256);
								printf("retval = %d str: %s", retval, errstr);
								break;
							}
							for(i=strlen(argv[1])-1; i>0; i--)
							{
								if(argv[1][i]=='/')break;
							}
							i++;
							strncpy(path, &argv[1][i], 255);
						}
						else
						{
							printf("Using WC24 to download: %s\n", argv[1]);

							printf("Creating record+entry...\n");
							retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_TITLEDATA, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_RSA_VERIFY_DISABLE, 0x3c, 0x5a0, argv[1], "wc24boottitle/boot.dol");
							if(retval<0)
							{
								printf("WC24_CreateRecord returned %d\n", retval);
								WC24_Shutdown();
								break;
							}
							index = retval;

							printf("Downloading...\n");
							printf("Deleting record+entry...\n");
							WC24_DeleteRecord(index);
							retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)index, 0x0);
							if(retval<0)
							{
								printf("KD_Download returned %d\n", retval);
								WC24_Shutdown();
								break;
							}

							printf("Mounting VFF...\n");
							retval = WC24_MountWC24DlVFF();
							if(retval<0)
							{
								printf("WC24_MountWC24DlVFF returned %d\n", retval);
								WC24_Shutdown();
								break;
							}

							printf("Reading wc24dl.vff:/wc24boottitle/boot.dol...\n");
							fdol = fopen("wc24dl.vff:/wc24boottitle/boot.dol", "r");
							if(fdol==NULL)
							{
								printf("Failed to open wc24dl.vff:/wc24boottitle/boot.dol\n");
							}
							else
							{
								stat("wc24dl.vff:/wc24boottitle/boot.dol", &dolstats);
								fread((void*)0x90100000, 1, dolstats.st_size, fdol);
								fclose(fdol);
								unlink("wc24dl.vff:/wc24boottitle/boot.dol");
								DCFlushRange((void*)0x90100000, dolstats.st_size);
							}

							printf("Unmounting VFF...\n");
							VFF_Unmount("wc24dl.vff");
						}
					}
					else
					{
						strncpy(path, argv[1], 255);
					}

					if(strncmp(path, "dvd", 3)==0)ISO9660_Mount();
					stat(path, &dolstats);
					fdol = fopen(path, "r");
					if(fdol==NULL)
					{
						printf("Dol doesn't exist: %s\n", argv[1]);
						break;
					}
					else
					{
						fread((void*)0x90100000, 1, dolstats.st_size, fdol);
						DCFlushRange((void*)0x90100000, dolstats.st_size);
						fclose(fdol);
					}
					memset(path, 0, 256);
				}

				DCFlushRange((void*)0x80001800, loader_bin_size);
				DCFlushRange(path, 256);
				if(!boothbdirect)WII_SetNANDBootInfoLaunchcode(0);
				if(!boothbdirect)
				{
					printf("Booting: %s\n", path);
				}
				else
				{
					printf("Booting homebrew directly from RAM buffer.\n");
				}
				WC24_Shutdown();
				DI_Close();
				WPAD_Shutdown();
				FlushLog();
				//IOS_ReloadIOS(36);
				SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
				entry();
			break;

			case 2://Boot game disc
				memcpy((void*)0x80001800, loader_bin, loader_bin_size);
				memset(path, 0, 256);
				memcpy((void*)0x90100000, tinyload_dol, tinyload_dol_size);
				DCFlushRange((void*)0x80001800, loader_bin_size);
				DCFlushRange(path, 256);
				DCFlushRange((void*)0x90100000, tinyload_dol_size);
				WII_SetNANDBootInfoLaunchcode(0);
				WC24_Shutdown();
				DI_Close();
				WPAD_Shutdown();
				printf("Booting game disc.\n");
				FlushLog();
				entry();
			break;

			default:
			break;
		}
	}

	printf("Invalid launchcode or argc, or invalid titleID: %x %x\n", launchcode, argc);
	#ifdef DEBUG
	printf("Press A to contine.\n");
	while(1)
	{
		WPAD_ScanPads();
		if(WPAD_ButtonsDown(0) & WPAD_BUTTON_A)break;
		VIDEO_WaitVSync();
	}
	#endif
	printf("Shutting down...\n");
	printf("Shutting down WC24...\n");
	WC24_Shutdown();
	DI_Close();
	FlushLog();
	WPAD_Shutdown();
	WII_Shutdown();
}

s32 ProcessWC24()//This installs entries for wc24boottitle auto-update, and processes the downloaded auto-update content downloaded via WC24. When wc24boottitle is deleted by the installer app, these entries aren't deleted by the installer app. When KD downloads title data entries, and can't find wc24dl.vff, KD deletes the entries since the data dir containing wc24dl.vff was deleted by ES when the installer app deleted wc24boottitle.
{
	s32 retval;
	u32 entry_bitmask = 0xf;
	FILE *fdol = NULL, *fver = NULL, *fconfig = NULL;
	int i;
	char *configbuf;
	char *updateinfobuf;
	char *configlines[0x10];
	char *updateinfolines[0x10];
	struct stat dolstats;
	u32 consoleID;
	int dlfreq;
	#ifdef LAN
	dlfreq = 1;//Set dl frequency to every 2 minutes for LAN, hourly for Internet.
	#else
	dlfreq = 0x3c;
	#endif

	retval = WC24_CreateWC24DlVFF(0x200000, 1);//2MB
	if(retval<0 && retval!=-105)//Return when VFF creation fails, except when the VFF already exists.
	{
		printf("WC24_CreateWC24DlVFF returned %d\n", retval);
		return retval;
	}

	retval = ES_GetDeviceID(&consoleID);
	if(retval<0)
	{
		printf("ES_GetDeviceID returned %d\n", retval);
	}
	memset(url_id, 0, 256);
	snprintf(url_id, 255, ".php?cid=%08x", consoleID);

	memset(url, 0, 256);
	snprintf(url, 255, "%swc24boottitle/installer.dol%s", SRVR_BASEURL, url_id);
	retval = WC24_FindEntry((u32)curtitleid, "installer.dol", &myent, 1);
	if(retval>=0)
	{
		if(strncmp(myent.url, url, 0xec))
		{
			WC24_DeleteRecord(retval);
			retval = LIBWC24_ENOENT;
		}
		else
		{
			retval = -1;
		}
	}

	if(retval==LIBWC24_ENOENT)//Only create the entry when it doesn't exist. When there's an entry with "installer.dol" for the URL filename but the whole URL doesn't match the one we're going to install, that entry is deleted then we create a new one.
	{
		printf("Creating record+entry for wc24boottitle auto-update installer...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_TITLEDATA, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_HB, dlfreq, 0x5a0, url, "wc24boottitle/installer.dol");
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return retval;
		}
		entry_bitmask |= BIT(0);
	}

	memset(url, 0, 256);
	snprintf(url, 255, "%swc24boottitle/verinfo%s", SRVR_BASEURL, url_id);
	retval = WC24_FindEntry((u32)curtitleid, "verinfo", &myent, 1);
	if(retval>=0)
	{
		if(strncmp(myent.url, url, 0xec))
		{
			WC24_DeleteRecord(retval);
			retval = LIBWC24_ENOENT;
		}
		else
		{
			retval = -1;
		}
	}

	if(retval==LIBWC24_ENOENT)
	{
		printf("Creating record+entry for wc24boottitle auto-update version info...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_TITLEDATA, WC24_RECORD_FLAGS_DEFAULT, WC24_FLAGS_HB, dlfreq, 0x5a0, url, "wc24boottitle/verinfo");
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return retval;
		}
		entry_bitmask |= BIT(1);
	}

	memset(url, 0, 256);
	snprintf(url, 255, "%swc24boottitle/bootmailupd%s", SRVR_BASEURL, url_id);
	retval = WC24_FindEntry((u32)curtitleid, "bootmailupd", &myent, 1);
	if(retval>=0)
	{
		if(strncmp(myent.url, url, 0xec))
		{
			WC24_DeleteRecord(retval);
			retval = LIBWC24_ENOENT;
		}
		else
		{
			retval = -1;
		}
	}

	if(retval==LIBWC24_ENOENT)
	{
		printf("Creating record+entry for wc24boottitle auto-update boot mail...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_MSGBOARD, WC24_FLAGS_HB, WC24_FLAGS_RSA_VERIFY_DISABLE, dlfreq, 0x5a0, url, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return retval;
		}
		entry_bitmask |= BIT(2);
	}

	memset(url, 0, 256);
	snprintf(url, 255, "%swc24boottitle/bootmailbt%s", SRVR_BASEURL, url_id);
	retval = WC24_FindEntry((u32)curtitleid, "bootmailbt", &myent, 1);
	if(retval>=0)
	{
		if(strncmp(myent.url, url, 0xec))
		{
			WC24_DeleteRecord(retval);
			retval = LIBWC24_ENOENT;
		}
		else
		{
			retval = -1;
		}
	}

	if(retval==LIBWC24_ENOENT)
	{
		printf("Creating record+entry for wc24boottitle boot mail...\n");
		retval = WC24_CreateRecord(&myrec, &myent, 0, 0, 0x4842, WC24_TYPE_MSGBOARD, WC24_FLAGS_HB, WC24_FLAGS_RSA_VERIFY_DISABLE, dlfreq, 0x5a0, url, NULL);
		if(retval<0)
		{
			printf("WC24_CreateRecord returned %d\n", retval);
			return retval;
		}
		entry_bitmask |= BIT(3);
	}

	if(entry_bitmask)
	{
		printf("New entries were installed, downloading the content immediately...\n");

		if(entry_bitmask & BIT(0))
		{
			memset(url, 0, 256);
			snprintf(url, 255, "%swc24boottitle/installer.dol%s", SRVR_BASEURL, url_id);
			retval = WC24_FindEntry((u32)curtitleid, url, &myent, 0);
			if(retval<0)
			{
				printf("Failed to find entry for wc24boottitle auto-update installer.\n");
			}
			else
			{
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download for wc24boottitle auto-update installer entry failed: %d\n", retval);
				if(myent.error_code!=0 && myent.error_code!=WC24_EHTTP304)printf("WC24 error code: %d\n", myent.error_code);
			}
		}

		if(entry_bitmask & BIT(1))
		{
			memset(url, 0, 256);
			snprintf(url, 255, "%swc24boottitle/verinfo%s", SRVR_BASEURL, url_id);
			retval = WC24_FindEntry((u32)curtitleid, url, &myent, 0);
			if(retval<0)
			{
				printf("Failed to find entry for wc24boottitle auto-update version info.\n");
			}
			else
			{
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download for wc24boottitle auto-update version info entry failed: %d\n", retval);
				if(myent.error_code!=0 && myent.error_code!=WC24_EHTTP304)printf("WC24 error code: %d\n", myent.error_code);
			}
		}

		if(entry_bitmask & BIT(2))
		{
			memset(url, 0, 256);
			snprintf(url, 255, "%swc24boottitle/bootmailupd%s", SRVR_BASEURL, url_id);
			retval = WC24_FindEntry((u32)curtitleid, url, &myent, 0);
			if(retval<0)
			{
				printf("Failed to find entry for wc24boottitle auto-update boot mail.\n");
			}
			else
			{
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download for wc24boottitle auto-update boot mail entry failed: %d\n", retval);
				if(myent.error_code!=0 && myent.error_code!=WC24_EHTTP304)printf("WC24 error code: %d\n", myent.error_code);
			}
		}

		if(entry_bitmask & BIT(3))
		{
			memset(url, 0, 256);
			snprintf(url, 255, "%swc24boottitle/bootmailbt%s", SRVR_BASEURL, url_id);
			retval = WC24_FindEntry((u32)curtitleid, url, &myent, 0);
			if(retval<0)
			{
				printf("Failed to find entry for wc24boottitle boot mail.\n");
			}
			else
			{
				retval = KD_Download(KD_DOWNLOADFLAGS_MANUAL, (u16)retval, 0x0);
				if(retval<0)printf("KD_Download for wc24boottitle boot mail entry failed: %d\n", retval);
				if(myent.error_code!=0 && myent.error_code!=WC24_EHTTP304)printf("WC24 error code: %d\n", myent.error_code);
			}
		}
	}

	printf("Processing content in wc24dl.vff...\n");
	retval = WC24_MountWC24DlVFF();
	if(retval<0)
	{
		printf("WC24_MountWC24DlVFF returned %d\n", retval);
		return retval;
	}

	fdol = fopen("wc24dl.vff:/wc24boottitle/installer.dol", "r");
	if(fdol==NULL)
	{
		printf("wc24dl.vff:/wc24boottitle/installer.dol doesn't exist, no update is available.\n");
	}
	else
	{
		fver = fopen("wc24dl.vff:/wc24boottitle/verinfo", "r");
		if(fver==NULL)
		{
			printf("wc24dl.vff:/wc24boottitle/verinfo doesn't exist, no update is available.\n");
		}
		else
		{
			configbuf = (char*)malloc(0x200);
			updateinfobuf = (char*)malloc(0x200);
			memset(configbuf, 0, 0x200);
			memset(updateinfobuf, 0, 0x200);
			for(i=0; i<0x10; i++)
			{
				configlines[i] = &configbuf[i*0x20];
				updateinfolines[i] = &updateinfobuf[i*0x20];
			}
			fread(updateinfobuf, 1, 0x200, fver);
			fclose(fver);

			fconfig = fopen("wc24dl.vff:/wc24boottitle/config", "r+");
			if(fconfig==NULL)
			{
				printf("Config file doesn't exist, creating.\n");
				fconfig = fopen("wc24dl.vff:/config", "w+");
				strncpy(configlines[0], WC24BOOTTITLE_VERSION, 0x1f);
				snprintf(configlines[1], 0x1f, "Revision: %d", 0);//SVN beta auto-update isn't implemented yet.
				fwrite(configbuf, 1, 0x200, fconfig);
				fseek(fconfig, 0, SEEK_SET);
			}
			else
			{
				fread(configbuf, 1, 0x200, fconfig);
				fseek(fconfig, 0, SEEK_SET);

				if(strncmp(configlines[0], WC24BOOTTITLE_VERSION, 0x1f)<0)
				{
					printf("The stable version contained in the config is older than the version contained in the title dol, updating the config version...\n");
					strncpy(configlines[0], WC24BOOTTITLE_VERSION, 0x1f);
					fwrite(configbuf, 1, 0x200, fconfig);
					fseek(fconfig, 0, SEEK_SET);
				}
			}

			if(strncmp(configlines[0], updateinfolines[0], 0x1f)<0)
			{
				printf("The stable version contained in the config is older than the version contained in the stable wc24boottitle version info WC24 dl content, updating...\n");
				strncpy(configlines[0], updateinfolines[0], 0x1f);
				fwrite(configbuf, 1, 0x200, fconfig);
				fseek(fconfig, 0, SEEK_SET);

				stat("wc24dl.vff:/installer.dol", &dolstats);
				fread((void*)0x90100000, 1, dolstats.st_size, fdol);
				DCFlushRange((void*)0x90100000, dolstats.st_size);
				printf("Update size: %x\n", (u32)dolstats.st_size);

				free(updateinfobuf);
				free(configbuf);
				fclose(fconfig);
				fclose(fdol);
				fclose(fver);
				unlink("wc24dl.vff:/wc24boottitle/installer.dol");
				unlink("wc24dl.vff:/wc24boottitle/verinfo");
				VFF_Unmount("wc24dl.vff");

				ProcessArgs(2, NULL, 1);
			}
			else
			{
				printf("Deleting wc24dl.vff:/installer.dol and wc24dl.vff:/verinfo since no update is available.\n");
				unlink("wc24dl.vff:/wc24boottitle/installer.dol");
				unlink("wc24dl.vff:/wc24boottitle/verinfo");
			}

			free(updateinfobuf);
			free(configbuf);
			fclose(fconfig);
		}
	}

	if(fdol)fclose(fdol);
	if(fver)fclose(fver);
	VFF_Unmount("wc24dl.vff");

	return 0;
}

void ResetWakeup_Timestamp()
{
	s32 fd;
	unsigned char *miscbuf;

	printf("Resetting wakeup time...\n");
	fd = ISFS_Open("/shared2/wc24/misc.bin", ISFS_OPEN_RW);
	if(fd<0)
	{
		printf("Failed to open misc.bin: %d\n", fd);
	}
	else
	{
		miscbuf = (unsigned char*)memalign(32, 0x400);
		memset(miscbuf, 0, 0x400);
		ISFS_Read(fd, miscbuf, 0x400);
		ISFS_Seek(fd, 0, SEEK_SET);
		
		time_t curtime = (time_t)((u32*)&miscbuf[0x3c]);
		struct tm *misc_time = gmtime(&curtime);
		printf(" Wakeup timestamp time: %s\n", asctime(misc_time));
		
		*((u32*)&miscbuf[0x38]) = 0;
		*((u32*)&miscbuf[0x3c]) = 0;
		ISFS_Write(fd, miscbuf, 0x400);
	
		free(miscbuf);
		ISFS_Close(fd);
	}
}

int authentication_callback(YellHttp_Ctx *ctx, char *realm, char *authout, void* usrarg, int digest, int invalidcreds)
{
	int i, passi;
	char *auth = (char*)usrarg;
	if(invalidcreds)return YELLHTTP_ENOCREDS;

	printf("Authentication realm: %s\n", realm);
	if(!digest)strcpy(authout, auth);
	if(digest)
	{
		for(i=0; i<strlen(auth) && auth[i]!=':'; i++)authout[i] = auth[i];
		authout[i] = ':';
		i++;
		strncat(authout, realm, 256);	
		passi = strlen(authout);
		authout[passi] = ':';
		passi++;
		strncpy(&authout[passi], &auth[i], 256);
	}
	return 0;
}

void FlushLog()
{
	#ifdef DEBUGPOST
	s32 retval;
	u32 cid = 0;
	YellHttp_Ctx *ctx = YellHttp_InitCtx();
	time_t curtime;
	struct tm *timeconv;

	printf("Flushing log...\n");
	memset(errstr, 0, 256);
	memset(creds, 0, 256);
	strncpy(creds, "wc24boottitle:WC24HAXX", 256);
	YellHttp_SetAuthCb(authentication_callback, creds);
	ctx->server_flags |= YELLHTTP_SRVFLAG_NOCACHE;

	memset(log_name, 0, 256);
	ES_GetDeviceID(&cid);
	curtime = time(NULL);
	timeconv = gmtime(&curtime);
	snprintf(log_name, 255, "%08x_%d-%d-%d_%d-%d-%d", cid, timeconv->tm_mday, timeconv->tm_mon, timeconv->tm_year - 100, timeconv->tm_hour, timeconv->tm_min, timeconv->tm_sec);
	

	formentries[0].numfields = 2;
	formentries[0].fields = formfields;
	formfields[1].value = log_name;
	formentries[0].path = NULL;
	memset(formentries[0].content_type, 0, 512);
	strncpy(formentries[0].content_type, "text/plain", 511);
	formentries[0].data = (unsigned char*)log_buffer;
	formentries[0].data_length = strnlen(log_buffer, 0x4000);

	memset(localip, 0, 16);
	memset(netmask, 0, 16);
	memset(gateway, 0, 16);
	printf("Initializing network...\n");
	retval = if_config (localip, netmask, gateway, true);
	if(retval<0)
	{
		printf("Network init failed: %d\n", retval);
		YellHttp_FreeCtx(ctx);
		free(log_buffer);
		return;
	}

	printf("Executing ExecRequest...(GET)\n");
	retval = YellHttp_ExecRequest(ctx, "http://192.168.1.33/wc24boottitle/debuglogs/upload.php");
	YellHttp_GetErrorStr(retval, errstr, 256);
	if(retval!=0)
	{
		printf("retval = %d str: %s", retval, errstr);
	}

	retval = YellHttp_EncodePostMIME_MultipartFormData(ctx, formentries, 1);
	YellHttp_GetErrorStr(retval, errstr, 256);
	if(retval!=0)printf("Encode MIME retval = %d str: %s", retval, errstr);
	memset(errstr, 0, 256);

	printf("Executing ExecRequest...(POST)\n");
	retval = YellHttp_ExecRequest(ctx, "http://192.168.1.33/wc24boottitle/debuglogs/upload.php");
	YellHttp_GetErrorStr(retval, errstr, 256);
	if(retval!=0)
	{
		printf("retval = %d str: %s", retval, errstr);
	}

	YellHttp_FreeCtx(ctx);

	free(log_buffer);
	#endif
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	s32 retval;

	DI_Init();
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
	if(usb_isgeckoalive(1))CON_EnableGecko(1, 1);

	log_buffer = (char*)malloc(0x4000);
	memset(log_buffer, 0, 0x4000);

	printf("Initializing WC24...\n");
	retval = WC24_Init();
	if(retval<0)
	{
		printf("WC24_Init returned %d\n", retval);
		return retval;
	}
	curtitleid = WC24_GetTitleID();

	printf("\n\n");
	printf("Getting NANDBOOTINFO argv...\n");
	argv = WII_GetNANDBootInfoArgv(&argc, &launchcode);
	#ifdef WIILOADAPPDEBUG
		#ifdef WIILOADTEST_BOOTDISC	
		argc = 1;
		launchcode = 2;
		#endif

		#ifdef WIILOADTEST_BOOTHB	
		launchcode = 1;
		argc = 2;
		argv[1] = WIILOADTEST_BOOTHB;
		#endif
	#endif

	ResetWakeup_Timestamp();
	#ifndef WIILOADAPPDEBUG
	retval = ProcessWC24();//Don't do any WC24 stuff with HBC wiiload, only with the actual installed wc24boottitle.
	#endif
	ProcessArgs(argc, argv, 0);
	printf("Shutting down WC24...\n");
	WC24_Shutdown();
	DI_Close();
	FlushLog();

	return 0;
}
