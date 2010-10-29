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

#define WC24MAIL_FCTYPE_WII 1
#define WC24MAIL_FCTYPE_EMAIL 2
#define WC24MAIL_FCUNCONFIRMED 1//The confirm field is always WC24MAIL_FCUNCONFIRMED for type email.
#define WC24MAIL_FCCONFIRMED 2

typedef struct _sNWC24MsgCfg//See also: http://wiibrew.org/wiki//shared2/wc24/nwc24msg.cfg#File_structure
{
	u32 magic;
	u32 unk4;//Unknown, must always be 8.
	u64 friend_code;//Wii msg board friend code/"Wii ID"/NWC24 ID.
	u32 nwc24idgen;//see http://wiibrew.org/wiki//shared2/wc24/nwc24msg.cfg
	u32 id_registered;//"1 = ID is not registered yet, 2 = ID has been registered." see http://wiibrew.org/wiki//shared2/wc24/nwc24msg.cfg
	char email_domain[0x40];//The Wii E-Mail address domain, includes the '@'. 
	char unk58[0x20];
	char mlchkid[0x24];//This is the mlchkid string value used in HTTP(S) mail POST requests. 
	char urls[5][0x80];
	u8 reserved[0xdc];
	u32 wc24titleboot_enableflag;//Normally set to zero when sysmenu creates the config files. Enables WC24 title booting when non-zero, see: http://wiibrew.org/wiki/WiiConnect24#WC24_title_booting
	u32 checksum;
} NWC24MsgCfg;

typedef struct _sNWC24FlHeader//See also: http://wiibrew.org/wiki//shared2/wc24/nwc24fl.bin
{
	u32 magic;//0x5763466C "WcFl"
	u32 unk4;
	u32 max_entries;
	u32 num_entries;
	u8 pad[0x30];
} NWC24FlHeader;

typedef struct _sNWC24Fl_FC
{
	union
	{
		u64 wii_fc;
		char partial_email[8];//Contains some of the E-Mail address.
	};
} NWC24Fl_FC;

typedef struct _sNWC24Fl_Entry
{
	u32 type;
	u32 confirmed;
	u16 nickname[12];
	u32 mii_id;
	u32 system_id;
	u8 unk28[24];

	union
	{
		struct
		{
			u64 wii_fc;
			u8 pad[0x58];
		};

		struct
		{
			char email_address[0x60];
		};
	};
	u8 unka0[0xa0];
} NWC24Fl_Entry;

extern NWC24MsgCfg *wc24mail_nwc24msgcfg;
extern NWC24FlHeader *wc24mail_nwc24fl_hdr;
extern NWC24Fl_FC *wc24mail_nwc24fl_fc;
extern NWC24Fl_Entry *wc24mail_nwc24fl_entries;

s32 WC24Mail_Init();
void WC24Mail_Shutdown();
s32 WC24Mail_CfgRead();//Reads/updates the nwc24msg.cfg buffer in RAM.
s32 WC24Mail_CfgUpdate();//Updates/writes the NAND config files.

s32 WC24Mail_WC24RecvMount();//Mounts /shared2/wc24/mbox/wc24recv.mbx as device wc24recv.mbx, this stores recieved WC24 mail.
s32 WC24Mail_WC24SendMount();//Mounts /shared2/wc24/mbox/wc24send.mbx as device wc24send.mbx, mail that will be sent by KD is stored here.
s32 WC24Mail_WC24RecvCreate(u32 filesize, int delvff);//Normally doesn't need to be used, unless you completely replace sysmenu. Filesize is 7MB by default when input filesize is zero. See vff.h for the delvff param.
s32 WC24Mail_WC24SendCreate(u32 filesize, int delvff);//Filesize is 2MB by default when input filesize is zero. See vff.h for the delvff param.

s32 WC24Mail_Read();//Updates the read nwc24fl.bin buffers from NAND.
s32 WC24Mail_FlUpdate();//Writes the nwc24fl.bin struct buffers to NAND.

#ifdef __cplusplus
   }
#endif

#endif

