#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <string.h>

#include "sha1.h"

#include "title_tmd.h"
#include "title_tik.h"
#include "haxx_certs_bin.h"
#include "meta_app.h"
#include "wc24boottitle_dol.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

void aes_set_key(u8 *key);
void aes_encrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);

int get_title_key(signed_blob *s_tik, u8 *key);
int forge_tmd(signed_blob *s_tmd);
int forge_tik(signed_blob *s_tik);

u8 *contents[2] = {(u8*)meta_app, (u8*)wc24boottitle_dol};

void Install()
{
	u16 ci;
	s32 retval, cfd;
	tmd *TitleTMD;
	u8 titlekey[16];
	u8 iv[16];

	printf("Installing ticket...\n");
	forge_tik((signed_blob*)title_tik);
	retval = ES_AddTicket((signed_blob*)title_tik, STD_SIGNED_TIK_SIZE, (signed_blob*)haxx_certs_bin, haxx_certs_bin_size, NULL, 0);
	if(retval<0)
	{
		printf("ES_AddTicket returned %d\n", retval);
		return;
	}

	TitleTMD = SIGNATURE_PAYLOAD(title_tmd);
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

	printf("Installing content...\n");
	for(ci=0; ci<(int)TitleTMD->num_contents; ci++)
	{
		printf("Installing content %x/%x\n", ci+1, (int)TitleTMD->num_contents);
		
		memcpy(iv, &ci, 2);
		retval = ES_AddContentStart(TitleTMD->title_id, TitleTMD->contents[ci].cid);
		cfd = retval;
		if(retval<0)
		{
			printf("ES_AddContentStart returned %d\n", retval);
			ES_AddTitleCancel();
			return;
		}

		aes_encrypt(iv, contents[ci], contents[ci], TitleTMD->contents[ci].size);
		retval = ES_AddContentData(cfd, contents[ci], TitleTMD->contents[ci].size);
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
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------
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


	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");

	printf("Press the home button to exit.\n");
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if(pressed & WPAD_BUTTON_HOME) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
