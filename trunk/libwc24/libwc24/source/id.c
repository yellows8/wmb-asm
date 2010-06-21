//id.c
//Copyright (C) 2008 by raven
//This is Free Software released under the GNU/GPL License.

#ifdef HW_RVL//This define was added by yellowstar6, for Linux libwc24 compatibility.

#include <stdio.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>

#include "certs_bin.h"
#include "su_tik_bin.h"
#include "su_tmd_bin.h"
//#include "menu_tik_bin.h"
//#include "menu_tmd_bin.h"

u64 build_id(char *title_hi, char *title_lo)
{
	u64 titleID = 0;
	char buf[100];
	sprintf(buf, "%s%s", title_hi, title_lo);
	sscanf(buf, "%016llx", &titleID);
	return titleID;
}

//Borrowed from tona's AnyRegion_Changer.
//Original name was ISFS_ReadFileToArray.
s32 readfile(char *filepath, u8 *filearray, u32 max_size, u32 *file_size)
{
	s32 ret, fd;
	static fstats filestats ATTRIBUTE_ALIGN(32);
	
	*file_size = 0;
	ret = ISFS_Open(filepath, ISFS_OPEN_READ);
	if (ret <= 0)
	{
		printf("Error! ISFS_Open (result = %d)\n", ret);
		return -1;
	}
	
	fd = ret;
	
	ret = ISFS_GetFileStats(fd, &filestats);
	if (ret < 0)
	{
		printf("Error! ISFS_GetFileStats (result = %d)\n", ret);
		return -1;
	}
	
	*file_size = filestats.file_length;
	
	if (*file_size > max_size)
	{
		printf("File is too large! Size: %u Max: %u\n", *file_size, max_size);
		return -1;
	}
	
	ret = ISFS_Read(fd, filearray, *file_size);
	*file_size = ret;
	if (ret < 0)
	{
		printf("Error! ISFS_Read (result = %d)\n", ret);
		return -1;
	}
	else if (ret != filestats.file_length)
	{
		printf("Error! ISFS_Read Only read: %d\n", ret);
		return -1;
	}
	
	ret = ISFS_Close(fd);
	if (ret < 0)
	{
		printf("Error! ISFS_Close (result = %d)\n", ret);
		return -1;
	}
	return 0;
}

s32 identify_SU()
{
	s32 res;
	//Identify as su.
	printf("Identifying as SU... ");
	res = ES_Identify((signed_blob *)certs_bin, certs_bin_size, (signed_blob *)su_tmd_bin, su_tmd_bin_size, (signed_blob *)su_tik_bin, su_tik_bin_size, NULL);
	if(res < 0)
	{
		printf("Error! (result = %d)\n", res);
		return -1;
	}
	printf("Done!\n");
	return 0;
}

/*s32 identify_menu()
{
	u64 titleID = 0;
	u32 tmdSize;
	s32 res;
	signed_blob *ptmd;
	
	//Identify as menu.
	printf("Identifying as menu (1/2)... ");
	res = ES_Identify((signed_blob*)certs_bin, certs_bin_size, (signed_blob*)menu_tmd_bin, menu_tmd_bin_size, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	if(res < 0)
	{
		printf("Error! (result = %d)\n", res);
		return -1;
	}
	printf("Done!\n");
	
	//Get current title ID.
	ES_GetTitleID(&titleID);
	if(titleID != 0x0000000100000002ULL)
	{
		printf("Error! (Title ID = %016llx)\n", titleID);
		return -1;
	}
	
	//Get tmd size.
	res = ES_GetStoredTMDSize(titleID, &tmdSize);
	if(res < 0)
	{
		printf("ES_GetStoredTMDSize: Error! (result = %d)\n", res);
		return -1;
	}
	
	//Get tmd.
	ptmd = memalign(32, tmdSize);
	res = ES_GetStoredTMD(titleID, ptmd, tmdSize);
	if(res < 0)
	{
		printf("ES_GetStoredTMD: Error! (result = %d)\n", res);
		return -1;
	}
	
	//Identify as menu.
	printf("Identifying as menu (2/2)... ");
	res = ES_Identify((signed_blob*)certs_bin, certs_bin_size, ptmd, tmdSize, (signed_blob*)menu_tik_bin, menu_tik_bin_size, NULL);
	if(res < 0)
	{
		printf("Error! (result = %d)\n", res);
		return -1;
	}
	printf("Done!\n");
	
	free(ptmd);
	
	return 0;
}*/

s32 identify_title(char *title_hi, char *title_lo)
{
	u64 titleID;
	u32 tmdSize;
	s32 res;
	signed_blob *ptmd;
	u8 *tik;
	u32 tik_size;
	char buf[ISFS_MAXPATH + 1];
	
	//identify_SU();
	
	titleID = build_id(title_hi, title_lo);
	
	//Get tmd size.
	res = ES_GetStoredTMDSize(titleID, &tmdSize);
	if(res < 0)
	{
		printf("ES_GetStoredTMDSize: Error! (result = %d)\n", res);
		return -1;
	}
	
	//Get tmd.
	ptmd = memalign(32, tmdSize);
	res = ES_GetStoredTMD(titleID, ptmd, tmdSize);
	if(res < 0)
	{
		printf("ES_GetStoredTMD: Error! (result = %d)\n", res);
		return -1;
	}
	
	//Get ticket.
	tik = memalign(32, STD_SIGNED_TIK_SIZE * 4);
	sprintf(buf, "/ticket/%s/%s.tik", title_hi, title_lo);
	res = readfile(buf, tik, STD_SIGNED_TIK_SIZE * 4, &tik_size);
	if(res < 0)
	{
		printf("Error reading ticket!\n");
		return -1;
	}
	
	//Identify.
	printf("Identifying as %016llx... ", titleID);
	/*res = ES_Identify((signed_blob*)certs_bin, certs_bin_size, ptmd, tmdSize, (signed_blob*)tik, tik_size, NULL);
	if(res < 0)
	{
		printf("Error! (result = %d)\n", res);
		return -1;
	}*/
	printf("Done!\n");
	
	free(ptmd);
	free(tik);
	
	return 0;
}

s32 launch_title(u64 titleID)
{
	u32 tvcnt;
	s32 res;
	tikview *views;
	
	identify_SU();
	
	res = ES_GetNumTicketViews(titleID, &tvcnt);
	if(res < 0)
	{
		printf("ES_GetNumTicketViews: Error! (result = %d)\n", res);
		return -1;
	}
	
	views = memalign(32, sizeof(tikview));
	res = ES_GetTicketViews(titleID, views, tvcnt);
	if(res < 0)
	{
		printf("ES_GetTicketViews: Error! (result = %d)\n", res);
		free(views);
		return -1;
	}
	
	printf("Launching...\n");
	res = ES_LaunchTitle(titleID, views);
	if(res < 0)
	{
		printf("ES_LaunchTitle: Error! (result = %d)\n", res);
		free(views);
		return -1;
	}
	
	return 0;
}

#endif

