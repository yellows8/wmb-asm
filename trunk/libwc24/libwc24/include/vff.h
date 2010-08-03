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

s32 VFF_CreateVFF(char *path, u32 filesize, int delvff);//Creates VFF at path with ISFS. Returns zero on success, ISFS error otherwise. When delvff is zero, fail when the VFF already exists, otherwise when non-zero delete the VFF then create it.
s32 VFF_Mount(char *path, char *mntname);//Mounts a VFF. mntname is the devoptab device name, when NULL the default is the filename specified in path. With the libwc24 Wii build, to mount VFF files from devoptab devices(SD etc), you must include the full path like "fat:/" etc.
s32 VFF_Unmount(char *mntname);//Unmount a VFF. mntname is the mntname input used with VFF_Mount, when that was NULL this input should the filename from the path.
u32 VFF_GetFATSize(u32 filesize);
u32 VFF_GetFATType(u32 filesize);

#ifndef HW_RVL
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

#endif

#ifdef __cplusplus
   }
#endif

#endif

