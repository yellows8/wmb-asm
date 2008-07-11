//This whole file is written by yellowstar.

#include <time.h>


#define TYPE_802_11 105
#define TYPE_802_11_AND_AVS 127

typedef struct//This structure is at the start of every capture
{
	unsigned int magic;
	unsigned short majorVersion;
	unsigned short minorVersion;
	unsigned int timeZoneOffset;
	unsigned int timeStampAccuracy;
	unsigned int snapshotLength;
	unsigned int linkLayerType;
} PACKED pcapDumpHeader;

typedef struct//After the pcapDumpHeader, there's the packets. First there's this pcapDumpPacketHeader struct, then the actual packet data. Then there's the next packet, done the same way. And so on.
{
	long tv_sec;
    long tv_usec;
	unsigned int packetLength;
	unsigned int captureLength;
}  PACKED pcapDumpPacketHeader;

#define AVS_magic    0x08021100//If you think about it, this magic string looks like 0 802.11 00
#define AVS_revision 0x01//1/01
#define AVS_PHY_type 4//67108864//4//802.11 type I guess... (DSSS 802.11b in this case)
#define AVS_DATA_RATE 20//335544320//14
#define AVS_SSI_TYPE 1//16777216//1
#define AVS_PREAMBLE 1//-805175296//1
#define AVS_ENCODING 1//-1//1

typedef struct//AVS WLAN Capture header. Put before the actual packet data, but after the pibpcap packet header.
{
       unsigned int magic_revision;//Conatins both a magic number and the revision in one.
       //Use if((avs->magic_revision & AVS_magic) && (((unsigned char*)&avs->magic_revision)[3] & AVS_revision)) to check if it's correct
       unsigned int header_length;//The length of the rest of of the AVS header
       double MAC_timestamp;//I have no idea about this two timestamps...
       double host_timestamp;
       unsigned int PHY_type;//Should be AVS_PHY_type
       unsigned int channel;
       unsigned int data_rate;//AKA transfer rate. I.e 2.0 mbps...
       unsigned int antenna;
       unsigned int priority;
       unsigned int SSI_type;
       unsigned int RSSI_signal;
       unsigned char unk[4];//Some 32-bit field that Wireshark passed over...(Or 4-bytes long)
       unsigned int preamble;
       unsigned int encoding_type;

} PACKED AVS_header;
