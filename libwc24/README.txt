libwc24 v1.1 by yellowstar6
This library allows Wii homebrew to use WiiConnect24. You need certain IOS to use libwc24 when accessing HBC data dir for creating and reading wc24dl.vff which contains the downloaded title data WC24 content. With IOS prior to the 3.4 update, you only need to identify as HBC, and wc24app has a option to identify as HBC. With IOS updated since the 3.4 update, you need a IOS with NAND permissions patches, since patched ES_Identify doesn't work for the second time it's called. These IOS requirements are needed because HBC reloads IOS when loading homebrew, which resets the current identification. In this directory tree is libwc24 and wc24app. The former is the actual library, and the latter is the WC24 test app. Each time libwc24 is compiled successfully, the header files and libwc24.a will be copied to $DEVKITPRO/libogc/include/wc24 and $DEVKITPRO/libogc/lib libwc24 can be also be installed by running this in the libwc24 directory: make install Optionally you can install libwc24 by copying the headers and library to the above libogc directories.
For more WC24 info, see the Wiibrew pages: http://wiibrew.org/wiki/WiiConnect24 http://wiibrew.org/wiki//shared2/wc24/nwc24dl.bin http://wiibrew.org/wiki/WC24_Content
The source is included with the download tarball, and is also available on SVN at: http://code.google.com/p/wmb-asm/source/checkout

These are the Internet server URLs that wc24app can install: http://members.iglide.net/ticeandsons/yellowstar/wc24test http://members.iglide.net/ticeandsons/yellowstar/wc24testmail
The former is the title data URL, the latter is the message board mail URL.
Refrence the Wiibrew WC24_Content page for info on the required WC24 header when content length is less than 0x140 bytes. Also see the above URLs installed by wc24app for template WC24 content. The title data file is supposed to only contain the payload text when written to wc24dl.vff, however the whole file is written there. The cause of this is currently unknown, but the all-zero 0x140 bytes-long header isn't needed when the actual content length is larger than 0x140 bytes.

Features and limitations:
VFF files can be read and written, with LFN support.(Long filenames)
The download frequency is specified in minutes, see: http://wiibrew.org/wiki//shared2/wc24/nwc24dl.bin
RSA signature verification for content can be disabled.
Content can optionally be downloaded only in idle/"standby" mode with a flag bit.
WC24 can download subTasks, where multiple URLs are downloaded with one task, but libwc24 can't use this.
Download content to a file in wc24dl.vff under HBC data dir.
Download message board mail "annoucements", like Nintendo's system update messages and channel messages.
Download content immediately.
Get and set the KD UTC time.

Known bugs:
Bugs not listed here can be reported at: http://code.google.com/p/wmb-asm/issues/list

1) With wc24app when you reload IOS when your system has IOS updated since the 3.4 update, ES_GetTitleID fails after the reload. This seems to be a libogc bug, ES_GetTitleID works fine when IOS isn't reloaded. libwc24 uses the HBC JODI titleid as default when ES_GetTitleID fails, so this error isn't a problem unless you reload from another title.

2) The VFF devoptab directory reading code doesn't convert dirent names, short names need converted to simple names without the '?'.

Changelog:
v1.1:
The libwc24 v1.0 source tarball(not the pre-compiled library and dol) had a ISFS heap buffer overflow bug. Only 0x10 bytes for the size of a record is allocated for reading and writing records, but ISFS always reads/writes at least 0x20 bytes. This was fixed in libwc24 v1.1.
Added support for KD_SetNextWakeup and KD_GetTimeTriggers.
Added support for KD_Download flag sync UTC time.
KD_Open and KD_Close are now called by WC24_Init and WC24_Shutdown.
VFF files can be read and written with libff, but only MS-DOS 8.3 can be used still since LFN needs a Unicode library.
Fixed typo for the WC24_EHTTP304 define value. In libwc24 v1.0 this caused wc24app to skip VFF reading since wc24app/libwc24 didn't recognize the error from ent.error_code.
VFF files are now read and written with libff, with long filenames support.

v1.0:
Initial release.

Credits:
Raven's id.c from fs_browser for identifying as HBC
Bushing for VFF FAT size code: http://wiibrew.org/wiki/VFF
Elm Chan FatFs

