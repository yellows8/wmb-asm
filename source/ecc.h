#ifndef __ECC_H__
#define __ECC_H__

void calc_ecc(u8 *data, u8 *ecc);
int check_ecc(u8 *data);
static int is_ecc_blank(u8 *data);

#endif
