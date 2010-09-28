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
#include <string.h>
#include <time.h>
#include <malloc.h>
#ifdef HW_RVL
#include <gccore.h>
#else
#include <endian.h>
#endif
#include <sys/stat.h>
#include "diskio.h"
#include "ffconf.h"
#include "vff.h"

/*#ifndef HW_RVL
FILE *disk_vff_handles[_DRIVES];
#else*/
void* disk_vff_handles[_DRIVES];
//#endif
unsigned int vff_types[_DRIVES];
unsigned int vff_filesizes[_DRIVES];
unsigned int vff_fatsizes[_DRIVES];
unsigned int vff_fat_types[_DRIVES];
extern int vff_totalmountedfs;

u8 diskio_buffer[0x200] __attribute__((aligned(32)));

#ifdef HW_RVL
unsigned int htole16(unsigned int x);
unsigned short htole32(unsigned short x);
#endif

DRESULT diskio_generatefatsector(BYTE drv, DWORD sector, BYTE *buff);//Generate boot sector etc.
DRESULT diskio_generatebootsector(BYTE drv, BYTE *buff);
DRESULT diskio_generatefsinfosector(BYTE drv, BYTE *buff);

void diskio_storele16(BYTE *buff, unsigned short x);
void diskio_storele32(BYTE *buff, unsigned int x);

int GetFileLength(FILE *f)
{
	int savepos, endpos;

	savepos = ftell(f);
	fseek(f, 0, SEEK_END);
	endpos = ftell(f);
	fseek(f, savepos, SEEK_SET);

	return endpos;
}


DSTATUS disk_initialize(BYTE drv)
{
	s32 retval = 0;
	#ifdef HW_RVL
	fstats *stats = (fstats*)memalign(32, sizeof(fstats));
	#endif	
	struct stat filestats;
	if((int)drv >= vff_totalmountedfs)return STA_NOINIT;
	#ifdef HW_RVL
	if(vff_types[(int)drv]==0)retval = ISFS_GetFileStats((s32)disk_vff_handles[(int)drv], stats);
	#endif
	if(vff_types[(int)drv]==1)
	{
		retval = fstat(fileno((FILE*)disk_vff_handles[(int)drv]), &filestats);
	}
	if(retval<0)
	{
		printf("getfilestats returned %d\n", retval);
		return STA_NOINIT;
	}
	#ifdef HW_RVL
	if(vff_types[(int)drv]==0)vff_filesizes[(int)drv] = stats->file_length;
	free(stats);
	#endif
	if(vff_types[(int)drv]==1)vff_filesizes[(int)drv] = filestats.st_size;
	//vff_filesizes[(int)drv] = (unsigned int)GetFileLength(disk_vff_handles[(int)drv]);
	vff_fatsizes[(int)drv] = VFF_GetFATSize(vff_filesizes[(int)drv]);
	vff_fat_types[(int)drv] = VFF_GetFATType(vff_filesizes[(int)drv]);
	printf("disk_init ok: filesize %x fatsize %x fat_type %d\n", vff_filesizes[(int)drv], vff_fatsizes[(int)drv], vff_fat_types[(int)drv]);

	return 0;
}

DSTATUS disk_status (BYTE drv)
{
	if((int)drv < vff_totalmountedfs)
	{
		return 0;
	}
	return STA_NODISK;
}

DRESULT disk_read(BYTE drv,BYTE *buff, DWORD sector, BYTE count)
{
	int retval = 0;
	printf("disk read sector %x num %x\n", (unsigned int)sector, (unsigned int)count);
	if(sector==0)return diskio_generatefatsector(drv, sector, buff);
	if(sector==1 && vff_fat_types[(int)drv]==32)return diskio_generatefatsector(drv, sector, buff);
	sector--;
	if((sector>0 && sector<31) && vff_fat_types[(int)drv]==32)
	{
		memset(buff, 0, count*0x200);
		return 0;
	}
	if(sector>=31 && vff_fat_types[(int)drv]==32)sector-=31;
	//sector-=2;

	#ifdef HW_RVL	
	if(vff_types[(int)drv]==0)
	{
		retval = ISFS_Seek((s32)disk_vff_handles[(int)drv], 0x20 + (sector*0x200), SEEK_SET);	
		if(retval<0)
		{
			printf("seek fail\n");
			return RES_PARERR;
		}
	}
	#endif
	if(vff_types[(int)drv]==1)
	{
		retval = fseek((FILE*)disk_vff_handles[(int)drv], 0x20 + (sector*0x200), SEEK_SET);
		if(retval<0)return RES_PARERR;
	}

	if(vff_types[(int)drv]==1)
	{
		retval = fread(buff, 0x200, count, (FILE*)disk_vff_handles[(int)drv]);
		if(retval!=(0x200 * count))return RES_PARERR;
	}
	else
	{
		while(count>0)
		{
			#ifdef HW_RVL
			retval = ISFS_Read((s32)disk_vff_handles[(int)drv], diskio_buffer, 0x200);
			#endif
			if(retval!=0x200)
			{
				printf("read only %x bytes, wanted %x bytes\n", retval, 0x200);
				return RES_PARERR;
			}		
			memcpy(buff, diskio_buffer, 0x200);
			buff+=0x200;
			count--;
		}
	}
	return 0;
}

#if _READONLY == 0
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
	int retval = 0;
	printf("disk write sector %x num %x\n", (unsigned int)sector, (unsigned int)count);
	if(sector==0)return 0;
	if(sector==1 && vff_fat_types[(int)drv]==32)return 0;
	sector--;
	if((sector>=0 && sector<31) && vff_fat_types[(int)drv]==32)
	{
		return 0;
	}
	if(sector>=31 && vff_fat_types[(int)drv]==32)sector-=31;
	//sector-=2;

	#ifdef HW_RVL	
	if(vff_types[(int)drv]==0)
	{
		retval = ISFS_Seek((s32)disk_vff_handles[(int)drv], 0x20 + (sector*0x200), SEEK_SET);	
		if(retval<0)
		{
			printf("seek fail\n");
			return RES_PARERR;
		}
	}
	#endif
	if(vff_types[(int)drv]==1)
	{
		retval = fseek((FILE*)disk_vff_handles[(int)drv], 0x20 + (sector*0x200), SEEK_SET);
		if(retval<0)return RES_PARERR;
	}

	if(vff_types[(int)drv]==1)
	{
		retval = fwrite(buff, 0x200, count, (FILE*)disk_vff_handles[(int)drv]);
		if(retval!=(0x200 * count))return RES_PARERR;
	}
	else
	{
		while(count>0)
		{
			memcpy(diskio_buffer, buff, 0x200);
			#ifdef HW_RVL
			retval = ISFS_Write((s32)disk_vff_handles[(int)drv], diskio_buffer, 0x200);
			#endif
			if(retval!=0x200)
			{
				printf("wrote only %x bytes, wanted %x bytes\n", retval, 0x200);
				return RES_PARERR;
			}		
			buff+=0x200;
			count--;
		}
	}
	return 0;
}
#endif

DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
	printf("disk ioctl %x\n", (unsigned int)ctrl);
	return RES_PARERR;
}

DRESULT diskio_generatefatsector(BYTE drv, DWORD sector, BYTE *buff)//Generate boot sector etc.
{
	memset(buff, 0, 0x200);
	buff[0x1fe] = 0x55;
	buff[0x1ff] = 0xaa;
	if(sector==0)return diskio_generatebootsector(drv, buff);
	if(sector==1)return diskio_generatefsinfosector(drv, buff);
	return 0;
}

DRESULT diskio_generatebootsector(BYTE drv, BYTE *buff)
{
	int temp;
	diskio_storele16(&buff[0x0b], 0x200);//Bytes per sector.
	buff[0x0d] = 1;//Sectors per cluster.
	if(vff_fat_types[(int)drv]!=32)
	{
		diskio_storele16(&buff[0x0e], 1);//Reserved sectors.
	}
	else
	{
		diskio_storele16(&buff[0x0e], 32);
	}
	buff[0x10] = 2;//Number of FATs.
	if(vff_fat_types[(int)drv]!=32)diskio_storele16(&buff[0x11], 0x80);//Number of root directory entries.
	if(vff_fat_types[(int)drv]!=32)
	{
		diskio_storele16(&buff[0x13], vff_filesizes[(int)drv] / 0x200);//Number of sectors.
	}
	else
	{
		diskio_storele16(&buff[0x20], vff_filesizes[(int)drv] / 0x200);
	}
	temp = (vff_fatsizes[(int)drv] / 0x200);
	if(temp==0)temp++;
	diskio_storele16(&buff[0x16], temp);//Number of sectors per FAT.
	snprintf((char*)&buff[0x36], 8, "FAT%d   ", vff_fat_types[(int)drv]);
	if(vff_fat_types[(int)drv]==32)snprintf((char*)&buff[0x52], 8, "FAT32   ");
	return 0;
}

DRESULT diskio_generatefsinfosector(BYTE drv, BYTE *buff)
{
	return 0;
}

void diskio_storele16(BYTE *buff, unsigned short x)
{
	x = htole16(x);
	memcpy(buff, &x, 2);
}

void diskio_storele32(BYTE *buff, unsigned int x)
{
	x = htole32(x);
	memcpy(buff, &x, 4);
}

DWORD get_fattime(void)
{
	DWORD fattime = 0;
	time_t timeval = time(NULL);
	struct tm *curtime = gmtime(&timeval);
	fattime = curtime->tm_sec / 2;
	fattime |= curtime->tm_min << 5;
	fattime |= curtime->tm_hour << 11;
	fattime |= curtime->tm_mday << 16;
	fattime |= (curtime->tm_mon + 1) << 21;
	fattime |= (curtime->tm_year - 80) << 25;
	return fattime;
}


