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
#include <sys/iosupport.h>
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

s32 vff_fd = 0;
vff_header *vff_hdr;
s16 *vff_fat;
fat_dirent *vff_rootdir;
u32 vff_fatsize;
u32 vff_datastart;

u8 MBLFN[0x20] = {0x41, 0x6D, 0x00, 0x62, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x94, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

FATFS vff_filesystems[_DRIVES];
//extern FILE *disk_vff_handles[_DRIVES];
extern s32 disk_vff_handles[_DRIVES];
int vff_totalmountedfs = 0;

/*const devoptab_t dotab_vff = {
	"vff",
	sizeof (FILE_STRUCT),
	_FAT_open_r,
	_FAT_close_r,
	_FAT_write_r,
	_FAT_read_r,
	_FAT_seek_r,
	_FAT_fstat_r,
	_FAT_stat_r,
	_FAT_link_r,
	_FAT_unlink_r,
	_FAT_chdir_r,
	_FAT_rename_r,
	_FAT_mkdir_r,
	sizeof (DIR_STATE_STRUCT),
	_FAT_diropen_r,
	_FAT_dirreset_r,
	_FAT_dirnext_r,
	_FAT_dirclose_r,
	_FAT_statvfs_r,
	_FAT_ftruncate_r,
	_FAT_fsync_r,
	NULL,
	NULL,
	NULL
};*/

s32 VFF_ReadCluster(u32 cluster, void* buffer);

#ifdef HW_RVL
u32 le32toh(u32 x);
u16 le16toh(u16 x);
u32 htole32(u32 x);
u16 htole16(u16 x);
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
	char isfspath[256];
	s32 retval;
	
	memset(isfspath, 0, 256);
	/*#ifdef HW_RVL
	strncpy(isfspath, "isfs:", 255);
	#endif*/
	strncat(isfspath, path, 255);

	printf("Mounting %s\n", isfspath);
	//disk_vff_handles[vff_totalmountedfs] = fopen(isfspath, "r+");
	disk_vff_handles[vff_totalmountedfs] = ISFS_Open(isfspath, ISFS_OPEN_RW);
	//if(disk_vff_handles[vff_totalmountedfs]==NULL)return -1;
	if(disk_vff_handles[vff_totalmountedfs]<0)return disk_vff_handles[vff_totalmountedfs];
	retval = (s32)f_mount((BYTE)vff_totalmountedfs, &vff_filesystems[vff_totalmountedfs]);
	if(retval!=0)return retval;
	vff_totalmountedfs++;
	return 0;
}

s32 VFF_Unmount()
{
	vff_totalmountedfs--;
	//fclose(disk_vff_handles[vff_totalmountedfs]);
	ISFS_Close(disk_vff_handles[vff_totalmountedfs]);
	return 0;
}

FIL *VFF_Open(char *path)
{
	s32 retval;
	FIL *f = (FIL*)malloc(sizeof(FIL));
	TCHAR lfnpath[128];
	memset(f, 0, sizeof(FIL));
	memset(lfnpath, 0, 256);
	for(retval=0; retval<strlen(path); retval++)lfnpath[retval] = path[retval];
	retval = (s32)f_open(f, lfnpath, FA_READ | FA_WRITE);
	if(retval!=0)
	{
		printf("f_open returned %d\n", retval);
		free(f);
		return NULL;
	}
	return f;
}

void VFF_Close(FIL *ctx)
{
	if(ctx==NULL)return;
	f_close(ctx);
	free(ctx);
}

s32 VFF_Read(FIL *ctx, u8 *buffer, u32 length)
{
	UINT readbytes = 0;
	f_read(ctx, buffer, length, &readbytes);
	return readbytes;
}

s32 VFF_Write(FIL *ctx, u8 *buffer, u32 length)
{
	UINT writtenbytes = 0;
	f_write(ctx, buffer, length, &writtenbytes);
	return writtenbytes;
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

u32 htole32(u32 x)
{
	return ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

u16 htole16(u16 x)
{
	return ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8);
}
#endif

