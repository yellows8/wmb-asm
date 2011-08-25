// Copyright 2007,2008  Segher Boessenkool  <segher@kernel.crashing.org>
// Licensed under the terms of the GNU GPL, version 2
// http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

#include "tools.h"

#include <stddef.h>	// to accommodate certain broken versions of openssl
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif
#include <endian.h>

//#include "rijndael.h"
#include "sha1.h"
//#include "md5.h"

//
// basic data types
//

u16 be16(const u8 *p)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return (p[0] << 8) | p[1];
#else
	return (u16)*p;
#endif
}

u32 be32(const u8 *p)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
#else
	return (u32)*p;
#endif
}

u64 be64(const u8 *p)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	return ((u64)be32(p) << 32) | be32(p + 4);
#else
	return (u64)*p;
#endif
}

u64 be34(const u8 *p)
{
	return 4 * (u64)be32(p);
}

void wbe16(u8 *p, u16 x)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	p[0] = x >> 8;
	p[1] = x;
#else
	(u16)*p = x;
#endif
}

void wbe32(u8 *p, u32 x)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	wbe16(p, x >> 16);
	wbe16(p + 2, x);
#else
	(u32)*p = x;
#endif
}

void wbe64(u8 *p, u64 x)
{
	wbe32(p, x >> 32);
	wbe32(p + 4, x);
}

//
// debug printf
//

unsigned int verbosity_level;

void debugf(const unsigned int verbosity, const char * stuff, ...) {
	if(verbosity <= verbosity_level) {
		unsigned int i;
		for(i = 0; i < verbosity; i++) {
			printf("  ");
		}
		va_list ap;
		va_start(ap, stuff);
		printf(stuff, ap);
		va_end(ap);
	}
}

//
// crypto
//

/*void md5(u8 *data, u32 len, u8 *hash)
{
	struct MD5Context ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, data, len);
	MD5Final(hash, &ctx);
}*/

/*void sha(u8 *data, u32 len, u8 *hash)
{
	struct SHA1Context ctx;
	int i;

	SHA1Reset(&ctx);
	SHA1Input(&ctx, data, len);
	SHA1Result(&ctx);

	for(i = 0; i < 5; i++)
	{
		*hash++ = ctx.Message_Digest[i] >> 24 & 0xff;
		*hash++ = ctx.Message_Digest[i] >> 16 & 0xff;
		*hash++ = ctx.Message_Digest[i] >> 8 & 0xff;
		*hash++ = ctx.Message_Digest[i] & 0xff;
	}
}*/

static struct wii_keys keys;
struct wii_keys *get_keys()
{
	return &keys;
}

void get_key(const char *name, u8 *key, u32 len)
{
	char path[256];
	char *home;
	FILE *fp;

	home = getenv("HOME");
	if (home == 0)
		fatal("cannot find HOME");
	snprintf(path, sizeof path, "%s/.wii/%s", home, name);

	fp = fopen(path, "rb");
	if (fp == 0)
		fatal("cannot open %s from %s", name, path);
	if (fread(key, len, 1, fp) != 1)
		fatal("error reading %s from %s", name, path);
	fclose(fp);
}

void load_keys(char * wiiname)
{
	char temp[256]; // TODO: support rediculously long paths
	get_key("common-key", keys.common_key, sizeof keys.common_key);
	strcpy(temp, wiiname);
	strcat(temp, "/nand-key");
	get_key(temp, keys.nand_key, sizeof keys.nand_key);
	strcpy(temp, wiiname);
	strcat(temp, "/nand-hmac");
	get_key(temp, keys.nand_hmac, sizeof keys.nand_hmac);
	get_key("root-key", keys.root_key, sizeof keys.root_key);
}

void load_keys_otp(const char *path)
{
	// FIXME: add error handling (maybe..)
	FILE *fp;
	char otp[128];

	fp = fopen(path, "r");
	fread(otp, 128, 1, fp);
	fclose(fp);

	memcpy(keys.common_key, otp + 20, 16);
	memcpy(keys.nand_key, otp + 88, 16);
	memcpy(keys.nand_hmac, otp + 68, 20);
	get_key("root-key", keys.root_key, sizeof keys.root_key);
}

/*void aes_cbc_dec(u8 *key, u8 *iv, u8 *in, u32 len, u8 *out)
{
	aes_set_key(key);
	aes_decrypt(iv, in, out, len);
}

void aes_cbc_enc(u8 *key, u8 *iv, u8 *in, u32 len, u8 *out)
{
	aes_set_key(key);
	aes_encrypt(iv, in, out, len);
}

void decrypt_title_key(u8 *tik, u8 *title_key)
{
	u8 iv[16];

	memset(iv, 0, sizeof iv);
	memcpy(iv, tik + 0x01dc, 8);
	aes_cbc_dec(get_keys()->common_key, iv, tik + 0x01bf, 16, title_key);
}*/

static u8 *get_root_key(void)
{
	return get_keys()->root_key;
}

static u32 get_sig_len(u8 *sig)
{
	u32 type;

	type = be32(sig);
	switch (type - 0x10000) {
	case 0:
		return 0x240;

	case 1:
		return 0x140;

	case 2:
		return 0x80;
	}

	fprintf(stderr, "get_sig_len(): unhandled sig type %08x\n", type);
	return 0;
}

static u32 get_sub_len(u8 *sub)
{
	u32 type;

	type = be32(sub + 0x40);
	switch (type) {
	case 0:
		return 0x2c0;

	case 1:
		return 0x1c0;

	case 2:
		return 0x100;
	}

	fprintf(stderr, "get_sub_len(): unhandled sub type %08x\n", type);
	return 0;
}

struct _check_rsa_cache
{
	u8 h[20];
	u8 sig[20];
	u8 key[20];
	u32 n;
	unsigned int use_strncmp;
	int result;
	struct _check_rsa_cache *next;
};
static struct _check_rsa_cache *check_rsa_cache = NULL;
/*static int check_rsa(u8 *h, u8 *sig, u8 *key, u32 n, int cache, unsigned int use_strncmp)
{
	u8 correct[0x200];
	u8 x[0x200];
	int ret;
	struct _check_rsa_cache *ptr, *result;
	static const u8 ber[16] = "\x00\x30\x21\x30\x09\x06\x05\x2b"
	                          "\x0e\x03\x02\x1a\x05\x00\x04\x14";

	if (use_strncmp)
		use_strncmp = 1;

	if (cache) {
		// this works because SHA is *much* faster than RSA
		result = calloc(1, sizeof(struct _check_rsa_cache));

		// FIXME: check the size
		sha(sig, n + 0x40, result->sig);
		sha(key, n + 4, result->key);

		ptr = check_rsa_cache;
		while (ptr != NULL) {
			if (memcmp(ptr->h, h, 20) == 0 &&
	  			memcmp(ptr->sig, result->sig, 20) == 0 &&
				memcmp(ptr->key, result->key, 20) == 0 &&
				ptr->n == n &&
				ptr->use_strncmp <= use_strncmp)
			{
				free(result);
				return ptr->result;
			}
			ptr = ptr->next;
		}
		memcpy(result->h, h, 20);
		result->n = n;
	}


//fprintf(stderr, "n = %x\n", n);
//fprintf(stderr, "key:\n");
//hexdump(key, n);
//fprintf(stderr, "sig:\n");
//hexdump(sig, n);

	correct[0] = 0;
	correct[1] = 1;
	memset(correct + 2, 0xff, n - 38);
	memcpy(correct + n - 36, ber, 16);
	memcpy(correct + n - 20, h, 20);
//fprintf(stderr, "correct is:\n");
//hexdump(correct, n);

	bn_exp(x, sig, key, n, key + n, 4);
//fprintf(stderr, "x is:\n");
//hexdump(x, n);

	if (use_strncmp && strncmp(correct + n - 20, x + n - 20, 20) == 0)
		ret = 0;
	else if(memcmp(correct, x, n) == 0)
		ret = 0;
	else
		ret = -5;



	if (cache)
	{ 
		result->result = ret;
		result->use_strncmp = use_strncmp;
		ptr = check_rsa_cache;
		if (ptr == NULL)
			check_rsa_cache = result;
		else
		{
			while (ptr->next != NULL)
				ptr = ptr->next;
			ptr->next = result;
		}

		return result->result;
	} else {
		return ret;
	}
}*/

static int check_hash(u8 *h, u8 *sig, u8 *key, int cache, unsigned int use_strncmp)
{
	u32 type;

	type = be32(sig) - 0x10000;
	if (type != be32(key + 0x40))
		return -6;

	switch (type) {
	case 1:
		return check_rsa(h, sig + 4, key + 0x88, 0x100, cache,
				use_strncmp);
	}

	return -7;
}

static u8 *find_cert_in_chain(u8 *sub, u8 *cert, u32 cert_len)
{
	char parent[64];
	char *child;
	u32 sig_len, sub_len;
	u8 *p;
	u8 *issuer;

	strncpy(parent, sub, sizeof parent);
	parent[sizeof parent - 1] = 0;
	child = strrchr(parent, '-');
	if (child)
		*child++ = 0;
	else {
		*parent = 0;
		child = sub;
	}

	for (p = cert; p < cert + cert_len; p += sig_len + sub_len) {
		sig_len = get_sig_len(p);
		if (sig_len == 0)
			return 0;
		issuer = p + sig_len;
		sub_len = get_sub_len(issuer);
		if (sub_len == 0)
			return 0;

		if (strcmp(parent, issuer) == 0
		    && strcmp(child, issuer + 0x44) == 0)
			return p;
	}

	return 0;
}

/*int check_cert_chain(u8 *data, u32 data_len, u8 *cert, u32 cert_len,
		unsigned int use_strncmp)
{
	u8 *sig;
	u8 *sub;
	u32 sig_len;
	u32 sub_len;
	u8 h[20];
	u8 *key_cert;
	u8 *key;
	int ret;
	int cache;

	sig = data;
	sig_len = get_sig_len(sig);
	if (sig_len == 0)
		return -1;
	sub = data + sig_len;
	sub_len = data_len - sig_len;
	if (sub_len == 0)
		return -2;

	cache = 0;
	for (;;) {
//fprintf(stderr, ">>>>>> checking sig by %s...\n", sub);
		if (strcmp(sub, "Root") == 0) {
			key = get_root_key();
			sha(sub, sub_len, h);
			if (be32(sig) != 0x10000)
				return -8;
			return check_rsa(h, sig + 4, key, 0x200, 1,
					use_strncmp);
		}

		key_cert = find_cert_in_chain(sub, cert, cert_len);
		if (key_cert == 0)
			return -3;

		key = key_cert + get_sig_len(key_cert);

		sha(sub, sub_len, h);
		ret = check_hash(h, sig, key, cache, use_strncmp);
		if (ret)
			return ret;

		sig = key_cert;
		sig_len = get_sig_len(sig);
		if (sig_len == 0)
			return -4;
		sub = sig + sig_len;
		sub_len = get_sub_len(sub);
		if (sub_len == 0)
			return -5;
		cache = 1;
	}
}*/

//
// compression
//

void do_yaz0(u8 *in, u32 in_size, u8 *out, u32 out_size)
{
	u32 nout;
	u8 bits;
	u32 nbits;
	u32 n, d, i;

	bits = 0;
	nbits = 0;
	in += 0x10;
	for (nout = 0; nout < out_size; ) {
		if (nbits == 0) {
			bits = *in++;
			nbits = 8;
		}

		if ((bits & 0x80) != 0) {
			*out++ = *in++;
			nout++;
		} else {
			n = *in++;
			d = *in++;
			d |= (n << 8) & 0xf00;
			n >>= 4;
			if (n == 0)
				n = 0x10 + *in++;
			n += 2;
			d++;

			for (i = 0; i < n; i++) {
				*out = *(out - d);
				out++;
			}
			nout += n;
		}

		nbits--;
		bits <<= 1;
	};
}

//
// error handling
//

void fatal(const char *s, ...)
{
	char message[256];
	va_list ap;

	va_start(ap, s);
	vsnprintf(message, sizeof message, s, ap);

	perror(message);

	exit(1);
}

//
// output formatting
//

void print_bytes(u8 *x, u32 n)
{
	u32 i;

	for (i = 0; i < n; i++)
		fprintf(stderr, "%02x", x[i]);
}

void hexdump(u8 *x, u32 n)
{
	u32 i, j;

	for (i = 0; i < n; i += 16) {
		fprintf(stderr, "%04x:", i);
		for (j = 0; j < 16 && i + j < n; j++) {
			if ((j & 3) == 0)
				fprintf(stderr, " ");
			fprintf(stderr, "%02x", *x++);
		}
		fprintf(stderr, "\n");
	}
}

void dump_tmd(u8 *tmd)
{
	u32 i, n;
	u8 *p;

	printf("       issuer: %s\n", tmd + 0x140);
	printf("  sys_version: %016llx\n", be64(tmd + 0x0184));
	printf("     title_id: %016llx\n", be64(tmd + 0x018c));
	printf("   title_type: %08x\n", be32(tmd + 0x0194));
	printf("     group_id: %04x\n", be16(tmd + 0x0198));
	printf("title_version: %04x\n", be16(tmd + 0x01dc));
	printf(" num_contents: %04x\n", be16(tmd + 0x01de));
	printf("   boot_index: %04x\n", be16(tmd + 0x01e0));

	n = be16(tmd + 0x01de);
	p = tmd + 0x01e4;
	for (i = 0; i < n; i++) {
		printf("cid %08x  index %04x  type %04x  size %08llx\n",
		       be32(p), be16(p + 4), be16(p + 6), be64(p + 8));
		p += 0x24;
	}
}
