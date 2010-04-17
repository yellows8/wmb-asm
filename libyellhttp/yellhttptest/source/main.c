#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yellhttp.h>

#ifdef ARM9
#include <nds.h>
#include <fat.h>
#include <dswifi9.h>

void init_nds();
void OnKeyPressed(int key);
#endif

#ifdef HW_RVL
#include <ogcsys.h>
#include <gccore.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <wiiuse/wpad.h>
#include <fat.h>

void init_wii();

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

char localip[16];
char gateway[16];
char netmask[16];
#endif

void console_pause();

int main(int argc, char **argv)
{
	int retval;	
	char *url;
	char errstr[256];

	#ifdef ARM9
	init_nds();
	Keyboard *kbd = keyboardDemoInit();
	kbd->OnKeyPressed = OnKeyPressed;	
	url = (char*)malloc(256);
	memset(url, 0, 256);
	scanf("%s", url);
	argc = 2;
	#endif
	
	#ifdef HW_RVL
	init_wii();
	
	url = (char*)malloc(256);
	memset(url, 0, 256);
	strncpy(url, "http://192.168.1.33/", 256);
	argc = 2;
	#endif
	
	#ifdef WIN32
	printf("yellhttptest\n");
	if(argc==2)url = argv[1];	
	#endif

	#ifdef LINUX
	printf("yellhttptest\n");
	if(argc==2)url = argv[1];
	#endif

	if(argc!=2)return -2;
	YellHttp_Ctx *ctx = YellHttp_InitCtx();
	if(ctx==NULL)
	{
		printf("initctx alloc fail\n");
		#ifdef ARM9		
		free(url);
		console_pause();
		#endif
		#ifdef HW_RVL
		free(url);
		console_pause();
		#endif
		return -1;
	}
	memset(errstr, 0, 256);
	printf("Executing ExecRequest...(URL: %s)\n", url);
	retval = YellHttp_ExecRequest(ctx, url);
	YellHttp_GetErrorStr(retval, errstr, 256);
	printf("retval = %d str: %s", retval, errstr);
	YellHttp_FreeCtx(ctx);

	#ifdef ARM9
	free(url);
	console_pause();
	#endif
	#ifdef HW_RVL
	free(url);
	console_pause();
	#endif
	return 0;
}

#ifdef ARM9
void console_pause()
{
	while(1)
	{
		scanKeys();
		if(keysDown() & KEY_A)exit(0);
		swiWaitForVBlank();
	}
}

void init_nds()
{
	consoleDemoInit();
	printf("yellhttptest\n");
	printf("Initializing FAT...\n");
	if(!fatInitDefault())
	{
		printf("FAT init failed.\n");
		console_pause();
	}
	printf("Connecting via WFC data ...\n");

	if(!Wifi_InitDefault(WFC_CONNECT))
	{
		printf("Failed to connect!\nPress A to exit.\n");
		console_pause();
	}
	printf("Connected.\n");
}

void OnKeyPressed(int key) {
   if(key > 0)
      iprintf("%c", key);
}
#endif

#ifdef HW_RVL
void console_pause()
{
	while(1)
	{
		VIDEO_WaitVSync();
		WPAD_ScanPads();
		
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)exit(0);
	}
}

void init_wii()
{
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

	printf("yellhttptest\n");
	/*printf("Initializing FAT...\n");
	if(!fatInitDefault())
	{
		printf("FAT init failed.\n");
		console_pause();
	}*/
	printf("Configuring network ...\n");
	memset(localip, 0, 16);
	memset(netmask, 0, 16);
	memset(gateway, 0, 16);
	s32 ret = if_config (localip, netmask, gateway, true);
	if(ret<0)
	{
		printf("Network config failed: %d\n", ret);
		console_pause();
	}
	else
	{
		printf("Network config done.\n");
	}
}
#endif

