Wmb Asm Module v2.0b r2 by yellowstar 07/11/08

This module assembles libpcap .cap files of DS WMB Download Play captures into a .nds.

Setup instructions: http://code.google.com/p/wmb-asm/wiki/Setup

Right now, this tool is made for compiled for Windows only. However, other platforms are supported, but it needs to be compiled
for the other platforms. The general public needs to wait for devs to compile & host those binaries, before you can use it on those platforms.

Compiling
This program was built with the GNU compiler, but others should work, but some changes might be needed for other compilers.
Makefile.win is the Makefile for compiling for PC/Windows. You'll probably need to edit this to make it compile on other platforms.
Makefile is the Makefile for compiling on DS. You can obtain the source code from SVN, from the Google Code project page.

Wmb Asm Google Code Project page: http://code.google.com/p/wmb-asm/

Credits:
Juglak, for his data structures and defines for WMB.
Masscat, for his WMB client for reference and code.
Frz, for his CRC calculation code, and other things.
sgstair, for his packet capture on DS example code.
