getwiimsg v1.1 by yellowstar6. Download Wii Message Board announcements from server, decrypt,
and dump to .eml and .txt files. The internal KD AES key for wc24msgboardkey.bin is required.
In IOS35 v3088, this key is stored at 0x5f780 in 00000006.app. getwiimsg requires wc24decrypt.

Help displayed when run without parameters:
Usage:
getwiimsg <country code number> <language code> <wc24msgboardkey.bin> <optional list of alternate msg files to process from a server or locally>
language code can be one of the following: ja, en, de, fr, es, it, nl.

Source is available on SVN at http://wmb-asm.googlecode.com/svn/trunk/getwiimsg/

Credits:
Bob Trower for his Base 64 implementation.

Changelog:

v1.1
Added downloading of missing first 2 of 4 server messages.(msg #4 is newest, #3 older than #4, and so on.)

v1.0
Inital release.
