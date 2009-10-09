#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void encode( FILE *infile, FILE *outfile, int linesize );
void decode( FILE *infile, FILE *outfile );

int GetFileLength(FILE* _pfile);
unsigned int GetLangCode(char *lang);
void Base64Decode(unsigned char *data, unsigned char *dest, unsigned char bytes);

char language_codes[7][3] = {{"ja"}, {"en"}, {"de"}, {"fr"}, {"es"}, {"it"}, {"nl"}};
char str[256];
char str2[256];

void ProcessMail(unsigned int index)
{
	FILE *fmail, *fmail2;
	char *buffer, *newbuf, *base64, *decodebuf;
	int filelen, newlen, base64len, base64decode_len;
	unsigned int bom = 0xfffe;
	sprintf(str, "decrypt%d.txt", index);
	fmail = fopen(str, "rb");
	if(fmail==NULL)
	{
		printf("Failed to open %s\n", str);
		return;
	}
	filelen = GetFileLength(fmail);
	buffer = (char*)malloc(filelen);
	if(buffer==NULL)
	{
		printf("Failed to alloc memory.\n");
		fclose(fmail);
		return;
	}
	fread(buffer, 1, filelen, fmail);
	fclose(fmail);
	
	newbuf = strstr(buffer, "Date");
	if(newbuf==NULL)
	{
		printf("Empty msg file.\n");
		free(buffer);
		return;
	}
	newlen = ((unsigned int)strstr(newbuf, "--BoundaryForDL") - (unsigned int)newbuf);
	sprintf(str, "mail%d.eml", index);
	fmail = fopen(str, "wb");
	if(fmail==NULL)
	{
		printf("Failed to open %s\n", str);
		free(buffer);
		return;
	}
	fwrite(newbuf, 1, newlen, fmail);
	
	fclose(fmail);
	
	base64 = (char*)(((unsigned int)strstr(newbuf, "base64")) + 10);
	base64len = ((unsigned int)strstr(base64, "------") - (unsigned int)base64 - 2);
	base64decode_len = (base64len + (base64len * 2)) / 4;
	decodebuf = (char*)malloc(base64len);
	if(decodebuf==NULL)
	{
		printf("Failed to alloc memory.\n");
		fclose(fmail);
		return;
	}
	memset(decodebuf, 0, base64len);
	sprintf(str, "b64_%d.txt", index);
	fmail = fopen(str, "wb");
	fwrite(base64, 1, base64len, fmail);
	
	sprintf(str2, "msg%d.txt", index);
	fclose(fmail);
	fmail = fopen(str, "rb");
	fmail2 = fopen(str2, "wb");
	fwrite(&bom, 1, 2, fmail2);
	decode(fmail, fmail2);
	
	
	fclose(fmail);
	fclose(fmail2);
	free(buffer);
	free(decodebuf);
}

int GetFileLength(FILE* _pfile)
{
	int l_iSavPos, l_iEnd;

	l_iSavPos = ftell(_pfile);
	fseek(_pfile, 0, SEEK_END);
	l_iEnd = ftell(_pfile);
	fseek(_pfile, l_iSavPos, SEEK_SET);

	return l_iEnd;
}

unsigned int GetLangCode(char *lang)
{
	unsigned int i;
	for(i=0; i<7; i++)
	{
		if(strcmp(lang, language_codes[i])==0)break;
	}
	return i;
}

int main(int argc, char **argv)
{
	if(argc<4)
	{
		printf("getwiimsg v1.0 by yellows8.\n");
		printf("Download Wii Message Board mail from server, decrypt, and dump to .eml and .txt files.\n");
		printf("Local encrypted mail can be decrypted and dumped as well.\n");
		printf("Internal KD WC24 AES key is needed.\n");
		printf("Usage:\n");
		printf("getwiimsg <country code number> <language code> <wc24msgboardkey.bin> <optional list of\nalternate msg files to process from a server or locally>\n");
		printf("language code can be one of the following: ja, en, de, fr, es, it, nl.\n");
	}
	else
	{
		unsigned int country_code, language_code, argi, index = 0;
		memset(str, 0, 256);
		sscanf(argv[1], "%d", &country_code);
		language_code = GetLangCode(argv[2]);
		if(argc==4)
		{
			sprintf(str, "wc24decrypt http://cfh.wapp.wii.com/announce/%03d/%d/%d.bin decrypt%d.txt %s", country_code, language_code, 1, index, argv[3]);
			printf("%s\n", str);
			system(str);
			memset(str, 0, 256);
			ProcessMail(index);
			index++;
			
			sprintf(str, "wc24decrypt http://cfh.wapp.wii.com/announce/%03d/%d/%d.bin decrypt%d.txt %s", country_code, language_code, 2, index, argv[3]);
			printf("%s\n", str);
			system(str);
			memset(str, 0, 256);
			ProcessMail(index);
			index++;
		}
		else if(argc>4)
		{
			for(argi=4; argi<argc; argi++)
			{
				sprintf(str, "wc24decrypt %s decrypt%d.txt %s", argv[argi], index, argv[3]);
				printf("%s\n", str);
				system(str);
				memset(str, 0, 256);
				ProcessMail(index);
				index++;
			}
		}
	}
	return 0;
}
