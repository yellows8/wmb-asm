/*
vffmount is licensed under the MIT license:
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
#include <endian.h>

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

void fs_destroy(void* usr)
{
	closelog();
}

int fs_statfs(const char *path, struct statvfs *fsinfo)
{
	/*int freeblocks = 0;
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
		if(SFFS.files[i].attr!=0)freeblocks--;
	}
	fsinfo->f_ffree = freeblocks;
	fsinfo->f_files = 6143 - freeblocks;
	return 0;*/
}

static int
fs_getattr(const char *path, struct stat *stbuf)
{
	/*nandfs_file_node cur;
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
	return 0;*/
}

static int
fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
           off_t offset, struct fuse_file_info *fi)
{
	/*nandfs_file_node cur;
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

	return 0;*/
}

int fs_rename(const char *path, const char *newpath)
{
	/*int i, i2, ind;
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
	for(i2=strlen(path)-1; i>0 && path[i2]!='/'; i2--);
	i2++;

	strncpy(SFFS.files[nand_nodeindex].name, &newpath[i], 12);
	if(i==i2 && strncmp(path, newpath, i)==0)
	{
		update_sffs();
	}
	else
	{
		ind = nandfs_unlink(path, 1, 1);
		syslog(0, "Unlink retval %d", ind);
		if(ind<0)return ind;
		syslog(0, "renaming");
		if((i = nandfs_create(newpath, 1, 0, 0, 0, ind))<0)return i;//nandfs_create calls update_sffs, so we don't need to call it again here.
	}

	return 0;*/
}

int fs_unlink(const char *path)
{
	//return nandfs_unlink(path, 1, 0);
}

int fs_rmdir(const char *path)
{
	//return nandfs_unlink(path, 2, 0);
}

int fs_mknod(const char *path, mode_t mode, dev_t dev)
{
	/*if(!(mode & S_IFREG))return -EINVAL;
	return nandfs_create(path, 1, mode, 0, 0, -1);*/
}

int fs_mkdir(const char *path, mode_t mode)
{
	//return nandfs_create(path, 2, mode, 0, 0, -1);
}

int fs_truncate(const char *path, off_t size)
{
	/*int i, way;
	unsigned int num, num2, newclusters, oldclusters;
	unsigned short tempcluster, tempclus;
	nandfs_file_node cur;

	if(!write_enable)return 0;
	syslog(0, "truncate: path %s size %x", path, size);
	if(nandfs_open(&cur, path, 1, &nand_nodeindex)==-1)
	{
		syslog(0, "no ent: %s", path);
		return -ENOENT;
	}

	oldclusters = (be32(cur.size)) / 0x4000;
	if((be32(cur.size)) % 0x4000)oldclusters++;
	if(size > be32(cur.size))
	{
		num = size - be32(cur.size);
		way = 0;
	}
	else
	{
		num = be32(cur.size) - size;
		way = 1;
	}
	if(num==0)return 0;

	newclusters = size / 0x4000;
	if(size % 0x4000)newclusters++;
	SFFS.files[nand_nodeindex].size = be32(size);	
	if((oldclusters==newclusters && size!=0) || be16(cur.first_cluster)==0xffff)
	{
		update_sffs();
		return 0;
	}


	if(way==0)
	{
		num = newclusters - oldclusters;
		tempcluster = be16(cur.first_cluster);	
		while(tempcluster!=0xfffb)
		{
			tempclus = be16(SFFS.cluster_table[tempcluster]);
			//SFFS.cluster_table[tempcluster] = be16(0xfffe);
			if(tempclus!=be16(0xfffb))tempcluster = tempclus;
		}
		while(num>0)
		{
			SFFS.cluster_table[tempcluster] = be16(nandfs_allocatecluster());
			SFFS.cluster_table[tempcluster] = be16(SFFS.cluster_table[tempcluster]);
			num--;
		}
	}
	else
	{
		num = oldclusters - newclusters;
		num2 = newclusters;
		tempcluster = be16(cur.first_cluster);	
		while(tempcluster!=0xfffb)
		{
			tempclus = be16(SFFS.cluster_table[tempcluster]);
			if(num2==0)SFFS.cluster_table[tempcluster] = be16(0xfffe);
			if(num2>0)num2--;
			if(tempclus!=be16(0xfffb))tempcluster = tempclus;
		}
		while(num>0)
		{
			SFFS.cluster_table[tempcluster] = be16(nandfs_allocatecluster());
			SFFS.cluster_table[tempcluster] = be16(SFFS.cluster_table[tempcluster]);
			num--;
		}
	}
	update_sffs();
	return 0;*/
}

static int
fs_open(const char *path, struct fuse_file_info *fi)
{
	/*int i;
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
	fd_array[i].cluster_index = 0;
	fd_array[i].nodeindex = nand_nodeindex;

	fi->fh = (uint64_t)i;
	return 0;*/
}

static int
fs_release(const char *path, struct fuse_file_info *fi)
{
	/*syslog(0, "closing fd %d", (int)fi->fh);
	if(!(used_fds & 1<<fi->fh))return -EBADF;
	used_fds &= ~1<<(unsigned int)fi->fh;
	memset(&fd_array[(int)fi->fh], 0, sizeof(nandfs_fp));
	syslog(0, "done");
	return 0;*/
}

static int
fs_read(const char *path, char *buf, size_t size, off_t offset,
        struct fuse_file_info *fi)
{
	/*syslog(0, "read fd %d offset %x size %x", (int)fi->fh, (int)offset, (int)size);
	if(!(used_fds & 1<<fi->fh))return -EBADF;
	syslog(0, "fd valid");
	memset(buf, 0, size);
	nandfs_seek(&fd_array[(int)fi->fh], offset, SEEK_SET);
	int num = nandfs_read(buf, size, 1, &fd_array[(int)fi->fh]);
	syslog(0, "readbytes %x", num);
	return num;*/
}

static int
fs_write(const char *path, const char *buf, size_t size, off_t offset,
         struct fuse_file_info *fi)
{
	/*syslog(0, "write fd %d offset %x size %x", (int)fi->fh, (int)offset, (int)size);

	if(!(used_fds & 1<<fi->fh))return -EBADF;
	syslog(0, "fd valid");
	nandfs_seek(&fd_array[(int)fi->fh], offset, SEEK_SET);
	int num = nandfs_write(buf, size, 1, &fd_array[(int)fi->fh]);
	syslog(0, "writebytes %x", num);
	return num;*/
}

static const struct fuse_operations fsops = {
   .destroy = fs_destroy,
   .statfs = fs_statfs,
   .getattr = fs_getattr,
   .readdir = fs_readdir,
   .rename  = fs_rename,
   .unlink  = fs_unlink,
   .rmdir  = fs_rmdir,
   .mknod  = fs_mknod,
   .mkdir  = fs_mkdir,
   .truncate = fs_truncate,
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
	//int argi;

	printf("vffmount v1.0 by yellowstar6\n");
	//memset(keyname, 0, 256);
	//strncpy(keyname, "default", 255);
	if(argc<3)
	{
		printf("Mount Wii VFF files with FUSE.\n");
		printf("Usage:\n");
		printf("vffmount <fs.vff> <mount point>\n");
		
		return 0;
	}
	
	/*for(argi=1; argi<argc; argi++)
	{
		
	}*/

	int retval = VFF_Mount(argv[1], 1);
	if(retval<0)
	{
		printf("VFF mount failed: %d\n", retval);
		return retval;
	}

	fuse_opt_add_arg(&args, argv[0]);
	fuse_opt_add_arg(&args, argv[2]);
	fuse_opt_add_arg(&args, "-o");
	fuse_opt_add_arg(&args, "allow_root");//Allow root to access the FS.

	openlog("vffmount", 0, LOG_USER);
	syslog(0, "STARTED");

	
	
   	return fuse_main(args.argc, args.argv, &fsops, NULL);
}

