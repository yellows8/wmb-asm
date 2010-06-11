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

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <string.h>
#include <malloc.h>

#include "wc24.h"
#include "id.h"

u32 wc24_did_init = 0;
s32 nwc24dlbin_fd = 0;
nwc24dl_header *NWC24DL_Header;
u64 wc24_titleid = 0x000100014a4f4449LL;

s32 WC24_Init(int id)
{
	if(wc24_did_init)return LIBWC24_EINIT;
	s32 retval = ISFS_Initialize();
	if(retval<0)return retval;
	
	if(id)
	{
		retval = identify_title("00010001", "4a4f4449");
		if(retval<0)return retval;
	}	

	retval = WC24_OpenNWC4DLBin();
	if(retval<0)return retval;
	
	NWC24DL_Header = (nwc24dl_header*)memalign(32, sizeof(nwc24dl_header));
	if(NWC24DL_Header==NULL)
	{
		printf("Failed to allocate hdr buf.\n");
		ISFS_Close(nwc24dlbin_fd);
		nwc24dlbin_fd = 0;
		return 0;
	}
	retval = ISFS_Seek(nwc24dlbin_fd, 0, SEEK_SET);
	if(retval<0)return retval;
	retval = ISFS_Read(nwc24dlbin_fd, NWC24DL_Header, sizeof(nwc24dl_header));
	if(retval<0)return retval;
	retval = KD_Open();
	if(retval<0)
	{
		printf("KD_Open failed %d\n", retval);
		WC24_CloseNWC4DLBin();
		return retval;
	}

	wc24_did_init = 1;
	retval = ES_GetTitleID(&wc24_titleid);
	if(retval<0)
	{
		printf("ES_GetTitleID returned %d\n", retval);
		//return retval;
	}

	return WC24_CloseNWC4DLBin();
}

s32 WC24_Shutdown()
{
	s32 retval = 0;
	if(!wc24_did_init)return LIBWC24_EINIT;
	wc24_did_init = 0;
	free(NWC24DL_Header);
	retval = KD_Close();
	if(retval<0)
	{
		printf("KD_Close failed: %d\n", retval);
		if(nwc24dlbin_fd)WC24_CloseNWC4DLBin();
		return retval;
	}
	if(nwc24dlbin_fd)retval = WC24_CloseNWC4DLBin();
	return retval;
}

s32 WC24_OpenNWC4DLBin()
{
	nwc24dlbin_fd = ISFS_Open("/shared2/wc24/nwc24dl.bin", ISFS_OPEN_RW);
	return nwc24dlbin_fd;
}

s32 WC24_CloseNWC4DLBin()
{
	s32 retval = ISFS_Close(nwc24dlbin_fd);
	nwc24dlbin_fd = 0;
	return retval;
}

s32 WC24_ReadRecord(u32 index, nwc24dl_record *rec)
{
	u32 open = 1;
	s32 retval;
	u32 *buf = (u32*)memalign(32, 0x20);
	if(buf==NULL)
	{
		printf("WC24_ReadRecord buf alloc fail.\n");
		return 0;
	}
	if(nwc24dlbin_fd)open = 0;
	if(open)
	{
		retval = WC24_OpenNWC4DLBin();
		if(retval<0)
		{
			free(buf);
			return retval;
		}
	}
	
	retval = ISFS_Seek(nwc24dlbin_fd, 0x80 + (index * sizeof(nwc24dl_record)), SEEK_SET);
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	retval = ISFS_Read(nwc24dlbin_fd, buf, sizeof(nwc24dl_record));
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	memcpy(rec, buf, sizeof(nwc24dl_record));
	free(buf);

	if(open)
	{
		retval = WC24_CloseNWC4DLBin();
		if(retval<0)return retval;
	}
	return 0;
}

s32 WC24_WriteRecord(u32 index, nwc24dl_record *rec)
{
	u32 open = 1;
	s32 retval;
	u32 *buf = (u32*)memalign(32, 0x20);
	if(nwc24dlbin_fd)open = 0;
	if(open)
	{
		retval = WC24_OpenNWC4DLBin();
		if(retval<0)
		{
			free(buf);
			return retval;
		}
	}
	
	memcpy(buf, rec, sizeof(nwc24dl_record));
	retval = ISFS_Seek(nwc24dlbin_fd, 0x80 + (index * sizeof(nwc24dl_record)), SEEK_SET);
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	retval = ISFS_Write(nwc24dlbin_fd, buf, sizeof(nwc24dl_record));
	free(buf);
	if(retval<0)
	{
		return retval;
	}
	
	if(open)
	{
		retval = WC24_CloseNWC4DLBin();
		if(retval<0)return retval;
	}
	return 0;
}

s32 WC24_ReadEntry(u32 index, nwc24dl_entry *ent)
{
	u32 open = 1;
	s32 retval;
	u32 *buf = (u32*)memalign(32, sizeof(nwc24dl_entry));
	if(nwc24dlbin_fd)open = 0;
	if(open)
	{
		retval = WC24_OpenNWC4DLBin();
		if(retval<0)
		{
			free(buf);
			return retval;
		}
	}
	
	retval = ISFS_Seek(nwc24dlbin_fd, 0x800 + (index * sizeof(nwc24dl_entry)), SEEK_SET);
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	retval = ISFS_Read(nwc24dlbin_fd, buf, sizeof(nwc24dl_entry));
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	memcpy(ent, buf, sizeof(nwc24dl_entry));
	free(buf);	

	if(open)
	{
		retval = WC24_CloseNWC4DLBin();
		if(retval<0)return retval;
	}
	return 0;
}

s32 WC24_WriteEntry(u32 index, nwc24dl_entry *ent)
{
	u32 open = 1;
	s32 retval;
	u32 *buf = (u32*)memalign(32, sizeof(nwc24dl_entry));
	if(nwc24dlbin_fd)open = 0;
	if(open)
	{
		retval = WC24_OpenNWC4DLBin();
		if(retval<0)
		{
			free(buf);
			return retval;
		}
	}
	
	memcpy(buf, ent, sizeof(nwc24dl_entry));
	retval = ISFS_Seek(nwc24dlbin_fd, 0x800 + (index * sizeof(nwc24dl_entry)), SEEK_SET);
	if(retval<0)
	{
		free(buf);
		return retval;
	}
	retval = ISFS_Write(nwc24dlbin_fd, buf, sizeof(nwc24dl_entry));
	free(buf);
	if(retval<0)
	{
		return retval;
	}
	
	if(open)
	{
		retval = WC24_CloseNWC4DLBin();
		if(retval<0)return retval;
	}
	return 0;
}

s32 WC24_FindRecord(u32 id, nwc24dl_record *rec)
{
	s32 retval = 0;
	u32 found = 0;
	if(!wc24_did_init)return LIBWC24_EINIT;
	u32 i;
	for(i=0; i<NWC24DL_Header->max_entries; i++)
	{
		retval = WC24_ReadRecord(i, rec);
		if(retval<0)break;
		if(rec->ID==id)
		{
			found = 1;
			retval = i;
			break;
		}
	}
	if(!found && retval==0)retval = LIBWC24_ENOENT;
	
	return retval;
}

s32 WC24_FindEntry(u32 id, char *url, nwc24dl_entry *ent)
{
	s32 retval = 0;
	u32 found = 0;
	if(!wc24_did_init)return LIBWC24_EINIT;
	u32 i;
	for(i=0; i<NWC24DL_Header->max_entries; i++)
	{
		retval = WC24_ReadEntry(i, ent);
		if(retval<0)break;
		if(ent->ID==id)
		{
			if(url)
			{
				if(strncmp(ent->url, url, 0xec)==0)
				{
					found = 1;
					retval = i;
					break;
				}
			}
			else
			{
				found = 1;
				retval = i;
				break;
			}
		}
	}
	if(!found && retval==0)retval = LIBWC24_ENOENT;
	
	return retval;
}

s32 WC24_FindEmptyRecord(nwc24dl_record *rec)
{
	return WC24_FindRecord(0, rec);
}

s32 WC24_FindEmptyEntry(nwc24dl_entry *ent)
{
	nwc24dl_record rec;
	s32 retval = WC24_FindEmptyRecord(&rec);
	if(retval<0)return retval;
	s32 rval = WC24_ReadEntry((u32)retval, ent);
	if(rval<0)return rval;
	return retval;
}

s32 WC24_CreateRecord(nwc24dl_record *rec, nwc24dl_entry *ent, u32 id, u64 titleid, u16 group_id, u8 type, u8 record_flags, u32 flags, u16 dl_freq_perday, u16 dl_freq_days, char *url, char *filename)
{
	s32 retval = -1;
	u32 index;
	if(id==0 && titleid==0)retval = WC24_FindEntry(id, url, ent);
	if(retval<0)
	{
		if(retval==LIBWC24_ENOENT)
		{
			retval = WC24_FindEmptyEntry(ent);
		}
		else
		{
			return retval;
		}
	}
	else
	{
		memset(ent, 0, sizeof(nwc24dl_entry));
	}
	if(id==0)id = (u32)wc24_titleid;
	if(titleid==0)titleid = wc24_titleid;
	index = (u32)retval;
	rec->ID = id;
	rec->next_dl = 0;
	rec->last_modified = 0;
	rec->record_flags = WC24_RECORD_FLAGS_DEFAULT;
	memset(ent, 0, sizeof(nwc24dl_entry));
	ent->index = (u32)index;
	ent->type = type;
	ent->record_flags = WC24_RECORD_FLAGS_DEFAULT;
	ent->flags = flags;
	ent->ID = id;
	ent->titleid = titleid;
	ent->group_id = group_id;
	ent->unk18 = 0x17;//Varies, from HATE.
	ent->dl_freq_perday = dl_freq_perday;//Download hourly.
	ent->dl_freq_days = dl_freq_days;//Download daily.
	strncpy(ent->url, url, 0xec);
	if(type==WC24_TYPE_TITLEDATA)strncpy(ent->filename, filename, 0x40);

	retval = WC24_WriteRecord(index, rec);
	if(retval<0)return retval;
	retval = WC24_WriteEntry(index, ent);
	if(retval<0)return retval;
	return index;
}

s32 WC24_DeleteRecord(u32 index)
{
	s32 retval;
	nwc24dl_record rec;
	nwc24dl_entry ent;
	memset(&rec, 0, sizeof(nwc24dl_record));
	memset(&ent, 0, sizeof(nwc24dl_entry));
	ent.index = (u16)index;
	ent.type = WC24_TYPE_EMPTY;
	
	retval = WC24_WriteRecord(index, &rec);
	if(retval<0)return retval;
	retval = WC24_WriteEntry(index, &ent);
	if(retval<0)return retval;
	return 0;
}

s32 WC24_CreateWC24DlVFF(u32 filesize)
{
	s32 retval;
	char *filename = (char*)memalign(32, 256);
	memset(filename, 0, 256);
	retval = ES_GetDataDir(wc24_titleid, filename);
	if(retval<0)
	{
		free(filename);
		return retval;
	}
	strcat(filename, "/wc24dl.vff");
	retval = VFF_CreateVFF(filename, filesize);
	free(filename);
	return retval;
}

s32 WC24_MountWC24DlVFF()
{
	s32 retval;
	char *filename = (char*)memalign(32, 256);
	memset(filename, 0, 256);
	retval = ES_GetDataDir(wc24_titleid, filename);
	if(retval<0)
	{
		free(filename);
		return retval;
	}
	strcat(filename, "/wc24dl.vff");
	retval = VFF_Mount(filename);
	free(filename);
	return retval;
}

time_t WC24_TimestampToSeconds(u32 timestamp)
{
	return timestamp * 60;
}

u32 WC24_SecondsToTimestamp(time_t timestamp)
{
	return timestamp / 60;
}

void WC24_SetTitleID(u64 titleid)
{
	wc24_titleid = titleid;
}

u64 WC24_GetTitleID()
{
	return wc24_titleid;
}

