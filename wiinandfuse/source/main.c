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

//Some nandfs structures and functions are based on Bootmii MINI ppcskel nandfs.c and MINI nand.c

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

unsigned char nand_hmackey[20];
unsigned char nand_aeskey[16];

unsigned char nand_cryptbuf[0x4000];

static unsigned char buffer[8*2048];

unsigned short nand_nodeindex = 0;

void aes_set_key(unsigned char *key);
void aes_decrypt(unsigned char *iv, unsigned char *inbuf, unsigned char *outbuf, unsigned long long len);
void aes_encrypt(unsigned char *iv, unsigned char *inbuf, unsigned char *outbuf, unsigned long long len);

int nand_correct(unsigned int pageno, void *data, void *ecc)//From Bootmii MINI nand.c, not used currently.
{
	unsigned char *dp = (unsigned char*)data;
	unsigned int *ecc_read = (unsigned int*)((unsigned char*)ecc+0x30);
	unsigned int *ecc_calc = (unsigned int*)((unsigned char*)ecc+0x40);
	int i;
	int uncorrectable = 0;
	int corrected = 0;
	
	for(i=0;i<4;i++) {
		unsigned int syndrome = *ecc_read ^ *ecc_calc; //calculate ECC syncrome
		// don't try to correct unformatted pages (all FF)
		if ((*ecc_read != 0xFFFFFFFF) && syndrome) {
			if(!((syndrome-1)&syndrome)) {
				// single-bit error in ECC
				corrected++;
			} else {
				// byteswap and extract odd and even halves
				unsigned short even = (syndrome >> 24) | ((syndrome >> 8) & 0xf00);
				unsigned short odd = ((syndrome << 8) & 0xf00) | ((syndrome >> 8) & 0x0ff);
				if((even ^ odd) != 0xfff) {
					// oops, can't fix this one
					uncorrectable++;
				} else {
					// fix the bad bit
					dp[odd >> 3] ^= 1<<(odd&7);
					corrected++;
				}
			}
		}
		dp += 0x200;
		ecc_read++;
		ecc_calc++;
	}
	if(uncorrectable || corrected)
		syslog(0, "ECC stats for NAND page 0x%x: %d uncorrectable, %d corrected\n", pageno, uncorrectable, corrected);
	if(uncorrectable)
		return NAND_ECC_UNCORRECTABLE;
	if(corrected)
		return NAND_ECC_CORRECTED;
	return NAND_ECC_OK;
}

void nand_read_sector(int sector, int num_sectors, unsigned char *buffer, unsigned char *ecc)
{
	if(sector<0 || num_sectors<=0 || buffer==NULL)return;

	if(has_ecc)lseek(nandfd, sector * 0x840, SEEK_SET);
	if(!has_ecc)lseek(nandfd, sector * 0x800, SEEK_SET);
	for(; num_sectors>0; num_sectors--)
	{
		read(nandfd, buffer, 0x800);
		buffer+= 0x800;
		if(has_ecc)
		{
			if(ecc)
			{
				read(nandfd, ecc, 0x40);
				ecc+= 0x40;
			}
			else
			{
				lseek(nandfd, 0x40, SEEK_CUR);
			}
		}
	}
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
				write(nandfd, ecc, 0x40);
			}
			else
			{
				write(nandfd, null, 0x40);
			}
		}
	}
}

void nand_read_cluster(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char eccbuf[0x200];
	nand_read_sector(cluster_number * 8, 8, cluster, eccbuf);
	if(ecc)
	{	
		memcpy(ecc, &eccbuf[0x40 * 6], 0x40);
		if(strnlen((char*)ecc, 0x40)==0)memcpy(ecc, &eccbuf[0x40 * 7], 0x40);//HMAC is in the 7th and 8th sectors.
	}
}

void nand_write_cluster(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char eccbuf[0x200];
	memset(eccbuf, 0, 0x200);
	if(ecc)
	{	
		memcpy(&eccbuf[0x40 * 6], ecc, 0x40);
		memcpy(&eccbuf[0x40 * 7], ecc, 0x40);//HMAC is in the 7th and 8th sectors.
	}
	nand_write_sector(cluster_number * 8, 8, cluster, eccbuf);
}

void nand_read_cluster_decrypted(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char iv[16];
	memset(iv, 0, 16);
	nand_read_cluster(cluster_number, nand_cryptbuf, ecc);
	aes_decrypt(iv, nand_cryptbuf, cluster, 0x4000);
}

void nand_write_cluster_encrypted(int cluster_number, unsigned char *cluster, unsigned char *ecc)
{
	unsigned char iv[16];
	memset(iv, 0, 16);
	aes_encrypt(iv, cluster, nand_cryptbuf, 0x4000);
	nand_write_cluster(cluster_number, nand_cryptbuf, ecc);
}

inline unsigned int be32(unsigned int x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

inline unsigned int le32(unsigned int x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

inline unsigned short be16(unsigned short x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

inline unsigned short le16(unsigned short x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

int sffs_init()
{
	unsigned int sffs_version = 0, sffs_cluster = 0;
	int i;
	unsigned char buf[0x800];
	nandfs_sffs *bufptr = (nandfs_sffs*)buf;
	unsigned char *sffsptr = (unsigned char*)&SFFS;
	if(!SFFS_only)i = 0x7f00;
	if(SFFS_only)i = 0;

	for(; i<0x7fff; i+=0x10)
	{
		nand_read_sector(i*8, 1, buf, NULL);
		if(i==0x7f00)
		{
			FILE *fdump = fopen("dump", "w");
			fwrite(buf, 1, 0x800, fdump);
			fclose(fdump);
		}
		if(memcmp(bufptr->magic, "SFFS", 4)!=0)continue;
		if(sffs_cluster==0 || sffs_version < bufptr->version)
		{
			sffs_cluster = i;
			sffs_version = bufptr->version;
		}
	}

	if(sffs_cluster==0)
	{
		printf("No SFFS supercluster found. Your NAND SFFS is seriously broken.\n");
		return -1;
	}

	for(i=0; i<16; i++)
	{
		nand_read_cluster(sffs_cluster + i, sffsptr, NULL);
		sffsptr+= 0x4000;
	}

	FILE *fmeta = fopen("sffs", "w");
	fwrite(&SFFS, 1, sizeof(nandfs_sffs), fmeta);
	fclose(fmeta);
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

int nandfs_read(void *ptr, unsigned int size, unsigned int nmemb, nandfs_fp *fp)//From Bootmii MINI ppcskel nandfs.c
{
	unsigned int total = size*nmemb;
	unsigned int copy_offset, copy_len;
	int realtotal = (unsigned int)total;

	if (fp->offset + total > fp->size)
		total = fp->size - fp->offset;

	if (total == 0)
		return 0;

	realtotal = (unsigned int)total;
	while(total > 0) {
		nand_read_cluster_decrypted(fp->cur_cluster, buffer, NULL);
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
	fsinfo->f_flag = ST_RDONLY;
	fsinfo->f_namemax = 12;
	for(i=0; i<32768; i++)
	{
		if(be16(SFFS.cluster_table[i])==0xFFFE)freeblocks++;
	}
	freeblocks*= 8;
	fsinfo->f_bfree = 0;
	fsinfo->f_bavail = freeblocks;
	return 0;
}

static int
fs_getattr(const char *path, struct stat *stbuf)
{
	nandfs_file_node cur;
	int type = -1;
	unsigned int perms = 0;
	int i;
	unsigned int perm = 400;
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
		//stbuf->st_mode = S_IFREG | 0444;
        	//stbuf->st_nlink = 1;
        	//stbuf->st_size = 0;
		syslog(0, "no ent");
		return -ENOENT;
		//return 0;
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
				perms += perm;
				syslog(0, "perm %x has r new perms %d %d", i, perms, perm);
			}
			if((cur.attr >> (6-(i*2))) & 2)
			{
				perms += (perm / 2);
				syslog(0, "perm %x has w new perms %d %d", i, perms, perm / 2);
			}
			perm /= 10;
		}
	}

	if(type==0)
	{
		if(!use_nand_permissions)perms = 0755;
		if(use_nand_permissions)perms+= 111;
	        stbuf->st_mode = S_IFDIR | perms;
	        stbuf->st_nlink = 2;
		stbuf->st_blksize = 2048;
		syslog(0, "Type directory perms %d", perms);
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
		syslog(0, "Type file %s %02x %d", cur.name, cur.attr, perms);
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
		sprintf(str, "no ent: %s", path);
		syslog(0, str);
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
			memset(str, 0, 256);
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
	syslog(0, "closing fd %d", fi->fh);
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

static const struct fuse_operations fsops = {
   .destroy = fs_destroy,
   .statfs = fs_statfs,
   .getattr = fs_getattr,
   .readdir = fs_readdir,
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
	}

	nandfd = open(argv[1], O_RDWR);
	if(nandfd<0)
	{

		printf("Failed to open %s\n", argv[1]);
		return -1;
	}

	if(sffs_init()<0)
	{
		close(nandfd);
		return 0;
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

	//close(nandfd);
   return fuse_main(args.argc, args.argv, &fsops, NULL);
   //return fuse_main(1, &argv[2], &fsops, NULL);
}

