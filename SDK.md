# Wmb Asm SDK #

This page describes what the Wmb Asm SDK is, and, how developers can use it, when
the SDK is ready.


## What is it? ##

The Wmb Asm SDK will allow developers to use the Wmb Asm Module, to use it to assemble any capture that the Wmb Asm Module is capable of assembling. The Command-line will use the SDK, and the work-in-progress front-end also.

There will also be a module known as a Packet Module Plugin. The SDK will also allow developers to develop these plugins. This plugins will handle the packets. The Wmb Asm will do some pre-processing, such as handling the AVS header and other things, but the plugins handle the rest of packet handling. The Wmb Asm Module will no longer handle protocols with built-in code, the plugins will handle that. The plugins will handle packets, and put data into either the built-in data structure for assembly, or there own data structures. The Wmb Asm will handle the final stage in which the assembled .nds is written out. If the developers choose to do so, they can use there own data structures, and use there own code for writing the .nds. The Wmb Asm Module will allow developers to use there own code for writing the .nds, instead of the built-in .nds writing code in Wmb Asm Module, if they choose to do so.