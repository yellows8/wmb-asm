/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the �Software�), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED �AS IS�, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef _H_DIRSCAN
#define _H_DIRSCAN

#ifdef __cplusplus
  extern "C" {
#endif

typedef struct TFILE_LIST
{
    char *filename;
    TFILE_LIST *next;
} __attribute__ ((__packed__)) FILE_LIST;

#ifdef BUILDING_DLL
    DLLIMPORT FILE_LIST *ScanDirectory(FILE_LIST *filelist, char *dirname, char *ext = NULL);
    DLLIMPORT void FreeDirectory(FILE_LIST *filelist);
#endif

#ifndef BUILDING_DLL

    #ifndef NDS

        typedef FILE_LIST * (*lpScanDirectory)(FILE_LIST *filelist, char *dirname, char *ext);
        typedef void (*lpFreeDirectory)(FILE_LIST* dir);

            #ifdef DLLMAIN
                lpScanDirectory ScanDirectory=NULL;
                lpFreeDirectory FreeDirectory=NULL;
            #endif

                    #ifndef DLLMAIN
                        extern lpScanDirectory ScanDirectory;
                        extern lpFreeDirectory FreeDirectory;
                    #endif

    #endif

#endif

#ifdef __cplusplus
  }
#endif

#endif
