#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wc24/wc24.h>

nwc24dl_record dlrec;
nwc24dl_entry dlent;

void ProcessEntry()
{
	int i;
	unsigned int temp, temp2;
	int stemp;
	char id[10];
	struct tm *time;
	time_t dltime;
	printf("Found entry index %x\n", be16toh(dlent.index));

	printf("Type: ");
	switch(dlent.type)
	{
		case WC24_TYPE_MSGBOARD:
			printf("mail");
		break;

		case WC24_TYPE_TITLEDATA:
			printf("title data");
		break;

		default:
			printf("unknown %x", dlent.type);
		break;
	}
	printf("\n");
	printf("record_flags: %x\n", dlent.record_flags);
	
	temp = be32toh(dlent.flags);
	printf("flags: %x ", temp);
	for(i=0; i<32; i++)
	{
		if(temp & (1<<i))
		{
			switch(i)
			{
				case 1:
					printf("RSA_WC24PUBKMOD ");
				break;

				case 2:
					printf("RSA_DISABLE ");
				break;

				case 3:
					printf("AES_WC24PUBKMOD ");
				break;

				case 4:
					printf("IDLEONLY ");
				break;

				case 30:
					printf("MAIL_DLFREQHDR_RANGESKIP ");
				break;

				default:
					printf("Unknown flag bit %x ", i);
				break;
			}
		}
	}
	printf("\n");

	temp = be32toh(dlent.ID);
	memset(id, 0, 5);
	memcpy(id, &dlent.ID, 4);
	printf("ID: %08x(%s)\n", temp, id);

	dltime = WC24_TimestampToSeconds(be32toh(dlrec.next_dl));
	time = localtime(&dltime);
	if(dltime)printf("next_dl: %s", asctime(time));
	if(!dltime)printf("next_dl is zero this because was never downloaded.\n");

	dltime = WC24_TimestampToSeconds(be32toh(dlrec.last_modified));
	time = localtime(&dltime);
	if(dltime)printf("last_modified: %s", asctime(time));
	if(!dltime)printf("last_modified is zero, this was never downloaded or an error occurred.\n");

	temp = (unsigned int)be64toh(dlent.titleid);
	temp2 = ((unsigned int)be32toh(dlent.titleid));
	memset(id, 0, 10);
	if(temp > 0x21<<24)
	{
		memcpy(id, (void*)(((int)&dlent.titleid) + 4), 4);
	}
	else
	{
		snprintf(id, 9, "%08x", temp);
	}
	printf("titleid: %08x%08x(%08x-%s)\n", temp2, temp, temp2, id);

	temp = be16toh(dlent.group_id);
	memset(id, 0, 5);
	memcpy(id, &dlent.group_id, 2);
	printf("group_id: %04x(%s)\n", temp, id);

	printf("cnt_nextdl: %x\n", be16toh(dlent.cnt_nextdl));
	printf("total_errors: %x\n", be16toh(dlent.total_errors));
	printf("total_errors: %x\n", be16toh(dlent.total_errors));
	printf("dl_freq_perday: %x\n", be16toh(dlent.dl_freq_perday));
	printf("dl_freq_days: %x\n", be16toh(dlent.dl_freq_days));
	
	stemp = (int)be32toh(dlent.error_code);
	if(stemp==0)printf("error_code is zero, either this wasn't downloaded yet or the download was successful.\n");
	if(stemp!=0)
	{
		printf("error_code: %d ", stemp);
		if(stemp==WC24_EINVALVFF)printf("EINVALVFF");
		if(stemp==WC24_EVFFPATH)printf("EVFFPATH");
		if(stemp==WC24_ESIGFAIL)printf("ESIGFAIL");
		if(stemp==WC24_EHTTP304)printf("EHTTP304");
		if((abs(stemp) - 107000) < 11000)printf("HTTP %d", (abs(stemp) - 117000));
		printf("\n");
	}

	printf("\n");
}

void DlQuene()
{
	int i;
	for(i=0; i<be16toh(NWC24DL_Header->max_entries); i++)
	{
		WC24_ReadRecord(i, &dlrec);
		WC24_ReadEntry(i, &dlent);
		if(dlrec.ID==0 && dlent.type==WC24_TYPE_EMPTY)continue;

		ProcessEntry();
	}
}

int main(int argc, char **argv)
{
	int retval;
	printf("wc24emu v1.0 by yellowstar6\n");
	if(argc<2)
	{
		printf("App to emulate WC24.\n");
		printf("Usage:\n");
		printf("wc24emu <nand_dump_path>\n");
		return 0;
	}

	retval = WC24_Init(argv[1]);
	if(retval<0)
	{
		printf("WC24_Init failed %d\n", retval);
		return 0;
	}

	DlQuene();

	WC24_Shutdown();
	
	return 0;
}

