/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

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

#include "..\SDK\include\wmb_asm_sdk_client.h"

unsigned char *ConvertBinBuff(unsigned char *data, int length);

void ConvertBin(char *filename)
{
    printf("Converting %s...\n",filename);
    
    FILE *fbin, *fnds;
    unsigned char *input_buffer, *output_buffer;
    int length;
    char str[256];
    memset(str, 0, 256);
    
    fbin = fopen(filename, "rb");
    
    if(fbin==NULL)
    {
        printf("Failed to open file %s\n",filename);
        return;
    }
    
    strncpy(str, filename, strlen(filename) - 4);
    strcat(str, ".nds");
    
    length = GetFileLength(fbin);//Nintendo Channel .bin files for demos, have a special header 0x1C8 bytes long. Remove that, and you got an .nds.
    input_buffer = (unsigned char*)malloc(length);
    memset(input_buffer, 0, length);
    fread(input_buffer, 1, length, fbin);
    
    fclose(fbin);
    
    output_buffer = ConvertBinBuff(input_buffer, length);
    
    fnds = fopen(str, "wb");
    if(fnds==NULL)
    {
        printf("Failed to open file %s\n");
        return;
    }
    
    fwrite(output_buffer, 1, length, fnds);
    
    fclose(fnds);
    
    printf("Successfully converted the file to %s!\n",str);
}

unsigned char *ConvertBinBuff(unsigned char *data, int length)
{
    unsigned char *nds = (unsigned char*)malloc(length - 0x1C8);
    memset(nds, 0, length - 0x1C8);
    
    memcpy(nds, &data[0x1C8], length - 0x1C8);
    
    return nds;
}
