Wmb Asm Command-Line v2.0b r2 by yellowstar 07/11/08

This command-line program assembles libpcap .cap files of DS WMB Download Play captures into a .nds.

Setup instructions: http://code.google.com/p/wmb-asm/wiki/Setup

Right now, this tool is made for compiled for Windows only. However, other platforms are supported, but it needs to be compiled
for the other platforms. The general public needs to wait for devs to compile & host those binaries, before you can use it on those platforms.

The following is what appears when you start the program without passing any parameters to it:



wmb_asm.exe v2.0b r2 by yellowstar built on [Time and date the software was compiled will be displayed here]
Usage:
wmb_asm.exe <options> [List of captures and directories]
Options:
-rsa The program will check the RSA-signature, with this option
-nds_capdir Assembled binaries will be written to the captures' directory(defaul
t)
-nds_curdir Assembled binaries will be written to the program's working/current
directory
-nds_dirDir Assembled binaries will be written to Dir.
-run Opens the output once done as if you double-clicked it in Windows Explorer.

-stop Displays the "Press any key to continue..." when done.(Default)
-nostop Similar to the previous option, except that text isn't displayed, if the
 captures were assembled successfully.
-copy_dirDir Similar to -nds_dirDir, except the output is copied to Dir after th
e assembly.
-showtime Display how long the the whole assembly process took, at the end, in s
econds.(Default)
-notime Similar to the previous option, except the time elapsed isn't displayed.

-debug Write to a debug log file. Intended for debugging, and fixing binaries no
t assembled correctly.
-nodebug Don't write to a debug log file.(Default)
Example: wmb_asm.exe -rsa -nds_dirbinaries capture1.cap capture2.cap captures mo
re_captures...
With that, the RSA-siganture would be checked if ndsrsa.exe is in the current di
rectory,
output would be written to the binaries directory,
capture1.cap, capture2.cap would be used,
and captures and more_captures directories would be scanned for captures.



To use this easily, copy your captures into the captures directory,
then execute/double-click assemble.bat. Note that with this way,
the RSA-signature is not checked. To have the program assemble & check the RSA, do the exact same thing,
except this time double-click-execute assembleRSA.bat.

The RSA check will not work unless you have ndsrsa.exe. Goto this URL for that.
Download it, then copy ndsrsa.exe to this directory. Wmb_asm will now check the RSA, when told to, or when you
use assembleRSA.bat.

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
