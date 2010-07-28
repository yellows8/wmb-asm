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

#ifdef HW_RVL

#include <gctypes.h>
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "wc24.h"

NWC24MsgCfg *wc24mail_nwc24msgcfg;

s32 __WC24Mail_CfgRead(int which);
s32 __WC24Mail_CfgUpdate(int which);

u32 CalcMailCfgChecksum(void* buffer, u32 length)
{
	u32 *buf = (u32*)buffer;
	u32 sum = 0;
	while(length>0)
	{
		sum+=*buf;
		buf++;
		length-=4;
	}
	return sum;
}

s32 WC24Mail_Init()
{
	s32 retval;
	u32 cbk_checksum;
	wc24mail_nwc24msgcfg = memalign(32, sizeof(NWC24MsgCfg));
	if(wc24mail_nwc24msgcfg==NULL)return ENOMEM;
	memset(wc24mail_nwc24msgcfg, 0, sizeof(NWC24MsgCfg));

	retval = __WC24Mail_CfgRead(0);
	if(retval<0)
	{
		WC24Mail_Shutdown();
		return retval;
	}
	cbk_checksum = wc24mail_nwc24msgcfg->checksum;
	
	retval = __WC24Mail_CfgRead(1);
	if(retval<0)
	{
		WC24Mail_Shutdown();
		return retval;
	}

	if(cbk_checksum!=wc24mail_nwc24msgcfg->checksum)return WC24MAIL_EMISMATCHSUM;

	return 0;
}

void WC24Mail_Shutdown()
{
	if(wc24mail_nwc24msgcfg)free(wc24mail_nwc24msgcfg);
}

s32 __WC24Mail_CfgRead(int which)
{
	s32 fd;
	if(which==0)fd = ISFS_Open("/shared2/wc24/nwc24msg.cbk", ISFS_OPEN_RW);
	if(which==1)fd = ISFS_Open("/shared2/wc24/nwc24msg.cfg", ISFS_OPEN_RW);
	if(fd<0)return fd;

	ISFS_Read(fd, wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg));
	ISFS_Close(fd);

	if(CalcMailCfgChecksum(wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg)-4)!=wc24mail_nwc24msgcfg->checksum)return WC24MAIL_EINVALSUM;
	return 0;
}

s32 __WC24Mail_CfgUpdate(int which)
{
	s32 fd;
	if(which==0)fd = ISFS_Open("/shared2/wc24/nwc24msg.cbk", ISFS_OPEN_RW);
	if(which==1)fd = ISFS_Open("/shared2/wc24/nwc24msg.cfg", ISFS_OPEN_RW);
	if(fd<0)return fd;

	wc24mail_nwc24msgcfg->checksum = CalcMailCfgChecksum(wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg)-4);
	ISFS_Write(fd, wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg));
	ISFS_Close(fd);

	return 0;
}

s32 WC24Mail_CfgRead()
{
	return __WC24Mail_CfgRead(1);
}

s32 WC24Mail_CfgUpdate()
{
	s32 retval;
	retval = __WC24Mail_CfgUpdate(0);
	if(retval<0)return retval;
	retval = __WC24Mail_CfgUpdate(1);
	return retval;
}

s32 WC24Mail_WC24RecvMount()
{
	return VFF_Mount("/shared2/wc24/mbox/wc24recv.mbx");
}

s32 WC24Mail_WC24SendMount()
{
	return VFF_Mount("/shared2/wc24/mbox/wc24send.mbx");
}

s32 WC24Mail_WC24RecvCreate(u32 filesize)
{
	if(filesize==0)filesize = 0x700000;//7MB
	return VFF_CreateVFF("/shared2/wc24/mbox/wc24recv.mbx", filesize);
}

s32 WC24Mail_WC24SendCreate(u32 filesize)
{
	if(filesize==0)filesize = 0x200000;//2MB
	return VFF_CreateVFF("/shared2/wc24/mbox/wc24recv.mbx", filesize);
}



#endif

