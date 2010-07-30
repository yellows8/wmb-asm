/*-------------------------------------------------------------

wiilaunch.c -- Wii NAND title launching and argument passing

Copyright (C) 2008
Hector Martin (marcan)

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1.	The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.

2.	Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3.	This notice may not be removed or altered from any source
distribution.

-------------------------------------------------------------*/

//This was modified for nandinftool.

//#if defined(HW_RVL)

#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
/*#include "ipc.h"
#include "asm.h"
#include "processor.h"
#include "es.h"
#include "video.h"*/
#include "wiilaunch.h"

static char *__nandbootinfo = NULL;

static int __initialized = 0;

typedef struct {
	u32 checksum;
	u32 argsoff;
	u8 unk1;
	u8 unk2;
	u8 apptype;
	u8 titletype;
	u32 launchcode;
	u32 unknown[2];
	u64 launcher;
	u8 argbuf[0x1000];
} NANDBootInfo;

#define TYPE_RETURN 3
#define TYPE_NANDBOOT 4
#define TYPE_SHUTDOWNSYSTEM 5
#define RETURN_TO_MENU 0
#define RETURN_TO_SETTINGS 1
#define RETURN_TO_ARGS 2


static NANDBootInfo nandboot;

static char **nandboot_argv = NULL;
static int nandboot_argc = 0;

static void __WII_NANDBootInfoUpdateArgv();

static u32 __CalcChecksum(u32 *buf, int len)
{
	u32 sum = 0;
	int i;
	len = (len/4);

	for(i=1; i<len; i++)
		sum += be32toh(buf[i]);

	return sum;
}

static void __SetChecksum(void *buf, int len)
{
	u32 *p = (u32*)buf;
	p[0] = htobe32(__CalcChecksum(p, len));
}

static int __ValidChecksum(void *buf, int len)
{
	u32 *p = (u32*)buf;
	return be32toh(p[0]) == __CalcChecksum(p, len);
}

static s32 __WII_ReadNANDBootInfo(void)
{
	//int fd;
	FILE *fd;
	int ret;

	fd = fopen(__nandbootinfo, "rb");
	if(fd==NULL)return WII_EINTERNAL;
	/*fd = IOS_Open(__nandbootinfo,IPC_OPEN_READ);
	if(fd < 0) {
		memset(&nandboot,0,sizeof(nandboot));
		return WII_EINTERNAL;
	}*/

	ret = fread(&nandboot, 1, sizeof(nandboot), fd);
	/*IOS_Read(fd, &nandboot, sizeof(nandboot));
	IOS_Close(fd);*/
	fclose(fd);
	if(ret != sizeof(nandboot)) {
		memset(&nandboot,0,sizeof(nandboot));
		return WII_EINTERNAL;
	}
	if(!__ValidChecksum(&nandboot, sizeof(nandboot))) {
		memset(&nandboot,0,sizeof(nandboot));
		return WII_ECHECKSUM;
	}

	__WII_NANDBootInfoUpdateArgv();

	return 0;
}

static s32 __WII_WriteNANDBootInfo(void)
{
	FILE *fd;
	//int fd;
	int ret;

	__SetChecksum(&nandboot, sizeof(nandboot));

	/*fd = IOS_Open(__nandbootinfo,IPC_OPEN_READ|IPC_OPEN_WRITE);
	if(fd < 0) {
		return WII_EINTERNAL;
	}*/
	fd = fopen(__nandbootinfo, "r+");
	if(fd==NULL)return WII_EINTERNAL;

	ret = fwrite(&nandboot, 1, sizeof(nandboot), fd);
	fclose(fd);
	/*ret = IOS_Write(fd, &nandboot, sizeof(nandboot));
	IOS_Close(fd);*/
	if(ret != sizeof(nandboot)) {
		return WII_EINTERNAL;
	}

	__WII_NANDBootInfoUpdateArgv();

	return 0;
}

static void __WII_NANDBootInfoUpdateArgv()//This was added by yellowstar6.
{
	int i;
	unsigned int *argbuf;
	char *argvbuf;
	if(be32toh(nandboot.argsoff)<0x1000)return;//Invalid argsoff, this NANDBOOTINFO doesn't have any arguments.
	if(nandboot_argv)
	{
		for(i=0; i<256; i++)
		{
			if(nandboot_argv[i]==NULL)break;
			free(nandboot_argv[i]);
			nandboot_argv[i] = NULL;
		}
	}
	else
	{
		nandboot_argv = (char**)malloc(256 * 4);
		memset(nandboot_argv, 0, 256 * 4);
	}

	argbuf = (unsigned int*)&nandboot.argbuf[be32toh(nandboot.argsoff) - 0x1000];
	nandboot_argc = be32toh(*argbuf);
	argbuf++;
	
	for(i=0; i<nandboot_argc; i++)
	{
		argvbuf = (char*)&nandboot.argbuf[be32toh(*argbuf) - 0x1000];
		argbuf++;
		nandboot_argv[i] = (char*)malloc(256);
		memset(nandboot_argv[i], 0, 256);
		strncpy(nandboot_argv[i], argvbuf, 255);
	}
}

s32 WII_Initialize(char *path)
{
	__nandbootinfo = path;
	__WII_ReadNANDBootInfo();
	__initialized = 1;
	return 0;
}

s32 __WII_SetArgs(const char **argv)
{
	int argslen = 0;
	int buflen = 0;
	int argvp, argp;
	int argc = 0;
	int i;

	if(!__initialized)
		return WII_ENOTINIT;
	for(i=0; argv[i] != NULL; i++) {
		argslen += strlen(argv[i]) + 1;
		argc++;
	}
	buflen = (argslen + 3) & ~3;
	buflen += 4 * argc;
	buflen += 8;
	if(buflen > 0x1000)
		return WII_E2BIG;

	argp = 0x1000 - argslen;
	argvp = 0x1000 - buflen;
	memset(&nandboot.argbuf, 0, sizeof(nandboot.argbuf));
	nandboot.argsoff = htobe32(0x1000 + argvp);
	*((u32*)&nandboot.argbuf[argvp]) = htobe32(argc);
	argvp += 4;
	for(i=0; i<argc; i++) {
		strcpy((char*)&nandboot.argbuf[argp], argv[i]);
		*((u32*)&nandboot.argbuf[argvp]) = htobe32(argp + 0x1000);
		argvp += 4;
		argp += strlen(argv[i]) + 1;
	}
	*((u32*)&nandboot.argbuf[argvp]) = 0;

	return 0;
}

s32 WII_LaunchTitleWithArgs(u64 titleID, int launchcode, ...)
{
	char argv0[20];
	const char *argv[256];
	int argc = 1;
	va_list args;

	if(!__initialized)
		return WII_ENOTINIT;

	sprintf(argv0, "%016llx", titleID);

	argv[0] = argv0;

	va_start(args, launchcode);

	do {
		argv[argc++] = va_arg(args, const char *);
	} while(argv[argc - 1] != NULL);

	va_end(args);

	//if(ES_GetTitleID(&nandboot.launcher) < 0)
		nandboot.launcher = htobe64(0x100000002LL);

	if(titleID == 0x100000002LL)
		nandboot.titletype = 4;
	else
		nandboot.titletype = 2;
	nandboot.apptype = 0x80;
	nandboot.launchcode = htobe32(launchcode);

	//stateflags.type = TYPE_RETURN;
	//stateflags.returnto = RETURN_TO_ARGS;

	__WII_SetArgs(argv);

	//__WII_WriteStateFlags();
	__WII_WriteNANDBootInfo();

	//return WII_LaunchTitle(titleID);
	return 0;
}

s32 WII_LaunchTitleWithArgsWC24(u64 titleID, int launchcode, char **_argv)
{
	char argv0[20];
	const char *argv[256];
	int argc = 1;
	//va_list args;

	if(!__initialized)
		return WII_ENOTINIT;

	sprintf(argv0, "%016llx", titleID);

	argv[0] = argv0;
	//va_start(args, launchcode);

	do {
		//argv[argc++] = va_arg(args, const char *);
		argv[argc] = _argv[argc-1];
		argc++;
	} while(argv[argc - 1] != NULL);

	//va_end(args);
	//if(ES_GetTitleID(&nandboot.launcher) < 0)
		nandboot.launcher = htobe64(0x100000002LL);

	nandboot.titletype = 8;
	nandboot.apptype = 0x80;
	nandboot.launchcode = htobe32(launchcode);

	//if(dolaunch)stateflags.type = 4;
	__WII_SetArgs(argv);
	//__WII_WriteStateFlags();
	__WII_WriteNANDBootInfo();

	//if(dolaunch)return WII_LaunchTitle(nandboot.launcher);
	return 0;
}

s32 WII_OpenURL(const char *url)
{
	/*u32 tmdsize;
	u64 tid = 0;
	u64 *list;
	u32 titlecount;
	s32 ret;
	u32 i;

	if(!__initialized)
		return WII_ENOTINIT;

	ret = ES_GetNumTitles(&titlecount);
	if(ret < 0)
		return WII_EINTERNAL;

	list = memalign(32, titlecount * sizeof(u64) + 32);

	ret = ES_GetTitles(list, titlecount);
	if(ret < 0) {
		free(list);
		return WII_EINTERNAL;
	}

	for(i=0; i<titlecount; i++) {
		if((list[i] & ~0xFF) == 0x1000148414400LL) {
			tid = list[i];
			break;
		}
	}
	free(list);

	if(!tid)
		return WII_EINSTALL;

	if(ES_GetStoredTMDSize(tid, &tmdsize) < 0)
		return WII_EINSTALL;*/

	return WII_LaunchTitleWithArgs(0x1000148414400LL, 0, url, NULL);
}

char **WII_GetNANDBootInfoArgv(int *argc, u32 *launchcode)
{
	if(argc)*argc = (int)nandboot_argc;
	if(launchcode)*launchcode = be32toh(nandboot.launchcode);
	return (char**)nandboot_argv;
}

void WII_SetNANDBootInfoLaunchcode(u32 launchcode)
{
	nandboot.launchcode = htobe32(launchcode);
	__WII_WriteNANDBootInfo();
}

//#endif
