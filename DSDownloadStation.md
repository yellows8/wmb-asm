# DS Download Station protocol #

This page contains technical information on the DS Download Station protocol,
in a wiki format, based on Filb's gbadev [posts](http://forum.gbadev.org/viewtopic.php?p=157872#157872) and images. This wiki page is conversion of that information into a wiki format. Some of the information was found by yellowstar6, but most of it is Filb's, rewritten/redone in wiki format.

# Details #

Download procedure:
Download DLStation client over WMB -> Get clientID -> Request Menu -> Download Menu -> Request a menu item to download -> Download the item

## WMB client download ##

The WMB transfer is different from a normal transfer. The header size is 250 bytes long, about half the size of the normal header. The rest of the header needs appended with the header data from a official demo in order to get a complete header. The first data packet, after the header, is "fake". The real data doesn't start until the next packet. The data size is 102 bytes. Some demos transferred over WMB do the same thing. If a packet with data size 102 is detected as the first data seq, it can be ignored safely, and only the next data packets can be used for the actual data.

## Obtaining the clientID ##

After the client is downloaded and booted, the client connects to the host. The AID value, Association ID, in the Association Response packet is the clientID for the client. This is needed so the clients can check which data/menu packets are intended for itself, the client.

## DLStation packet format ##

The first byte after the 802.11 frame header is the type of this packet. 0x04 is download request, 0x06 is data packets. The data packets are very similar to WMB data packets. The main differences are the fact that the flags field was removed, and the data added after the actual data being transferred in the packets. Some packets can be mistaken as DLStation download request packets when they are really WMB ack packets. WMB Client-to-host ack packets use the same 0x04 byte for the first byte in the payload, but the byte after that is fixed. Filtering out the values of the second byte used in WMB ack packets fixes this problem. Note that the format used for the table for the formats isn't very good, due to issues with Google Code wiki. None of these DLStation fields overlap.

Download request packet format

> byte 0 | byte 1 | byte 2 - end of data

> 0x04   | Unknown | Request

  * Unknown: It is unknown what this is.
  * Request: If this is not 0xFF, then this packet is a download request, otherwise it's packet with a unknown purpose. The request is a UTF-8 string, without any null-terminating characters. The data ends at the 02 00 marker at the end of the packet. This marker marks the end of the actual data, and also signifies that there's an FCS at the very end of the packet, as in WMB. When the first 4 characters of the request are "menu",(without the quotes)
> then this packet is a menu request. When the first 4 characters are not "menu", it is a download request for a demo/download.


Data packet format

> byte 0 - 3  | byte 4 | byte 5 | byte 6 - 7 | byte 8 - byte 8 + size | 2 bytes | rest of packet

> 06 02 01 00 | size | type | seq number | data                       | clientID | Unknown

  * size: Size of the data in 32-bit words. Multiple this field by 4 to get the datasize.
  * type: Type of the data packet. When 0x1e, this is a menu data packet. When 0x1f, this is a item/demo data packet.
  * data: The LZO1x compressed data. The first seq has a [header](http://forum.gbadev.org/viewtopic.php?p=78611#78611) before the compressed data.
  * clientID: The clientID that this packet is intended for. Somewhat similar to the IDs distributed in Association Response packets. This is the bit number(s) of the clientID(s) that the packet is intended for, starting with bit 1. Multiple bits can be set for multiple destination clients. Bit 1 is client 1, bit 2 is client 2, and so on. This is most likely the same for the following IDs and clientIDs.

## Packet request, menu, and download stage ##

After the clientID is obtained, the client sends a menu request to the host. The first four bytes are menu, the next character is an underscore, and the last 3 are the client's language. ger is for German. eng is for English. jpn is for Japanese. It seems the host always sends the same menu no matter what the client's language is. When the host receives the menu request packet, the host will eventually start sending the menu data packets with type 0x1e. Before the actual LZO compressed data for the menu, there is a [header.](http://forum.gbadev.org/viewtopic.php?p=78611#78611) Once the whole menu has been transferred, the host will idle and send pings for a while. Then it will send a data packet with data sequence number 0xFFFF. This seems to tell the client that the whole menu/download has been transferred. Following this, the client will decompress the LZO compressed data, then the menu will be displayed and viewed. When the first 4 characters of the download request are not "menu", it is a download request for a demo/download. The request contains a 8-byte gameID, from one of the menu items' gameID. This request is sent when the client chooses a item to download. After many handshakes and other things, the item will be transferred, in the same way as the menu, with the same LZO compression and header. The decompressed data is an .nds. It contains extra data from the DLStation, including controls, maybe the ESRB rating, and other unknown data.

## Menu data format ##

The first 4 bytes of the decompressed menu are either an magic number, or the DLStation volume/version number minus one, for example: DS Download Station vol. 7. In one menu from vol. 7, this was 08 00 00 00. Following this, is the menu items. Each item is 0x72C bytes long. First, there is 0x200 bytes of icon data, and 0x20 bytes of palette data, similar to the banner in a .nds. Following this, is the title of the item, 0x80 bytes long. Next is a subtitle, 0x80 bytes long. The next 0x402 bytes contain ESRB and controls information. The format of this section is unknown. Lastly, there is the 8-byte UTF-8 gameID string, without any null-terminating characters, then the item ends with two zero bytes.