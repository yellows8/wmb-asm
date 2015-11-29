This page explains how to setup Wmb Asm. This software only uses libpcap .cap capture files
for the input files. You need a way to obtain [captures](captures.md). If you can't obtain captures,
this software isn't of much use.

Wmb Asm can be setup by downloading the latest package download. Separate archives for the module and command-line are available. Both are needed when not using the package archive.

### RSA ###

Wmb Asm can check the RSA-signature. This can be used to make sure the assembled .nds wasn't corrupted. However, the tool used to check/verify the signature, the download link is missing from the Read Me. This will be fixed in the next release, but here's the link now: [nds\_rsa\_1\_1.zip](http://users.belgacom.net/bn931507/nds_rsa_1_1.zip)(Not written by yellowstar6)
The signature can't checked without this tool, so if you want to check the signature, or confirm that the binary was assembled correctly, you'll need this tool.