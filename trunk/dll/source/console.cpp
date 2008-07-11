#ifndef BUILDING_DLL
#define BUILDING_DLL
#endif
#include "..\include\wmb_asm.h"

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
