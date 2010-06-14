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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#ifdef HW_RVL
#include <gccore.h>
#else
#include <endian.h>
#endif

#include "vff.h"
#include "wc24.h"
#include "ffconf.h"

//These two defines are from bushing's FAT size code: http://wiibrew.org/wiki/VFF
#define ALIGN_FORWARD(x,align) \
        ((typeof(x))((((u32)(x)) + (align) - 1) & (~((align)-1))))
#define CLUSTER_SIZE 0x200

//The proper way to implement VFF code is to port and modify a FAT implementation.
//The directory entries and FATs are little-endian, but the VFF header is big-endian.

s32 vff_fd = 0;
vff_header *vff_hdr;
s16 *vff_fat;
fat_dirent *vff_rootdir;
u32 vff_fatsize;
u32 vff_datastart;

u8 MBLFN[0x20] = {0x41, 0x6D, 0x00, 0x62, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x94, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

s32 VFF_ReadCluster(u32 cluster, void* buffer);

#ifdef HW_RVL
u32 le32toh(u32 x);
u16 le16toh(u16 x);
#endif

u32 VFF_GetFATSize(u32 filesize)
{
    u32 num_clusters = filesize / CLUSTER_SIZE;//This code is written by bushing: http://wiibrew.org/wiki/VFF
    u32 fat_bits = 32;
    if (num_clusters < 0xFFF5) fat_bits = 16;
    if (num_clusters < 0xFF5)  fat_bits = 12;
 
    return ALIGN_FORWARD(num_clusters * fat_bits / 8, CLUSTER_SIZE);
}

u32 VFF_GetFATType(u32 filesize)//This is based on the above function VFF_GetFATSize by bushing.
{
    u32 num_clusters = filesize / CLUSTER_SIZE;
    u32 fat_bits = 32;
    if (num_clusters < 0xFFF5) fat_bits = 16;
    if (num_clusters < 0xFF5)  fat_bits = 12;
 
    return fat_bits;
}

s32 VFF_CreateVFF(char *path, u32 filesize)
{
	s32 retval, fd;
	u32 fatsz = VFF_GetFATSize(filesize);
	if(path==NULL)return 0;
	vff_header *header = (vff_header*)memalign(32, sizeof(vff_header));	
	s16 *fat = (s16*)memalign(32, fatsz);
	u8 *buf;
	header->magic = VFF_MAGIC;
	header->byteorder = 0xfeff;
	header->unk6 = 0x100;
	header->filesize = filesize;
	header->header_size = 0x20;
	memset(header->reserved, 0, 0x12);
	int i;
	
	retval = ISFS_CreateFile(path, 0, 3, 3, 3);
	if(retval<0)
	{
		if(retval==-105)
		{
			ISFS_Delete(path);
			retval = ISFS_CreateFile(path, 0, 3, 3, 3);
		}
		if(retval<0)
		{
			printf("Failed to create file\n");
			free(header);
			free(fat);
			return retval;
		}
	}
	retval = ISFS_Open(path, ISFS_OPEN_RW);
	if(retval<0)
	{
		printf("Failed to open file\n");
		free(header);
		free(fat);
		return retval;
	}
	fd = retval;

	retval = ISFS_Write(fd, header, sizeof(vff_header));
	free(header);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(fat);
		return retval;
	}
	
	free(header);
	memset(fat, 0, fatsz);
	fat[0] = 0xf0ff;
	fat[1] = 0xffff;
	fat[2] = 0x0f00;
	//fat[2] = 0xffff;
	retval = ISFS_Seek(fd, 0x20, SEEK_SET);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(fat);
		return retval;
	}
	
	retval = ISFS_Write(fd, fat, fatsz);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(fat);
		return retval;
	}
	retval = ISFS_Write(fd, fat, fatsz);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(fat);
		return retval;
	}
	free(fat);
	
	buf = memalign(32, 0x1000);
	memset(buf, 0, 0x1000);
	memcpy(buf, MBLFN, 0x20);
	strncpy((char*)&buf[0x20], "MB", 11);
	buf[0x20 + 0xb] = 0x10;
	buf[0x20 + 0x1a] = 0x02;

	retval = ISFS_Write(fd, buf, 0x1000);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(buf);
	}

	memset(buf, 0, 0x200);
	strncpy((char*)&buf[0x0 + 0], ".", 11);
	buf[0xb] = 0x10;
	buf[0x1a] = 0x02;
	strncpy((char*)&buf[0x20], "..", 11);
	buf[0x20 + 0xb] = 0x10;

	retval = ISFS_Write(fd, buf, 0x200);
	if(retval<0)
	{
		ISFS_Close(fd);
		free(buf);
	}

	memset(buf, 0, 0x20);
	i = 0x20 + (fatsz*2) + 0x1000 + 0x200;
	while(i<filesize)
	{
		retval = ISFS_Write(fd, buf, 0x20);
		if(retval<0)
		{
			printf("Write fail: %d\n", retval);
			ISFS_Close(fd);
			free(buf);
			return -1;
		}
		i+=0x20;
	}

	free(buf);
	retval = ISFS_Close(fd);
	if(retval<0)
	{
		return retval;
	}
	return 0;
}

s32 VFF_Mount(char *path)
{
	s32 retval;
	if(vff_fd!=0 || path==NULL)return 0;
	retval = ISFS_Open(path, ISFS_OPEN_RW);
	if(retval<0)return retval;
	vff_fd = retval;

	vff_hdr = (vff_header*)memalign(32, sizeof(vff_header));
	if(vff_hdr==NULL)
	{
		ISFS_Close(vff_fd);
		vff_fd = 0;
		return -1;
	}
	memset(vff_hdr, 0, sizeof(vff_header));
	ISFS_Read(vff_fd, vff_hdr, sizeof(vff_header));
	vff_fatsize = VFF_GetFATSize(vff_hdr->filesize);
	vff_datastart = vff_hdr->header_size + (vff_fatsize*2) + 0x1000;

	vff_fat = (s16*)memalign(32, vff_fatsize);
	if(vff_fat==NULL)
	{
		free(vff_hdr);
		vff_hdr = NULL;
		ISFS_Close(vff_fd);
		vff_fd = 0;
		return -1;
	}
	ISFS_Read(vff_fd, vff_fat, vff_fatsize);
	ISFS_Seek(vff_fd, vff_datastart - 0x1000, SEEK_SET);
	
	vff_rootdir = (fat_dirent*)memalign(32, 0x1000);
	if(vff_rootdir==NULL)
	{
		free(vff_hdr);
		free(vff_fat);
		vff_hdr = NULL;
		vff_fat = NULL;
		ISFS_Close(vff_fd);
		vff_fd = 0;
		return -1;
	}
	ISFS_Read(vff_fd, vff_rootdir, 0x1000);
	return 0;
}

s32 VFF_Unmount()
{
	if(vff_fd==0)return 0;
	if(vff_hdr)free(vff_hdr);
	if(vff_fat)free(vff_fat);
	if(vff_rootdir)free(vff_rootdir);
	return ISFS_Close(vff_fd);
}

fat_filectx *VFF_Open(char *path)
{
	int isroot = 1;
	int curlen = 0;
	int pathi = 0;
	int i = 0, clus = 0;
	int found = 0;
	int found_dir = 0;
	int lastpathi = 0;
	u32 cluster = 3;
	fat_dirent *ent = vff_rootdir, *tempent;
	fat_filectx *ctx = NULL;
	if(path==NULL)return 0;
	curlen = strlen(path);
	
	if(strstr(path, "/"))curlen = (int)strstr(&path[pathi], "/") - (int)&path[pathi];

	while(pathi<strlen(path))
	{
		for(clus=0; clus<(isroot*7)+1; clus++)
		{
			for(i=0; i<0x10; i++)
			{
				if(ent[i].name[0]==0xe5)continue;
				if(ent[i].name[0]==0)
				{
					pathi = strlen(path);
					break;
				}
				if(strncmp(ent[i].name, &path[pathi], curlen)==0)
				{
					pathi+= curlen;
					if(ent[i].attr & 0x10)
					{
						found_dir = 1;
						pathi++;
					}
					if(!(ent[i].attr & 0x10))found = 1;
					break;
				}
			}
			if(found)break;
			if(found_dir)break;
			ent+=0x10;
		}
		if(found || found_dir)cluster = le16toh(ent[i].clusterlow) | (le16toh(ent[i].clusterhigh) << 16);
		if(found_dir)
		{
			if(isroot)
			{
				ent = (fat_dirent*)memalign(32, 0x200);
				isroot = 0;
			}
			memset(ent, 0, 0x200);
			VFF_ReadCluster(cluster, ent);
		}
		if(found)break;
		if(!found && pathi==lastpathi)break;
		lastpathi = pathi;
	}

	if(!found || ent[i].attr & 0x10)
	{
		if(!isroot)free(ent);
		return NULL;//Don't return ctx structures for directories.
	}

	ctx = (fat_filectx*)malloc(sizeof(fat_filectx));
	if(ctx)
	{
		tempent = (fat_dirent*)malloc(sizeof(fat_dirent));
		memcpy(tempent, &ent[i], sizeof(fat_dirent));
		memset(ctx, 0, sizeof(fat_filectx));
		ctx->cluster = cluster;
		ctx->filesize = le32toh(ent[i].filesize);
		ctx->ent = tempent;
	}
	if(!isroot)free(ent);

	return ctx;
}

void VFF_Close(fat_filectx *ctx)
{
	if(ctx==NULL)return;
	free(ctx->ent);
	free(ctx);
}

s32 VFF_Read(fat_filectx *ctx, u8 *buffer, u32 length)
{
	u8 *buf = (u8*)memalign(32, 0x200);
	int len = length;
	int i = 0;
	s32 retval = 0;
	if(len>0x200)len = 0x200;
	while(length>0)
	{
		retval = VFF_ReadCluster(ctx->cluster + i, buf);
		length-=len;
		memcpy(buffer, buf, len);
		buffer+=len;
		i++;
		if(length<0x200)len = length;
		if(retval<0)break;
	}
	free(buf);
	return retval;
}

s32 VFF_ReadCluster(u32 cluster, void* buffer)
{
	ISFS_Seek(vff_fd, vff_datastart + ((cluster - 2) * 0x200), SEEK_SET);
	return ISFS_Read(vff_fd, buffer, 0x200);
}

#ifdef HW_RVL
u32 le32toh(u32 x)
{
	return ((x & 0xff000000) >> 24) | ((x & 0x00ff0000) >> 8) | ((x & 0x0000ff00) << 8) | ((x & 0x000000ff) << 24);
}

u16 le16toh(u16 x)
{
	return ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8);
}
#endif

