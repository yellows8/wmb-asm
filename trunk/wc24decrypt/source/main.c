#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int GetFileLength(FILE* _pfile)
{
	int l_iSavPos, l_iEnd;

	l_iSavPos = ftell(_pfile);
	fseek(_pfile, 0, SEEK_END);
	l_iEnd = ftell(_pfile);
	fseek(_pfile, l_iSavPos, SEEK_SET);

	return l_iEnd;
}

void ConvertEndian(void* input, void* output, int input_length);

inline u16 be16(u16 x)
{
    return (x>>8) |
        (x<<8);
}

inline u32 be32(u32 x)
{
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

// __int64 for MSVC, "long long" for gcc
inline u64 be64(u64 x)
{
    u64 out;
    ConvertEndian(&x, &out, 8);
    return out;

    /*u64 out;
    unsigned char temp, *ptr = (unsigned char*)&out;
    temp = ptr[7];
    ptr[7] = ptr[0];
    ptr[0] = temp;

    temp = ptr[6];
    ptr[6] = ptr[1];
    ptr[1] = temp;

    temp = ptr[5];
    ptr[5] = ptr[2];
    ptr[2] = temp;

    temp = ptr[4];
    ptr[4] = ptr[3];
    ptr[3] = temp;

    return out;*/

    /*return (x>>56) |
        ((x<<40) & (u64)0x00FF000000000000LL) |
        ((x<<24) & (u64)0x0000FF0000000000LL) |
        ((x<<8)  & (u64)0x000000FF00000000LL) |
        ((x>>8)  & (u64)0x00000000FF000000LL) |
        ((x>>24) & (u64)0x0000000000FF0000LL) |
        ((x>>40) & (u64)0x000000000000FF00LL) |
        (x<<56);*/
}

void ConvertEndian(void* input, void* output, int input_length)
{
     //if(machine_endian==ENDIAN_BIG)return;

     unsigned char *in, *out;
     in=(unsigned char*)malloc((size_t)input_length);
     out=(unsigned char*)malloc((size_t)input_length);
     if(in==NULL || out==NULL)
     {
            printf("FATAL ERROR: FAILED TO ALLOCATE MEMORY FOR CONVERTENDIAN.\n");
            system("PAUSE");
            #ifndef NDS
            exit(1);
            #endif
     }

     memset(out,0,(size_t)input_length);
     memset(in,0,(size_t)input_length);
     memcpy(in,input,(size_t)input_length);

     int I=input_length;
     int i;
     for(i=1; i<=input_length; i++)
     {
             out[I-1]=in[i-1];
             I--;

             //if(debug)printf("IN %d %d\n",i,(int)in[i-1]);
     }

     memcpy(output,out,(size_t)input_length);

     free(out);
     free(in);
}

int main(int argc, char **argv)
{
    unsigned char magic[4] = "WC24";
    u32 temp;
    swc24pubkmod keys;
    char str[256];
    char fnstr[256];
    printf("wc24decrypt v1.0 by yellows8\n");
    if(argc<3 || argc>4)
    {
        printf("Decrypt WC24 content, content downloaded with WC24 with the WC24 header.\n");
        printf("Usage:\n");
        printf("wc24decrypt <content.bin> <decrypt.bin> <wc24pubk.mod>\n");
        printf("The wc24pubk.mod can also by a 16 byte key.\n");
        printf("The mod/key filename can be excluded if the content is not encrypted.\n");
        printf("The content.bin filename can be a http(s) URL to download and decrypt.\n");
    }
    else
    {
            FILE *fwc24, *fout, *fkeys = NULL;
            unsigned char *inbuf, *outbuf;
            unsigned int inlen;
            memset(str, 0, 256);
            memset(fnstr, 0, 256);
            if(strncmp(argv[1], "http", 4)==0)
            {
                memset(str, 0, 256);
                strcpy(str, argv[1]);
                int i = strlen(str);
                while(str[i-1]!='/')i--;
                strncpy(fnstr, &str[i], strlen(str) - i);

                if(argv[1][4]!='s')
                {
                    sprintf(str, "wget -N %s", argv[1]);
                    printf("%s\n", str);
                    system(str);
                }
                else
                {
                    sprintf(str, "curl -o %s %s", fnstr, argv[1]);
                    printf("%s\n", str);
                    system(str);
                }
            }
            else
            {
                strcpy(fnstr, argv[1]);
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

            if(argc==4)
            {
                fkeys = fopen(argv[3], "rb");
                if(fkeys==NULL)
                {
                    fclose(fwc24);
                    fclose(fout);
                    printf("Failed to open %s\n", argv[3]);
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
            temp = be32(temp);
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
                if(argc!=4)
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
    }
    return 0;
}
