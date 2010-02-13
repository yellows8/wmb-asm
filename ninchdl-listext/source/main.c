/*
ninchdl-listext is licensed under the MIT license:
Copyright (c) 2009 and 2010 yellowstar6

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <iconv.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

int gbalzss_main(int argc, char *argv[]);
char argv_str[256];

char country_codes[172][3] = {
{"JP"}, {"AI"}, {"AG"}, {"AR"}, {"AW"}, {"BS"}, {"BB"}, {"BZ"}, {"BO"}, {"BR"}, {"VG"},
{"CA"}, {"KY"}, {"CL"}, {"CO"}, {"CR"}, {"DM"}, {"DO"}, {"EC"}, {"SV"}, {"GF"}, {"GD"},
{"GP"}, {"GT"}, {"GY"}, {"HT"}, {"HN"}, {"JM"}, {"MQ"}, {"MX"}, {"MS"}, {"AN"}, {"NI"},
{"PA"}, {"PY"}, {"PE"}, {"KN"}, {"LC"}, {"VC"}, {"SR"}, {"TT"}, {"TC"}, {"US"}, {"UY"},
{"VI"}, {"VE"}, {"AL"}, {"AU"}, {"AT"}, {"BE"}, {"BA"}, {"BW"}, {"BG"}, {"HR"}, {"CY"},
{"CZ"}, {"DK"}, {"EE"}, {"FI"}, {"FR"}, {"DE"}, {"GR"}, {"HU"}, {"IS"}, {"IE"}, {"IT"},
{"LV"}, {"LS"}, {"LI"}, {"LT"}, {"LU"}, {"MK"}, {"MT"}, {"ME"}, {"MZ"}, {"NA"}, {"NL"},
{"NZ"}, {"NO"}, {"PL"}, {"PT"}, {"RO"}, {"RU"}, {"RS"}, {"SK"}, {"SI"}, {"ZA"}, {"ES"},
{"SZ"}, {"SE"}, {"CH"}, {"TR"}, {"GB"}, {"ZM"}, {"ZW"}, {"TW"}, {"KR"}, {"HK"}, {"MO"},
{"ID"}, {"SG"}, {"TH"}, {"PH"}, {"MY"}, {"CN"}, {"AE"}, {"IN"}, {"EG"}, {"OM"}, {"QA"},
{"KW"}, {"SA"}, {"SY"}, {"BH"}, {"JO"} };

char language_codes[7][3] = {{"ja"}, {"en"}, {"de"}, {"fr"}, {"es"}, {"it"}, {"nl"}};

typedef struct _DLlist_timestamp
{
    u16 year;
    u8 month;//Zero based.
    u8 day_of_month;
} __attribute((packed)) DLlist_timestamp;

typedef struct _DLlist_rating_entry//v3
{
	u8 ratingID;
	u8 boardID;//? For US, 2.
	u8 age;//Required years of age.
	u16 title[11];//Title/text of the rating.
} __attribute((packed)) DLlist_rating_entry;

typedef struct _DLlist_rating_entry_v4//v4
{
	u8 ratingID;
	u8 unk;
	u8 age;//Required years of age.
	u8 unk2;
	u32 JFIF_offset;//.jpg pic for this rating.
	u32 unk3;
	u16 title[11];//Title/text of the rating.
} __attribute((packed)) DLlist_rating_entry_v4;

typedef struct _DLlist_title_type_entry//v3
{
	u8 typeID;
	u8 groupID;
	u16 title[31];//Name of type.
} __attribute((packed)) DLlist_title_type_entry;

typedef struct _DLlist_title_type_entry_v4//v4
{
	u8 typeID;
	char console_model[3];//console model code.
	u16 title[31];//Name of type.
	u8 groupID;
	u8 unk;//usually matches the rating list "header" last byte.
} __attribute((packed)) DLlist_title_type_entry_v4;

typedef struct _DLlist_company_entry//v3
{
	u32 ID;//?
	u16 devtitle[31];
	u16 pubtitle[31];
} __attribute((packed)) DLlist_company_entry;

typedef struct _DLlist_title_entry//v3
{
	u32 ID;
	u32 titleID;//Or DS Game code.
	u8 title_type;
	u8 unk;
	u32 unk2;
	u16 company_offset;
	u32 release_date;
	u8 ratingID;
	u8 unk4[0x13];
	u16 title[62];
	u16 subtitle[31];
} __attribute((packed)) DLlist_title_entry;

typedef struct _DLlist_title_entry_v4//v4
{
	u32 ID;
	u32 titleID;//Or DS Game code.
	u8 title_type;
	u8 unk;
	u32 unk2;
	u16 company_offset;
	u32 release_date;
	u8 ratingID;
	u8 unk4[0x1d];
	u16 title[31];
	u16 subtitle[31];
	u16 short_title[31];
} __attribute((packed)) DLlist_title_entry_v4;

typedef struct _DLlist_video_entry//v3
{
	u32 ID;//Decimal ID for URL filename.
	u16 time_length;//Length of video measured in seconds.
	u32 titleid;//The assocaited title entry's ID.
	u8 icon;//0=Circle, 1=Wii, 2=WiiWare, 3=VC, 4=DS, 5=DS (DSiWare), 6=Blank
	u8 unk;
	u8 is_wide;
	u8 new_tag;
	u8 unk2;
	u16 title[51];
} __attribute((packed)) DLlist_video_entry;

typedef struct _DLlist_video_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 time_length;//Length of video measured in seconds.
	u32 titleid;//The assocaited title entry's ID.
	u8 unk2[0xf];
	u8 unk0;
	u8 ratingID;
	u8 unk1;
	u8 new_tag;
	u8 video_index;
	u8 unk4[2];
	u16 title[123];
} __attribute((packed)) DLlist_video_entry_v4;

typedef struct _DLlist_video2_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 unk;//time_length?
	u32 titleid;//The assocaited title entry's ID.
	u8 unk2[0x14];
	u16 title[102];
} __attribute((packed)) DLlist_video2_entry_v4;

typedef struct _DLlist_demo_entry//v3
{
	u32 ID;//Decimal ID for URL filename.
	u16 title[31];
	u16 subtitle[31];//Optional
	u32 titleid;//ID of title entry, not title entry titleid.
	u32 company_offset;
	u32 removal_timestamp;//0xffffffff when there is no end of distrubition date, timestamp otherwise.
	u8 unk[2];
} __attribute((packed)) DLlist_demo_entry;

typedef struct _DLlist_demo_entry_v4//v4
{
	u32 ID;//Decimal ID for URL filename.
	u16 title[31];
	u16 subtitle[31];//Optional
	u32 titleid;//ID of title entry, not title entry titleid.
	u32 company_offset;
	u32 removal_timestamp;//0xffffffff when there is no end of distrubition date, timestamp otherwise.
	u32 unk1;
	u8 ratingID;
	u8 new_tag;//When non-zero, NinCh displays the "NEW" tag?
	u8 new_tag_index;//Might be the index for the order of demos with the "NEW" tag?
	u8 unk2[0xcd];
} __attribute((packed)) DLlist_demo_entry_v4;

typedef struct _DLlist_detailed_rating_entry_v4
{
    u8 rating_group;
    u8 ratingID;
    u16 title[102];
} __attribute((packed)) DLlist_detailed_rating_entry_v4;

typedef struct _DLlist_header//v3
{
	u16 unk0;
	u8 version;
	u8 unk_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u32 filesize;//filesize of dl list.
	u8 unk5[7];
	u32 country_code;
	u32 language_code;
	u8 unk6[0x17];
	u32 ratings_total;
	u32 ratings_offset;
	u32 total_title_types;
	u32 main_title_types_offset;
	u32 companies_total;
	u32 companies_offset;
	u32 sub_title_types_offset;
	u32 titles_offset;
	u32 unk_title;
	u32 unk_offset;
	u32 videos_total;
	u32 videos_offset;
	u32 demos_total;
	u32 demos_offset;
	u32 wc24msg_opt_offset;
} __attribute((packed)) DLlist_header;

typedef struct _DLlist_header_wrapper//v3
{
	u16 unk0;
	u8 version;
	u8 unk_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u8 unk4;
	u32 filesize;//filesize of dl list.
	u8 unk5[7];
	u32 country_code;
	u32 language_code;
	u8 unk6[0x17];
	u32 ratings_total;
	DLlist_rating_entry *ratings;
	u32 total_title_types;
	DLlist_title_type_entry *main_title_types;
	u32 companies_total;
	DLlist_company_entry *companies;
	DLlist_title_entry *sub_title_types;
	DLlist_title_entry *titles;
	u32 unk_title;
	u32 *unk_data;
	u32 videos_total;
	DLlist_video_entry *videos;
	u32 demos_total;
	DLlist_demo_entry *demos;
	u16 *wc24msg_opt;
} __attribute((packed)) DLlist_header_wrapper;

typedef struct _DLlist_header_v4
{
	u16 unk0;
	u8 version;
	u8 unkb_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u32 filesize;//filesize of dl list.
	u32 unk8;
	u32 DlListID;
	u32 unk_region;
	u32 country_code;
	u8 unk18[0x2];
	u8 language_code;
	u8 unk6[0x9];
	u32 ratings_total;
	u32 ratings_offset;
	u32 total_title_types;
	u32 title_types_offset;
	u32 companies_total;
	u32 companies_offset;
	u32 total_titles;
	u32 titles_offset;
	u32 unk[2];
	u32 videos0_total;
	u32 videos0_offset;
	u32 unk1[2];
	u32 demos_total;
	u32 demos_offset;
	u8 unk20[0x20];
	u32 videos1_total;//Unknown what this list is for exactly.
	u32 videos1_offset;
	u32 total_detailed_ratings;
	u32 detailed_ratings_offset;
} __attribute((packed)) DLlist_header_v4;

typedef struct _DLlist_header_wrapper_v4
{
	u16 unk0;
	u8 version;
	u8 unkb_region;//? NinCh v3: 0, NinCh v4 JP: 0x39
	u32 filesize;//filesize of dl list.
	u32 unk8;
	u32 DlListID;
	u32 unk_region;
	u32 country_code;
	u32 language_code;
	u8 unk6[0x9];
	u32 ratings_total;
	DLlist_rating_entry_v4 *ratings;
	u32 total_title_types;
	DLlist_title_type_entry_v4 *title_types;
	u32 companies_total;
	DLlist_company_entry *companies;
	u32 total_titles;
	DLlist_title_entry_v4 *titles;
	u32 unk[2];
	u32 videos0_total;
	DLlist_video_entry_v4 *videos0;//All 60 videos.
	u32 unk1[2];
	u32 demos_total;
	DLlist_demo_entry_v4 *demos;
	u8 unk20[0x20];
	u32 videos1_total;
	DLlist_video2_entry_v4 *videos1;//Old NinCh v3 videos.
	u32 total_detailed_ratings;
	DLlist_detailed_rating_entry_v4 *detailed_ratings;
} __attribute((packed)) DLlist_header_wrapper_v4;

inline u32 be32(u32 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

inline u32 le32(u32 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>24) |
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
    #endif
}

inline u16 be16(u16 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER==BIG_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

inline u16 le16(u16 x)//From update_download by SquidMan.
{
    #if BYTE_ORDER == LITTLE_ENDIAN
    return x;
    #else
    return (x>>8) |
        (x<<8);
    #endif
}

DLlist_title_entry *LookupTitle(u32 ID, DLlist_header_wrapper *header)
{
    u32 i;
    u32 numtitles = ((u32)header->videos - (u32)header->titles) / sizeof(DLlist_title_entry);
    for(i = 0; i<numtitles; i++)
    {
        if(header->titles[i].ID==ID)
        {
            return &header->titles[i];
        }
    }
    return NULL;
}

DLlist_title_entry_v4 *LookupTitleV4(u32 ID, DLlist_header_wrapper_v4 *header)
{
    u32 i;
    u32 numtitles = header->total_titles;
    for(i = 0; i<numtitles; i++)
    {
        if(header->titles[i].ID==ID)
        {
            return &header->titles[i];
        }
    }
    return NULL;
}

char *GetCountryCode(u32 code)
{
    if(code==1)
    {
        return (char*)country_codes[0];
    }
    else if(code > 7 && code < 53)
    {
        return (char*)country_codes[code - 7];
    }
    else if(code > 63 && code < 113)
    {
        return (char*)country_codes[code - 18];
    }
    else if(code > 147 && code < 158)
    {
        return (char*)country_codes[code - 52];
    }
    else if(code > 167 && code < 178)
    {
        return (char*)country_codes[code - 62];
    }
    else
    {
        return NULL;
    }
}

char GetRegionCode(u32 code)
{
    if(code==1)
    {
        return 'j';
    }
    else if(code > 7 && code < 53)
    {
        return 'u';
    }
    else if(code > 63 && code < 113)
    {
        return 'e';
    }
    return 'e';
}

void GetTimestamp(u32 input, DLlist_timestamp *timestamp)
{
    memcpy(timestamp, &input, 4);
    timestamp->year = be16(timestamp->year);
}

void ConvertUTF16ToUTF8(u16 *utf16, char *out)
{
	char *UTF16 = (char*)utf16;
	int i = 0;
	size_t inlen = 0, outlen = 256;
	char **inbuf = &UTF16;
	size_t *inbytesleft = &inlen;
	char **outbuf = &out;
	size_t *outbytesleft = &outlen;
	while(utf16[i]!=0)i++;
	inlen = i * 2;
	iconv_t cd = iconv_open("UTF-8", "UTF-16BE");
	if(cd<0)
	{
		printf("iconv_open error\n");
		return;
	}

	if(iconv(cd, inbuf, inbytesleft, outbuf, outbytesleft)<0)
	{
		printf("iconv error\n");
	}

	iconv_close (cd);
}

int main(int argc, char **argv)
{
	char str[256];
	char pstr[256];
	char vff_dumpfn[256];
	unsigned char *buffer = NULL;
	int is_csdata = 0;
	int is_vff = 1;
	int version = 3;
	FILE *fil;

    memset(pstr, 0, 256);

    printf("ninchdl-listext v1.1 by yellowstar6.\n");
    if(argc==1 || (argc<5 && argc>3) || argc>=8)
    {
        printf("Usage:\n");
        printf("ninchdl-listext <wc24dl.vff> <options>\n");
        printf("Input can be a compressed dl list as well.\n");
        printf("Alternate usage:\n");
        printf("ninchdl-listext <country code> <language code>\n<region char> <version> <wc24pubk.mod> <options>\n");
        printf("See either source code or google code wmb-asm NintendoChannel wiki page for list of country and language codes.\n");
        printf("region char must be either u, e, or j.\n");
        printf("wc24pubk.mod is the filename for the NinCh WC24 keys.(Can also be the 16 byte\nAES key.) The default is wc24pubk.mod if this is ommitted.\n");
	    printf("Options:\n");
		printf("-l<ID> List titles with a title type ID or console model ID. If the ID is ommitted, the whole title list is listed.\n");
        return 0;
    }

	memset(str, 0, 256);
	memset(vff_dumpfn, 0, 256);

    strcpy(vff_dumpfn, argv[1]);
    struct stat fstats;
    if(strstr(vff_dumpfn, ".vff"))
    {
        FILE *fil = fopen(vff_dumpfn, "rb");
        if(fil==NULL)
        {
            sprintf(pstr, "Failed to open %s\n", str);
            printf(pstr);
            return -3;
        }

		stat(vff_dumpfn, &fstats);
		buffer = (unsigned char*)malloc(fstats.st_size);
		if(buffer==NULL)
		{
			sprintf(pstr, "Failed to allocate 0x%x bytes.\n", (int)fstats.st_size);
			printf(pstr);
			return -2;
		}
		fread(buffer, 1, fstats.st_size, fil);
		fclose(fil);
		
		if(strncmp((char*)buffer, "VFF ", 4)!=0)
		{
            if(buffer[0]!=0x10)
            {
                printf("Invalid VFF, and input file is not raw compressed data.\n");
                return -4;
            }
            else
            {
                is_vff = 0;
                return -4;
            }
        }
    }
    else
    {
        is_vff = 0;
    }

    if(is_vff)
    {
        if(memcmp(&buffer[0x2080], "WC24DATABIN", 11)==0)
        {
            printf("Found WC24Data.bin in VFF.\n");
        }
        else if(memcmp(&buffer[0x480], "CSDATA  BIN", 11)==0)
        {
            printf("Found CSData.bin in VFF; only decompression is supported.\n");
            is_csdata = 1;
        }
        else
        {
            printf("NinCh dl list not found in VFF.\n");
            return -3;
        }
    }

    u32 DlListID = 0;
    if(!is_vff && strstr(argv[1], ".LZ")==NULL)
    {
        memset(pstr, 0, 256);
        memset(str, 0, 256);
        sscanf(argv[4], "%d", &version);
        if(version<4)strcpy(str, "wc24data.LZ");
        if(version>=4)strcpy(str, "csdata.LZ");
        sprintf(pstr, "http://ent%c.wapp.wii.com/%d/%s/%s/%s", argv[3][0], version, argv[1], argv[2], str);
        memset(str, 0, 256);
        sprintf(str, "wc24decrypt %s ", pstr);
        memset(pstr, 0, 256);
        if(version<4)strcpy(pstr, "WC24Data.bin");
        if(version>=4)strcpy(pstr, "CSData.bin");
        strcat(str, pstr);
        strcat(str, " ");
        if(argc<6)
        {
            strcat(str, "wc24pubk.mod");
        }
        else if(strstr(argv[5], ",mod"))
        {
            strcat(str, argv[5]);
        }
		else
		{
			strcat(str, "wc24pubk.mod");
		}
        printf("%s\n", str);
        if(system(str)!=0)return -5;

        if(version>=4)
        {
            FILE *f;
            memset(argv_str, 0, 256);
            sprintf(argv_str, "gbalzss%cd%c%s%c%s", 0, 0, pstr, 0, "decom.bin");
            unsigned int Argv_[4];
            Argv_[0] = (unsigned int)&argv_str[0];
            Argv_[1] = (unsigned int)&argv_str[8];
            Argv_[2] = (unsigned int)&argv_str[10];
            Argv_[3] = (unsigned int)&argv_str[11 + strlen(pstr)];
            gbalzss_main(4, (char**)Argv_);
            memset(argv_str, 0, 256);
            memset(str, 0, 256);

            f = fopen("decom.bin", "rb");
            if(f==NULL)return -3;
            fseek(f, 0xc, SEEK_SET);
            fread(&DlListID, 1, 4, f);
            fclose(f);
            DlListID = be32(DlListID);
            sprintf(str, "curl -k --output %u.LZ https://ent%cs.wapp.wii.com/%s/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/list/%s/%s/%u.LZ", (unsigned int)DlListID, argv[3][0], argv[4], argv[1], argv[2], (unsigned int)DlListID);
            printf("%s\n", str);
            if(system(str)!=0)return -5;
        }
    }

    if(is_vff)
    {
        sprintf(str, "Extracting %s...\n", is_csdata==0?"WC24Data.bin":"CSData.bin");
        printf(str);

        u32 datasize;
        if(is_csdata)memcpy(&datasize, &buffer[0x49c], 4);
        if(!is_csdata)memcpy(&datasize, &buffer[0x209c], 4);
        datasize = le32(datasize);
        memset(str, 0, 256);
        sprintf(str, "%s", is_csdata==0?"WC24Data.bin":"CSData.bin");

        fil = fopen(str, "wb");
        if(fil==NULL)
        {
            sprintf(pstr, "Failed to open %s\n", str);
            printf(pstr);
            return -3;
        }

        fwrite(&buffer[is_csdata==0?0x3220:0x1620], 1, datasize, fil);
        fclose(fil);
        free(buffer);
    }
    else
    {
        memset(str, 0, 256);
        if(strstr(argv[1], ".LZ")==NULL && strstr(argv[1], ".")==NULL)
        {
            if(version<4)strcpy(str, "WC24Data.bin");
            if(version>=4)sprintf(str, "%u.LZ", (unsigned int)DlListID);
        }
        else if(strstr(argv[1], "."))
        {
            strcpy(str, argv[1]);
        }
    }

    printf("Decompressing...\n");
    memset(argv_str, 0, 256);
    memset(pstr, 0, 256);
    sprintf(pstr, "decom.bin");
    sprintf(argv_str, "gbalzss%cd%c%s%c%s", 0, 0, str, 0, pstr);
    unsigned int Argv[4];
    Argv[0] = (unsigned int)&argv_str[0];
    Argv[1] = (unsigned int)&argv_str[8];
    Argv[2] = (unsigned int)&argv_str[10];
    Argv[3] = (unsigned int)&argv_str[11 + strlen(str)];
    gbalzss_main(4, (char**)Argv);

	if(is_csdata)
	{
		printf("Done.\n");
		return 0;
	}


    DLlist_header_wrapper *header = NULL;
    DLlist_header_wrapper_v4 *header_v4 = NULL;
    sprintf(str, "decom.bin");

    printf("Parsing...\n");
    fil = fopen(str, "rb");
    if(fil==NULL)
    {
        sprintf(pstr, "Failed to open %s\n", str);
        printf(pstr);
        return -3;
    }

    stat(str, &fstats);
    buffer = (unsigned char*)malloc(fstats.st_size);
    if(buffer==NULL)
    {
        sprintf(pstr, "Failed to allocate 0x%x bytes.\n", (int)fstats.st_size);
        printf(pstr);
        return -2;
    }
    fread(buffer, 1, fstats.st_size, fil);
    fclose(fil);

	if(strstr(argv[1], ".LZ") ||  strstr(argv[1], ".bin"))
	{
		version = (int)buffer[2];
		if(version==0xfe)
		{
			printf("Unknown version, this dl list is outdated and a newer version of NinCh was released for this region.\n");
			free(buffer);
			return -4;
		}
    }
	if(version<4)header = (DLlist_header_wrapper*)buffer;
    if(version>=4)header_v4 = (DLlist_header_wrapper_v4*)buffer;
    char *country_code;
   	char *language_code;
   	char region_code;

	if(version<4)
	{
        header->country_code = be32(header->country_code);
        header->language_code = be32(header->language_code);
	}
	else
	{
	    header_v4->country_code = be32(header_v4->country_code);
        header_v4->language_code = be32(header_v4->language_code);
	}

   	if(version<4)
   	{
        country_code = GetCountryCode(header->country_code);
        language_code = (char*)language_codes[header->language_code];
   	}
   	else
   	{
        country_code = GetCountryCode(header_v4->country_code);
        language_code = (char*)language_codes[header_v4->language_code];
   	}

	if(country_code==NULL)
    {
        free(buffer);
        sprintf(pstr, "Unknown country code: %d\n", (int)header->country_code);
        printf(pstr);
        return -4;
    }

    if(version<4)region_code = GetRegionCode(header->country_code);
    if(version>=4)region_code = GetRegionCode(header_v4->country_code);
    memset(str, 0, 256);
    sprintf(str, "dump%c.txt", region_code);
    fil = fopen(str, "wb");
    if(fil==NULL)
    {
		sprintf(pstr, "Failed to open %s", str);
		printf(pstr);
		return -3;
    }

	fprintf(fil, "Dl list version: ");
	printf("Dl list version: ");
	if(strstr(argv[1], ".LZ")==NULL && strstr(argv[1], ".bin")==NULL)
	{
		if(buffer[2]==0xfe)
		{
			fprintf(fil, "This dl list is outdated, a newer version of NinCh was released for this region.\r\n");
			printf("This dl list is outdated, a newer version of NinCh was released for this region.\r\n");
		}
	}
	if(buffer[2]!=0xfe)
	{
		fprintf(fil, "%d\n", version);
		printf("%d\n", version);
	}

    if(version>4)
    {
        sprintf(pstr, "Unsupported dl list version for parser: %d\r\n", header->version);
        free(buffer);

        printf(pstr);
        fclose(fil);
        return 0;
    }
	fprintf(fil, "Dl list ID: %u\r\nCountry code: %s\r\nLanguage code: %s\r\nRegion code: %c\r\n\r\n", be32(*((u32*)&buffer[0xc])), country_code, language_code, region_code);
	printf("Dl list ID: %u\nCountry code: %s\nLanguage code: %s\nRegion code: %c\n", be32(*((u32*)&buffer[0xc])), country_code, language_code, region_code);
	
    u16 utf_temp;
    u32 i, texti, ratingi;
    u32 total_demos, total_videos;

    if(version<4)
    {
        header->ratings = (DLlist_rating_entry*)(be32((u32)header->ratings) + (u32)buffer);
        header->main_title_types = (DLlist_title_type_entry*)(be32((u32)header->main_title_types) + (u32)buffer);
        header->companies = (DLlist_company_entry*)(be32((u32)header->companies) + (u32)buffer);
        header->sub_title_types = (DLlist_title_entry*)(be32((u32)header->sub_title_types) + (u32)buffer);
        header->titles = (DLlist_title_entry*)(be32((u32)header->titles) + (u32)buffer);
        header->unk_data = (u32*)(be32((u32)header->unk_data) + (u32)buffer);
        header->videos = (DLlist_video_entry*)(be32((u32)header->videos) + (u32)buffer);
        header->demos = (DLlist_demo_entry*)(be32((u32)header->demos) + (u32)buffer);
        header->wc24msg_opt = (u16*)(be32((u32)header->wc24msg_opt) + (u32)buffer);
    }
    else
    {
        header_v4->ratings = (DLlist_rating_entry_v4*)(be32((u32)header_v4->ratings) + (u32)buffer);
        header_v4->title_types = (DLlist_title_type_entry_v4*)(be32((u32)header_v4->title_types) + (u32)buffer);
        header_v4->companies = (DLlist_company_entry*)(be32((u32)header_v4->companies) + (u32)buffer);
        header_v4->titles = (DLlist_title_entry_v4*)(be32((u32)header_v4->titles) + (u32)buffer);
        header_v4->demos = (DLlist_demo_entry_v4*)(be32((u32)header_v4->demos) + (u32)buffer);
        header_v4->videos0 = (DLlist_video_entry_v4*)(be32((u32)header_v4->videos0) + (u32)buffer);
        header_v4->videos1 = (DLlist_video2_entry_v4*)(be32((u32)header_v4->videos1) + (u32)buffer);
        header_v4->detailed_ratings = (DLlist_detailed_rating_entry_v4*)(be32((u32)header_v4->detailed_ratings) + (u32)buffer);
    }

    if(version<4)
    {
        header->ratings_total = be32(header->ratings_total);
        header->total_title_types = be32(header->total_title_types);
        header->companies_total = be32(header->companies_total);
        header->unk_title = be32(header->unk_title);
        header->videos_total = be32(header->videos_total);
        header->demos_total = be32(header->demos_total);
        total_demos = header->demos_total;
        total_videos = header->videos_total;
    }
    else
    {
        header_v4->ratings_total = be32(header_v4->ratings_total);
        header_v4->total_title_types = be32(header_v4->total_title_types);
        header_v4->companies_total = be32(header_v4->companies_total);
        header_v4->demos_total = be32(header_v4->demos_total);
        header_v4->videos0_total = be32(header_v4->videos0_total);
        header_v4->videos1_total = be32(header_v4->videos1_total);
        header_v4->total_detailed_ratings = be32(header_v4->total_detailed_ratings);
        total_demos = header_v4->demos_total;
        total_videos = header_v4->videos0_total;
	    header_v4->total_titles = be32(header_v4->total_titles);
    }

    fprintf(fil, "Demos:\r\n");
    fprintf(fil, "Number of demos: %d\r\n\r\n", (unsigned int)total_demos);

    DLlist_title_entry *title_ptr = NULL;
    DLlist_title_entry_v4 *title_ptr_v4 = NULL;
    DLlist_timestamp timestamp;
    u32 timestamp_u32;
    for(i=0; i<total_demos; i++)
    {
	memset(str, 0, 256);
	if(version<4)ConvertUTF16ToUTF8(header->demos[i].title, str);
	if(version>=4)ConvertUTF16ToUTF8(header_v4->demos[i].title, str);	
	fprintf(fil, str);
        fprintf(fil, "\r\n");

	memset(str, 0, 256);
	if(version<4)ConvertUTF16ToUTF8(header->demos[i].subtitle, str);
	if(version>=4)ConvertUTF16ToUTF8(header_v4->demos[i].subtitle, str);
	fprintf(fil, str);
        fprintf(fil, "\r\n");

        if(version>=4)
        {
            if(header_v4->demos[i].new_tag)
            {
                sprintf(str, "NEW (index %d)\r\n", header_v4->demos[i].new_tag_index);
                fputs(str, fil);
            }
        }

        u32 demo_ID = 0;
        if(version<4)demo_ID = be32(header->demos[i].ID);
        if(version>=4)demo_ID = be32(header_v4->demos[i].ID);
        fprintf(fil, "ID: %u\r\n", (unsigned int)demo_ID);

        u32 ratingID = 0;
        if(version<4)
        {
            title_ptr = LookupTitle(header->demos[i].titleid, header);
            if(title_ptr)ratingID = title_ptr->ratingID;
        }
        else
        {
            title_ptr_v4 = LookupTitleV4(header_v4->demos[i].titleid, header_v4);
            ratingID = header_v4->demos[i].ratingID;
        }
        if((version<4 && title_ptr) || version>=4)
        {
            fprintf(fil, "Rating: ");
        }
        if(version<4 && title_ptr)
        {
            for(ratingi=0; ratingi<header->ratings_total; ratingi++)
            {
                if(header->ratings[ratingi].ratingID==ratingID)
                {
		    memset(str, 0, 256);
		    ConvertUTF16ToUTF8(header->ratings[ratingi].title, str);
		    fprintf(fil, str);
                    break;
                }
            }
        }
        else if(version>=4)
        {
            for(ratingi=0; ratingi<header_v4->ratings_total; ratingi++)
            {
                if(header_v4->ratings[ratingi].ratingID==ratingID)
                {
		    memset(str, 0, 256);
		    ConvertUTF16ToUTF8(header_v4->ratings[ratingi].title, str);
		    fprintf(fil, str);
                }
            }
        }
        fprintf(fil, "\r\n");

        DLlist_company_entry *comp = NULL;
        if(version<4 && title_ptr)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr->company_offset));
        if(version>=4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be32(header_v4->demos[i].company_offset));
        if(comp)
        {
	    memset(str, 0, 256);
	    ConvertUTF16ToUTF8(comp->devtitle, str);	
	    fprintf(fil, str);
        }
        fprintf(fil, "\r\n");

		if((version<4 && title_ptr) || (version>=4))
		{
            		if(memcmp(comp->devtitle, comp->pubtitle, 31 * 2)!=0)
            		{
				memset(str, 0, 256);
	    			ConvertUTF16ToUTF8(comp->devtitle, str);	
	    			fprintf(fil, str);
            		}
		}
        fprintf(fil, "\r\n");

        if(version<4)timestamp_u32 = header->demos[i].removal_timestamp;
        if(version>=4)timestamp_u32 = header_v4->demos[i].removal_timestamp;

        if(timestamp_u32==0xffffffff)
        {
            fprintf(fil, "No removal date.");
        }
        else
        {
            GetTimestamp(timestamp_u32, &timestamp);
            fprintf(fil, "Removal date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
        }
        fprintf(fil, "\r\n");

        if(title_ptr || title_ptr_v4)
        {
            if(version<4)timestamp_u32 = title_ptr->release_date;
            if(version>=4)timestamp_u32 = title_ptr_v4->release_date;

            if(timestamp_u32==0xffffffff)
            {
                fprintf(fil, "No title release date.");
            }
            else
            {
                GetTimestamp(timestamp_u32, &timestamp);
                fprintf(fil, "Title release date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
            }
        }
        fprintf(fil, "\r\n\r\n");

	if(title_ptr || title_ptr_v4)
	{

	u32 tid, titleid;
	    if(version<4)
	    {
		tid = title_ptr->titleID;
	    	titleid = be32(title_ptr->ID);
	    }
	    if(version>=4)
	    {		
		tid = title_ptr_v4->titleID;
		titleid = be32(title_ptr_v4->ID);
	    }	    

	    fprintf(fil, "Title ID: %c%c%c%c\r\nTitle entry ID: %u\r\n", (char)tid, (char)(tid>>8), (char)(tid>>16), (char)(tid>>24), titleid);

	int ti;
	u32 tt_num;
	if(version<4)tt_num = be32(header->total_title_types);
	if(version>=4)tt_num = be32(header_v4->total_title_types);
	for(ti = 0; ti<tt_num; ti++)
				{
				    if(version<4)
				    {
                        if(title_ptr->title_type==header->main_title_types[ti].typeID)
                        {
                            break;
                        }
				    }
				    else
                    {
                        if(title_ptr_v4->title_type==header_v4->title_types[ti].typeID)
                        {
                            break;
                        }
                    }
				}

	fprintf(fil, "Title type: ");
	memset(str, 0, 256);
	if(version<4)ConvertUTF16ToUTF8(header->main_title_types[ti].title, str);
	if(version>=4)ConvertUTF16ToUTF8(header_v4->title_types[ti].title, str);
	fprintf(fil, str);
        if(version<4)fprintf(fil, " (typeID %02x)\n", title_ptr->title_type);
        if(version>=4)fprintf(fil, " (typeID %02x)\n", title_ptr_v4->title_type);

	fprintf(fil, "Title info URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/soft/%s/%s/%u.info\r\n\r\n", region_code, (int)version, country_code, language_code, (unsigned int)titleid);

	}

        fprintf(fil, "URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/dstrial/%s/%s/%u.bin\r\n\r\n", region_code, (int)version, country_code, language_code, (unsigned int)demo_ID);
        fflush(fil);
    }

    fprintf(fil, "Videos:\r\n");
    fprintf(fil, "Number of videos: %d\r\n\r\n", (unsigned int)total_videos);

    for(i=0; i<total_videos; i++)
    {
        u32 txt_len = 0;
        if(version<4)txt_len = 51;
        if(version>=4)txt_len = 123;
	memset(str, 0, 256);
	if(version<4)ConvertUTF16ToUTF8(header->videos[i].title, str);
	if(version>=4)ConvertUTF16ToUTF8(header_v4->videos0[i].title, str);
	fprintf(fil, str);
        fprintf(fil, "\r\n");

        if(version>=4)
        {
            if(header_v4->videos0[i].new_tag)
            {
                fprintf(fil, "NEW\r\n");
            }
        }

        u16 time_length;
        if(version<4)time_length = be16(header->videos[i].time_length);
        if(version>=4)time_length = be16(header_v4->videos0[i].time_length);
        fprintf(fil, "Time length %02d:%02d\r\n", time_length / 60, time_length % 60);

	if(version<4)
	{
		fprintf(fil, "Icon: ");
		switch(header->videos[i].icon)
		{
	 		case 0:
				fprintf(fil, "Circle");
			break;

			case 1:
				fprintf(fil, "Wii");
			break;

			case 2:
				fprintf(fil, "WiiWare");
			break;			

			case 3:
				fprintf(fil, "VC");
			break;
			
			case 4:
				fprintf(fil, "DS");
			break;

			case 5:
				fprintf(fil, "DSiWare");
			break;

			case 6:
				fprintf(fil, "Blank");
			break;
		}
		fprintf(fil, "\r\n");
	}

        u32 video_ID, video_titleid, video1_ID = 0;
        if(version<4)video_ID = be32(header->videos[i].ID);
        if(version>=4)
        {
            video_ID = be32(header_v4->videos0[i].ID);
            video1_ID = be32(header_v4->videos1[i].ID);
        }
        if(version<4)
        {
            fprintf(fil, "ID: %u\r\n", (unsigned int)video_ID);
        }
        else
        {
            fprintf(fil, "ID: %u\r\n", (unsigned int)video_ID);
        }

        if(version<4)video_titleid = header->videos[i].titleid;
        if(version>=4)video_titleid = header_v4->videos0[i].titleid;
        title_ptr = NULL;
        title_ptr_v4 = NULL;
        if(video_titleid!=0)
        {
            fprintf(fil, "Rating: ");

            u32 ratingID = 0;
            if(version<4)
            {
                title_ptr = LookupTitle(header->videos[i].titleid, header);
                if(title_ptr)ratingID = title_ptr->ratingID;
            }
            else
            {
                title_ptr_v4 = LookupTitleV4(header_v4->videos0[i].titleid, header_v4);
                ratingID = header_v4->videos0[i].ratingID;
            }

            if(version<4 && title_ptr)
            {
                for(ratingi=0; ratingi<header->ratings_total; ratingi++)
                {
                    if(header->ratings[ratingi].ratingID==ratingID)
                    {
			memset(str, 0, 256);
			ConvertUTF16ToUTF8(header->ratings[ratingi].title, str);
			fprintf(fil, str);
                    }
                }
            }
            else if(version>=4)
            {
                for(ratingi=0; ratingi<header_v4->ratings_total; ratingi++)
                {
                    if(header_v4->ratings[ratingi].ratingID==ratingID)
                    {
			memset(str, 0, 256);
			ConvertUTF16ToUTF8(header_v4->ratings[ratingi].title, str);
			fprintf(fil, str);
                    }
                }
            }

            fprintf(fil, "\r\n");

            DLlist_company_entry *comp = NULL;
            if(version<4 && title_ptr)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr->company_offset));
            if(version>=4 && title_ptr_v4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr_v4->company_offset));
            if(title_ptr || title_ptr_v4)
            {
		memset(str, 0, 256);
		ConvertUTF16ToUTF8(comp->devtitle, str);
		fprintf(fil, str);
            }

            fprintf(fil, "\r\n");
            if(title_ptr || title_ptr_v4)
            {
                if(memcmp(comp->devtitle, comp->pubtitle, 31 * 2)!=0)
                {
		    memset(str, 0, 256);
		    ConvertUTF16ToUTF8(comp->pubtitle, str);
		    fprintf(fil, str);
                }
            }
		}
        fprintf(fil, "\r\n");

        if(title_ptr || title_ptr_v4)
        {
            if(version<4)timestamp_u32 = title_ptr->release_date;
            if(version>=4)timestamp_u32 = title_ptr_v4->release_date;

            if(timestamp_u32==0xffffffff)
            {
                fprintf(fil, "No title release date.");
            }
            else
            {
                GetTimestamp(timestamp_u32, &timestamp);
                fprintf(fil, "Title release date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
            }
        }
        fprintf(fil, "\r\n\r\n");

	if(title_ptr || title_ptr_v4)
	{

	u32 tid, titleid;
	    if(version<4)
	    {
		tid = title_ptr->titleID;
	    	titleid = be32(title_ptr->ID);
	    }
	    if(version>=4)
	    {		
		tid = title_ptr_v4->titleID;
		titleid = be32(title_ptr_v4->ID);
	    }	    

	    fprintf(fil, "Title ID: %c%c%c%c\r\nTitle entry ID: %u\r\n", (char)tid, (char)(tid>>8), (char)(tid>>16), (char)(tid>>24), titleid);

	int ti;
	u32 tt_num;
	if(version<4)tt_num = be32(header->total_title_types);
	if(version>=4)tt_num = be32(header_v4->total_title_types);
	for(ti = 0; ti<tt_num; ti++)
				{
				    if(version<4)
				    {
                        if(header->titles[i].title_type==header->main_title_types[ti].typeID)
                        {
                            break;
                        }
				    }
				    else
                    {
                        if(header_v4->titles[i].title_type==header_v4->title_types[ti].typeID)
                        {
                            break;
                        }
                    }
				}

	fprintf(fil, "Title type: ");
	memset(str, 0, 256);
	if(version<4)ConvertUTF16ToUTF8(header->main_title_types[ti].title, str);
	if(version>=4)ConvertUTF16ToUTF8(header_v4->title_types[ti].title, str);
	fprintf(fil, str);
        if(version<4)fprintf(fil, " (typeID %02x)\n", title_ptr->title_type);
        if(version>=4)fprintf(fil, " (typeID %02x)\n", title_ptr_v4->title_type);

	fprintf(fil, "Title info URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/soft/%s/%s/%u.info\r\n\r\n", region_code, (int)version, country_code, language_code, (unsigned int)titleid);

	}

        if(version<4)
        {
            fprintf(fil, "URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u.3gp\r\n\r\n", region_code, version, country_code, language_code, (unsigned int)video_ID);
        }
        else
        {
            fprintf(fil, "Low Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-l.mo\r\n", region_code, version, country_code, language_code, (unsigned int)video_ID);
            fprintf(fil, "High Quality URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/movie/%s/%s/%u-h.mo\r\n\r\n", region_code, version, country_code, language_code, (unsigned int)video_ID);
        }
        fflush(fil);
    }

	fclose(fil);
    if((argc>5 && strncmp(argv[5], "-l", 2)==0) || (argc==3 && strncmp(argv[2], "-l", 2)==0))
	{
		char *lsarg = NULL;
		if((argc>5 && strncmp(argv[5], "-l", 2)==0))lsarg = argv[5];
		if((argc==3 && strncmp(argv[2], "-l", 2)==0))lsarg = argv[2];
		u32 num_titles = 0;
		printf("Listing title(s)...\n");
		sprintf(str, "title%c.txt", region_code);
		fil = fopen(str, "w");
		if(version<4)num_titles = ((u32)header->videos - (u32)header->titles) / sizeof(DLlist_title_entry);
		if(version>=4)num_titles = header_v4->total_titles;
		u32 i;
		unsigned int typeID = 0;
		u32 ti = 0, tt_num;
		if(version<4)tt_num = header->total_title_types;
		if(version>=4)tt_num = header_v4->total_title_types;
		if(strlen(lsarg)==5)
		{
			fprintf(fil, "Titles with console model %s:\n", &lsarg[2]);
		}
		else if(strlen(lsarg)==4)
		{
			sscanf(&lsarg[2], "%02x", &typeID);
			for(ti = 0; ti<tt_num; ti++)
			{
				if(version<4)
				{
					if(header->main_title_types[ti].typeID==typeID)break;
				}
				else
				{
					if(header_v4->title_types[ti].typeID==typeID)break;
				}
			}
			if(version<4)fprintf(fil, "Titles with title type ID %02x, description ", (unsigned int)typeID);
			if(version>=4)fprintf(fil, "Titles with title type ID %02x, model %c%c%c description ", (unsigned int)typeID, header_v4->title_types[ti].console_model[0], header_v4->title_types[ti].console_model[1], header_v4->title_types[ti].console_model[2]);
			memset(str, 0, 256);
			if(version<4)ConvertUTF16ToUTF8(header->main_title_types[ti].title, str);
			if(version>=4)ConvertUTF16ToUTF8(header_v4->title_types[ti].title, str);
			fprintf(fil, str);
			fprintf(fil, "\n");
		}

		u32 found;
        u32 numtitles = 0;

		for(i=0; i<num_titles; i++)
		{
		    if(version<4)title_ptr = &header->titles[i];
		    if(version>=4)title_ptr_v4 = &header_v4->titles[i];
            found = 0;
            if(strlen(lsarg)==2)
            {
                found = 1;
                for(ti = 0; ti<tt_num; ti++)
				{
				    if(version<4)
				    {
                        if(header->titles[i].title_type==header->main_title_types[ti].typeID)
                        {
                            break;
                        }
				    }
				    else
                    {
                        if(header_v4->titles[i].title_type==header_v4->title_types[ti].typeID)
                        {
                            break;
                        }
                    }
				}
            }
            if(strlen(lsarg)>2)
            {
			if(strlen(lsarg)==5 && version>=4)
			{
				for(ti = 0; ti<tt_num; ti++)
				{
				    if(header_v4->titles[i].title_type==header_v4->title_types[ti].typeID)
				    {
                        if(memcmp(header_v4->title_types[ti].console_model, &lsarg[2], 3)==0)
                        {
                            found = 1;
                            break;
                        }
				    }
				}
			}
			else if(strlen(lsarg)==4)
			{
				if(version<4)
				{
					if(header->titles[i].title_type!=typeID)continue;
				}
				else
				{
					if(header_v4->titles[i].title_type!=typeID)continue;
				}
				found = 1;
			}
            }
            if(!found)continue;
            numtitles++;
		}

        fprintf(fil, "Total titles: %u\n\n", (unsigned int)numtitles);
		for(i=0; i<num_titles; i++)
		{
		    if(version<4)title_ptr = &header->titles[i];
		    if(version>=4)title_ptr_v4 = &header_v4->titles[i];
            found = 0;
            if(strlen(lsarg)==2)
            {
                found = 1;
                for(ti = 0; ti<tt_num; ti++)
				{
				    if(version<4)
				    {
                        if(header->titles[i].title_type==header->main_title_types[ti].typeID)
                        {
                            break;
                        }
				    }
				    else
                    {
                        if(header_v4->titles[i].title_type==header_v4->title_types[ti].typeID)
                        {
                            break;
                        }
                    }
				}
            }
            if(strlen(lsarg)>2)
            {
                if(strlen(lsarg)==5 && version>=4)
                {
                    for(ti = 0; ti<tt_num; ti++)
                    {
                        if(header_v4->titles[i].title_type==header_v4->title_types[ti].typeID)
                        {
                            if(memcmp(header_v4->title_types[ti].console_model, &lsarg[2], 3)==0)
                            {
                                found = 1;
                                break;
                            }
                        }
                    }
                }
                else if(strlen(lsarg)==4)
                {
                    if(version<4)
                    {
                        if(header->titles[i].title_type!=typeID)continue;
                    }
                    else
                    {
                        if(header_v4->titles[i].title_type!=typeID)continue;
                    }

                    found = 1;
                }
            }
            if(!found)continue;

			memset(str, 0, 256);
			if(version<4)ConvertUTF16ToUTF8(header->titles[i].title, str);
			if(version>=4)ConvertUTF16ToUTF8(header_v4->titles[i].title, str);
			fprintf(fil, str);

			fprintf(fil, "\n");
			memset(str, 0, 256);
			if(version<4)ConvertUTF16ToUTF8(header->titles[i].subtitle, str);
			if(version>=4)ConvertUTF16ToUTF8(header_v4->titles[i].subtitle, str);
			fprintf(fil, str);

			fprintf(fil, "\n");
			if(version>=4)
			{
				if(memcmp(header_v4->titles[i].title, header_v4->titles[i].short_title, 31)!=0 && header_v4->titles[i].short_title[0]!=0)
				{
				    	fprintf(fil, "Short title: ");
				        memset(str, 0, 256);
					ConvertUTF16ToUTF8(header_v4->titles[i].short_title, str);
					fprintf(fil, str);
					fprintf(fil, "\n");
				}
			}

			if(strlen(lsarg)!=4)
			{
				memset(str, 0, 256);
				if(version<4)ConvertUTF16ToUTF8(header->main_title_types[ti].title, str);
				if(version>=4)ConvertUTF16ToUTF8(header_v4->title_types[ti].title, str);
				fprintf(fil, str);
                if(version<4)fprintf(fil, " (typeID %02x)\n", header->main_title_types[ti].typeID);
                if(version>=4)fprintf(fil, " (typeID %02x)\n", header_v4->title_types[ti].typeID);
			}

            DLlist_company_entry *comp = NULL;
            if(version<4 && title_ptr)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr->company_offset));
            if(version>=4 && title_ptr_v4)comp = (DLlist_company_entry*)((u32)buffer + (u32)be16(title_ptr_v4->company_offset));
            if(comp)
            {
		memset(str, 0, 256);
		ConvertUTF16ToUTF8(comp->devtitle, str);
		fprintf(fil, str);
            }

            fputs("\r\n", fil);
            if((version<4 && title_ptr) || (version>=4 && title_ptr_v4))
            {
                if(memcmp(comp->devtitle, comp->pubtitle, 31 * 2)!=0)
                {
		    memset(str, 0, 256);
		    ConvertUTF16ToUTF8(comp->pubtitle, str);
		    fprintf(fil, str);
                }
            }

            sprintf(str, "Rating: ");
            fputs(str, fil);

            u32 ratingID = 0;
            if(version<4)
            {
                ratingID = title_ptr->ratingID;
            }
            else
            {
                ratingID = title_ptr_v4->ratingID;
            }

            if(version<4)
            {
                for(ratingi=0; ratingi<header->ratings_total; ratingi++)
                {
                    if(header->ratings[ratingi].ratingID==ratingID)
                    {
			memset(str, 0, 256);
		    	ConvertUTF16ToUTF8(header->ratings[ratingi].title, str);
		    	fprintf(fil, str);
                    }
                }
            }
            else if(version>=4)
            {
                for(ratingi=0; ratingi<header_v4->ratings_total; ratingi++)
                {
                    if(header_v4->ratings[ratingi].ratingID==ratingID)
                    {
			memset(str, 0, 256);
		    	ConvertUTF16ToUTF8(header_v4->ratings[ratingi].title, str);
		    	fprintf(fil, str);
                    }
                }
            }
            fprintf(fil, "\r\n");

            if(version<4)timestamp_u32 = title_ptr->release_date;
            if(version>=4)timestamp_u32 = title_ptr_v4->release_date;

            if(timestamp_u32==0xffffffff)
            {
                fprintf(fil, "No title release date.");
            }
            else
            {
                GetTimestamp(timestamp_u32, &timestamp);
                fprintf(fil, "Title release date: %d/%d/%d", timestamp.month + 1, timestamp.day_of_month, timestamp.year);
            }

            fprintf(fil, "\r\n");
            if(version<4)fprintf(fil, "ID: %u\r\n", (unsigned int)be32(title_ptr->ID));
            if(version>=4)fprintf(fil, "ID: %u\r\n", (unsigned int)be32(title_ptr_v4->ID));

            u32 titleid;
            if(version<4)titleid = title_ptr->titleID;
            if(version>=4)titleid = title_ptr_v4->titleID;
            sprintf(str, "titleID: %c%c%c%c\r\n", (char)titleid, (char)(titleid>>8), (char)(titleid>>16), (char)(titleid>>24));
            fputs(str, fil);
			fprintf(fil, "\n");

	if(version<4)fprintf(fil, "Title info URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/soft/%s/%s/%u.info\r\n\r\n", region_code, (int)version, country_code, language_code, (unsigned int)be32(title_ptr->ID));

	if(version>=4)fprintf(fil, "Title info URL: https://a248.e.akamai.net/f/248/49125/1h/ent%cs.wapp.wii.com/%d/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/soft/%s/%s/%u.info\r\n\r\n", region_code, (int)version, country_code, language_code, (unsigned int)be32(title_ptr_v4->ID));

		}
		fclose(fil);
	}

	free(buffer);
	printf("Done.\n");
	return 0;
}

