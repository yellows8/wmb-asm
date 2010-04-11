Wmb Asm Command-Line v2.0b r2 by yellowstar 07/11/08

This terminal program assembles libpcap .cap files of WMB/"DS WMB Download Play" and DS Download Station transfers into a .nds.

The following is what appears when you start the program without passing any parameters to it:

wmb_asm v2.0b r2 by yellowstar6
Usage:
wmb_asm <options> [List of captures and directories]\n");
Options:\n");
-rsa The program will check the RSA-signature, with this option
-nds_capdir Assembled binaries will be written to the captures' directory(default)
-nds_curdir Assembled binaries will be written to the program's working/current directory
-nds_dirDir Assembled binaries will be written to Dir.
-run Executes the emulator command/path defined by env variable NDSEMU with the filename being the assembled .nds.
-copy_dirDir Similar to -nds_dirDir, except the output is copied to Dir after the assembly.
-showtime Display how long the the whole assembly process took, at the end, in seconds.(Default)
-notime Similar to the previous option, except the time elapsed isn't displayed.
-debug Write to a debug log file. Intended for debugging, and fixing binaries not assembled correctly.
-nodebug Don't write to a debug log file.(Default)
Example: wmb_asm -rsa -nds_dirbinaries capture1.cap capture2.cap captures more_captures...
With that, the RSA-siganture would be checked if ndsrsa is in the current directory,\noutput would be written to the binaries directory,\ncapture1.cap, capture2.cap would be used,\nand captures and more_captures directories would be scanned for captures.



On Windows, to use this easily, copy your captures into the captures directory,
then execute/double-click assemble.bat. Note that with this way,
the RSA-signature is not checked. To have the program assemble & check the RSA, do the exact same thing,
except this time double-click-execute assembleRSA.bat.

The RSA check will not work unless you have ndsrsa.exe. Goto this URL for that.
http://users.belgacom.net/bn931507/nds_rsa_1_1.zip
Download it, then copy ndsrsa.exe to this directory, if your using Windows. Wmb_asm will now check the RSA signature, when told to, or when you use assembleRSA.bat on Windows. You'll need to adjust the ndsrsa source code, and compile it yourself, if your using Linux.

Compiling
This program was built with GCC, but others should work, but some changes might be needed for other compilers.
Makefile.win is the Makefile for compiling for Windows. Makefile.nix is the Makefile for compiling on *nix based platforms.(Linux etc)
Makefile is the Makefile for compiling on DS. You can obtain the source code from SVN, from the Google Code project page.

Wmb Asm Google Code Project page: http://code.google.com/p/wmb-asm/

Credits:
Juglak, for his data structures and defines for WMB.
Masscat, for his WMB client for reference and code.
Frz, for his CRC calculation code, and other things.
chishm, for his DS Demo Decompressor tool, and for the DS Download Station LZO header format information.
