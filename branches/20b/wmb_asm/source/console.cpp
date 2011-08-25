/*
Wmb Asm and all software in the Wmb Asm package are licensed under the MIT license:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the “Software”), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies
or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "..\dll\include\wmb_asm.h"

#ifdef NDS
void InitConsoleNDS();
#include <fat.h>
#endif

void InitConsoleStuff()
{

#ifdef NDS
InitConsoleNDS();
#endif

}

#ifdef NDS
int system(const char *str)
{
while(1)swiWaitForVBlank();

return 1;
}

void InitConsoleNDS()
{
irqInit();
irqEnable(IRQ_VBLANK);

powerON(POWER_ALL);
videoSetMode(0);	//not using the main screen
videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);	//sub bg 0 will be used to print text
vramSetBankC(VRAM_C_SUB_BG);

SUB_BG0_CR = BG_MAP_BASE(31);

BG_PALETTE_SUB[255] = RGB15(31,31,31);	//by default font will be rendered with color 255

//consoleInit() is a lot more flexible but this gets you up and running quick
consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	if(!fatInitDefault())
	{
	printf("ERROR: Failed to initialize FAT.\nDid you DLDI patch this .nds for your card?\n");
	system("PAUSE");
	}
}
#endif
