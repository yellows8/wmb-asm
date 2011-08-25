#include <stdio.h>
#include <string.h>
#include "tools.h"

/* Wii ECC code, originally taken from segher's unecc.c */

static u8 parity(u8 x)
{
	u8 y = 0;

	while (x) {
		y ^= (x & 1);
		x >>= 1;
	}

	return y;
}

void calc_ecc(u8 *data, u8 *ecc)
{
	u8 a[12][2];
	int i, j;
	u32 a0, a1;
	u8 x;

	memset(a, 0, sizeof a);
	for (i = 0; i < 512; i++) {
		x = data[i];
		for (j = 0; j < 9; j++)
			a[3+j][(i >> j) & 1] ^= x;
	}

	x = a[3][0] ^ a[3][1];
	a[0][0] = x & 0x55;
	a[0][1] = x & 0xaa;
	a[1][0] = x & 0x33;
	a[1][1] = x & 0xcc;
	a[2][0] = x & 0x0f;
	a[2][1] = x & 0xf0;

	for (j = 0; j < 12; j++) {
		a[j][0] = parity(a[j][0]);
		a[j][1] = parity(a[j][1]);
	}

	a0 = a1 = 0;
	for (j = 0; j < 12; j++) {
		a0 |= a[j][0] << j;
		a1 |= a[j][1] << j;
	}

	ecc[0] = a0;
	ecc[1] = a0 >> 8;
	ecc[2] = a1;
	ecc[3] = a1 >> 8;
}

static int is_ecc_blank(u8 *data)
{
	u8 i;
	
	for(i=0; i<16; i++) if (data[2048+48+i]!=0xff) return 0;
	return 1;
}

int check_ecc(u8 *data)
{
	u8 ecc[16];

	if (is_ecc_blank(data)) return -2;  // uninitialized page; ECC is meaningless
	calc_ecc(data, ecc);
	calc_ecc(data + 512, ecc + 4);
	calc_ecc(data + 1024, ecc + 8);
	calc_ecc(data + 1536, ecc + 12);

	if (memcmp(data + 2048 + 48, ecc, 16)) {
		fprintf(stderr, "Stored: ");
		hexdump(data + 2048 + 48, 16);
		fprintf(stderr, "\nCalc: ");
		hexdump(ecc, 16);
		fprintf(stderr, "\n");
		return -1; // ECC incorrect
	}
	return 0; // ECC correct
}
