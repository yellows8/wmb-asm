#ifndef _H_WC24TYPES
#define _H_WC24TYPES

#ifdef HW_RVL
#include <gctypes.h>
#else
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <errno.h>

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned long long u64;
typedef signed int s32;
typedef signed short s16;
typedef signed char s8;
typedef signed long long s64;

struct _reent
{
	int _errno;
};

typedef struct _DIR_ITER
{
	void* dirStruct;
} DIR_ITER;

#endif

#endif

