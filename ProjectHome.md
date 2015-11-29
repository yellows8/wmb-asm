Assemble packet [captures](captures.md) into a .nds, to be played anytime you want with a flash card.
Also access the Nintendo Channel servers with ninchdl-listext, wc24decrypt, and binconv. As of Nintendo Channel v4 released in JP/US/PAL as of December 15 2009, videos are currently unplayable since the format was switched to a custom .mo format.
Mount Wii NAND images with FUSE via wiinandfuse. Use WiiConnect24 with Wii homebrew via libwc24. The libwc24 v1.0 tarball source has a ISFS heap buffer overflow [bug](http://code.google.com/p/wmb-asm/source/browse/trunk/libwc24/README), this is fixed in SVN.(The pre-compiled library and wc24app.dol do not have this bug.)

Supported input formats:
  * libpcap .cap [captures](captures.md)

Supported protocols/services:
  * [WMB/"DS Download Play"](WMB.md)
  * [DS Download Station](DSDownloadStation.md)
  * [Nintendo Channel](NintendoChannel.md), via ninchdl-listext, wc24decrypt, and binconv

Supported platforms:
  * Windows

  * wmb\_asm: Linux is supported, but must be compiled from SVN since binaries aren't available

  * Linux is fully supported for other tools

  * wmb\_asm works with WINE, but Radiotap [captures](captures.md) can only be assembled with SVN wmb\_asm