#ifndef DIRENT_INCLUDED
#define DIRENT_INCLUDED

/*

    Declaration of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
    Rights:  See end of file.
    
*/
//Modified by yellowstar to be used in the Wmb Asm Module.

#ifdef __cplusplus
extern "C"
{
#endif

#include <io.h>

typedef struct DIR DIR;

struct dirent
{
    char *d_name;
};

#ifdef BUILDING_DLL
    DLLIMPORT DIR           *opendir(const char *);
    DLLIMPORT int           closedir(DIR *);
    DLLIMPORT struct dirent *readdir(DIR *);
    DLLIMPORT void          rewinddir(DIR *);
#endif

#ifndef BUILDING_DLL

    #ifndef NDS

        typedef DIR* (*lpopendir)(const char *dirname);
        typedef int (*lpclosedir)(DIR* dir);
        typedef struct dirent* (*lpreaddir)(DIR* dir);
        typedef void (*lprewinddir)(DIR* dir);

            #ifdef DLLMAIN
                lpopendir opendir=NULL;
                lpclosedir closedir=NULL;
                lpreaddir readdir=NULL;
                lprewinddir rewinddir=NULL;
            #endif

                    #ifndef DLLMAIN
                        extern lpopendir opendir;
                        extern lpclosedir closedir;
                        extern lpreaddir readdir;
                        extern lprewinddir rewinddir;
                    #endif

    #endif

#endif

/*

    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.
    
    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

*/

#ifdef __cplusplus
}
#endif

#endif
