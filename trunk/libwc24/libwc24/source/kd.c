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

#include <gctypes.h>
#include <gccore.h>
#include <stdio.h>
#include <string.h>

#include "wc24.h"
#include "kd.h"

s32 __kdtime_fd, __kdreq_fd;

#define IOCTL_KD_GETUTCTIME 0x14
#define IOCTL_KD_SETUTCTIME 0x15
#define IOCTL_KD_CORRECTRTC 0x17

#define IOCTL_KD_GETTIMETRIGGERS 0x4
#define IOCTL_KD_DOWNLOAD 0xe
#define IOCTL_KD_SETNEXTWAKEUP 0x21

s32 KD_Open()
{
    __kdtime_fd = IOS_Open("/dev/net/kd/time", 0);
    if(__kdtime_fd<0)
    {
        printf("Failed to open %s\n", "/dev/net/kd/time");
        return __kdtime_fd;
    }

    __kdreq_fd = IOS_Open("/dev/net/kd/request", 1);
    if(__kdreq_fd<0)
    {
	IOS_Close(__kdtime_fd);
        printf("Failed to open %s\n", "/dev/net/kd/request");
        return __kdreq_fd;
    }
    return 0;
}

s32 KD_Close()
{
    IOS_Close(__kdtime_fd);
    IOS_Close(__kdreq_fd);
    __kdtime_fd = 0;
    __kdreq_fd = 0;
    return 0;
}

// -- /dev/net/kd/time --

s32 KD_GetUTCTime(kd_timebuf *time)
{
    if(__kdtime_fd==0)return LIBWC24_EINIT;
    s32 retval = IOS_Ioctl(__kdtime_fd,IOCTL_KD_GETUTCTIME,NULL,0,time,12);
    DCInvalidateRange(time, sizeof(kd_timebuf));
    return retval;
}

s32 KD_SetUTCTime(kd_timebuf *time)
{
    if(__kdtime_fd==0)return LIBWC24_EINIT;
    DCFlushRange(time, sizeof(kd_timebuf));
    s32 retval = IOS_Ioctl(__kdtime_fd,IOCTL_KD_SETUTCTIME,time,12,time,12);
    DCInvalidateRange(time, sizeof(kd_timebuf));
    return retval;
}

s32 KD_CorrectRTC(u64 diff)
{
    if(__kdtime_fd==0)return LIBWC24_EINIT;
    kd_timebuf time;
    DCFlushRange(&time, sizeof(kd_timebuf));
    s32 retval = IOS_Ioctl(__kdtime_fd,IOCTL_KD_SETUTCTIME,&diff,8,&time,12);
    DCInvalidateRange(&time, sizeof(kd_timebuf));
    return retval;
}

// -- /dev/net/kd/request --

s32 KD_Download(u32 flags, u16 index, u32 subTaskBitmap)
{
	s32 retval;
	u32 inbuf[8];
	u32 outbuf[8];
	
        if(__kdreq_fd==0)return LIBWC24_EINIT;
	memset(inbuf, 0, 32);
	memset(outbuf, 0, 32);
	inbuf[0] = flags;
	inbuf[1] = (u32)index;
	inbuf[2] = subTaskBitmap;
	DCFlushRange(inbuf, 32);
	DCFlushRange(outbuf, 32);

	retval = IOS_Ioctl(__kdreq_fd,IOCTL_KD_DOWNLOAD,inbuf,32,outbuf,32);
	DCInvalidateRange(outbuf, 32);
	if(retval==0)retval = outbuf[0];
	return retval;
}

s32 KD_SetNextWakeup(u32 seconds)
{
	s32 retval;
	u32 inbuf[1];
	u32 outbuf[1];
	
        if(__kdreq_fd==0)return LIBWC24_EINIT;
	memset(inbuf, 0, 4);
	memset(outbuf, 0, 4);
	inbuf[0] = seconds;
	DCFlushRange(inbuf, 4);
	DCFlushRange(outbuf, 4);

	retval = IOS_Ioctl(__kdreq_fd,IOCTL_KD_SETNEXTWAKEUP,inbuf,32,outbuf,32);
	DCInvalidateRange(outbuf, 4);
	if(retval==0)retval = outbuf[0];
	return retval;
}

s32 KD_GetTimeTriggers(u32 *triggers)
{
	s32 retval;
	u32 inbuf[1];
	u32 outbuf[3];
	
        if(__kdreq_fd==0)return LIBWC24_EINIT;
	memset(inbuf, 0, 4);
	memset(outbuf, 0, 12);
	DCFlushRange(inbuf, 4);
	DCFlushRange(outbuf, 12);

	retval = IOS_Ioctl(__kdreq_fd,IOCTL_KD_GETTIMETRIGGERS,inbuf,4,outbuf,12);
	DCInvalidateRange(inbuf, 4);
	DCInvalidateRange(outbuf, 12);
	triggers[0] = outbuf[1];
	triggers[1] = outbuf[2];
	if(retval==0)retval = outbuf[0];
	return retval;
}

