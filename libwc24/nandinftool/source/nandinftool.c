#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	int i = 0;
	int len;
	FILE *f;
	if(argc!=2)
	{
		printf("nandinftool v1.0 by yellows8\n");
		printf("Calculate the checksum for NANDBOOTINFO.\n");
		return 0;
	}

	f = fopen(argv[1], "r+");
	if(f==NULL)return -1;
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(len!=0x1020)
	{
		printf("Invalid NANDBOOTINFO size: %x\n", len);
		fclose(f);
		return 0;
	}

	buffer = (unsigned int*)malloc(len);
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

	return 0;
}

