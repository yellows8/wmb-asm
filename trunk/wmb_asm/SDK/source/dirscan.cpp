/*
Wmb Asm, the SDK, and all software in the Wmb Asm package are licensed under the MIT license:
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

#define BUILDING_SDK

#include "..\include\wmb_asm_sdk.h"

#define USE_MY_CODE

#pragma warning(disable:4786)
#include <windows.h>
#include <queue>
#include <sstream>
#include <iostream>
#include <algorithm>

// I think MS's names for some things are obnoxious.
const HANDLE HNULL = INVALID_HANDLE_VALUE;
const int A_DIR = FILE_ATTRIBUTE_DIRECTORY;

inline void AllocDir(FILE_LIST *files)
{

        files->filename = (char*)malloc(256);
        strcpy(files->filename,"");
        files->next = (FILE_LIST*)malloc(sizeof(FILE_LIST));
        memset(files->next,0,sizeof(FILE_LIST));
}

void process(std::string name, FILE_LIST *files) { 
    if(files->filename==NULL)AllocDir(files);
   strcpy(files->filename, name.c_str());
   
   printf("FOUND %s\n", files->filename);
}

void PrintFolder(std::string const &folder_name, 
                 std::string const &fmask, FILE_LIST *filelist) 
{
    HANDLE finder;          // for FindFirstFile
    WIN32_FIND_DATA file;   // data about current file.
    std::priority_queue<std::string, 
                        std::vector<std::string>, 
                        std::greater<std::string> 
                       > dirs;
    dirs.push(folder_name); // start with passed directory 

FILE_LIST *files = filelist;

    if(filelist!=NULL)
    {
        files = filelist;
        while(files->next!=NULL)
        {
            files = files->next;
        }

        if(files->filename!=NULL && files->next!=NULL)files = files->next;


    }

    FILE_LIST *cur_file = files;

    do {
        std::string path = dirs.top();// retrieve directory to search
        dirs.pop();

        if (path[path.size()-1] != '\\')  // normalize the name.
            path += "\\";

        std::string mask = path + fmask;    // create mask for searching

        // traverse a directory.
        if (HNULL==(finder=FindFirstFile(mask.c_str(), &file))) {
            continue;
        }
        do { 
            if (!(file.dwFileAttributes & A_DIR))
            {
                process(path + file.cFileName, cur_file);
                cur_file = cur_file->next;
            }
        } while (FindNextFile(finder, &file));
        FindClose(finder);
        if (HNULL==(finder=FindFirstFile((path + "*").c_str(), &file)))
            continue;
        do { 
            if ((file.dwFileAttributes & A_DIR) &&
                (file.cFileName[0] != '.'))
            {
                dirs.push(path + file.cFileName);
            }
        } while (FindNextFile(finder, &file));
        FindClose(finder);
    } while (!dirs.empty());
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

#ifndef USE_MY_CODE
DLLIMPORT FILE_LIST *ScanDirectory(FILE_LIST *filelist, char *dirname, char *ext = NULL)
{
    PrintFolder(dirname, "*", filelist);
    
    return filelist;
}
#endif

#ifdef USE_MY_CODE
DLLIMPORT FILE_LIST *ScanDirectory(FILE_LIST *filelist, char *dirname, char *ext = NULL)
{
    
    FILE_LIST *files=NULL;
    FILE_LIST *cur_files=NULL;
    DIR *dir = NULL;
    dirent *ent = NULL;
    char *DirName = (char*)malloc(256);
    strcpy(DirName,dirname);
    if(filelist!=NULL)
    {
        files = filelist;
        while(files->next!=NULL)
        {
            files = files->next;
        }
        
        if(files->filename!=NULL && files->next!=NULL)files = files->next;
        
        
    }
    
    cur_files = files;
    
    FILE *f = fopen(DirName,"rb");
    if(f!=NULL)
    {
        printf("FOUND BA %s\n",DirName);
        
        if(ext!=NULL)
        {
            if(strcmp((const char*)&ent->d_name[strlen(ent->d_name)-4],(const char*)ext)!=0)
            {
                return files;
            }
        }
        
        if(cur_files==NULL)
        {
            cur_files = (FILE_LIST*)malloc(sizeof(FILE_LIST));
            memset(cur_files,0,sizeof(FILE_LIST));
        }
        
        if(cur_files->filename==NULL)
        AllocDir(cur_files);
        
        fclose(f);
        strcpy(cur_files->filename,DirName);
        printf("FOUND %s\n",cur_files->filename);
        return files;
    }
    
    char c = DirName[strlen(DirName)];

    if(c!='/' && c!='\\')
    {
    sprintf(DirName,"%s\\",DirName);
    }
    
    dir=opendir(DirName);
    if(dir==NULL)
    {
        return NULL;
    }
    else
    {
        printf("Scanning directory %s\n",DirName);
    }
    
    char *str;
    str = (char*)malloc(256);
    strcpy(str,"");
    int skip=0;
    
    if(strcmp(DirName,".\\")==0)
    {
        /*char *temp = (char*)malloc(256);
        memset(temp, 0, 256);
        
        if(getcwd(temp, 255)==NULL)printf("ACK! GETCWD ERROR! %d\n",errno);
        
        sprintf(temp, "%s\\",temp);
        
        strcpy(DirName, temp);
        printf("Working directory is %s\n",temp);
        
        free(temp);*/
        
        //strcpy(DirName, "");
    }
    
    if(files==NULL)
    {
        if(files==NULL)
        {
            files = (FILE_LIST*)malloc(sizeof(FILE_LIST));
            memset(files,0,sizeof(FILE_LIST));
        }
        
        AllocDir(files);
        cur_files = files;
    }
    
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
                    {
                        printf("Failed to open file or directory %s\n",Str);
                    }
                    else
                    {
                        while(cur_files->next!=NULL)
                        {
                            cur_files = cur_files->next;
                        }

                        if(cur_files->filename!=NULL && cur_files->next!=NULL)cur_files = cur_files->next;
                    }
        
                free(Str);
            }
            else
            {
                fclose(f);
                
                printf("FOUND A %s\n",ent->d_name);
                
                    if(ext!=NULL)
                    {
                        if(strcmp((const char*)&ent->d_name[strlen(ent->d_name)-4],(const char*)ext)!=0)
                        {
                            continue;
                        }
                    }
        
                if(cur_files->filename==NULL)
                AllocDir(cur_files);
        
                strcpy(cur_files->filename,str);
                
                printf("FOUND B %s dir %s\n",ent->d_name, cur_files->filename);
                
                cur_files = cur_files->next;
            }
        }
    }
    
    closedir(dir);
    free(str);
    free(DirName);
    
    return files;
}
#endif
