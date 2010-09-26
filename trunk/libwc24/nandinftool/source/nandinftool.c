#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wiilaunch.h"

unsigned int CalcSum(unsigned int *buf, unsigned int len)
{
	unsigned int temp, cursum = 0;
	int i;
	for(i=1; i<len/4; i++)
	{
		temp = be32toh(buf[i]);
		cursum+= temp;
	}
	return cursum;
}

int main(int argc, char **argv)
{
	unsigned int *buffer;
	unsigned int oldsum = 0;
	unsigned int cursum = 0;
	unsigned int temp = 0;
	unsigned int launchcode = 0;
	unsigned long long titleID;
	int i = 0;
	int len = 0;
	char **nandboot_argv;
	int nandboot_argc;
	FILE *f;
	if(argc<2)
	{
		printf("nandinftool v1.0 by yellowstar6\n");
		printf("Calculate the checksum for NANDBOOTINFO, and read/write launchcode and arguments.\n");
		printf("If the input file doesn't exist, it will be created. When writing the NANDBOOTINFO, the titletype is set to 8 for WC24 title booting.\n");
		printf("Usage:\n");
		printf("nandinftool <NANDBOOTINFO> <launchcode> <titleID> <arguments>\n");
		printf("The parameters following the filename are optional, those are only needed when writing the NANDBOOTINFO.\n");
		return 0;
	}

	f = fopen(argv[1], "r+");
	if(f==NULL)len = 0;
	if(f)
	{	
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET);
	}
	buffer = (unsigned int*)malloc(0x1020);
	memset(buffer, 0, 0x1020);

	if(len!=0x1020)
	{
		printf(" Invalid input filesize, or file doesn't exist: %x\n", len);
		if(f)fclose(f);
		f = fopen(argv[1], "w+");
		fwrite(buffer, 1, 0x1020, f);
		fseek(f, 0, SEEK_SET);
		printf("Created valid empty NANDBOOTINFO as input filename.\n");
	}

	fread(buffer, 1, len, f);
	oldsum = be32toh(buffer[i]);
	printf("Original sum: %x\n", oldsum);

	cursum = CalcSum(buffer, len);
	printf("Current sum: %x\n", cursum);
	if(oldsum!=cursum)
	{
		cursum = htobe32(cursum);
		fseek(f, 0, SEEK_SET);
		fwrite(&cursum, 1, 4, f);
		printf("Sum updated.\n");
	}
	free(buffer);
	fclose(f);

	if((i = WII_Initialize(argv[1]))<0)
	{
		printf("WII_Initialize failed: %d\n", i);
		return 0;
	}
	nandboot_argv = WII_GetNANDBootInfoArgv(&nandboot_argc, &launchcode);
	printf("Current launchcode: %x\n", launchcode);
	printf("Current argc: %d\n", nandboot_argc);
	for(i=0; i<nandboot_argc; i++)
	{
		printf("arg%d: %s\n", i, nandboot_argv[i]);
	}
	if(argc<4)return 0;

	nandboot_argc = argc - 4;	
	nandboot_argv = (char**)malloc((nandboot_argc+2) * 4);
	memset(nandboot_argv, 0, (nandboot_argc+2) * 4);
	for(i=0; i<nandboot_argc; i++)
	{
		nandboot_argv[i] = (char*)malloc(256);
		memset(nandboot_argv[i], 0, 256);
		strncpy(nandboot_argv[i], argv[i+4], 255);
	}
	sscanf(argv[2], "%x", &launchcode);
	sscanf(argv[3], "%016llx", &titleID);
	WII_LaunchTitleWithArgsWC24(titleID, launchcode, nandboot_argv);
	for(i=0; i<nandboot_argc; i++)
	{
		if(nandboot_argv[i])free(nandboot_argv[i]);
	}
	free(nandboot_argv);
	printf("Updated launchcode and arguments.\n");

	return 0;
}

