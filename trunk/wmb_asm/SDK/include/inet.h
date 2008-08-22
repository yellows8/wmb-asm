#ifndef _H_WMB_ASM_SDK_INET
#define _H_WMB_ASM_SDK_INET

struct EthernetHeader//Ethernet II/2 header, as seen in Wireshark
{
    unsigned char src[6];
    unsigned char dst[6];
    unsigned short type;
} __attribute__ ((__packed__));

struct IPHeader
{
    unsigned char length:4, version:4;
    unsigned char diff_services;
    unsigned short total_length;
    unsigned short id;
    unsigned short flags_fragment;
    unsigned char time_to_live;
    unsigned char protocol;
    unsigned short header_checksum;
    unsigned int src;
    unsigned int dst;
} __attribute__ ((__packed__));

struct TCPHeader
{
    unsigned short src_port;
    unsigned short dest_port;
    unsigned int sequence_number;
    unsigned int ack_number;
    unsigned char reserved:4, header_length:4;
    unsigned char CWR:1, ECE:1, URG:1, ACK:1, PSH:1, RST:1, SYN:1, FIN:1;
    unsigned short window_size;
    unsigned short checksum;
    unsigned short urgent_pointer;
    unsigned int options;
} __attribute__ ((__packed__));

struct TCPseudoHeader//Pseudo header that is pre-appended to the the TCP header data in a seperate buffer, when doing checksum calculation.
{
    unsigned int src;//Src/dst are the same as the fields from the IP Header.
    unsigned int dst;
    unsigned char reserved;//Always zero.
    unsigned char protocol;//The protocol specified in the IP Header, always 6 since we're using only TCP.
    unsigned short tcp_length;//Length of the tcp header and data.
} __attribute__ ((__packed__));

struct EthernetHeader *CheckGetEthernet(unsigned char *data, int length, unsigned short type);
unsigned char *GetEthernet(unsigned char *data, int length, unsigned short type);

struct IPHeader *CheckGetIP(unsigned char *data, int length);
unsigned char *GetIP(unsigned char *data, int length);

struct TCPHeader *GetTCP(struct IPHeader *iphdr, unsigned char *data, int length, unsigned char **payload);

#endif
