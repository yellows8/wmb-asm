//This whole file is code from wifi_lib_test, written by sgstair.

#include "ascii_bin.h"

#define		VRAM1		((u16 *) 0x06000000)
#define		VRAM2		((u16 *) 0x06200000)
#define PAL_BG1	((volatile u16 *)0x05000000)
#define PAL_BG2	((volatile u16 *)0x05000400)
#define RGB(r,g,b) ((r) | ((g)<<5) | ((b)<<10))

touchPosition touchXY;

int psych_mode;
int framectr;
const u16 psychtab1[8] = {
	0x7FFF, 0x7F7B, 0x7EF7, 0x7E73, 0x7E10, 0x7E73, 0x7EF7, 0x7F7B
};

const u16 psychtab2[8] = {
	0x1F, 0x2FF, 0x3EF, 0x1FE0, 0x7FE0, 0x7CE0, 0x7C0F, 0x5C1F
};

const u16 psychtab3[8] = {
	0x1F, 0x173, 0x2E7, 0xF60, 0x3DE0, 0x6C60, 0x5C07, 0x2C13
};

void Do_Play_PacketCapture();

//---------------------------------------------------------------------------------
void WaitVbl() {
//---------------------------------------------------------------------------------
	int i;
	
	swiWaitForVBlank();
	scanKeys();
	framectr++;
	touchXY=touchReadXY();
	switch(psych_mode) {
	case 0:  // nothing
	case 1: // normal
		for(i=0;i<8;i++) {
			PAL_BG1[i+1]=RGB(31,31,31);
			PAL_BG2[i+1]=RGB(31,31,31);
		}
		break;
	case 2: // psych1
		for(i=0;i<8;i++) {
			PAL_BG1[i+1]=psychtab1[((framectr>>2)+i)&7];
			PAL_BG2[i+1]=psychtab1[((framectr>>2)-i)&7];
		}
		break;	
	case 13: // psych2
		for(i=0;i<8;i++) {
			PAL_BG1[i+1]=psychtab2[((framectr>>2)+i)&7];
			PAL_BG2[i+1]=psychtab2[((framectr>>2)-i)&7];
		}
		break;	
	case 14: // psych3
		for(i=0;i<8;i++) {
			PAL_BG1[i+1]=psychtab3[((framectr>>2)+i)&7];
			PAL_BG2[i+1]=psychtab3[((framectr>>2)-i)&7];
		}
		break;
	case 3: // loop around
		psych_mode=1;
		break;
	default:
		psych_mode=0;
	}
}

 // 2bit top, 2bit right, 2bit bottom, 2bit left (0bTTRRBBLL)
unsigned char pipe_dream[192]; // auto generated;
const unsigned char reverse_pipe_dream[256] = { //(0bTTRRBBLL)
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 00
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 10
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 20
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 30
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 40
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 50
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 60
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 70
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 80
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // 90
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // A0
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0x44, 0x45, 0x46, 0x89, 0x09, // B0
	0x06, 0x8A, 0x88, 0x0A, 0x82, 0x81, 0x42, 0x05, 
	0x50, 0x51, 0x15, 0x54, 0x11, 0x55, 0x64, 0x98, // C0
	0xA0, 0x28, 0xA2, 0x2A, 0xA8, 0x22, 0xAA, 0x62, 
	0x91, 0x26, 0x19, 0x90, 0x60, 0x24, 0x18, 0x99, // D0
	0x66, 0x41, 0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // E0
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // F0
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
};

void ScrollUpTop() {
	int i;
	for(i=0;i<23*32;i++)
		VRAM1[0x07C00+i]=VRAM1[0x07C00+i+32];
	for(i=0;i<32;i++) {
		VRAM1[0x07C00+23*32+i]=' ';
	}
}
void ClsBtm() {
	int i;
	for(i=0;i<24*32;i++)
		VRAM2[0x07C00+i]=0x20;
}
void printtop(const char * str) {
	int x;
	ScrollUpTop();
	x=0;
	while(*str) {
		if(x==32) {x=0; ScrollUpTop();}
		VRAM1[0x07C00+23*32+x++]=*str;
		str++;
	}
}

int cursorxalt[14];
int cursoryalt[14];
int lastchar[14];
void ClsTopAlt(int alt) {
   int i;
   int base=0x07800-alt*0x400;
   for(i=0;i<32*24;i++) {
      VRAM1[base+23*32+i]=' ';
   }
   cursorxalt[alt]=0;
   cursoryalt[alt]=0;
}
void ScrollUpTopAlt(int alt) {
   int i;
   int base=0x07800-alt*0x400;
   for(i=0;i<23*32;i++)
      VRAM1[base+i]=VRAM1[base+i+32];
   for(i=0;i<32;i++) {
      VRAM1[base+23*32+i]=' ';
   }
}
void PrintCharTopAlt(int alt, int ch) {
   int base=0x07800-alt*0x400;
   if(ch<32) {
      if(ch==0x0D || ch==0x0A) {
         if(lastchar[alt]==ch || (lastchar[alt]!=0x0D && lastchar[alt]!=0x0A)) { // skip half of CR-LF, but still maintain effect.
            cursoryalt[alt]++;
            cursorxalt[alt]=0; 
            if(cursoryalt[alt]>=24) {
               ScrollUpTopAlt(alt);
               cursoryalt[alt]=23;
            }
         } else {
            lastchar[alt]=0;
         }
      }
      if(ch==0x08) {
         cursorxalt[alt]--;
         if(cursorxalt[alt]<0) {
            if(cursoryalt[alt]>0){
               cursorxalt[alt]=31;
               cursoryalt[alt]--;
            } else {
               cursorxalt[alt]=0;
            }
         }
      }
   } else {
      VRAM1[base+cursorxalt[alt]+cursoryalt[alt]*32]=ch;
      cursorxalt[alt]++;
      if(cursorxalt[alt]>=32) {
         cursorxalt[alt]=0;
         cursoryalt[alt]++;
         if(cursoryalt[alt]>=24) {
            ScrollUpTopAlt(alt);
            cursoryalt[alt]=23;
         }
      }
   }
   lastchar[alt]=ch;
}
void SwitchTopDisplay(int alt) { // -1=status, 0..13 = alt displays
   BG0_CR=((0x1E - alt)<<8) | 0x80;
}

void sgIP_dbgprint(char * txt, ...) {
	char buffer[256];
	va_list args;
	va_start(args,txt);
	vsprintf(buffer,txt,args);
	printtop(buffer);
}
void printbtm(int x, int y, const char * str) {
	while(*str) {
		if(x==32) {x=0; y++;}
		if(y==24) return;
		VRAM2[0x07C00+y*32+x++]=*str;
		str++;
	}
}
void printtopalt(int alt, int x, int y, const char * str) {
	int base=0x07800-alt*0x400;
	while(*str) {
		if(x==32) {x=0; y++;}
		if(y==24) return;
		VRAM1[base+y*32+x++]=*str;
		str++;
	}
}
void printbtmn(int x, int y, const char * str, int n) {
	while(*str && 0<=--n) {
		if(x==32) {x=0; y++;}
		if(y==24) return;
		VRAM2[0x07C00+y*32+x++]=*str;
		str++;
	}
}
void gen_pipe_dream() {
	int i;
	for(i=0;i<192;i++) {
		pipe_dream[i]=0;
	}
	for(i=0;i<256;i++) {
		if(reverse_pipe_dream[i]!=0xFF) {
			if(pipe_dream[reverse_pipe_dream[i]]) {
				printtop("Error: Duplicate pipe!");
			} else {
				pipe_dream[reverse_pipe_dream[i]] = i;
			}
		}
	}
}
void btm_drawch_pipedream(int x_, int y_, int c_) {
	unsigned char pdsrc, pddst;
	pdsrc=reverse_pipe_dream[c_];
	pddst=reverse_pipe_dream[VRAM2[0x07C00+y_*32+x_]];
	if(pdsrc!=0xFF && pddst!=0xFF) {
		if(!(pdsrc & 0xC0)) pdsrc |= (pddst&0xC0);
		if(!(pdsrc & 0x30)) pdsrc |= (pddst&0x30);
		if(!(pdsrc & 0xC)) pdsrc |= (pddst&0xC);
		if(!(pdsrc & 0x3)) pdsrc |= (pddst&0x3);
		pddst= pipe_dream[pdsrc];
		if(pddst!=0) c_ = pddst;
	}
	VRAM2[0x07C00+y_*32+x_]=c_;
}
void inline btm_drawch(int x_, int y_, int c_) {
	VRAM2[0x07C00+y_*32+x_]=c_;
}
void btm_drawrect1(int x1,int y1, int x2, int y2) {
	int i;
	if(x1<0 || y1<0 || x2>31 || y2>23) return;
	for(i=x1+1;i<x2;i++) { btm_drawch(i,y1,0xC4); btm_drawch(i,y2,0xC4); }
	for(i=y1+1;i<y2;i++) { btm_drawch(x1,i,0xB3); btm_drawch(x2,i,0xB3); }
	btm_drawch(x1,y1,0xDA);btm_drawch(x1,y2,0xC0);btm_drawch(x2,y1,0xBF);btm_drawch(x2,y2,0xD9);
}
void btm_drawrect2(int x1,int y1, int x2, int y2) {
	int i;
	if(x1<0 || y1<0 || x2>31 || y2>23) return;
	for(i=x1+1;i<x2;i++) { btm_drawch(i,y1,0xCD); btm_drawch(i,y2,0xCD); }
	for(i=y1+1;i<y2;i++) { btm_drawch(x1,i,0xBA); btm_drawch(x2,i,0xBA); }
	btm_drawch(x1,y1,0xC9);btm_drawch(x1,y2,0xC8);btm_drawch(x2,y1,0xBB);btm_drawch(x2,y2,0xBC);
}
void btm_drawbutton(int x1,int y1, int x2, int y2, int s, const char * btntxt) {
	if(s) btm_drawrect2(x1,y1,x2,y2); else btm_drawrect1(x1,y1,x2,y2); 
	s=strlen(btntxt);
	if(s>(x2-x1-1)) {
		printbtmn(x1+1,(y1+y2)>>1,btntxt,(x2-x1-1));
	} else {
		printbtm((x1+x2-s+1)>>1,(y1+y2)>>1,btntxt);
	}
}

void btm_pddrawrect1(int x1,int y1, int x2, int y2) {
	int i;
	if(x1<0 || y1<0 || x2>31 || y2>23) return;
	for(i=x1+1;i<x2;i++) { btm_drawch_pipedream(i,y1,0xC4); btm_drawch_pipedream(i,y2,0xC4); }
	for(i=y1+1;i<y2;i++) { btm_drawch_pipedream(x1,i,0xB3); btm_drawch_pipedream(x2,i,0xB3); }
	btm_drawch_pipedream(x1,y1,0xDA);btm_drawch_pipedream(x1,y2,0xC0);btm_drawch_pipedream(x2,y1,0xBF);btm_drawch_pipedream(x2,y2,0xD9);
}
void btm_pddrawrect2(int x1,int y1, int x2, int y2) {
	int i;
	if(x1<0 || y1<0 || x2>31 || y2>23) return;
	for(i=x1+1;i<x2;i++) { btm_drawch_pipedream(i,y1,0xCD); btm_drawch_pipedream(i,y2,0xCD); }
	for(i=y1+1;i<y2;i++) { btm_drawch_pipedream(x1,i,0xBA); btm_drawch_pipedream(x2,i,0xBA); }
	btm_drawch_pipedream(x1,y1,0xC9);btm_drawch_pipedream(x1,y2,0xC8);btm_drawch_pipedream(x2,y1,0xBB);btm_drawch_pipedream(x2,y2,0xBC);
}
void btm_pddrawbutton(int x1,int y1, int x2, int y2, int s, const char * btntxt) {
	if(s) btm_pddrawrect2(x1,y1,x2,y2); else btm_pddrawrect1(x1,y1,x2,y2); 
	s=strlen(btntxt);
	if(s>(x2-x1-1)) {
		printbtmn(x1+1,(y1+y2)>>1,btntxt,(x2-x1-1));
	} else {
		printbtm((x1+x2-s+1)>>1,(y1+y2)>>1,btntxt);
	}
}

//---------------------------------------------------------------------------------
void initDisplay() {
//---------------------------------------------------------------------------------
	int i;
	DISPLAY_CR=0x00012100;
	SUB_DISPLAY_CR=0x00012100;

	vramSetMainBanks(VRAM_A_MAIN_BG,VRAM_B_MAIN_BG,VRAM_C_SUB_BG,VRAM_D_LCD);

	// clear vram
	for(i=0;i<32768;i++) {
		VRAM1[i]=VRAM2[i]=0;
	}
	BG0_CR=0x1f80;
	SUB_BG0_CR=0x1f80;										   

	PAL_BG1[0]=RGB(0,0,0);
	PAL_BG2[0]=RGB(0,0,0);
	WIN_IN=0x000f;
	WIN_OUT=0x00ff;
	BLEND_CR=0x00E0;
	BLEND_Y=15;
	SUB_WIN_IN=0x000f;
	SUB_WIN_OUT=0x00ff;
	SUB_BLEND_CR=0x00E0;
	SUB_BLEND_Y=15;


	//	WAIT_CR &= ~0x0880;
	for(i=0;i<256*8*4;i++) {
		((u16 *)BG_TILE_RAM(0))[i]=((u16 *)ASCII_bin)[i];
		((u16 *)BG_TILE_RAM_SUB(0))[i]=((u16 *)ASCII_bin)[i];
	}
	for(i=0;i<8;i++) {
		PAL_BG1[i+1]=RGB(31,31,31);
		PAL_BG2[i+1]=RGB(31,31,31);
	}
}
