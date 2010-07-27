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

#ifndef _H_KD
#define _H_KD

#include "types.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define KD_DOWNLOADFLAGS_MANUAL BIT(1)//Specify a entry to download, must be set when used from Broadway.
#define KD_DOWNLOADFLAGS_SUBTASKS BIT(2)//Set this to download subTasks.
#define KD_DOWNLOADFLAGS_UTCSYNC BIT(31)//Sync UTC time with time server. KD syncs time when KD_Download flags is negative.

//These errors can be returned by KD_Download.
#define KD_ESIGFAIL -45//RSA signature verification failed. Or KD_CreateRSA failed.
#define KD_EHTTP -32//HTTP error or time server error?
#define KD_ECACHE -15//"Detected AKAMAI's cache refreshing."
#define KD_EINVAL -3//Invalid input.
#define KD_ESHUTDOWN -48//Only happens when STM_Wakeup needs to be called by KD: "Shutdown required. Quit processing."
#define KD_ENOSPACE -6//No space is available in VFF.

typedef struct _skd_timebuf
{
	s32 retval;//Zero for success, -30 when "Universal time is not ready.".
	u64 time;//UTC time in seconds.
} kd_timebuf;

s32 KD_Open();//KD_Open and KD_Close are called by WC24_Init and WC24_Shutdown.
s32 KD_Close();

s32 KD_GetUTCTime(kd_timebuf *time);//If KD_Open wasn't called when any of these KD ioctl functions are called, LIBWC24_EINIT will be returned.
s32 KD_SetUTCTime(kd_timebuf *time);
s32 KD_CorrectRTC(u64 diff);

s32 KD_SaveMail();//Should be called after downloading mail immediately. This saves the dlcnt.bin mail content to nwc24recv.mbx, if more mail would be downloaded immediately dlcnt.bin mail content would be overwritten without saving it.
s32 KD_Download(s32 flags, u16 index, u32 subTaskBitmap);//flags is the KD_DOWNLOADFLAG defines, index is nwc24dl_entry index, subTaskBitmap is a bitmap of subTasks to download starting at bit zero for subTask zero when flags KD_DOWNLOADFLAGS_SUBTASKS is set.
s32 KD_SetNextWakeup(u32 seconds);//Sets the next time STM_Wakeup is called by KD relative to the current UTC time.
s32 KD_GetTimeTriggers(u32 *triggers);//Input should be a buffer 8 bytes long. Gets the minutes of the hour for when KD_Download and KD_CheckMail is automatically triggered.

#ifdef __cplusplus
   }
#endif

#endif
