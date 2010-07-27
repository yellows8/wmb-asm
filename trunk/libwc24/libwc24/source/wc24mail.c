/*
libwc24 is licensed under the MIT license:
Copyright (c) 2009 and 2010 yellowstar6

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the Software), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifdef HW_RVL

#include <gctypes.h>
#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include "wc24.h"

NWC24MsgCfg *wc24mail_nwc24msgcfg;

u32 CalcMailCfgChecksum(u32 *buffer, u32 length)
{
	u32 sum = 0;
	for(; length>=0; length-=4)
	{
		sum+=*buffer;
		buffer++;
	}
	return sum;
}

s32 WC24Mail_Init()
{
	wc24mail_nwc24msgcfg = memalign(32, sizeof(NWC24MsgCfg));
	if(wc24mail_nwc24msgcfg==NULL)return ENOMEM;
	memset(wc24mail_nwc24msgcfg, 0, sizeof(NWC24MsgCfg));

	return 0;
}

void WC24Mail_Shutdown()
{
	if(wc24mail_nwc24msgcfg)free(wc24mail_nwc24msgcfg);
}

#endif

