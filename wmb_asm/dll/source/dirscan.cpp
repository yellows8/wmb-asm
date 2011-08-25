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

#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif

#include "..\include\wmb_asm.h"

inline void AllocDir(FILE_LIST *files)
{
    files->filename = (char*)malloc(256);
        strcpy(files->filename,"");
        files->next = (FILE_LIST*)malloc(sizeof(FILE_LIST));
        memset(files->next,0,sizeof(FILE_LIST));
}

DLLIMPORT FILE_LIST *ScanDirectory(FILE_LIST *filelist, char *dirname, char *ext)
{
    FILE_LIST *files=NULL;
    FILE_LIST *cur_files=NULL;
    DIR *dir;
    dirent *ent;
    char DirName[256];
    strcpy(DirName,dirname);
    if(filelist!=NULL)
    {
        files = filelist;
        while(files->next!=NULL)
        {
            files = files->next;
        }


    }

    cur_files = files;

    FILE *f = fopen(DirName,"rb");
    if(f!=NULL)
    {
        AllocDir(files);
        fclose(f);
        strcpy(cur_files->filename,DirName);
        return files;
    }

    dir=opendir(DirName);
    if(dir==NULL)
    {
        return NULL;
    }

    char c = DirName[strlen(DirName)];

    if(c!='/' && c!='\\')
    {
    sprintf(DirName,"%s/",DirName);
    }

    char *str;
    str = (char*)malloc(256);
    strcpy(str,"");
    int skip=0;

    if(files==NULL)AllocDir(files);

    ent=(dirent*)1;
    while(ent!=NULL)
    {
        ent = readdir(dir);
        if(ent==NULL)break;

        if(skip<2)
        {
            skip++;
            continue;
        }
        else
        {

        sprintf(str,"%s%s",DirName,ent->d_name);
        f = fopen(str,"rb");
        if(f==NULL)
        {

        char *Str = (char*)malloc(256);
        strcpy(Str,str);
        if(ScanDirectory(cur_files,Str,ext)==NULL)
        printf("Failed to open file or directory %s\n",Str);

        free(Str);
        }
        else
        {
        fclose(f);

        if(ext!=NULL)
        {
            if(strcmp((const char*)&ent->d_name[strlen(ent->d_name)-4],(const char*)ext)!=0)
            {
                continue;
            }
        }

        AllocDir(cur_files);

        strcpy(cur_files->filename,str);

        cur_files = cur_files->next;
        }
        }
    }

    closedir(dir);
    free(str);

    return files;
}

DLLIMPORT void FreeDirectory(FILE_LIST *filelist)
{

    FILE_LIST *files = filelist;
    FILE_LIST *next_file = files;
    files=next_file;
    while(files!=NULL)
    {
        next_file = files->next;
        free(files->filename);
        free(files);
        files = next_file;
    }
}
