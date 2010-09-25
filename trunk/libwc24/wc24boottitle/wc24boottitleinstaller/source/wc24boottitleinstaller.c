#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>
#include <ogc/machine/processor.h>
#include <unistd.h>
#include <malloc.h>
#include <fat.h>
#include <wc24/wc24.h>

#include "sha1.h"
#include "RuntimeIOSPatch.h"

#include "title_tmd.h"
#include "title_tik.h"
#include "haxx_certs_bin.h"
#include "meta_app.h"
#include "wc24boottitle_dol.h"

#ifndef BIT
#define BIT(n) 1<<n
#endif

#define WC24ID 0x57434254//WCBT

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void aes_set_key(u8 *key);
void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);

int get_title_key(signed_blob *s_tik, u8 *key);
int forge_tmd(signed_blob *s_tmd);
int forge_tik(signed_blob *s_tik);

u8 *contents[2] = {(u8*)meta_app, (u8*)wc24boottitle_dol};

u64 wc24boottitle_titleID = 0x0001000857434254LL;

void Install(int arg)
{
	int which = -1;;
	u16 ci;
	s32 retval, cfd;
	tmd *TitleTMD;
	u8 *encrypt_content;
	u8 titlekey[16];
	u8 iv[16];

	printf("Install wc24boottitle? Press A to install, press B to skip install.\n");
	while(1)
	{
		WPAD_ScanPads();
		if((WPAD_ButtonsDown(0) & WPAD_BUTTON_A) || (arg & BIT(0)))which = 0;
		if((WPAD_ButtonsDown(0) & WPAD_BUTTON_B) || (arg & BIT(1)))which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}
	if(which==1)return;

	which = IOSPATCH_Apply();
	printf("IOS patches count: %x\n", which);

	printf("Installing ticket...\n");
	forge_tik((signed_blob*)title_tik);
	retval = ES_AddTicket((signed_blob*)title_tik, STD_SIGNED_TIK_SIZE, (signed_blob*)haxx_certs_bin, haxx_certs_bin_size, NULL, 0);
	if(retval<0)
	{
		printf("ES_AddTicket returned %d\n", retval);
		return;
	}

	TitleTMD = (tmd*)SIGNATURE_PAYLOAD((signed_blob*)title_tmd);
	TitleTMD->contents[0].size = (u64)meta_app_size;
	SHA1((u8*)meta_app, TitleTMD->contents[0].size, TitleTMD->contents[0].hash);
	TitleTMD->contents[1].size = (u64)wc24boottitle_dol_size;
	SHA1((u8*)wc24boottitle_dol, TitleTMD->contents[1].size, TitleTMD->contents[1].hash);
	forge_tmd((signed_blob*)title_tmd);

	printf("Installing TMD...\n");
	retval = ES_AddTitleStart((signed_blob*)title_tmd, title_tmd_size, (signed_blob*)haxx_certs_bin, haxx_certs_bin_size, NULL, 0);
	if(retval<0)
	{
		printf("ES_AddTitleStart returned %d\n", retval);
		return;
	}

	memset(titlekey, 0, 16);
	memset(iv, 0, 16);
	if(get_title_key((signed_blob*)title_tik, titlekey))
	{
		ES_AddTitleCancel();
		return;
	}
	aes_set_key(titlekey);

	printf("Installing content...(total %x)\n", (u32)TitleTMD->num_contents);
	for(ci=0; ci<TitleTMD->num_contents; ci++)
	{
		printf("Installing content %x/%x\n", ci+1, (int)TitleTMD->num_contents);
		
		memset(iv, 0, 16);
		memcpy(iv, &ci, 2);
		retval = ES_AddContentStart(TitleTMD->title_id, TitleTMD->contents[ci].cid);
		cfd = retval;
		if(retval<0)
		{
			printf("ES_AddContentStart returned %d\n", retval);
			ES_AddTitleCancel();
			return;
		}

		encrypt_content = (u8*)memalign(32, (u32)TitleTMD->contents[ci].size);
		aes_encrypt(iv, contents[ci], encrypt_content, TitleTMD->contents[ci].size);
		retval = ES_AddContentData(cfd, encrypt_content, TitleTMD->contents[ci].size);
		free(encrypt_content);
		if(retval<0)
		{
			printf("ES_AddContentData returned %d\n", retval);
			ES_AddContentFinish(cfd);
			ES_AddTitleCancel();
			return;
		}

		retval = ES_AddContentFinish(cfd);
		if(retval<0)
		{
			printf("ES_AddContentFinish returned %d\n", retval);
			ES_AddTitleCancel();
			return;
		}
	}

	printf("Finishing install...\n");
	retval = ES_AddTitleFinish();
	if(retval<0)
	{
		printf("ES_AddTitleFinish returned %d\n", retval);
		return;
	}
	printf("Done.\n");

	printf("Launching title...\n");
	WPAD_Shutdown();
	retval = WII_LaunchTitle(wc24boottitle_titleID);
	if(retval<0)printf("WII_LaunchTitle returned %d\n", retval);
}

void Delete(int arg)
{
	nwc24dl_record rec;
	u32 retval, numviews = 0;
	int which = -1;
	tikview *views;
	retval = ES_GetNumTicketViews(wc24boottitle_titleID, &numviews);
	if(retval<0 || numviews>4)return;

	printf("Detected wc24boottitle, delete the title? Press A to delete, press B to skip deleting.\n");
	while(1)
	{
		WPAD_ScanPads();
		if((WPAD_ButtonsDown(0) & WPAD_BUTTON_A) || ((arg>>2) & BIT(0)))which = 0;
		if((WPAD_ButtonsDown(0) & WPAD_BUTTON_B) || ((arg>>2) & BIT(1)))which = 1;
		if(which>-1)break;
		VIDEO_WaitVSync();
	}
	if(which==1)return;

	printf("Deleting...\n");
	views = (tikview*)memalign(32, sizeof(tikview) * numviews);
	memset(views, 0, sizeof(tikview) * numviews);
	retval = ES_GetTicketViews(wc24boottitle_titleID, views, numviews);
	if(retval<0)
	{
		printf("ES_GetTicketViews returned %d", retval);
		free(views);
		return;
	}

	retval = ES_DeleteTitleContent(wc24boottitle_titleID);
	if(retval<0)
	{
		printf("ES_DeleteTitleContent returned %d", retval);
		free(views);
		return;
	}
	retval = ES_DeleteTitle(wc24boottitle_titleID);
	if(retval<0)
	{
		printf("ES_DeleteTitle returned %d", retval);
		free(views);
		return;
	}

	retval = ES_DeleteTicket(views);
	free(views);
	if(retval<0)
	{
		printf("ES_DeleteTitle returned %d", retval);
		return;
	}

	printf("Deleting WC24 entries...\n");
	retval = WC24_Init();
	if(retval<0)
	{
		printf("WC24_Init returned %d\n", retval);
		return;
	}
	while((retval = WC24_FindRecord(WC24ID, &rec))!=LIBWC24_ENOENT)
	{
		WC24_DeleteRecord(retval);
	}
	WC24_Shutdown();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
	int arg = 0;
	int temp = 0;
	IOS_ReloadIOS(36);
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

	printf("\n\n");
	if(argc>=2)
	{
		sscanf(argv[1], "%x", &temp);
		if(temp==0)temp = 2;
		arg |= temp & 3;
		printf("Install argument specified, wc24bootitle will %s\n", temp==1?"be installed":"not be installed");
	}

	if(argc>=3)
	{
		sscanf(argv[2], "%x", &temp);
		if(temp==0)temp = 2;
		arg |= (temp<<2) & (3<<2);
		printf("Delete argument specified, wc24bootitle will %s\n", temp==1?"be deleted":"not be deleted");
	}
	if(arg)printf("Arguments(%x) were specified, this installer will automatically exit if install/delete fails or if wc24boottitle isn't installed.\n", arg);

	fatInitDefault();
	Delete(arg);
	Install(arg);

	printf("Press the home button to exit.\n");
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if((pressed & WPAD_BUTTON_HOME) || arg) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
