/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "..\include\wmb_asm.h"

#ifdef NDS

#include <stdlib.h>
#include <stdio.h>
#include <fat.h>
#include "../dll/include/ndsdirent.h"

//This struct is similar to the Win32 one, too
struct DIR
{
    DIR_ITER                *handle; /* -1 for failed opening */
    //struct _finddata_t  info;
    dirent       result; /* d_name null iff first time */
    char                *name;  /* null-terminated char string */
};

DIR *opendir(const char *dirname)
{
    DIR *dir = (DIR*)malloc(sizeof(DIR));
	memset(dir,0,sizeof(DIR));
	dir->handle = diropen(dirname);
	if(dir->handle==NULL)return NULL;//Failed to open the directory
	dir->name = (char*)malloc(256);//MAXPATHLEN
	strcpy(dir->name,dirname);
	
	return dir;
}

int closedir(DIR *dir)
{
    if(dir==NULL)return -1;
	if(dir->handle==NULL)return -1;
	
	dirclose(dir->handle);
	free(dir->name);
	free(dir);
	
	return 0;
}

dirent *readdir(DIR *dir)
{
	 struct stat st;
	 memset(&st,0,sizeof(struct stat));
	if(dir==NULL)return NULL;
	if(dir->handle==NULL)return NULL;
	
	if(dir->result.d_name!=NULL)
	{
	free(dir->result.d_name);
	dir->result.d_name = NULL;
	}
	dir->result.d_name = (char*)malloc(256);
	strcpy(dir->result.d_name,"");
	
    if(dirnext(dir->handle,dir->result.d_name,&st)!=0)return NULL;
	
	dir->result.d_ino = st.st_ino;
	
	return &dir->result;
}

void rewinddir(DIR *dir)
{
    if(dir==NULL)return;
	if(dir->handle==NULL)return;
	
	dirreset(dir->handle);
}

#endif
