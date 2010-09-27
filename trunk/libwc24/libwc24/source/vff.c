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
#include <errno.h>
#include <fcntl.h>

#ifdef HW_RVL
#include <sys/iosupport.h>
#include <gccore.h>
#else
#include <endian.h>
#include <time.h>
#endif

#include "vff.h"
#include "wc24.h"
#include "ffconf.h"
#include "ff.h"

//These two defines are from bushing's FAT size code: http://wiibrew.org/wiki/VFF
#define ALIGN_FORWARD(x,align) \
        ((typeof(x))((((u32)(x)) + (align) - 1) & (~((align)-1))))
#define CLUSTER_SIZE 0x200

#define VFF_MAXFDS 32

s32 vff_fd = 0;
vff_header *vff_hdr;
s16 *vff_fat;
fat_dirent *vff_rootdir;
u32 vff_fatsize;
u32 vff_datastart;

u8 MBLFN[0x20] = {0x41, 0x6D, 0x00, 0x62, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00, 0x94, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};

FATFS vff_filesystems[_DRIVES];
//extern FILE *disk_vff_handles[_DRIVES];
extern void* disk_vff_handles[_DRIVES];
extern unsigned int vff_types[_DRIVES];
int vff_totalmountedfs = 0;

FIL *vffoptab_fds[VFF_MAXFDS];
#ifdef HW_RVL
devoptab_t *vffoptab_dev[VFF_MAXFDS];
#endif
char vffoptab_fd_paths[VFF_MAXFDS][256];
char vffoptab_devmntname[VFF_MAXFDS][256];
int vffoptab_total_fds = 0;

int _VFF_open_r (struct _reent *r, void *fileStruct, const char *path, int flags, int mode);
int _VFF_close_r (struct _reent *r, int fd);
ssize_t _VFF_read_r (struct _reent *r, int fd, char *ptr, size_t len);
ssize_t _VFF_write_r (struct _reent *r, int fd, const char *ptr, size_t len);
off_t _VFF_seek_r (struct _reent *r, int fd, off_t pos, int dir);
int _VFF_fstat_r (struct _reent *r, int fd, struct stat *st);
int _VFF_ftruncate_r (struct _reent *r, int fd, off_t len);
int _VFF_fsync_r (struct _reent *r, int fd);
int _VFF_stat_r (struct _reent *r, const char *path, struct stat *st);
int _VFF_unlink_r (struct _reent *r, const char *path);
int _VFF_chdir_r (struct _reent *r, const char *path);
int _VFF_rename_r (struct _reent *r, const char *oldName, const char *newName);
int _VFF_mkdir_r (struct _reent *r, const char *path, int mode);
int _VFF_statvfs_r (struct _reent *r, const char *path, struct statvfs *buf);
DIR_ITER* _VFF_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path);
int _VFF_dirreset_r (struct _reent *r, DIR_ITER *dirState);
int _VFF_dirnext_r (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat);
int _VFF_dirclose_r (struct _reent *r, DIR_ITER *dirState);

#ifdef HW_RVL
const devoptab_t dotab_vff = {
	"vff",
	sizeof(FIL),
	&_VFF_open_r,
	&_VFF_close_r,
	&_VFF_write_r,
	&_VFF_read_r,
	&_VFF_seek_r,
	&_VFF_fstat_r,
	&_VFF_stat_r,
	NULL,
	&_VFF_unlink_r,
	&_VFF_chdir_r,
	&_VFF_rename_r,
	&_VFF_mkdir_r,
	sizeof(DIR),
	&_VFF_diropen_r,
	&_VFF_dirreset_r,
	&_VFF_dirnext_r,
	&_VFF_dirclose_r,
	&_VFF_statvfs_r,
	&_VFF_ftruncate_r,
	&_VFF_fsync_r,
	NULL,
	NULL,
	NULL
};
#endif

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

#ifdef HW_RVL
s32 VFF_CreateVFF(char *path, u32 filesize, int delvff)
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
		if(retval==-105 && delvff)
		{
			ISFS_Delete(path);
			retval = ISFS_CreateFile(path, 0, 3, 3, 3);
		}
		if(retval<0)
		{
			free(header);
			free(fat);
			return retval;
		}
	}
	retval = ISFS_Open(path, ISFS_OPEN_RW);
	if(retval<0)
	{
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
			ISFS_Close(fd);
			free(buf);
			return retval;
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
#endif

s32 VFF_Mount(char *path, char *mntname)
{
	int i;
	s32 retval;
	unsigned int type = 0;
	char *mntnamebuf;
	#ifdef HW_RVL
	devoptab_t *dotab;
	#endif

	mntnamebuf = (char*)malloc(256);
	memset(mntnamebuf, 0, 256);
	if(mntname==NULL)
	{
		for(i=strlen(path)-1; i>0; i--)
		{
			if(path[i]=='/')
			{
				i++;
				break;
			}
		}
	}
	if(mntname==NULL)strncpy(mntnamebuf, &path[i], 255);
	if(mntname)strncpy(mntnamebuf, mntname, 255);

	printf("Mounting %s\n", path);
	#ifdef HW_RVL
	if(strchr(path, ':'))type = 1;
	#else
	type = 1;
	#endif
	#ifdef HW_RVL
	if(type==0)
	{
		disk_vff_handles[vff_totalmountedfs] = (void*)ISFS_Open(path, ISFS_OPEN_RW);
		if((s32)disk_vff_handles[vff_totalmountedfs]<0)return (s32)disk_vff_handles[vff_totalmountedfs];
	}
	#endif
	if(type==1)
	{
		disk_vff_handles[vff_totalmountedfs] = fopen(path, "r+");
		if((FILE*)disk_vff_handles[vff_totalmountedfs]==NULL)return -1;
	}
	vff_types[vff_totalmountedfs] = type;
	retval = (s32)fvff_mount((BYTE)vff_totalmountedfs, &vff_filesystems[vff_totalmountedfs]);
	if(retval!=0)return retval;
	#ifdef HW_RVL
	dotab = (devoptab_t*)malloc(sizeof(devoptab_t));
	memset(dotab, 0, sizeof(devoptab_t));
	memcpy(dotab, &dotab_vff, sizeof(devoptab_t));
	dotab->name = mntnamebuf;
	#endif
	memset(vffoptab_devmntname[vff_totalmountedfs], 0, 256);
	strncpy(vffoptab_devmntname[vff_totalmountedfs], mntnamebuf, 255);
	#ifdef HW_RVL
	AddDevice(dotab);
	vffoptab_dev[vff_totalmountedfs] = dotab;
	#endif
	vff_totalmountedfs++;

	#ifdef HW_RVL
	if(vff_totalmountedfs==1)
	{
		memset(vffoptab_fds, 0, VFF_MAXFDS*4);
	}
	#endif
	return 0;
}

s32 VFF_Unmount(char *mntname)
{
	int i;
	unsigned int type;
	vff_totalmountedfs--;
	for(i=0; i<=vff_totalmountedfs; i++)
	{
		if(strncmp(mntname, vffoptab_devmntname[i], 255)==0)break;
	}
	type = vff_types[i];
	#ifdef HW_RVL	
	if(type==0)ISFS_Close((s32)disk_vff_handles[i]);
	#endif
	if(type==1)fclose(disk_vff_handles[i]);
	#ifdef HW_RVL
	RemoveDevice(mntname);
	free((void*)vffoptab_dev[i]->name);
	free(vffoptab_dev[i]);
	#endif
	return 0;
}

int VFF_ConvertFFError(FRESULT error)
{
	switch(error)
	{
		case FR_OK:
			return 0;
		break;

		case FR_DISK_ERR:
		case FR_INT_ERR:
		case FR_NOT_READY:
		case FR_INVALID_DRIVE:
		case FR_NOT_ENABLED:
		case FR_NO_FILESYSTEM:
		case FR_MKFS_ABORTED:
			return ENXIO;
		break;

		case FR_NO_FILE:
		case FR_NO_PATH:
		case FR_INVALID_NAME:
		case FR_INVALID_OBJECT:
			return ENOENT;
		break;

		case FR_DENIED:
		case FR_LOCKED:
			return EACCES;
		break;

		case FR_EXIST:
			return EEXIST;
		break;

		case FR_WRITE_PROTECTED:
			return EROFS;
		break;

		case FR_TIMEOUT:
			return EWOULDBLOCK;
		break;

		case FR_NOT_ENOUGH_CORE:
			return ENOMEM;
		break;

		case FR_TOO_MANY_OPEN_FILES:
			return EMFILE;
		break;
	}
	return 0;
}

time_t VFF_GetFFLastMod(WORD fdate, WORD ftime)
{
	struct tm date;
	date.tm_sec = (ftime & 0x1f) * 2;
	date.tm_min = (ftime >> 5) & 0x3f;
	date.tm_hour = (ftime >> 11) & 0x3f;
	date.tm_mday = (ftime & 0x1f);
	date.tm_mon = ((ftime >> 5) & 0xf) - 1;
	date.tm_year = ((ftime >> 9) & 0x3f) + 80;
	return mktime(&date);
}

void VFF_ConvertFFInfoToStat(FILINFO *info, struct stat *st)
{
	if(!(info->fattrib & 0x10))st->st_size = info->fsize;
	st->st_blksize = 0x200;
	st->st_blocks = st->st_size / 0x200;
	if(st->st_size % 0x200)st->st_blocks++;
	st->st_atime = VFF_GetFFLastMod(info->ftime, info->fdate);
	st->st_mtime = st->st_atime;
	st->st_ctime = st->st_atime;
}

int _VFF_open_r (struct _reent *r, void *fileStruct, const char *path, int flags, int mode)
{
	int i, fdi, found = 0;
	BYTE _flags = 0;
	FIL *f = (FIL*)fileStruct;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(f, 0, sizeof(FIL));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		r->_errno = ENAMETOOLONG;
		return -1;
	}

	for(fdi=0; fdi<VFF_MAXFDS; fdi++)
	{
		if((int)vffoptab_fds[fdi]==(int)NULL)
		{
			found = 1;
			break;
		}
	}
	if(!found)
	{
		free(lfnpath);
		r->_errno = EMFILE;
		return -1;
	}

	if((flags & 3) == O_RDONLY)_flags |= FA_READ;
	if((flags & 3) == O_WRONLY)_flags |= FA_WRITE;
	if((flags & 3) == O_RDWR)_flags |= FA_READ | FA_WRITE;
	if(flags & O_CREAT)_flags |= FA_CREATE_NEW;
	if(flags & O_EXCL)_flags |= FA_CREATE_ALWAYS;
	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	r->_errno = VFF_ConvertFFError(fvff_open(f, lfnpath, _flags));
	free(lfnpath);
	if(r->_errno!=0)return -1;
	vffoptab_fds[fdi] = f;
	memset(vffoptab_fd_paths[fdi], 0, 256);
	strncpy(vffoptab_fd_paths[fdi], path, 255);
	vffoptab_total_fds++;
	return 0;
}

int _VFF_close_r (struct _reent *r, int fd)
{
	FIL *f;
	int fdi,  found = 0;
	f = (FIL*)fd;
	for(fdi=0; fdi<VFF_MAXFDS; fdi++)
	{
		if((int)vffoptab_fds[fdi]==fd)
		{
			found = 1;
			break;
		}
	}
	if(!found)
	{
		r->_errno = EMFILE;
		return -1;
	}

	r->_errno = VFF_ConvertFFError(fvff_close(f));
	if(r->_errno!=0)return -1;
	vffoptab_fds[fdi] = NULL;
	return 0;
}

ssize_t _VFF_read_r (struct _reent *r, int fd, char *ptr, size_t len)
{
	FIL *f;
	UINT bytes = 0;
	f = (FIL*)fd;
	r->_errno = VFF_ConvertFFError(fvff_read(f, ptr, len, &bytes));
	if(r->_errno!=0)return -1;
	return bytes;
}

ssize_t _VFF_write_r (struct _reent *r, int fd, const char *ptr, size_t len)
{
	FIL *f;
	UINT bytes = 0;
	f = (FIL*)fd;
	r->_errno = VFF_ConvertFFError(fvff_write(f, ptr, len, &bytes));
	if(r->_errno!=0)return -1;
	return bytes;
}

off_t _VFF_seek_r (struct _reent *r, int fd, off_t pos, int dir)
{
	FIL *f;
	int _pos = pos;
	f = (FIL*)fd;
	if(dir==SEEK_CUR)pos+= (int)f->fptr;
	if(dir==SEEK_END)pos+= (int)f->fsize;
	r->_errno = VFF_ConvertFFError(fvff_lseek(f, _pos));
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_fstat_r (struct _reent *r, int fd, struct stat *st)
{
	int i, fdi, found = 0;
	FILINFO info;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	TCHAR *lfnpathinfo = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	memset(lfnpathinfo, 0, 128 * sizeof(TCHAR));
	for(fdi=0; fdi<VFF_MAXFDS; fdi++)
	{
		if((int)vffoptab_fds[fdi]==fd)
		{
			found = 1;
			break;
		}
	}
	if(!found)
	{
		free(lfnpath);
		free(lfnpathinfo);
		r->_errno = EMFILE;
		return -1;
	}

	for(i=0; i<strlen(vffoptab_fd_paths[fdi]); i++)lfnpath[i] = vffoptab_fd_paths[fdi][i];
	info.lfname = lfnpathinfo;
	info.lfsize = 128;	
	r->_errno = VFF_ConvertFFError(fvff_stat(lfnpath, &info));
	free(lfnpath);
	free(lfnpathinfo);
	if(r->_errno!=0)return -1;
	VFF_ConvertFFInfoToStat(&info, st);
	return 0;
}

int _VFF_ftruncate_r (struct _reent *r, int fd, off_t len)
{
	FIL *f;
	unsigned int temp, num;
	f = (FIL*)fd;
	r->_errno = VFF_ConvertFFError(fvff_truncate(f));
	if(r->_errno!=0)return -1;

	temp = 0;
	num = 0;
	while(len>0)
	{
		r->_errno = VFF_ConvertFFError(fvff_write(f, &temp, 1, &num));
		len--;
		if(r->_errno!=0)return -1;
	}

	return 0;
}

int _VFF_fsync_r (struct _reent *r, int fd)
{
	FIL *f;
	f = (FIL*)fd;
	r->_errno = VFF_ConvertFFError(fvff_sync(f));
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_stat_r (struct _reent *r, const char *path, struct stat *st)
{
	int i;
	FILINFO info;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	TCHAR *lfnpathinfo = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	memset(lfnpathinfo, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		free(lfnpath);
		free(lfnpathinfo);
		r->_errno = ENAMETOOLONG;
		return -1;
	}

	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	info.lfname = lfnpathinfo;
	info.lfsize = 128;
	r->_errno = VFF_ConvertFFError(fvff_stat(lfnpath, &info));
	free(lfnpath);
	free(lfnpathinfo);
	if(r->_errno!=0)return -1;
	VFF_ConvertFFInfoToStat(&info, st);
	return 0;
}

int _VFF_unlink_r (struct _reent *r, const char *path)
{
	int i;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		free(lfnpath);
		r->_errno = ENAMETOOLONG;
		return -1;
	}
	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	r->_errno = VFF_ConvertFFError(fvff_unlink(lfnpath));
	free(lfnpath);
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_chdir_r (struct _reent *r, const char *path)
{
	int i;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		free(lfnpath);
		r->_errno = ENAMETOOLONG;
		return -1;
	}
	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	r->_errno = VFF_ConvertFFError(fvff_chdir(lfnpath));
	free(lfnpath);
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_rename_r (struct _reent *r, const char *oldName, const char *newName)
{
	int i;
	TCHAR *lfnpathold = (TCHAR*)malloc(128 * sizeof(TCHAR));
	TCHAR *lfnpathnew = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpathold, 0, 128 * sizeof(TCHAR));
	memset(lfnpathnew, 0, 128 * sizeof(TCHAR));
	if(strchr(oldName, ':'))oldName = strchr(oldName, ':')+1;
	if(strchr(newName, ':'))newName = strchr(newName, ':')+1;
	if(strlen(oldName)>127 || strlen(newName)>127)
	{
		free(lfnpathold);
		free(lfnpathnew);
		r->_errno = ENAMETOOLONG;
		return -1;
	}
	for(i=0; i<strlen(oldName); i++)lfnpathold[i] = oldName[i];
	for(i=0; i<strlen(newName); i++)lfnpathnew[i] = newName[i];
	r->_errno = VFF_ConvertFFError(fvff_rename(lfnpathold, lfnpathnew));
	free(lfnpathold);
	free(lfnpathnew);
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_mkdir_r (struct _reent *r, const char *path, int mode)
{
	int i;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		free(lfnpath);
		r->_errno = ENAMETOOLONG;
		return -1;
	}
	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	r->_errno = VFF_ConvertFFError(fvff_mkdir(lfnpath));
	free(lfnpath);
	if(r->_errno!=0)return -1;
	return 0;
}

int _VFF_statvfs_r (struct _reent *r, const char *path, struct statvfs *buf)//Not supported by libff.
{
	r->_errno = ENOTSUP;
	return -1;
}

DIR_ITER* _VFF_diropen_r(struct _reent *r, DIR_ITER *dirState, const char *path)
{
	int i;
	DIR *dir = (DIR*)dirState->dirStruct;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	memset(lfnpath, 0, 128 * sizeof(TCHAR));
	if(strchr(path, ':'))path = strchr(path, ':')+1;
	if(strlen(path)>127)
	{
		free(lfnpath);
		r->_errno = ENAMETOOLONG;
		return NULL;
	}
	for(i=0; i<strlen(path); i++)lfnpath[i] = path[i];
	r->_errno = VFF_ConvertFFError(fvff_opendir(dir, lfnpath));
	free(lfnpath);	
	if(r->_errno!=0)return NULL;
	return dirState;
}

int _VFF_dirreset_r (struct _reent *r, DIR_ITER *dirState)
{
	DIR *dir = (DIR*)dirState->dirStruct;
	r->_errno = VFF_ConvertFFError(fvff_readdir(dir, NULL));
	if(r->_errno!=0)return -1;
	return 0;
}

FILINFO _info;
int _VFF_dirnext_r (struct _reent *r, DIR_ITER *dirState, char *filename, struct stat *filestat)
{
	int i;
	DIR *dir = (DIR*)dirState->dirStruct;
	TCHAR *lfnpath = (TCHAR*)malloc(128 * sizeof(TCHAR));
	TCHAR *path = lfnpath;
	char ext[5];
	_info.lfname = lfnpath;
	_info.lfsize = 128;
	r->_errno = VFF_ConvertFFError(fvff_readdir(dir, &_info));
	if(r->_errno!=0)
	{
		free(lfnpath);
		return -1;
	}
	if(_info.fname[0]==0)
	{
		free(lfnpath);
		return -1;//End of directory.
	}
	if(filestat)VFF_ConvertFFInfoToStat(&_info, filestat);
	if(path[0]==0)path = _info.fname;
	for(i=0; path[i]!=0 && i<128; i++)filename[i] = (char)path[i];
	filename[i+1] = 0;
	if(filename[i]=='.' && lfnpath[0]!=0)filename[i] = 0;//Remove the extra '.' from long filenames without an file extension.
	if(lfnpath[0]==0)
	{
		memset(ext, 0, 5);
		strncpy(&ext[1], &filename[9], 3);
		for(i=0; i<3; i++)
		{
			if(ext[i+1]=='?')ext[i+1] = 0;
		}
		if(strlen(&ext[1])>0)ext[0] = '.';

		memset(&filename[8], 0, 4);
		for(i = 7; i>=0; i--)
		{
			if(filename[i]!='?')break;
			if(filename[i]=='?')filename[i] = 0;
		}
		if(filename[i]!=' ' && filename[i]!=0 && strlen(ext)>0)
		{
			strcpy(&filename[i+1], ext);
		}
	}

	free(lfnpath);
	return 0;
}

int _VFF_dirclose_r (struct _reent *r, DIR_ITER *dirState)
{
	return 0;
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

