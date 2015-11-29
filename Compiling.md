This wiki page explains how to compile Wmb Asm from [SVN.](http://en.wikipedia.org/wiki/Subversion_(software))

## Details ##

First you need software to checkout the Wmb Asm source code from the SVN respiratory. You can find a client from this [list,](http://subversion.tigris.org/links.html#clients) or use the command-line [client.](http://subversion.tigris.org/) If you're using TortoiseSVN, to checkout/download the source code, browse to the directory you want to checkout the source to, then right-click, choosing "SVN Checkout...". Copy the svn respiratory URL to the first text box, which is "https://wmb-asm.googlecode.com/svn". If you don't want to checkout the whole respiratory, add "/trunk" to the end of the URL. Remove the "\wmb-asm" at the of the checkout directory if you want to directly checkout into the current directory, instead of wmb-asm directory which will be created. Consult your client's documentation for instructions on doing checkouts, if you're not already aware how to checkout.

## Compiling ##

Wmb Asm can be compiled with the Makefiles: Makefile.nix for Linux, Makefile.win for Windows, and Makefile for NDS. Wmb Asm from SVN has bugs. See the Wmb Asm PC known bugs text [file](http://wmb-asm.googlecode.com/svn/trunk/wmb_asm/known_bugs.txt) for the PC version's issues, and the NDS known bugs text [file](http://wmb-asm.googlecode.com/svn/trunk/wmb_asm/NDS_known_bugs.txt) for the DS bugs.