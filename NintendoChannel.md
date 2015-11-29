Wikipedia explanation of the [Nintendo Channel.](http://en.wikipedia.org/wiki/Wii_Menu#Nintendo_Channel) As said on Wikipedia, the Wii Nintendo Channel allows you to send DS Demos to your DS, from the Wii. This wiki page has info on the format of the Nintendo Channel demo, video, ect, list, as well as the list URL and some WC24 info.
This page contains technical Nintendo Channel info. The Nintendo Channel servers can be accessed with ninchdl-listext and wc24decrypt, see downloads list. As of NinCh v4 released in JP/US/PAL as of December 15 2009, video format was changed from .3gp to .mo. This is a custom and currently unplayable format, seems to be based on Mobiclip encoding.

# Startup #

When the Nintendo Channel starts up, it requests WC24 to download wc24data.LZ from server entx.wapp.wii.com.(csdata.LZ with latest JP/US/PAL NinCh as of December 15 2009.) x varies, depends on region. A list of URLs for this file for various regions and languages is available below. The decrypted file is LZSS compressed. The list of DS Demos, and videos, that are available, are contained in this file, prior to NinCh v4. Complete WC24 content encapsulation format and encryption info has been moved to [Wiibrew.](http://wiibrew.org/wiki//shared2/wc24/nwc24dl.bin)

wc24data.bin/csdata.bin contained in wc24dl.vff, in the Nintendo Channel data directory, is a decrypted wc24data.LZ/csdata.LZ file. This is downloaded and decrypted by WC24/KD. Decompression must be done manually. wc24data.bin can be extracted from wc24dl.vff,(VFF resembles FAT16)
by copying data from 0x3220, length from little-endian u32 from offset 0x209c.(Filesize field for wc24data.bin dirent, relative offset to dirent is 0x1c.)(With lastest JP NinCh(NinCh v4), this file data appears to have been moved to 0x1620. The root dir has been moved as well, length field is at 0x49c.) Wii, DSi, VC, and Wii Channels are included in this file.(wc24data.bin) The header is 0x6b bytes long. First 5 bytes are the version? These byte are identical to the first 5 bytes in .bin demos. In non-jp bins, all of these bytes are zero except for the third. With latest JP NinCh, the fourth byte is 0x39.

The content downloaded by WC24, initiated by NinCh v4, is just WC24 banner data, version and region data. The header seems to be similar, however the v3 data starting at 0x2f has been removed. The v4 dl list format seems to be similar to the v3 format, though the format was changed slightly and more data was added.(See the next paragraph for v4 header info.)
At offset 0x4 is a u32 for the filesize of the whole dl list. At offset 0xc is a u32 the dl list ID. At offset 0x10 is a u32 for the country code. At offset 0x14 is a u32 for the language code.(Or console area.) At offset 0x2f is the number of ratings entries. At offset 0x35 is the u32 offset for the ratings list, for the first entry.(Offset doesn't include the highest age byte.) At offset 0x37 is the number of title type entries. At offset 0x3b is the offset for the "main-title" type list.(Wii(disc game), channels, WiiWare, ect.) At offset 0x3f is the u32 for the number of game dev and publishing companies entries. At offset 0x43 is the u32 offset for the game development and publishing companies list. At offset 0x47 is the u32 offset for the "sub-title" type list.(Specific types, like VC type.) At offset 0x4b is a offset for the title list. At offset 0x4f is a unknown u32, possibly related to the title list. At offset 0x53 is a u32 for a offset of unknown data between the title list and video list. At offset 0x57 is a u32 for the number of video entries. At offset 0x5b is the u32 offset for the video entries list. At offset 0x5f is a u32 for the number of demo entries. The following u32 at 0x63 is the offset of the demo entries list. The following u32 at offset 0x67 is a offset for the text appended to WC24 messages, ect. Following the header is 3 strings, each are allocated 0x100 bytes. These strings are identical, and match a portion of the URL used for .bin demos.

v4 header info as follows.(See the above paragraph for info before and at offset 0x14.)
At offset 0x25 is a u32 of the number of ratings. At offset 0x29 is a u32 for the offset of the ratings list. At offset 0x2d is a u32 for the number of title types. At offset 0x31 is a u32 for the offset of the title types list. At offset 0x35 is a u32 for the number of companies. At offset 0x39 is a u32 for the offset of the company list. At offset 0x3d is a u32 for the number of titles. At offset 0x41 is a u32 for the offset of the title list. At offset 0x4d is a u32 for the number of videos in list 0. At offset 0x51 is a u32 for the offset of video list 0. At offset 0x5d is a u32 for the number of demos. At offset 0x61 is a u32 for the offset of the demo list. At offset 0x95 is a u32 for the number of videos in list 1. At offset 0x99 is a u32 for the offset of the videos list 1. At offset 0x9d is a u32 for the number of detailed ratings. At offset 0xa1 is a u32 for the offset of the detailed ratings list.

Following these strings is the ratings list.(This paragraph pertains to v3 dl list, see next paragraph for v4.) The first byte,(not included in entries) may be the years of age for the highest rating.
Ratings board entry format as follows: First byte is the rating ID. The next byte is unknown. Perhaps it's an ID for the rating board for this region? With US/en, this is always 2. Next byte appears to be the required age for this rating. Next is the UTF-16 title for this rating.(E, E10, EC, ect)
Rating title has 0x16 bytes allocated. Entries are 0x19 bytes long?

v4 ratings list format as follows. Preceding the ratings list is 4 bytes. The first may be the years of age for the highest rating. The second byte may be the rating board ID? The following 2 bytes are unknown.
The size of the rating entry is now 0x22 bytes. First byte is rating ID. Next byte is unknown. Next byte at offset 2, is years of age required for this rating. Next byte is unknown at offset 3, usually matches last byte of ratings "header". The following u32 at offset 4, is the offset for the rating board JFIF(.jpg) image, for this rating. The following u32 at offset 8 is unknown.

Following this is a list of title types.(This paragraph pertains to the v3 dl list, see the next paragraph for v4.) Format as follows: first byte is title\_group? The next byte is title\_type. Following this is the UTF-16 title type. This has 0x3e bytes allocated. Entries are 0x40 bytes long.

v4 title type entry format as follows. Entry size is 0x44 bytes. First byte is type ID. Next 3 characters are the console model code. Following this, is the title type description, allocated 0x40 bytes. Following the description is the title type group ID. The next byte is unknown, usually matches the rating list "header" last byte.

After the title type list, there's a list of game development/publishing companies.(The v4 company entry format is identical to the v3 dl list format.) For each entry, there's an ID? Then there's the 2 UTF-16 strings. These are usually identical. Each string is allocated 0x3e bytes. Each entry is 0x80 bytes long.

Following the title type list is the title list. First u32 is a "titleid". Next there's a u32, which the Wii/DSi titleid, or the DS Game Card gamecode, depending on title type. The title\_type byte follows. Then there's an unknown byte. 4 bytes after that, is the absolute u16 offset for the game development/publishing company entry. 4 bytes following the offset is the rating id byte? 0x13 byte following that, is 2 UTF-16 strings for the title. The first string has 0x7c bytes allocated, while the second has 0x3e bytes allocated. Title entries are 0xe2 bytes long? v4 entry size is 0xec bytes. Known fields seem to be identical to v3 dl list entry format, however the description of the title is moved to offset 0x32.

Following the title list, is the list of videos.(This section is for v3, see end of this paragraph for v4.) First u32 of each entry is the decimal .3gp video ID. 2 bytes after the ID is the "titleid".(Associated title entry's ID.) 0x5 bytes after the "titleid" is the UTF-16 video title, allocated 0x48 bytes. Video entries are 0x75 bytes long? Following the video list is the list of demos. v4 entry size is expanded to 0x115 bytes.
Video ID is unknown. "titleid" at offset 6. 0x20 bytes following that is the video UTF-16 title.

v4 video entry2 format:
Entry size is 0xea bytes. Video ID is unknown. "titleid" at offset 2. 0x1e bytes following that is the video UTF-16 title.

Each demo entry begins with the u32 for the demo ID. This is the decimal ID for the .bin filename on the server. Following this is the UTF-16 title and subtitle, each allocated 0x3e bytes.(Subtitle is optional) Following the title is a u32 "titleid". This is associated with the "titleid" in the associated title. The following u32 is the offset of the company entry.(Should always be identical to the associated title's company's entry offset.) The following 4 bytes is a end of distribution timestamp? 0xffffffff when there is no removal date, a timestamp otherwise: first two bytes are a u16 for the year. Next byte is the zero-based month. Next byte is day of month. Following this is two unknown bytes.(v3 only) Demo entries are 0x8e bytes long? Not much changed for the v4 demo entry format. 0xd2 unknown bytes were added after the removal date timestamp. The first 4 unknown bytes are usually zero, and the following 3 bytes vary. The rest are usually zero.
Entry size is expanded to 0x160 bytes.

List of Nintendo Channel wc24data.LZ/csdata.LZ/dl list URLs:

The characters after 3/ are the country code, and the following characters after the region code is the language.) The country codes seem to match the country codes on [Wiibrew](http://wiibrew.org/wiki/Country_Codes) and [www.uspto.gov/patft/help/helpctry.htm www.uspto.gov]. The language code table is as follows: ja en de fr es it nl. It appears that if the country code is not in the NinCh country code table, it uses ? as the country code, and that will only produce an error. NinCh v4 is region locked. JP NinCh will not work on a US Wii, even with language code patches.(NinCh locks the language code to only to select languages, for each region.)
For NinCh v4, the real dl list is stored under https://a248.e.akamai.net/f/248/49125/1h/entus.wapp.wii.com/4/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/list/RR/ll/iiiiiiiii.LZ
Replace "entus" 'u' with the correct region char. RR and ll is the region and language codes. iiiiiiiii is the dl list ID, from offset 0xc of the decrypted and decompressed csdata.LZ.

USA WC24 banner data, ect:
http://entu.wapp.wii.com/4/US/en/csdata.LZ

Germany: http://ente.wapp.wii.com/3/DE/de/wc24data.LZ

France: http://ente.wapp.wii.com/3/FR/fr/wc24data.LZ

Spain: http://ente.wapp.wii.com/3/ES/es/wc24data.LZ

Great Britain: http://ente.wapp.wii.com/3/GB/en/wc24data.LZ

Denmark: http://ente.wapp.wii.com/3/DK/en/wc24data.LZ

Italy: http://ente.wapp.wii.com/3/IT/it/wc24data.LZ

Netherlands: http://ente.wapp.wii.com/3/NL/nl/wc24data.LZ

Belgium, French language: http://ente.wapp.wii.com/3/BE/fr/wc24data.LZ

Belgium, Netherlands language: http://ente.wapp.wii.com/3/BE/nl/wc24data.LZ

Australia:
http://ente.wapp.wii.com/3/AU/en/wc24data.LZ

Japan WC24 banner data, ect:
http://entj.wapp.wii.com/4/JP/ja/csdata.LZ

# Downloading #

When you click on any demo on the DS Download Service screen, the Wii will begin to download the demo from the server, with the "Preparing..." screen being displayed as it does this.

The server hostname the Nintendo Channel connects to is a248.e.akami.net. This is a redirection server. The server that the client is redirected to is the same server that the wc24data.LZ/csdata.LZ file is downloaded from, for this region.
Nintendo Channel uses this base URL for demos: https://a248.e.akamai.net/f/248/49125/1h/entus.wapp.wii.com/3/VHFQ3VjDqKlZDIWAyCY0S38zIoGAoTEqvJjr8OVua0G8UwHqixKklOBAHVw9UaZmTHqOxqSaiDd5bjhSQS6hk6nkYJVdioanD5Lc8mOHkobUkblWf8KxczDUZwY84FIV/dstrial/RR/ll/iiiiiiiii.bin
Replace RR with region, and ll with language.(Same region/language from wc24data.LZ/csdata.LZ URLs)
Replace iiiiiiiii with the demo/video ID. Replace the fourth char in "entus" with the region char.(Same as wc24data.LZ/csdata.LZ server domain name region char.) The 3 after "wapp.wii.com" is the NinCh version. The videos base URL is similar to demos, except "movie" replaces "dstrial", before the region/language directories. The NinCh v3 videos are 3gp, and NinCh v4 videos are .mo. With NinCh v4, -l or -h is appended to the ID, for low and high quality videos, respectively. a248.e.akamai.net is a redirection server to the http://entx.wapp.wii.com server, the server wc24data.LZ/csdata.LZ is downloaded from. NinCh v4 sample videos are stored under system rather than movie, and testmovie replaces the ID. The DS software used to obtain the ID of a game when recommending games, is downloaded from as similar path as demos, except "system" replaces "dstrial", and the ID is 1. When NinCh v4 begins streaming videos, it first downloads a file from entxm.wapp.wii.com with https, x being region code.(This excludes the sample video.) NinCh v4 will halt and throw an error if downloading of this file fails. Path is the same as the main server path, except .mo is removed. This file is 2 bytes: "1\n". A '1' char and LF, 0x0a.

# TLS #

[TLS](http://en.wikipedia.org/wiki/Secure_Sockets_Layer) 1.0 is used. /dev/net/ssl uses the simple handshake mode described on Wikipedia.(NinCh seems to use /dev/net/ssl, but it is unknown if WC24 uses SSL v3 or TLS.) Here's the client Cipher Specs in the ClientHello packet, from Wireshark:

  * Cipher Spec: TLS\_RSA\_WITH\_AES\_256\_CBC\_SHA (0x000035)
  * Cipher Spec: TLS\_RSA\_WITH\_AES\_128\_CBC\_SHA (0x00002f)
  * Cipher Spec: TLS\_RSA\_WITH\_3DES\_EDE\_CBC\_SHA (0x00000a)
  * Cipher Spec: TLS\_RSA\_WITH\_DES\_CBC\_SHA (0x000009)
  * Cipher Spec: TLS\_RSA\_WITH\_RC4\_128\_SHA (0x000005)
  * Cipher Spec: TLS\_RSA\_WITH\_RC4\_128\_MD5 (0x000004)

The issuer is GTE CyberTrust Global Root. And this shows up in another section: Akamai Technologies, Inc.

## Tracking ##

Nintendo Channel sends your Wii serial number and MAC address in the User Agent field sent to TLS secured Nintendo Channel servers. This is done for the main servers, but it is not known for certain if they do this for the cgi scripts used for play stats. Nintendo Channel sends play stats of every title with a ticket/tmd on your NAND which exists in the Nintendo Channel dl list title list database. Nintendo claims you remain anonymous.

## .bin ##

When the Wii downloads DS Demos, the file downloaded is a .bin file.
The first five bytes are the NinCh version, same as dl list URL version directory. Currently only the third byte is used.
All a NinCh demo .bin is, really, is some 0x1C8 byte long header, then the rest is an .nds.(0x14c bytes for NinCh v4.) Chop off the first 0x1C8/0x14c bytes, and you have a .nds. However, the banner description was changed. With .bin, the description, isn't the real description. It contains the name of the demo, and also the developer's company name.(With v4, the developer company name was removed.) The game name was moved to the .bin header, at offset 0x1c starting from the start of the .bin. The game name is 98 bytes, and is a UTF-16 Big-Endian string, as is the other strings in the header.This string is 49 UTF-16 characters long. Following the game name, at 0x7e, that's the game description, minus the game name. This is 194 bytes long, and is 97 UTF-16 characters long. After that text, at 0x140,(v3 only) is the demo name displayed on the Wii when the demo is being broadcasted. This is 123 bytes long, and is 61 UTF-16 characters long, with one byte of padding after the actual characters.
A stand-alone .bin to .nds converter is available in SVN and from the downloads section.