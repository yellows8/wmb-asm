/*
Wmb Asm, the SDK, and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

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

#ifndef lib_pcap_h

    #ifndef _H_CUSTOM_PCAP
    #define _H_CUSTOM_PCAP

    #define PCAP_VERSION_MAJOR 2
    #define PCAP_VERSION_MINOR 4
    
    #define PCAP_ERRBUF_SIZE 256
	
    typedef unsigned char u_char;
    typedef unsigned int u_int;
    
    typedef struct spcap_file_header {
	   unsigned int magic;
	   unsigned short majorVersion;
	   unsigned short minorVersion;
	   unsigned int timeZoneOffset;
	   unsigned int timeStampAccuracy;
	   unsigned int snapshotLength;
	   unsigned int linkLayerType;
    } __attribute__ ((__packed__)) pcap_file_header;

    typedef struct spcap_pkthdr {
	   long tv_sec;
        long tv_usec;	/* time stamp */
	   unsigned int caplen;	/* length of portion present */
	   unsigned int len;	/* length this packet (off wire) */
    } __attribute__ ((__packed__)) pcap_pkthdr;
    
    typedef struct spcap_t
	{
		FILE *file;
		bool swap;
		pcap_pkthdr header;
		unsigned char *pktdata;
		char *error_buffer;
	} __attribute__ ((__packed__)) pcap_t;
	
	#ifdef __cplusplus
    extern "C" {
    #endif
	
        pcap_t  *pcap_open_offline(const char *filename, char *errbuf);
        int     pcap_next_ex(pcap_t *file, pcap_pkthdr **hdr, const u_char **pktdata);
        void    pcap_close(pcap_t *file);
        char    *pcap_geterr(pcap_t *file);
        
        int GetPacketNumber();
        int GetSnapshotLength();
    
    #ifdef __cplusplus
    }
    #endif

    #endif
    
#endif
