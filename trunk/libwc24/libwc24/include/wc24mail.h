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

#ifndef _H_WC24MAIL
#define _H_WC24MAIL

#include "types.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define WC24MAIL_EINVALSUM -0x3001//Returned when config file(s) read by WC24Mail_Init or WC24Mail_Read have an invalid checksum.
#define WC24MAIL_EMISMATCHSUM -0x3002//Returned by WC24Mail_Init when the two config files' checksums don't match.

typedef struct _sNWC24MsgCfg//See also: http://wiibrew.org/wiki//shared2/wc24/nwc24msg.cfg#File_structure
{
	u32 magic;
	u32 unk4;//Unknown, must always be 8.
	u64 friend_code;//Wii msg board friend code/"Wii ID".
	u32 unk10;//Unknown, must always be less than 0x20.
	u32 unk14;//Unknown, usually 2? 
	char email_domain[0x40];//The Wii E-Mail address domain, includes the '@'. 
	char unk58[0x20];
	char mlchkid[0x24];//This is the mlchkid string value used in HTTP(S) mail POST requests. 
	char urls[5][0x80];
	u8 reserved[0xdc];
	u32 wc24titleboot_enableflag;//Normally set to zero when sysmenu creates the config files. Enables WC24 title booting when non-zero, see: http://wiibrew.org/wiki/WiiConnect24#WC24_title_booting
	u32 checksum;
} NWC24MsgCfg;
extern NWC24MsgCfg *wc24mail_nwc24msgcfg;

s32 WC24Mail_Init();
void WC24Mail_Shutdown();
s32 WC24Mail_CfgRead();//Reads/updates the nwc24msg.cfg buffer in RAM.
s32 WC24Mail_CfgUpdate();//Updates/writes the NAND config files.

#ifdef __cplusplus
   }
#endif

#endif

