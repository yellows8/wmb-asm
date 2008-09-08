Wmb Asm DS beta 0.1
This DS version will remain as a beta until all the bugs have been fixed. Once the bugs are fixed, the version
will be updated to the Wmb Asm PC version.

This DS version has serious bugs which stop the assembly of the .nds. So this doesn't work very well, if at all.

Usage:
First, DLDI patch wmb_asm.nds for your card. Copy it to your card.
Copy your capture to the root of your card, and rename it capture.cap.
Run wmb_asm.nds, and the capture should be assembled. Once done, the output should appear on the root of your card.
Restart your DS and goto your card's menu. The output should appear.

Compiling
Makefile is the Makefile for compiling on DS. You can obtain the source code from SVN, from the Google Code project page.
All the files in the source and include directories, in the wmb_asm directory, are for Wmb Asm DS.

Wmb Asm Google Code Project page: http://code.google.com/p/wmb-asm/

Credits:
Juglak, for his data structures and defines for WMB.
Masscat, for his WMB client for reference and code.
Frz, for his CRC calculation code, and other things.
sgstair, for his packet capture on DS example code.
chishm, for his DS Demo Decompressor tool, and for the DS Download Station LZO header format information.
