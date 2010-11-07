/*
wc24decrypt is licensed under the MIT license:
Copyright (c) 2009 - 2010 yellowstar6

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
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

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;

void aesofb_decrypt(u8 *iv, u8 *inbuf, u8 *outbuf, unsigned long long len);
void aes_set_key(u8 *key);

typedef struct _swc24pubkmod
{
    unsigned char rsa_public[256];
    unsigned char rsa_unk[256];
    unsigned char aes_key[16];
    unsigned char aes_iv[16];//always zero in wc24pubk.mod
} swc24pubkmod;

char str[256];
char fnstr[256];

int GetFileLength(FILE* _pfile)
{
	int l_iSavPos, l_iEnd;

	l_iSavPos = ftell(_pfile);
	fseek(_pfile, 0, SEEK_END);
	l_iEnd = ftell(_pfile);
	fseek(_pfile, l_iSavPos, SEEK_SET);

	return l_iEnd;
}

int main(int argc, char **argv)
{
    unsigned char magic[4] = "WC24";
    u32 temp;
    swc24pubkmod keys;
    printf("wc24decrypt v1.0 by yellowstar6\n");
    if(argc<3 || argc>5)
    {
        printf("Decrypt WC24 content, content downloaded with WC24 with the WC24 header.\n");
        printf("Usage:\n");
        printf("wc24decrypt <content.bin> <decrypt.bin> <wc24pubk.mod> <options>\n");
        printf("The wc24pubk.mod can also by a 16 byte key.\n");
        printf("The mod/key filename can be excluded if the content is not encrypted.\n");
        printf("The content.bin filename can be a http(s) URL to download and decrypt.\n");
	printf("Options:\n");
	printf("--cache: Cache the download, don't delete it before downloading and don't disable sending the If-Modified header. Default is no cache.\n");
    }
    else
    {
            FILE *fwc24, *fout, *fkeys = NULL;
            unsigned char *inbuf, *outbuf;
            unsigned int inlen;
	    int retval;
	    YellHttp_Ctx *ctx;
	    int cache = 0;
	    char *temp;
	    char errstr[256];
            memset(str, 0, 256);
            memset(fnstr, 0, 256);

	    if(argc>=5)
	    {	    
		if(strcmp(argv[4], "--cache")==0)cache = 1;
	    }

            if(strncmp(argv[1], "http", 4)==0)
            {
		ctx = YellHttp_InitCtx();
	    	if(ctx==NULL)
	    	{
			printf("YellHttp ctx alloc failed.\n");
			return -1;
	    	}

                memset(str, 0, 256);
		memset(errstr, 0, 256);
                strncpy(str, argv[1], 256);
                int i = strlen(str);
                while(str[i-1]!='/')i--;
                strncpy(fnstr, &str[i], strlen(str) - i);

		if(!cache)
		{
			remove(fnstr);
			ctx->server_flags |= YELLHTTP_SRVFLAG_NOCACHE;
		}
                retval = YellHttp_ExecRequest(ctx, str);
		YellHttp_GetErrorStr(retval, errstr, 256);
		if(retval!=0)
		{
			printf("Error: retval = %d str: %s", retval, errstr);
			YellHttp_FreeCtx(ctx);
			
			if(ctx->http_status==304)
			{
				printf("HTTP 304, content not modified stop.\n");
				return 34;
			}

			if(ctx->http_status==404)
			{
				printf("HTTP 404, content not found stop.\n");
				return 44;
			}

			if(ctx->http_status!=200)
			{
				printf("Aborting due to HTTP %d.\n", ctx->http_status);
				return 11;
			}
		}

		memset(str, 0, 256);
		snprintf(str, 255, "touch --date=\"%sUTC\" %s", asctime(localtime(&ctx->lastmodified)), fnstr);
		temp = strchr(str, 0x0a);
		if(temp)*temp = ' ';
		system(str);

		YellHttp_FreeCtx(ctx);
            }
            else
            {
                strncpy(fnstr, argv[1], 256);
            }

            fwc24 = fopen(fnstr, "rb");
            if(fwc24==NULL)
            {
                printf("Failed to open %s\n", fnstr);
                return 1;
            }

            fout = fopen(argv[2], "wb");
            if(fout==NULL)
            {
                printf("Failed to open %s\n", argv[2]);
                fclose(fwc24);
                return 1;
            }

            if(argc-cache==4)
            {
                fkeys = fopen(argv[3], "rb");
                if(fkeys==NULL)
                {
                    fclose(fwc24);
                    fclose(fout);
                    printf("Failed to open %s\n", argv[3]);
		    return 1;
                }
            }

            inlen = GetFileLength(fwc24);
            inbuf = (unsigned char*)malloc(inlen);
            if(inbuf==NULL)
            {
                printf("Failed to allocate memory.\n");
                fclose(fwc24);
                fclose(fout);
                if(fkeys)fclose(fkeys);
                return 1;
            }
            fread(inbuf, 1, inlen, fwc24);
            fclose(fwc24);

            if(memcmp(inbuf, magic, 4)!=0)
            {
                printf("WC24 header not found; raw content?\n");
            }

            memcpy(&temp, &inbuf[4], 4);
            temp = be32toh(temp);
            printf("WC24 header version is 0x%x.\n", (unsigned int)temp);
            if(temp>1)
            {
                printf("Unsupported version.\n");
                if(fkeys)fclose(fkeys);
                fclose(fout);
                free(inbuf);
                return 1;
            }

            if(inbuf[0xc]==0)
            {
                printf("Content is not encrypted.\n");
            }
            else if(inbuf[0xc]==1)
            {
                printf("Content is AES-128-OFB encrypted.\n");
            }
            else
            {
                printf("Unknown enryption type.\n");
                fclose(fout);
                if(fkeys)fclose(fkeys);
                free(inbuf);
                return 1;
            }

            outbuf = (unsigned char*)malloc(inlen - 0x140);
            if(outbuf==NULL)
            {
                printf("Failed to allocate memory.\n");
                if(fkeys)fclose(fkeys);
                fclose(fout);
                free(inbuf);
                return 1;
            }

            if(inbuf[0xc]==1)
            {
                if(argc-cache!=4)
                {
                    printf("wc24pubk.mod/keys filename not specified.\n");
                    fclose(fout);
                    free(inbuf);
                    free(outbuf);
                    return 1;
                }
                unsigned int readlen = fread(&keys, 1, sizeof(swc24pubkmod), fkeys);
                fclose(fkeys);
                if(readlen==16)memcpy(keys.aes_key, (void*)&keys, 16);
                memcpy(keys.aes_iv, &inbuf[0x30], 16);
            }

            if(inbuf[0xc]==1)
            {
                aes_set_key(keys.aes_key);
                aesofb_decrypt(keys.aes_iv, &inbuf[0x140], outbuf, inlen - 0x140);
                fwrite(outbuf, 1, inlen - 0x140, fout);
            }
            else
            {
                fwrite(&inbuf[0x140], 1, inlen - 0x140, fout);
            }
            fclose(fout);
            free(inbuf);
            free(outbuf);
	    printf("Done.\n");
    }
    return 0;
}
