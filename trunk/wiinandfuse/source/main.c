/*
wiinandfuse is licensed under the MIT license:
Copyright (c) 2010 yellowstar6

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

//Some nandfs structures and functions are based on Bootmii MINI ppcskel nandfs.c

#define FUSE_USE_VERSION  26
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>

#include "tools.h"
#include "fs_hmac.h"
#include "ecc.h"

#define	NANDFS_NAME_LEN	12

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#define NAND_ECC_OK 0//From Bootmii MINI nand.h
#define NAND_ECC_CORRECTED 1
#define NAND_ECC_UNCORRECTABLE -1

#define be32 be32_wrapper
#define be16 be16_wrapper

typedef struct _nandfs_file_node {//These _nandfs structs are based on MINI ppcskel nandfs.c
	char name[NANDFS_NAME_LEN];
	unsigned char attr;
	unsigned char unk;
	union {
		unsigned short first_child;
		unsigned short first_cluster;
	};
	unsigned short sibling;
	unsigned int size;
	unsigned int uid;
	unsigned short gid;
	unsigned int dummy;
} __attribute__((packed)) nandfs_file_node;

typedef struct _nandfs_sffs {
	unsigned char magic[4];
	unsigned int version;
	unsigned int dummy;

	unsigned short cluster_table[32768];
	nandfs_file_node files[6143];
	unsigned char padding[0x14];//Added by yellowstar6.
} __attribute__((packed)) nandfs_sffs;

typedef struct _nandfs_fp {
	signed short first_cluster;
	signed short cur_cluster;
	unsigned int size;
	unsigned int offset;
	nandfs_file_node *node;
} nandfs_fp;

nandfs_sffs SFFS;
unsigned int used_fds = 0;
nandfs_fp fd_array[32];

int nandfd;
int has_ecc = 1;
int SFFS_only = 0;
int use_nand_permissions = 0;
int check_sffs_hmac = 1;
int hmac_abort = 0;
int round_robin = -1;
int round_robin_didupdate = 0;
int write_enable = 1;
int ignore_ecc = 0;

int supercluster = -1;
unsigned int sffs_cluster = 0;
unsigned int sffs_version = 0;

unsigned char nand_hmackey[20];
unsigned char nand_aeskey[16];

unsigned char nand_cryptbuf[0x4000];

static unsigned char buffer[8*2048];

unsigned short nand_nodeindex = 0;

void aes_set_key(unsigned char *key);
void aes_decrypt(unsigned char *iv, unsigned char *inbuf, unsigned char *outbuf, unsigned long long len);
void aes_encrypt(unsigned char *iv, unsigned char *inbuf, unsigned char *outbuf, unsigned long long len);

unsigned int be32_wrapper(unsigned int x);
unsigned short be16_wrapper(unsigned short x);

int nand_read_sector(int sector, int num_sectors, unsigned char *buffer, unsigned char *ecc)
{
	int retval = NAND_ECC_OK;
	unsigned char buf[0x840];
	if(sector<0 || num_sectors<=0 || buffer==NULL)return NAND_ECC_OK;

	if(has_ecc)lseek(nandfd, sector * 0x840, SEEK_SET);
	if(!has_ecc)lseek(nandfd, sector * 0x800, SEEK_SET);
	for(; num_sectors>0; num_sectors--)
	{
		read(nandfd, buffer, 0x800);
		if(has_ecc)
		{
			if(ecc)
			{
				read(nandfd, ecc, 0x40);
				memcpy(buf, buffer, 0x800);
				memcpy(&buf[0x800], ecc, 0x40);
				retval = check_ecc(buf);
				ecc+= 0x40;
			}
			else
			{
				lseek(nandfd, 0x40, SEEK_CUR);
			}
		}
		buffer+= 0x800;
	}
	return retval;
}

void nand_write_sector(int sector, int num_sectors, unsigned char *buffer, unsigned char *ecc)
{
	unsigned char null[0x40];
	if(sector<0 || num_sectors<=0 || buffer==NULL)return;
	memset(null, 0, 0x40);

	if(has_ecc)lseek(nandfd, sector * 0x840, SEEK_SET);
	if(!has_ecc)lseek(nandfd, sector * 0x800, SEEK_SET);
	for(; num_sectors>0; num_sectors--)
	{
		write(nandfd, buffer, 0x800);
		if(has_ecc)
		{
			if(ecc)
			{
				calc_ecc(buffer, ecc + 0x30);
				calc_ecc(buffer + 512, ecc + 0x30 + 4);
				calc_ecc(buffer + 1024, ecc + 0x30 + 8);
				calc_ecc(buffer + 1536, ecc + 0x30 + 12);
				write(nandfd, ecc, 0x40);
				ecc+=0x40;
			}
			else
			{
				write(nandfd, null, 0x40);
			}
		}
		buffer+=0x800;
	}
}

int nand_read_cluster(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char eccbuf[0x200];
	int retval = nand_read_sector(cluster_number * 8, 8, cluster, eccbuf);
	if(ecc)
	{	
		memcpy(ecc, &eccbuf[0x40 * 6], 0x40);
		memcpy(&ecc[0x40], &eccbuf[0x40 * 7], 0x40);//HMAC is in the 7th and 8th sectors.
	}
	return retval;
}

void nand_write_cluster(int cluster_number, unsigned char *cluster, unsigned char *hmac)
{
	unsigned char eccbuf[0x200];
	memset(eccbuf, 0, 0x200);
	if(hmac)
	{
		memcpy(&eccbuf[(0x40 * 6) + 1], hmac, 0x14);
		memcpy(&eccbuf[(0x40 * 6) + 0x15], hmac, 12);
		memcpy(&eccbuf[(0x40 * 7) + 1], &hmac[12], 8);//HMAC is in the 7th and 8th sectors.
	}
	nand_write_sector(cluster_number * 8, 8, cluster, eccbuf);
}

int nand_read_cluster_decrypted(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char iv[16];
	int retval;	
	memset(iv, 0, 16);
	retval = nand_read_cluster(cluster_number, nand_cryptbuf, ecc);
	aes_decrypt(iv, nand_cryptbuf, cluster, 0x4000);
	return retval;
}

void nand_write_cluster_encrypted(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char iv[16];
	memset(iv, 0, 16);
	aes_encrypt(iv, cluster, nand_cryptbuf, 0x4000);
	nand_write_cluster(cluster_number, nand_cryptbuf, ecc);
}

int sffs_init(int ver)//Somewhat based on Bootmii MINI ppcskel nandfs.c SFFS init code.
{
	int i;
	int si = -1;
	unsigned char buf[0x800];
	nandfs_sffs *bufptr = (nandfs_sffs*)buf;
	unsigned char *sffsptr = (unsigned char*)&SFFS;
	unsigned char supercluster_hmac[0x80];
	unsigned char calc_hmac[20];
	
	if(!SFFS_only)i = 0x7f00;
	if(SFFS_only)i = 0;

	for(; i<0x7fff; i+=0x10)
	{
		si++;
		nand_read_sector(i*8, 1, buf, NULL);
		if(memcmp(bufptr->magic, "SFFS", 4)!=0)continue;
		if(ver==-2)
		{
			if(sffs_cluster==0 || sffs_version < be32(bufptr->version))
			{
				sffs_cluster = i;
				sffs_version = be32(bufptr->version);
				supercluster = si;
			}
		}
		else if(ver==-1)
		{
			printf("SFFS supercluster %x cluster %x version %x\n", si, i, be32(bufptr->version));
		}
		else
		{
			if(be32(bufptr->version) == (unsigned int)ver)
			{
				sffs_cluster = i;
				sffs_version = be32(bufptr->version);
				supercluster = si;
			}
		}
	}

	if(sffs_cluster==0 && ver!=-1)
	{
		if(ver<0)printf("No SFFS supercluster found. Your NAND SFFS is seriously broken.\n");
		if(ver>=0)printf("Failed to find SFFS supercluster with version %x.\n", ver);
		return -1;
	}
	else if(ver==-1)return -1;

	printf("supercluster %x cluster %x\n", supercluster, sffs_cluster);
	memset(&SFFS, 0, 0x40000);
	for(i=0; i<16; i++)
	{
		nand_read_cluster(sffs_cluster + i, sffsptr, supercluster_hmac);
		sffsptr+= 0x4000;
	}

	if(check_sffs_hmac)
	{
		memset(calc_hmac, 0, 20);
		fs_hmac_meta(&SFFS, sffs_cluster, calc_hmac);
		if(memcmp(&supercluster_hmac[1], calc_hmac, 20)==0)
		{
			printf("SFFS HMAC valid.\n");
		}
		else
		{
			printf("SFFS HMAC calc failed.\n");
			FILE *f = fopen("debug", "wb");
			fwrite(supercluster_hmac, 1, 0x80, f);
			fwrite(calc_hmac, 1, 20, f);
			fclose(f);
			if(hmac_abort)return -1;
		}
	}

	/*
	nandfs_file_node testnode;
		nandfs_open(&testnode, "/shared2/sys/SYSCONF", 1, NULL);		
		unsigned char *buf_ = (unsigned char*)malloc(0x4000);
		memset(buf_, 0, 0x4000);
		nand_read_cluster_decrypted(be16(testnode.first_cluster), buf_, supercluster_hmac);
		//nandfs_read(buf_, 1, be32(testnode.size), &testnode);
		FILE *f = fopen("SYSCONF", "w");
	fwrite(buf_, 1, 0x4000, f);
	fclose(f);
		fs_hmac_data(buf_, be32(testnode.uid), testnode.name, nand_nodeindex, be32(testnode.dummy), 0, calc_hmac);

	f = fopen("debug2", "wb");
	fwrite(supercluster_hmac, 1, 0x80, f);
	fwrite(calc_hmac, 1, 20, f);
	fclose(f);
	*/

	return 0;
}

int update_sffs()
{
	unsigned char calc_hmac[20];
	int i;
	unsigned char *sffsptr = (unsigned char*)&SFFS;
	if(!write_enable)return 0;
	memset(calc_hmac, 0, 20);
	
	if(round_robin==-2)
	{
		if(round_robin_didupdate==0)
		{
			supercluster++;
			round_robin_didupdate = 1;
		}
	}
	else
	{
		supercluster++;
	}
	if(supercluster>15)supercluster = 0;
	sffs_cluster = 0x7f00 + (supercluster * 0x10);
	sffs_version++;
	SFFS.version = be32(sffs_version);

	fs_hmac_meta(&SFFS, sffs_cluster, calc_hmac);
	for(i=0; i<16; i++)
	{
		nand_write_cluster(sffs_cluster + i, sffsptr, i==15?calc_hmac:NULL);
		sffsptr+= 0x4000;
	}

	return 0;
}

int DumpSFFS()
{
	int i;
	char str[16];
	for(i=0; i<6143; i++)
	{
		memset(str, 0, 16);
		strncpy(str, SFFS.files[i].name, 12);
		printf("%s\n", str);
	}

	return 0;
}

int nandfs_open(nandfs_file_node *fp, const char *path, int type, unsigned short *index)//This is based on the function from MINI ppcskel nandfs.c
{
	char *ptr, *ptr2;
	unsigned int len;
	nandfs_file_node *cur = SFFS.files;
	nand_nodeindex = 0;

	memset(fp, 0, sizeof(nandfs_file_node));

	if(strcmp(cur->name, "/") != 0) {
		syslog(0, "your nandfs is corrupted. fixit!\n");
		return -1;
	}

	if(strcmp(path, "/")!=0)
	{
		nand_nodeindex = be16(cur->first_child);
		cur = &SFFS.files[be16(cur->first_child)];
	}

	if(strcmp(path, "/")!=0)
	{
	ptr = (char *)path;
	do {
		ptr++;
		ptr2 = strchr(ptr, '/');
		if (ptr2 == NULL)
			len = strlen(ptr);
		else {
			ptr2++;
			len = ptr2 - ptr - 1;
		}
		if (len > 12)
		{
			printf("invalid length: %s %s %s [%d]\n",
					ptr, ptr2, path, len);
			return -1;
		}

		for (;;) {
			if(ptr2 != NULL && strncmp(cur->name, ptr, len) == 0
			     && strnlen(cur->name, 12) == len
			     && (cur->attr&3) == 2
			     && (signed short)(be16(cur->first_child)&0xffff) != (signed short)0xffff) {
				nand_nodeindex = be16(cur->first_child);				
				cur = &SFFS.files[be16(cur->first_child)];
				ptr = ptr2-1;
				break;
			} else if(ptr2 == NULL &&
				   strncmp(cur->name, ptr, len) == 0 &&
				   strnlen(cur->name, 12) == len &&
				   (cur->attr&3) == type) {
				break;
			} else if((cur->sibling&0xffff) != 0xffff) {
				nand_nodeindex = be16(cur->sibling);
				cur = &SFFS.files[be16(cur->sibling)];
			} else {
				return -1;
			}
		}
		
	} while(ptr2 != NULL);
	}

	memcpy(fp, cur, sizeof(nandfs_file_node));
	return 0;
}

int nandfs_read(void *ptr, unsigned int size, unsigned int nmemb, nandfs_fp *fp)//Based on Bootmii MINI ppcskel nandfs.c
{
	unsigned int total = size*nmemb;
	unsigned int copy_offset, copy_len;
	int retval;
	int realtotal = (unsigned int)total;

	if (fp->offset + total > fp->size)
		total = fp->size - fp->offset;

	if (total == 0)
		return 0;

	realtotal = (unsigned int)total;
	while(total > 0) {
		retval = nand_read_cluster_decrypted(fp->cur_cluster, buffer, NULL);
		if(retval<0 && !ignore_ecc)return -EIO;		
		copy_offset = fp->offset % (2048 * 8);
		copy_len = (2048 * 8) - copy_offset;
		if(copy_len > total)
			copy_len = total;
		memcpy(ptr, buffer + copy_offset, copy_len);
		ptr+= copy_len;
		total -= copy_len;
		fp->offset += copy_len;

		if ((copy_offset + copy_len) >= (2048 * 8))
			fp->cur_cluster = be16(SFFS.cluster_table[fp->cur_cluster]);
	}

	return realtotal;
}

int nandfs_seek(nandfs_fp *fp, unsigned int where, unsigned int whence)
{
	int clusters = 0;
	if(whence==SEEK_SET)fp->offset = where;
	if(whence==SEEK_CUR)fp->offset += where;
	if(whence==SEEK_END)fp->offset = fp->size;

	if(fp->offset)clusters = fp->offset / 0x4000;
	syslog(0, "seek: clusters %x where %x offset %x", clusters, where, fp->offset);
	fp->cur_cluster = fp->first_cluster;	
	while(clusters>0)
	{
		fp->cur_cluster = be16(SFFS.cluster_table[fp->cur_cluster]);
		clusters--;
	}

	return 0;
}

void fs_destroy(void* usr)
{
	close(nandfd);
	closelog();
}

int fs_statfs(const char *path, struct statvfs *fsinfo)
{
	int freeblocks = 0;
	int i;
	memset(fsinfo, 0, sizeof(struct statvfs));
	fsinfo->f_bsize = 2048;
	fsinfo->f_frsize = 2048 * 8;
	fsinfo->f_blocks = 0x8000 * 8;
	if(!write_enable)fsinfo->f_flag = ST_RDONLY;
	fsinfo->f_namemax = 12;
	for(i=0; i<32768; i++)
	{
		if(be16(SFFS.cluster_table[i])==0xFFFE)freeblocks++;
	}
	freeblocks*= 8;
	fsinfo->f_bfree = 0;
	fsinfo->f_bavail = freeblocks;
	freeblocks = 6143;
	for(i=0; i<6143; i++)
	{
		if(SFFS.files[i].unk!=0)freeblocks--;
	}
	fsinfo->f_ffree = freeblocks;
	fsinfo->f_files = 6143 - freeblocks;
	return 0;
}

static int
fs_getattr(const char *path, struct stat *stbuf)
{
	nandfs_file_node cur;
	int type = -1;
	unsigned int perms = 0;
	int i;
	unsigned int perm = 0400;
	syslog(0, "getattr %s", path);
	memset(stbuf, 0, sizeof(struct stat));
	if(nandfs_open(&cur, path, 1, &nand_nodeindex)>-1)
	{
		type = 1;
	}

	if(type==-1)
	{
		if(nandfs_open(&cur, path, 2, &nand_nodeindex)>-1)
		{
			type = 0;
		}
	}

	if(strcmp(path, "/")==0)type = 0;

	if(type==-1)
	{
		syslog(0, "no ent");
		return -ENOENT;
	}


	stbuf->st_atime = -1;
	stbuf->st_mtime = -1;
	stbuf->st_ctime = -1;
	if(use_nand_permissions)
	{	
		stbuf->st_uid = (uid_t)be32(cur.uid);
		stbuf->st_gid = (gid_t)be16(cur.gid);
		syslog(0, "perms: %x %x", cur.attr, cur.unk);
		for(i=0; i<3; i++)
		{
			if((cur.attr >> (6-(i*2))) & 1)
			{
				perms |= perm;
				syslog(0, "perm %03x has r new perms %o %o", i, perms, perm);
			}
			if((cur.attr >> (6-(i*2))) & 2)
			{
				perms |= perm / 02;
				syslog(0, "perm %03x has w new perms %o %o", i, perms, perm / 02);
			}
			perm /= 010;
		}
	}

	if(type==0)
	{
		if(!use_nand_permissions)perms = 0755;
		if(use_nand_permissions)perms |= 0111;
	        stbuf->st_mode = S_IFDIR | perms;
	        stbuf->st_nlink = 2;
		stbuf->st_blksize = 2048;
		syslog(0, "Type directory perms %o", perms);
    	}
	else
	{
		if(!use_nand_permissions)perms = 0444;
		stbuf->st_mode = S_IFREG | perms;
        	stbuf->st_nlink = 1;
        	stbuf->st_size = be32(cur.size);
		stbuf->st_blksize = 2048;
		stbuf->st_blocks = stbuf->st_size / 512;
		stbuf->st_ino = nand_nodeindex;
		syslog(0, "Type file %s %02x %o", cur.name, cur.attr, perms);
	}
	return 0;
}

static int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
           off_t offset, struct fuse_file_info *fi)
{
	nandfs_file_node cur;
	char fn[13];
	char str[256];
	int stop;
	memset(fn, 0, 13);
	memset(str, 0, 256);
	//syslog(0, "readdir: %s", path);
	if(nandfs_open(&cur, path, 2, &nand_nodeindex)==-1)
	{
		syslog(0, "no ent: %s", path);
		return -ENOENT;
	}
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if(cur.first_child!=0xffff)
	{
		stop = 0;
		memcpy(&cur, &SFFS.files[be16(cur.first_child)], sizeof(nandfs_file_node));
		do
		{
			memset(fn, 0, 13);
			strncpy(fn, cur.name, 12);
			filler(buf, fn, NULL, 0);
			//memset(str, 0, 256);
			//sprintf(str, "sib %x fn %s", be16(cur.sibling), fn);
			//syslog(0, str);
			if(cur.sibling!=0xffff)
			{
				memcpy(&cur, &SFFS.files[be16(cur.sibling)], sizeof(nandfs_file_node));
			}
			else
			{
				stop = 1;
			}
		} while(!stop);
	}

	return 0;
}

int fs_rename(const char *path, const char *newpath)
{
	int i;
	nandfs_file_node cur;

	if(!write_enable)return 0;
	syslog(0, "Rename: path %s newpath %s", path, newpath);
	if(nandfs_open(&cur, path, 1, &nand_nodeindex)==-1)
	{
		syslog(0, "no ent: %s", path);
		return -ENOENT;
	}
	
	for(i=strlen(newpath)-1; i>0 && newpath[i]!='/'; i--);
	i++;
	strncpy(SFFS.files[nand_nodeindex].name, &newpath[i], 12);
	
	update_sffs();

	return 0;
}

static int
fs_open(const char *path, struct fuse_file_info *fi)
{
	int i;
	nandfs_file_node cur;
	syslog(0, "open: %s", path);
	//if((fi->flags & 3) != O_RDONLY)return -EACCES;
	if(used_fds== ~0)return -ENFILE;	
	
	syslog(0, "Getting node struct...");
	if(nandfs_open(&cur, path, 1, &nand_nodeindex)==-1)
	{
		syslog(0, "No ent...");
		if(nandfs_open(&cur, path, 2, NULL)>-1)
		{
			syslog(0, "open: not directory %s", path);
			return -ENOTDIR;
		}
		syslog(0, "no ent: %s", path);
		return -ENOENT;
	}

	for(i=0; i<32; i++)
	{
		if(!(used_fds & 1<<i))break;
	}
	used_fds |= 1<<i;

	syslog(0, "Using fd %d nodeindex %x", i, (unsigned int)nand_nodeindex);
	memset(&fd_array[i], 0, sizeof(nandfs_fp));
	fd_array[i].node = &SFFS.files[nand_nodeindex];
	fd_array[i].first_cluster = be16(fd_array[i].node->first_cluster);
	fd_array[i].cur_cluster = fd_array[i].first_cluster;
	fd_array[i].size = be32(fd_array[i].node->size);
	fd_array[i].offset = 0;

	fi->fh = (uint64_t)i;
	return 0;
}

static int
fs_release(const char *path, struct fuse_file_info *fi)
{
	syslog(0, "closing fd %d", (int)fi->fh);
	if(!(used_fds & 1<<fi->fh))return -EBADF;
	used_fds &= ~1<<(unsigned int)fi->fh;
	memset(&fd_array[(int)fi->fh], 0, sizeof(nandfs_fp));
	syslog(0, "done");
	return 0;
}

static int
fs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
	syslog(0, "read fd %d offset %x size %x", (int)fi->fh, (int)offset, (int)size);
	if(!(used_fds & 1<<fi->fh))return -EBADF;
	syslog(0, "fd valid");
	memset(buf, 0, size);
	nandfs_seek(&fd_array[(int)fi->fh], offset, SEEK_SET);
	int num = nandfs_read(buf, size, 1, &fd_array[(int)fi->fh]);
	syslog(0, "readbytes %x", num);
	return num;
}

static int
fs_write(const char *path, const char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi)
{
	return 0;
}

#undef be32
#undef be16

unsigned int be32_wrapper(unsigned int x)
{
	return be32(&x);
}

unsigned short be16_wrapper(unsigned short x)
{
	return be16(&x);
}

static const struct fuse_operations fsops = {
   .destroy = fs_destroy,
   .statfs = fs_statfs,
   .getattr = fs_getattr,
   .readdir = fs_readdir,
   .rename  = fs_rename,
   .open   = fs_open,
   .release   = fs_release,
   .read   = fs_read,
   .write  = fs_write,
};

int main(int argc, char **argv)
{
	struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
	struct stat filestats;
	char str[256];
	char keyname[256];
	int argi;
	FILE *fkey;
	int ver = -2;
	int dump = 0;

	printf("wiinandfuse v1.0 by yellowstar6\n");
	memset(keyname, 0, 256);
	strncpy(keyname, "default", 255);
	if(argc<3)
	{
		printf("Mount Wii NAND images with FUSE.\n");
		printf("Usage:\n");
		printf("wiinandfuse <nand.bin> <mount point> <options>\n");
		printf("Options:\n");
		printf("-s: Dump contains only the 4MB SFFS. Reading/writing files will do nothing, the data reading buffers will be cleared.\n");
		printf("-k: Directory name of keys to use for raw NAND images. Default for keyname is \"default\". Path: $HOME/.wii/<keyname>\n");
		printf("-p: Use NAND permissions. UID and GUI of objects will be set to the NAND UID/GID, as well as the permissions. This option only enables setting the UID/GID and permissions in stat, the open and readdir functions don't check permissions.\n");
		printf("-g<SFFS version number>: Use the SFFS super cluster that has the specified version number. If no number is specified, the version numbers are listed.\n");
		printf("-d: Dump SFFS info of all nodes to stdout.\n");
		printf("-h: Disable SFFS HMAC verification. Default is enabled.\n");
		printf("-v: Abort/EIO if HMAC verification of SFFS or file data fails. If SFFS verification fails, wiinandfuse aborts and NAND isn't mounted. If file data verification fails, read will return EIO.\n");
		printf("-r<version>: Disable round-robin SFFS updating, default is on. When disabled, only the first metadata update has the version and supercluster increased. If version is specified, the supercluster with the specified version, has the version set to the version of the oldest supercluster minus one.\n");
		printf("-e: Ignore ECC errors, default is disabled. When disabled, when pages have invalid ECC reads return EIO.\n");
		return 0;
	}
	
	for(argi=3; argi<argc; argi++)
	{
		if(strcmp(argv[argi], "-s")==0)SFFS_only = 1;
		if(strncmp(argv[argi], "-k", 2)==0)
		{
			strncpy(keyname, &argv[argi][2], 255);
		}
		if(strcmp(argv[argi], "-p")==0)use_nand_permissions = 1;
		if(strncmp(argv[argi], "-g", 2)==0)
		{
			if(strlen(argv[argi])>2)
			{
				sscanf(&argv[argi][2], "%x", &ver);
			}
			else
			{
				ver=-1;
			}
		}
		if(strcmp(argv[argi], "-d")==0)dump = 1;
		if(strcmp(argv[argi], "-h")==0)
		{
			check_sffs_hmac = 0;
			write_enable = 0;
		}
		if(strcmp(argv[argi], "-v")==0)hmac_abort = 1;
		if(strncmp(argv[argi], "-r", 2)==0)
		{
			if(strlen(argv[argi])>2)
			{
				sscanf(&argv[argi][2], "%x", &round_robin);
			}
			else
			{
				round_robin=-2;
				round_robin_didupdate = 0;
			}
		}
		if(strcmp(argv[argi], "-e")==0)ignore_ecc = 1;
	}

	nandfd = open(argv[1], O_RDWR);
	if(nandfd<0)
	{

		printf("Failed to open %s\n", argv[1]);
		return -1;
	}

	fuse_opt_add_arg(&args, argv[0]);
	fuse_opt_add_arg(&args, argv[2]);

	openlog("wiinandfuse", 0, LOG_USER);
	syslog(0, "STARTED");

	fstat(nandfd, &filestats);
	if(!SFFS_only)
	{
		if(filestats.st_size==0x21000400)//The NAND image is a Bootmii dump with keys.
		{
			lseek(nandfd, 0x21000144, SEEK_SET);
			read(nandfd, nand_hmackey, 20);
			read(nandfd, nand_aeskey, 16);
		}
		else//The dump is raw without Bootmii keys.
		{
			if(filestats.st_size==0x21000000)
			{
				has_ecc = 1;
			}
			else if(filestats.st_size==0x20000000)
			{
				has_ecc = 0;
			}
			else
			{
				printf("NAND image filesize not recognized, it's corrupted. 0x%x\n", (unsigned int)filestats.st_size);
				close(nandfd);
				closelog();
				return 0;
			}

			memset(str, 0, 256);
			sprintf(str, "%s/.wii/%s/nand-hmac", getenv("HOME"), keyname);
			fkey = fopen(str, "r");
			if(fkey==NULL)
			{
				printf("Failed to open %s\n", str);
				close(nandfd);
				closelog();
				return 0;
			}			
			fread(nand_hmackey, 1, 20, fkey);
			fclose(fkey);

			memset(str, 0, 256);
			sprintf(str, "%s/.wii/%s/nand-key", getenv("HOME"), keyname);
			fkey = fopen(str, "r");
			if(fkey==NULL)
			{
				printf("Failed to open %s\n", str);
				close(nandfd);
				closelog();
				return 0;
			}			
			fread(nand_aeskey, 1, 16, fkey);
			fclose(fkey);
		}
	}
	else
	{
		write_enable = 0;
		if(filestats.st_size==0x420000)
		{
			has_ecc = 1;
		}
		else if(filestats.st_size==0x400000)
		{
			has_ecc = 0;
		}
		else
		{
			printf("SFFS filesize not recognized, it's corrupted. 0x%x\n", (unsigned int)filestats.st_size);
			close(nandfd);
			closelog();
			return 0;
		}
	}

	aes_set_key(nand_aeskey);
	fs_hmac_set_key(nand_hmackey, 20);

	if(sffs_init(ver)<0)
	{
		close(nandfd);
		closelog();
		return 0;
	}

	if(dump)
	{
		DumpSFFS();
		close(nandfd);
		closelog();
		return 0;
	}

	//close(nandfd);
   return fuse_main(args.argc, args.argv, &fsops, NULL);
   //return fuse_main(1, &argv[2], &fsops, NULL);
}

