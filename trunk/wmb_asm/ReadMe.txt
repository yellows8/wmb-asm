wmb_asm v2.0 by yellowstar 06/07/08

This command-line program assembles libpcap .cap files of DS WMB Download Play captures into a .nds.

Right now, this tool is made for compiled for Windows only. However, other platforms are supported, but it needs to be compiled
for the other platforms. The general public needs to wait for devs to compile & host those binaries, before you can use it on those platforms.

The following is what appears when you start the program without passing any parameters to it:

wmb_asm.exe by yellowstar 03/28/08
Usage:
wmb_asm.exe <options> [List of captures]
Options:
-rsa The program will check the RSA-signature, with this option
Example: wmb_asm.exe -rsa capture1.cap capture2.cap...

To use this easily, copy your capture into this directory,
rename it to capture.cap, then execute/double-click assemble.bat. Note that with this way,
the RSA-signature is not checked. To have the program assemble & check the RSA, do the exact same thing,
except this time double-click-execute assembleRSA.bat.

The RSA check will not work unless you have ndsrsa.exe. Goto this URL for that.
Download it, then copy ndsrsa.exe to this directory. Wmb_asm will now check the RSA, when told to, or when you
use assembleRSA.bat.

Nintendo DS use:
First, DLDI patch wmb_asm.nds for your card. Copy it to your card.
Copy your capture to the root of your card, and rename it capture.cap.
Run wmb_asm.nds, and the capture should be assembled. Once done, the output should appear on the root of your card.
Restart your DS and goto your card's menu. The output should appear.

Compiling
This program was built with the GNU compiler, but others should work, but some changes might be needed for other compilers.
Makefile.win is the Makefile for compiling for PC/Windows. You'll probably need to edit this to make it compile on other platforms.
Makefile is the Makefile for compiling on DS.

Credits:
Juglak, for his data structures and defines for WMB.
Masscat, for his WMB client for reference and code.
Frz, for his CRC calculation code, and other things.
sgstair, for his packet capture on DS example code.
