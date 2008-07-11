//This program, DSPacketCapture, is based on wifi_lib_test, which is written by sgstair.
/****************************************************************************** 
DSPacketCapture is licenced under the MIT open source licence:
Copyright (c) 2008 yellowstar

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

//Almost everything in this file is written by sgstair. Everything but the code noted to be written by yellowstar, is written by sgstair.

#include <nds.h>
#include <nds/arm7/serial.h>
#include <dswifi7.h>

//This struct is written by yellowstar.
typedef struct tCommandControl
{
bool shutdown;
} CommandControl;
#define commandControl ((CommandControl*)((uint32)(IPC) + sizeof(TransferRegion)))
//---------------------------------------------------------------------------------
void startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format) {
//---------------------------------------------------------------------------------
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_8BIT:SOUND_16BIT);
}


//---------------------------------------------------------------------------------
s32 getFreeSoundChannel() {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}

touchPosition first,tempPos;

//---------------------------------------------------------------------------------
void VcountHandler() {
//---------------------------------------------------------------------------------
	static int lastbut = -1;
	
	uint16 but=0, x=0, y=0, xpx=0, ypx=0, z1=0, z2=0;

	but = REG_KEYXY;

	if (!( (but ^ lastbut) & (1<<6))) {
 
		tempPos = touchReadXY();

		if ( tempPos.x == 0 || tempPos.y == 0 ) {
			but |= (1 <<6);
			lastbut = but;
		} else {
			x = tempPos.x;
			y = tempPos.y;
			xpx = tempPos.px;
			ypx = tempPos.py;
			z1 = tempPos.z1;
			z2 = tempPos.z2;
		}
		
	} else {
		lastbut = but;
		but |= (1 <<6);
	}
	
	IPC->touchX			= x;
	IPC->touchY			= y;
	IPC->touchXpx		= xpx;
	IPC->touchYpx		= ypx;
	IPC->touchZ1		= z1;
	IPC->touchZ2		= z2;
	IPC->buttons		= but;

}

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------

	u32 i;
	
	//sound code  :)
	TransferSound *snd = IPC->soundData;
	IPC->soundData = 0;

	if (0 != snd) {

		for (i=0; i<snd->count; i++) {
			s32 chan = getFreeSoundChannel();

			if (chan >= 0) {
				startSound(snd->data[i].rate, snd->data[i].data, snd->data[i].len, chan, snd->data[i].vol, snd->data[i].pan, snd->data[i].format);
			}
		}
	}

	Wifi_Update();
	
	//This code for this "if" is written by yellowstar.
	if(commandControl->shutdown)
	{
	writePowerManagement(PM_CONTROL_REG,PM_POWER_DOWN);
	}

}



//---------------------------------------------------------------------------------
void arm7_synctoarm9() { // send fifo message
//---------------------------------------------------------------------------------
   REG_IPC_FIFO_TX = 0x87654321;
}

//---------------------------------------------------------------------------------
enum {
//---------------------------------------------------------------------------------
	CMD_WAIT,
	WIFI_INIT,
};
	
u32 fifo_status = CMD_WAIT;

//---------------------------------------------------------------------------------
// interrupt handler to allow incoming notifications from arm9
//---------------------------------------------------------------------------------
void arm7_fifo() { // check incoming fifo messages
//---------------------------------------------------------------------------------
	while ( !(REG_IPC_FIFO_CR & (IPC_FIFO_RECV_EMPTY)) ) {
		u32 msg = REG_IPC_FIFO_RX;

		switch (fifo_status) {
		case WIFI_INIT:
			Wifi_Init(msg);
			Wifi_SetSyncHandler(arm7_synctoarm9); // allow wifi lib to notify arm9
			fifo_status = CMD_WAIT;
			break;

		case CMD_WAIT:
			if(msg==0x87654321)
				Wifi_Sync();

			if(msg==0x12345678) {
				fifo_status = WIFI_INIT;
			}
		}
	}
}

//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------
	
	//This main code was updated to the latest template code, to fix a RTC bug. This update to the code was done by yellowstar.
	//The Wifi-related code in here is from wifi_lib_test, written by sgstair.
	
	// read User Settings from firmware
	readUserSettings();

	//enable sound
	powerON(POWER_SOUND);
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE ) | PM_SOUND_AMP );
	SOUND_CR = SOUND_ENABLE | SOUND_VOL(0x7F);

	

	irqInit();

	// Start the RTC tracking IRQ
	initClockIRQ();

	SetYtrigger(80);
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);
	
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ | IPC_FIFO_SEND_CLEAR;

	irqSet(IRQ_WIFI, Wifi_Interrupt);
	irqSet(IRQ_FIFO_NOT_EMPTY,arm7_fifo);

	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_NETWORK | IRQ_WIFI | IRQ_FIFO_NOT_EMPTY);
	commandControl->shutdown=0;
	Wifi_SetSyncHandler(arm7_synctoarm9);

	// Keep the ARM7 out of main RAM
	while (1) swiWaitForVBlank();
	
	return 0;
}
