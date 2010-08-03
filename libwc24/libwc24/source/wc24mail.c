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
NWC24FlHeader *wc24mail_nwc24fl_hdr = NULL;
NWC24Fl_FC *wc24mail_nwc24fl_fc;
NWC24Fl_Entry *wc24mail_nwc24fl_entries;
s32 wc24mail_flmaxentries = -1;

s32 __WC24Mail_CfgRead(int which);
s32 __WC24Mail_CfgUpdate(int which);
s32 __WC24Mail_FlInit();
void __WC24Mail_FlShutdown();

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
	retval = __WC24Mail_FlInit();
	if(retval<0)return retval;

	return 0;
}

void WC24Mail_Shutdown()
{
	if(wc24mail_nwc24msgcfg)free(wc24mail_nwc24msgcfg);
	wc24mail_nwc24msgcfg = NULL;
	__WC24Mail_FlShutdown();
}

s32 __WC24Mail_CfgRead(int which)
{
	s32 fd = 0;
	if(which==0)fd = ISFS_Open("/shared2/wc24/nwc24msg.cbk", ISFS_OPEN_RW);
	if(which)fd = ISFS_Open("/shared2/wc24/nwc24msg.cfg", ISFS_OPEN_RW);
	if(fd<0)return fd;

	ISFS_Read(fd, wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg));
	ISFS_Close(fd);

	if(CalcMailCfgChecksum(wc24mail_nwc24msgcfg, sizeof(NWC24MsgCfg)-4)!=wc24mail_nwc24msgcfg->checksum)return WC24MAIL_EINVALSUM;
	return 0;
}

s32 __WC24Mail_CfgUpdate(int which)
{
	s32 fd = 0;
	if(which==0)fd = ISFS_Open("/shared2/wc24/nwc24msg.cbk", ISFS_OPEN_RW);
	if(which)fd = ISFS_Open("/shared2/wc24/nwc24msg.cfg", ISFS_OPEN_RW);
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
	return VFF_Mount("/shared2/wc24/mbox/wc24recv.mbx", NULL);
}

s32 WC24Mail_WC24SendMount()
{
	return VFF_Mount("/shared2/wc24/mbox/wc24send.mbx", NULL);
}

s32 WC24Mail_WC24RecvCreate(u32 filesize, int delvff)
{
	if(filesize==0)filesize = 0x700000;//7MB
	return VFF_CreateVFF("/shared2/wc24/mbox/wc24recv.mbx", filesize, delvff);
}

s32 WC24Mail_WC24SendCreate(u32 filesize, int delvff)
{
	if(filesize==0)filesize = 0x200000;//2MB
	return VFF_CreateVFF("/shared2/wc24/mbox/wc24recv.mbx", filesize, delvff);
}

s32 __WC24Mail_FlInit()
{
	WC24Mail_Read();

	return 0;
}

void __WC24Mail_FlShutdown()
{
	if(wc24mail_nwc24fl_hdr)free(wc24mail_nwc24fl_hdr);
	if(wc24mail_nwc24fl_fc)free(wc24mail_nwc24fl_fc);
	if(wc24mail_nwc24fl_entries)free(wc24mail_nwc24fl_entries);
	wc24mail_nwc24fl_hdr = NULL;
	wc24mail_nwc24fl_fc = NULL;
	wc24mail_nwc24fl_entries = NULL;
}

s32 WC24Mail_Read()
{
	s32 fd;
	if(wc24mail_nwc24fl_hdr==NULL)wc24mail_nwc24fl_hdr = (NWC24FlHeader*)memalign(32, sizeof(NWC24FlHeader));
	if(wc24mail_nwc24fl_hdr==NULL)return ENOMEM;
	memset(wc24mail_nwc24fl_hdr, 0, sizeof(NWC24FlHeader));

	fd = ISFS_Open("/shared2/wc24/nwc24fl.bin", ISFS_OPEN_RW);
	if(fd<0)return fd;

	ISFS_Read(fd, wc24mail_nwc24fl_hdr, sizeof(NWC24FlHeader));
	if(wc24mail_flmaxentries==-1)wc24mail_flmaxentries = (s32)wc24mail_nwc24fl_hdr->max_entries;
	if(wc24mail_flmaxentries != (s32)wc24mail_nwc24fl_hdr->max_entries)
	{
		if(wc24mail_nwc24fl_fc)free(wc24mail_nwc24fl_fc);
		if(wc24mail_nwc24fl_entries)free(wc24mail_nwc24fl_entries);
		wc24mail_nwc24fl_fc = NULL;
		wc24mail_nwc24fl_entries = NULL;
		wc24mail_flmaxentries = (s32)wc24mail_nwc24fl_hdr->max_entries;
	}

	if(wc24mail_nwc24fl_fc==NULL)wc24mail_nwc24fl_fc = (NWC24Fl_FC*)memalign(32, sizeof(NWC24Fl_FC) * wc24mail_nwc24fl_hdr->max_entries);
	if(wc24mail_nwc24fl_entries==NULL)wc24mail_nwc24fl_entries = (NWC24Fl_Entry*)memalign(32, sizeof(NWC24Fl_Entry) * wc24mail_nwc24fl_hdr->max_entries);
	if(wc24mail_nwc24fl_fc==NULL || wc24mail_nwc24fl_entries==NULL)
	{
		ISFS_Close(fd);
		__WC24Mail_FlShutdown();
		return ENOMEM;
	}
	memset(wc24mail_nwc24fl_fc, 0, sizeof(NWC24Fl_FC) * wc24mail_nwc24fl_hdr->max_entries);
	memset(wc24mail_nwc24fl_entries, 0, sizeof(NWC24Fl_Entry) * wc24mail_nwc24fl_hdr->max_entries);

	ISFS_Read(fd, wc24mail_nwc24fl_fc, sizeof(NWC24Fl_FC) * wc24mail_nwc24fl_hdr->max_entries);
	ISFS_Read(fd, wc24mail_nwc24fl_entries, sizeof(NWC24Fl_Entry) * wc24mail_nwc24fl_hdr->max_entries);
	ISFS_Close(fd);

	return 0;
}

s32 WC24Mail_FlUpdate()
{
	s32 fd;
	
	fd = ISFS_Open("/shared2/wc24/nwc24fl.bin", ISFS_OPEN_RW);
	if(fd<0)return fd;

	ISFS_Write(fd, wc24mail_nwc24fl_hdr, sizeof(NWC24FlHeader));
	ISFS_Write(fd, wc24mail_nwc24fl_fc, sizeof(NWC24Fl_FC) * wc24mail_nwc24fl_hdr->max_entries);
	ISFS_Write(fd, wc24mail_nwc24fl_entries, sizeof(NWC24Fl_Entry) * wc24mail_nwc24fl_hdr->max_entries);

	ISFS_Close(fd);
	
	return 0;
}

#endif

