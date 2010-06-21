/*
libwc24 is licensed under the MIT license:
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

#ifndef _H_VFF
#define _H_VFF

#include "types.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define VFF_MAGIC 0x56464620

//The directory entries are little-endian, but the VFF header and FATs are big-endian.
typedef struct _vff_header
{
	u32 magic;//"VFF " 0x56464620
	u16 byteorder;//0xfeff
	u16 unk6;//0x100
	u32 filesize;//Size of whole VFF.
	u16 header_size;//Size of whole header.
	u8 reserved[0x12];
} __attribute__((packed)) vff_header;

typedef struct _fat_dirent
{
	char name[11];
	u8 attr;
	u8 reserved;
	u8 createtime_seconds;
	u16 createtime;
	u16 createdate;
	u16 accessdate;
	u16 clusterhigh;
	u16 modified_date;
	u16 modified_time;
	u16 clusterlow;
	u32 filesize;
} __attribute__((packed)) fat_dirent;

s32 VFF_CreateVFF(char *path, u32 filesize);//Creates VFF at path with ISFS. Returns zero on success, ISFS error otherwise.
s32 VFF_Mount(char *path);//Opens a VFF, only one VFF can be open at a time.
s32 VFF_Unmount();
u32 VFF_GetFATSize(u32 filesize);
u32 VFF_GetFATType(u32 filesize);

#ifdef __cplusplus
   }
#endif

#endif

