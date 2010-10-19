/*
getwiimsg is licensed under the MIT license:
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
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

void encode( FILE *infile, FILE *outfile, int linesize );
void decode( FILE *infile, FILE *outfile );

int GetFileLength(FILE* _pfile);
unsigned int GetLangCode(char *lang);
void Base64Decode(unsigned char *data, unsigned char *dest, unsigned char bytes);

char language_codes[7][3] = {{"ja"}, {"en"}, {"de"}, {"fr"}, {"es"}, {"it"}, {"nl"}};
char str[256];
char str2[256];

int ProcessMail(unsigned int index)
{
	FILE *fmail, *fmail2;
	char *buffer, *newbuf, *base64, *decodebuf;
	int filelen, newlen, base64len, base64decode_len;
	unsigned int bom = 0xfffe;
	char base64begin[5];
	base64begin[0] = 0x0d;
	base64begin[1] = 0x0a;
	base64begin[2] = 0x0d;
	base64begin[3] = 0x0a;
	base64begin[4] = 0x00;
	sprintf(str, "decrypt%d.txt", index);
	fmail = fopen(str, "rb");
	if(fmail==NULL)
	{
		printf("Failed to open %s\n", str);
		return 2;
	}
	filelen = GetFileLength(fmail);
	buffer = (char*)malloc(filelen);
	if(buffer==NULL)
	{
		printf("Failed to alloc memory.(decrypt %d)\n", filelen);
		fclose(fmail);
		return 1;
	}
	fread(buffer, 1, filelen, fmail);
	fclose(fmail);
	
	newbuf = strstr(buffer, "Date");
	if(newbuf==NULL)
	{
		printf("Empty msg file.\n");
		free(buffer);
		return 3;
	}
	newlen = ((unsigned int)strstr(newbuf, "--BoundaryForDL") - (unsigned int)newbuf);
	sprintf(str, "mail%d.eml", index);
	fmail = fopen(str, "wb");
	if(fmail==NULL)
	{
		printf("Failed to open %s\n", str);
		free(buffer);
		return 2;
	}
	fwrite(newbuf, 1, newlen, fmail);
	
	fclose(fmail);
	
	base64 = (char*)(((unsigned int)strstr(newbuf, "base64")));
	base64 = strstr(base64, base64begin);
	base64+= 4;
	base64len = ((unsigned int)strstr(base64, "--") - (unsigned int)base64 - 2);
	base64decode_len = (base64len + (base64len * 2)) / 4;
	decodebuf = (char*)malloc(base64len);
	if(decodebuf==NULL)
	{
		printf("Failed to alloc memory.(base64 %d)\n", base64len);
		fclose(fmail);
		return 1;
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
	return 0;
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
	int lastfail = 0;
	if(argc<4)
	{
		printf("getwiimsg v1.1 by yellowstar6.\n");
		printf("Download Wii Message Board mail from server, decrypt, and dump to .eml and text files.\n");
		printf("Local encrypted mail can be decrypted and dumped as well.\n");
		printf("Internal KD WC24 AES key is needed.\n");
		printf("Usage:\n");
		printf("getwiimsg <country code number> <language code> <wc24msgboardkey.bin or wc24pubk.mod for title mail> <options> <optional list of\nalternate msg files to process from a server or locally>\n");
		printf("language code can be one of the following: ja, en, de, fr, es, it, nl.\n");
		printf("Options:\n");
		printf("--cache: Cache the download, don't delete it before downloading and don't disable sending the If-Modified header. Default is no cache.");
	}
	else
	{
		unsigned int country_code, language_code, argi, index = 0, cache = 0, numoptions = 0;
		int retval;
		memset(str, 0, 256);
		sscanf(argv[1], "%d", &country_code);
		language_code = GetLangCode(argv[2]);
		if(argc>4)
		{
			for(argi=4; argi<argc; argi++)
			{
				if(strcmp(argv[argi], "--cache")==0)
				{
					cache = 1;
					numoptions++;
				}
			}
		}

		if(argc-numoptions==4)
		{
			for(argi=1; argi<5; argi++)
			{
				snprintf(str, 255, "wc24decrypt http://cfh.wapp.wii.com/announce/%03d/%d/%d.bin decrypt%d.txt %s", country_code, language_code, argi, index, argv[3]);
				if(cache)strncat(str, " --cache", 255);
				printf("%s\n", str);
				retval = WEXITSTATUS(system(str));
				memset(str, 0, 256);
				if(retval!=0)
				{
					lastfail = retval;
					if(retval==34)
					{
						printf("Not modified.\n");
					}
					else if(retval==44)
					{
						printf("Not found.\n");
					}
					else
					{
						printf("HTTP fail.\n");
					}
				}
				else
				{
					retval = ProcessMail(index);
					if(retval!=0)lastfail = retval;
				}
				index++;
			}
		}
		else if(argc-numoptions>4)
		{
			for(argi=4+numoptions; argi<argc; argi++)
			{
				snprintf(str, 255, "wc24decrypt %s decrypt%d.txt %s", argv[argi], index, argv[3]);
				if(cache)strncat(str, " --cache", 255);
				printf("%s\n", str);
				retval = WEXITSTATUS(system(str));
				memset(str, 0, 256);
				if(retval!=0)
				{
					lastfail = retval;
					if(retval==34)
					{
						printf("Not modified.\n");
					}
					else if(retval==44)
					{
						printf("Not found.\n");
					}
					else
					{
						printf("HTTP fail.\n");
					}
				}
				else
				{
					retval = ProcessMail(index);
					if(retval!=0)lastfail = retval;
				}
				index++;
			}
		}
	}
	return lastfail;
}
