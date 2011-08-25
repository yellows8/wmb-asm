ninchdl-listext v1.1 by yellowstar6. Utility to decrypt, decompress, read and dump info and URLs from Nintendo Channel download lists, either locally from HD, or directly from the servers. ninchdl-listext requires libiconv.
wc24pubk.mod from Nintendo Channel is required. This is identical for all regions and versions of Nintendo Channel. This can be dumped from the Nintendo Channel data directory on Wii NAND. ninchdl-listext requires wc24decrypt.
This is stored at /title/00010001/484154xx/data/wc24pubk.mod, where xx is hex region code. 45 is E, 4a is J, 50 is P.
In US Nintendo Channel content 00000018.app, the WC24 RSA public key is located at 0x32b938 and the WC24 AES key is located at 0x32ba38.
In JP Nintendo Channel content 0000001f.app, the WC24 RSA public key ia located at 0x2f9670 and the WC24 AES key is located at 0x2f9770.
To create the wc24pubk.mod file from these keys, copy the public key to offset 0, length 0x100 bytes. Add zero bytes, length 0x100.
Copy the AES key to offset 0x200, length 0x10. Add zero bytes, length 0x10.
The AES key can be copied to a seperate file, but this requires an extra parameter.

Help displayed when run without paramaters:
Usage:
ninchdl-listext <wc24dl.vff> <options>
Input can be a compressed dl list as well.
Alternate usage:
ninchdl-listext <country code> <language code> <region char> <version> <wc24pubk.mod> <options>
See either source code or google code wmb-asm NintendoChannel wiki page for list of country and language codes.
region char must be either u, e, or j.
wc24pubk.mod is the filename for the NinCh WC24 keys.(Can also be the 16 byte AES key.) The default is wc24pubk.mod if this is ommitted.
Options:
-l<ID> List titles with a title type ID or console model ID. If the ID is ommitted, the whole title list is listed.

Source is available on SVN at http://wmb-asm.googlecode.com/svn/trunk/ninchdl-listext/

Changelog:
v1.1
Added proper UTF-16 to UTF-8 conversion with libiconv.
Fixed endian bug with dumped title IDs.
Added dumping of dl list header info: dl list ID, dl list version, country, language, and region.
Fixed crash that occurs when dumping outdated dl lists. A warning is written to the dump file and to the terminal when outdated dl lists are used, however these dl lists can still be parsed.(Outdated meaning old version.) Local outdated dl lists from HD can't be used.
Fixed crash that occurs when any of the executed software fails by immediately exiting.(wc24decrypt, wget, and curl.)

v1.0
Inital release.

Credits:
Andre Perrot for his gbalzss.c
