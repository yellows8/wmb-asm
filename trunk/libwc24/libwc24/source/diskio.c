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
#include "diskio.h"
#include "ffconf.h"
#include "vff.h"

FILE *disk_vff_handles[_DRIVES];
unsigned int vff_filesizes[_DRIVES];
unsigned int vff_fatsizes[_DRIVES];
unsigned int vff_fat_types[_DRIVES];

unsigned int htole16(unsigned int x);
unsigned short htole32(unsigned short x);

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
	if(disk_vff_handles[(int)drv]==NULL)return STA_NOINIT;
	vff_filesizes[(int)drv] = (unsigned int)GetFileLength(disk_vff_handles[(int)drv]);
	vff_fatsizes[(int)drv] = VFF_GetFATSize(vff_filesizes[(int)drv]);
	vff_fat_types[(int)drv] = VFF_GetFATType(vff_filesizes[(int)drv]);

	return 0;
}

DSTATUS disk_status (BYTE drv)
{
	if(disk_vff_handles[(int)drv]!=NULL)return 0;
	return STA_NODISK;
}

DRESULT disk_read(BYTE drv,BYTE *buff, DWORD sector, BYTE count)
{
	int retval = 0;
	if(sector==0)return diskio_generatefatsector(drv, sector, buff);
	if(sector==1 && vff_fat_types[(int)drv]==32)return diskio_generatefatsector(drv, sector, buff);
	sector--;
	if((sector>0 && sector<31) && vff_fat_types[(int)drv]==32)
	{
		memset(buff, 0, count*0x200);
		return 0;
	}
	if(sector>=31 && vff_fat_types[(int)drv]==32)sector-=31;

	retval = fseek(disk_vff_handles[(int)drv], 0x20 + sector*0x200, SEEK_SET);
	if(retval<0)return RES_PARERR;
	retval = fread(buff, 0x200, count, disk_vff_handles[(int)drv]);
	if(retval!=count*0x200)return RES_PARERR;
	return 0;
}

#if _READONLY == 0
DRESULT disk_write(BYTE drv, const BYTE *buff, DWORD sector, BYTE count)
{
	int retval = 0;
	if(sector==0)return 0;
	if(sector==1 && vff_fat_types[(int)drv]==32)return 0;
	sector--;
	if((sector>0 && sector<31) && vff_fat_types[(int)drv]==32)
	{
		return 0;
	}
	if(sector>=31 && vff_fat_types[(int)drv]==32)sector-=31;

	retval = fseek(disk_vff_handles[(int)drv], 0x20 + sector*0x200, SEEK_SET);
	if(retval<0)return RES_PARERR;
	retval = fwrite(buff, 0x200, count, disk_vff_handles[(int)drv]);
	if(retval!=count*0x200)return RES_PARERR;
	return 0;
}
#endif

DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
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
	diskio_storele16(&buff[0x11], 0x80);//Number of root directory entries.
	if(vff_fat_types[(int)drv]!=32)
	{
		diskio_storele16(&buff[0x13], vff_filesizes[(int)drv] / 0x200);//Number of sectors.
	}
	else
	{
		diskio_storele16(&buff[0x20], vff_filesizes[(int)drv] / 0x200);
	}
	diskio_storele16(&buff[0x16], (vff_fatsizes[(int)drv] / 0x200)+1);
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

