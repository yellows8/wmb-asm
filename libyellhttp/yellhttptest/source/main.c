/*
yellhttptest is licensed under the MIT license:
Copyright (c) 2010 yellowstar6

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

char localip[16] = {0};
char gateway[16] = {0};
char netmask[16] = {0};
#endif

char errstr[256];

void console_pause();

void authentication_callback(YellHttp_Ctx *ctx, char *realm, char *authout, void* usrarg, int digest)
{
	int i, passi;
	char *auth = (char*)usrarg;
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
}

int main(int argc, char **argv)
{
	int retval;	
	char *url;

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
	if(argc>=2)url = argv[1];
	#endif

	#ifdef LINUX
	printf("yellhttptest\n");
	if(argc>=2)url = argv[1];
	#endif

	#ifndef HW_RVL
	if(argc==1)return -2;
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

	if(argc==3)
	{
		YellHttp_SetAuthCb(authentication_callback, argv[2]);
	}
	printf("Executing ExecRequest...(URL: %s)\n", url);
	retval = YellHttp_ExecRequest(ctx, url);
	YellHttp_GetErrorStr(retval, errstr, 256);
	printf("retval = %d str: %s", retval, errstr);
	YellHttp_FreeCtx(ctx);
	#endif

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
	VIDEO_Init();
	WPAD_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
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
	//memset(localip, 0, 16);
	//memset(netmask, 0, 16);
	//memset(gateway, 0, 16);
	s32 ret = if_config (localip, netmask, gateway, true);
	if(ret<0)
	{
		printf("Network config failed: %d\n", ret);
		console_pause();
	}
	else
	{
		printf("Network config done ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);
	}
	while(1)
	{
		VIDEO_WaitVSync();
		WPAD_ScanPads();
		
		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_A)break;
	}
	printf("test net_socket() retval = %d\n", net_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
}
#endif

