/*
 *  Copyright 2014-2022 The GmSSL Project. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the License); you may
 *  not use this file except in compliance with the License.
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 */


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <gmssl/hex.h>
#include <gmssl/mem.h>
#include <gmssl/sm9_z256.h>
#include <gmssl/error.h>
#include <gmssl/endian.h>
#include <gmssl/rand.h>


const sm9_z256_t SM9_Z256_ZERO = {0,0,0,0};
const sm9_z256_t SM9_Z256_ONE = {1,0,0,0};
const sm9_z256_t SM9_Z256_TWO = {2,0,0,0};
const sm9_z256_t SM9_Z256_FIVE = {5,0,0,0};


// p =  b640000002a3a6f1d603ab4ff58ec74521f2934b1a7aeedbe56f9b27e351457d
// n =  b640000002a3a6f1d603ab4ff58ec74449f2934b18ea8beee56ee19cd69ecf25
// mu_p = 2^512 // p = 167980e0beb5759a655f73aebdcd1312af2665f6d1e36081c71188f90d5c22146
// mu_n = 2^512 // n
const sm9_z256_t SM9_Z256_P = {0xe56f9b27e351457d, 0x21f2934b1a7aeedb, 0xd603ab4ff58ec745, 0xb640000002a3a6f1};
const sm9_z256_t SM9_Z256_N = {0xe56ee19cd69ecf25, 0x49f2934b18ea8bee, 0xd603ab4ff58ec744, 0xb640000002a3a6f1};
const sm9_z256_t SM9_Z256_NEG_N = {0x1a911e63296130db, 0xb60d6cb4e7157411, 0x29fc54b00a7138bb, 0x49bffffffd5c590e};
// n - 1
const sm9_z256_t SM9_Z256_N_MINUS_ONE = {0xe56ee19cd69ecf24, 0x49f2934b18ea8bee, 0xd603ab4ff58ec744, 0xb640000002a3a6f1};


// e = p - 2 = b640000002a3a6f1d603ab4ff58ec74521f2934b1a7aeedbe56f9b27e351457b
// p - 2, used in a^(p-2) = a^-1
const sm9_z256_t SM9_Z256_P_MINUS_TWO = {0xe56f9b27e351457b, 0x21f2934b1a7aeedb, 0xd603ab4ff58ec745, 0xb640000002a3a6f1};


// P1.X 0x93DE051D62BF718FF5ED0704487D01D6E1E4086909DC3280E8C4E4817C66DDDD
// P1.Y 0x21FE8DDA4F21E607631065125C395BBC1C1C00CBFA6024350C464CD70A3EA616
const SM9_Z256_POINT _SM9_Z256_P1 = {
	{0xe8c4e4817c66dddd, 0xe1e4086909dc3280, 0xf5ed0704487d01d6, 0x93de051d62bf718f},
	{0x0c464cd70a3ea616, 0x1c1c00cbfa602435, 0x631065125c395bbc, 0x21fe8dda4f21e607},
	{1,0,0,0}
};
const SM9_Z256_POINT *SM9_Z256_P1 = &_SM9_Z256_P1;

/*
	X : [0x3722755292130b08d2aab97fd34ec120ee265948d19c17abf9b7213baf82d65bn,
	     0x85aef3d078640c98597b6027b441a01ff1dd2c190f5e93c454806c11d8806141n],
	Y : [0xa7cf28d519be3da65f3170153d278ff247efba98a71a08116215bba5c999a7c7n,
	     0x17509b092e845c1266ba0d262cbee6ed0736a96fa347c8bd856dc76b84ebeb96n],
	Z : [1n, 0n],
*/
const SM9_Z256_TWIST_POINT _SM9_Z256_P2 = {
	{{0xF9B7213BAF82D65B, 0xEE265948D19C17AB, 0xD2AAB97FD34EC120, 0x3722755292130B08},
	 {0x54806C11D8806141, 0xF1DD2C190F5E93C4, 0x597B6027B441A01F, 0x85AEF3D078640C98}},
	{{0x6215BBA5C999A7C7, 0x47EFBA98A71A0811, 0x5F3170153D278FF2, 0xA7CF28D519BE3DA6},
	 {0x856DC76B84EBEB96, 0x0736A96FA347C8BD, 0x66BA0D262CBEE6ED, 0x17509B092E845C12}},
	{{1,0,0,0}, {0,0,0,0}},
};
const SM9_Z256_TWIST_POINT *SM9_Z256_P2 = &_SM9_Z256_P2;


const SM9_Z256_TWIST_POINT _SM9_Z256_Ppubs = {
	{{0x8F14D65696EA5E32, 0x414D2177386A92DD, 0x6CE843ED24A3B573, 0x29DBA116152D1F78},
	 {0x0AB1B6791B94C408, 0x1CE0711C5E392CFB, 0xE48AFF4B41B56501, 0x9F64080B3084F733}},
	{{0x0E75C05FB4E3216D, 0x1006E85F5CDFF073, 0x1A7CE027B7A46F74, 0x41E00A53DDA532DA},
	 {0xE89E1408D0EF1C25, 0xAD3E2FDB1A77F335, 0xB57329F447E3A0CB, 0x69850938ABEA0112}},
	{{1,0,0,0}, {0,0,0,0}},
};
const SM9_Z256_TWIST_POINT *SM9_Z256_Ppubs = &_SM9_Z256_Ppubs;


// mont params (mod p)
// mu = p^-1 mod 2^64 = 0x76d43bd3d0d11bd5
// 2^512 mod p = 0x2ea795a656f62fbde479b522d6706e7b88f8105fae1a5d3f27dea312b417e2d2
// mont(1) mod p = 2^256 mod p = 0x49bffffffd5c590e29fc54b00a7138bade0d6cb4e58511241a9064d81caeba83
const uint64_t SM9_Z256_MODP_MU = 0x76d43bd3d0d11bd5;
const sm9_z256_t SM9_Z256_MODP_2e512 = {0x27dea312b417e2d2, 0x88f8105fae1a5d3f, 0xe479b522d6706e7b, 0x2ea795a656f62fbd};
#define SM9_Z256_NEG_P SM9_Z256_MODP_MONT_ONE
const sm9_z256_t SM9_Z256_MODP_MONT_ONE = {0x1a9064d81caeba83, 0xde0d6cb4e5851124, 0x29fc54b00a7138ba, 0x49bffffffd5c590e};
const sm9_z256_t SM9_Z256_MODP_MONT_FIVE = {0xb9f2c1e8c8c71995, 0x125df8f246a377fc, 0x25e650d049188d1c, 0x43fffffed866f63};


const SM9_Z256_POINT _SM9_Z256_MONT_P1 = {
	{0x22e935e29860501b, 0xa946fd5e0073282c, 0xefd0cec817a649be, 0x5129787c869140b5},
	{0xee779649eb87f7c7, 0x15563cbdec30a576, 0x326353912824efbf, 0x7215717763c39828},
	{0x1a9064d81caeba83, 0xde0d6cb4e5851124, 0x29fc54b00a7138ba, 0x49bffffffd5c590e}
};
const SM9_Z256_POINT *SM9_Z256_MONT_P1 = &_SM9_Z256_MONT_P1;

const SM9_Z256_TWIST_POINT _SM9_Z256_MONT_P2 = {
	{{0x260226a68ce2da8f, 0x7ee5645edbf6c06b, 0xf8f57c82b1495444, 0x61fcf018bc47c4d1},
	 {0xdb6db4822750a8a6, 0x84c6135a5121f134, 0x1874032f88791d41, 0x905112f2b85f3a37}},
	{{0xc03f138f9171c24a, 0x92fbab45a15a3ca7, 0x2445561e2ff77cdb, 0x108495e0c0f62ece},
	 {0xf7b82dac4c89bfbb, 0x3706f3f6a49dc12f, 0x1e29de93d3eef769, 0x81e448c3c76a5d53}},
	{{0x1a9064d81caeba83, 0xde0d6cb4e5851124, 0x29fc54b00a7138ba, 0x49bffffffd5c590e}, {0,0,0,0}},
};
const SM9_Z256_TWIST_POINT *SM9_Z256_MONT_P2 = &_SM9_Z256_MONT_P2;

const SM9_Z256_TWIST_POINT _SM9_Z256_MONT_Ppubs = {
	{{0xb2e0a02b40b3d927, 0x153e2b9e897e44a0, 0x47cd0690d256c1a9, 0x5d3123b78630320e},
	 {0x2c3c3f7ba9fc143e, 0x1f214aa16a4fa43f, 0x424e7e2f0dbc839b, 0x87eecef7fd6531c9}},
	{{0x07a059838aa95e77, 0x6e65e6d455509cae, 0xf921da6493e4f742, 0x9fcf05bded9f2d36},
	 {0xdc4fea9a756fc34e, 0xe4e34e772312a7b1, 0xbfa26e7682b1f64a, 0x7f1337b7cda2bf5e}},
	{{0x1a9064d81caeba83, 0xde0d6cb4e5851124, 0x29fc54b00a7138ba, 0x49bffffffd5c590e}, {0,0,0,0}},
};
const SM9_Z256_TWIST_POINT *SM9_Z256_MONT_Ppubs = &_SM9_Z256_MONT_Ppubs;


void sm9_z256_to_bits(const sm9_z256_t a, char bits[256])
{
	int i, j;
	for (i = 3; i >= 0; i--) {
		uint64_t w = a[i];
		for (j = 0; j < 64; j++) {
			*bits++ = (w & 0x8000000000000000) ? '1' : '0';
			w <<= 1;
		}
	}
}

int sm9_z256_rand_range(sm9_z256_t r, const sm9_z256_t range)
{
	uint8_t buf[256];

	do {
		rand_bytes(buf, sizeof(buf));
		sm9_z256_from_bytes(r, buf);
	} while (sm9_z256_cmp(r, range) >= 0);
	return 1;
}

void sm9_z256_from_bytes(sm9_z256_t r, const uint8_t in[32])
{
	r[3] = GETU64(in);
	r[2] = GETU64(in + 8);
	r[1] = GETU64(in + 16);
	r[0] = GETU64(in + 24);
}

void sm9_z256_to_bytes(const sm9_z256_t a, uint8_t out[32])
{
	PUTU64(out, a[3]);
	PUTU64(out + 8, a[2]);
	PUTU64(out + 16, a[1]);
	PUTU64(out + 24, a[0]);
}

void sm9_z256_copy(sm9_z256_t r, const sm9_z256_t a)
{
	r[3] = a[3];
	r[2] = a[2];
	r[1] = a[1];
	r[0] = a[0];
}

void sm9_z256_copy_conditional(sm9_z256_t dst, const sm9_z256_t src, uint64_t move)
{
	uint64_t mask1 = 0-move;
	uint64_t mask2 = ~mask1;

	dst[0] = (src[0] & mask1) ^ (dst[0] & mask2);
	dst[1] = (src[1] & mask1) ^ (dst[1] & mask2);
	dst[2] = (src[2] & mask1) ^ (dst[2] & mask2);
	dst[3] = (src[3] & mask1) ^ (dst[3] & mask2);
}

void sm9_z256_set_zero(sm9_z256_t r)
{
	sm9_z256_copy(r, SM9_Z256_ZERO);
}

static uint64_t is_zero(uint64_t in)
{
	in |= (0 - in);
	in = ~in;
	in >>= 63;
	return in;
}

uint64_t sm9_z256_equ(const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t res;

	res = a[0] ^ b[0];
	res |= a[1] ^ b[1];
	res |= a[2] ^ b[2];
	res |= a[3] ^ b[3];

	return is_zero(res);
}

int sm9_z256_cmp(const sm9_z256_t a, const sm9_z256_t b)
{
	if (a[3] > b[3]) return 1;
	else if (a[3] < b[3]) return -1;
	if (a[2] > b[2]) return 1;
	else if (a[2] < b[2]) return -1;
	if (a[1] > b[1]) return 1;
	else if (a[1] < b[1]) return -1;
	if (a[0] > b[0]) return 1;
	else if (a[0] < b[0]) return -1;
	return 0;
}

uint64_t sm9_z256_is_zero(const sm9_z256_t a)
{
	return
		is_zero(a[0]) &
		is_zero(a[1]) &
		is_zero(a[2]) &
		is_zero(a[3]);
}

uint64_t sm9_z256_add(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t t, c = 0;

	t = a[0] + b[0];
	c = t < a[0];
	r[0] = t;

	t = a[1] + c;
	c = t < a[1];
	r[1] = t + b[1];
	c += r[1] < t;

	t = a[2] + c;
	c = t < a[2];
	r[2] = t + b[2];
	c += r[2] < t;

	t = a[3] + c;
	c = t < a[3];
	r[3] = t + b[3];
	c += r[3] < t;

	return c;
}

uint64_t sm9_z256_sub(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t t, c = 0;

	t = a[0] - b[0];
	c = t > a[0];
	r[0] = t;

	t = a[1] - c;
	c = t > a[1];
	r[1] = t - b[1];
	c += r[1] > t;

	t = a[2] - c;
	c = t > a[2];
	r[2] = t - b[2];
	c += r[2] > t;

	t = a[3] - c;
	c = t > a[3];
	r[3] = t - b[3];
	c += r[3] > t;

	return c;
}

#ifndef ENABLE_SM9_Z256_ARMV8
void sm9_z256_mul_low(uint64_t r[4], const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t t[8];
	sm9_z256_mul(t, a, b);
	r[0] = t[0];
	r[1] = t[1];
	r[2] = t[2];
	r[3] = t[3];
}

void sm9_z256_mul(uint64_t r[8], const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t a_[8];
	uint64_t b_[8];
	uint64_t s[16] = {0};
	uint64_t u;
	int i, j;

	for (i = 0; i < 4; i++) {
		a_[2 * i] = a[i] & 0xffffffff;
		b_[2 * i] = b[i] & 0xffffffff;
		a_[2 * i + 1] = a[i] >> 32;
		b_[2 * i + 1] = b[i] >> 32;
	}

	for (i = 0; i < 8; i++) {
		u = 0;
		for (j = 0; j < 8; j++) {
			u = s[i + j] + a_[i] * b_[j] + u;
			s[i + j] = u & 0xffffffff;
			u >>= 32;
		}
		s[i + 8] = u;
	}

	for (i = 0; i < 8; i++) {
		r[i] = (s[2 * i + 1] << 32) | s[2 * i];
	}
}
#endif

uint64_t sm9_z512_add(uint64_t r[8], const uint64_t a[8], const uint64_t b[8])
{
	uint64_t t, c = 0;

	t = a[0] + b[0];
	c = t < a[0];
	r[0] = t;

	t = a[1] + c;
	c = t < a[1];
	r[1] = t + b[1];
	c += r[1] < t;

	t = a[2] + c;
	c = t < a[2];
	r[2] = t + b[2];
	c += r[2] < t;

	t = a[3] + c;
	c = t < a[3];
	r[3] = t + b[3];
	c += r[3] < t;

	t = a[4] + c;
	c = t < a[4];
	r[4] = t + b[4];
	c += r[4] < t;

	t = a[5] + c;
	c = t < a[5];
	r[5] = t + b[5];
	c += r[5] < t;

	t = a[6] + c;
	c = t < a[6];
	r[6] = t + b[6];
	c += r[6] < t;

	t = a[7] + c;
	c = t < a[7];
	r[7] = t + b[7];
	c += r[7] < t;

	return c;
}

int sm9_z256_get_booth(const uint64_t a[4], uint64_t window_size, int i)
{
	uint64_t mask = (1 << window_size) - 1;
	uint64_t wbits;
	int n, j;

	if (i == 0) {
		return ((a[0] << 1) & mask) - (a[0] & mask);
	}

	j = i * window_size - 1;
	n = j / 64;
	j = j % 64;

	wbits = a[n] >> j;
	if ((64 - j) < (window_size + 1) && n < 3) {
		wbits |= a[n + 1] << (64 - j);
	}
	return (wbits & mask) - ((wbits >> 1) & mask);
}

int sm9_z256_from_hex(sm9_z256_t r, const char *hex)
{
	uint8_t buf[32];
	size_t len;
	if (hex_to_bytes(hex, 64, buf, &len) < 0) {
		return -1;
	}
	sm9_z256_from_bytes(r, buf);
	return 1;
}

void sm9_z256_to_hex(const sm9_z256_t r, char hex[64])
{
	int i;
	for (i = 3; i >= 0; i--) {
		(void)sprintf(hex + 16*(3-i), "%016llx", r[i]);
	}
}

void sm9_z256_print_bn(const char *prefix, const sm9_z256_t a)
{
	char hex[65] = {0};
	sm9_z256_to_hex(a, hex);
	printf("%s\n%s\n", prefix, hex);
}

int sm9_z256_equ_hex(const sm9_z256_t a, const char *hex)
{
	sm9_z256_t b;
	sm9_z256_from_hex(b, hex);
	if (sm9_z256_cmp(a, b) == 0) {
		return 1;
	} else {
		return 0;
	}
}

int sm9_z256_print(FILE *fp, int ind, int fmt, const char *label, const sm9_z256_t a)
{
	format_print(fp, ind, fmt, "%s: %016lx%016lx%016lx%016lx\n", label, a[3], a[2], a[1], a[0]);
	return 1;
}

int sm9_z512_print(FILE *fp, int ind, int fmt, const char *label, const uint64_t a[8])
{
	format_print(fp, ind, fmt, "%s: %016lx%016lx%016lx%016lx%016lx%016lx%016lx%016lx\n",
		label, a[7], a[6], a[5], a[4], a[3], a[2], a[1], a[0]);
	return 1;
}

#ifndef ENABLE_SM9_Z256_ARMV8
void sm9_z256_fp_add(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t c;
	c = sm9_z256_add(r, a, b);

	if (c) {
		// a + b - p = (a + b - 2^256) + (2^256 - p)
		(void)sm9_z256_add(r, r, SM9_Z256_NEG_P);
		return;
	}
	if (sm9_z256_cmp(r, SM9_Z256_P) >= 0) {
		(void)sm9_z256_sub(r, r, SM9_Z256_P);
	}
}

void sm9_z256_fp_sub(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t c;
	c = sm9_z256_sub(r, a, b);

	if (c) {
		// a - b + p = (a - b + 2^256) - (2^256 - p)
		(void)sm9_z256_sub(r, r, SM9_Z256_NEG_P);
	}
}

void sm9_z256_fp_dbl(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_fp_add(r, a, a);
}

void sm9_z256_fp_tri(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_t t;
	sm9_z256_fp_add(t, a, a);
	sm9_z256_fp_add(r, t, a);
}

void sm9_z256_fp_div2(sm9_z256_t r, const sm9_z256_t a)
{
	uint64_t c = 0;

	if (a[0] & 1) {
		c = sm9_z256_add(r, a, SM9_Z256_P);
	} else {
		r[0] = a[0];
		r[1] = a[1];
		r[2] = a[2];
		r[3] = a[3];
	}

	r[0] = (r[0] >> 1) | ((r[1] & 1) << 63);
	r[1] = (r[1] >> 1) | ((r[2] & 1) << 63);
	r[2] = (r[2] >> 1) | ((r[3] & 1) << 63);
	r[3] = (r[3] >> 1) | ((c & 1) << 63);
}

void sm9_z256_fp_neg(sm9_z256_t r, const sm9_z256_t a)
{
	(void)sm9_z256_sub(r, SM9_Z256_P, a);
}
#endif

// (w0,w1) = a*b + c + d
#if 0
void sm9_u64_mul_add(uint64_t *w0, uint64_t *w1,
	const uint64_t a, const uint64_t b, const uint64_t c, const uint64_t d)
{
	uint64_t a_[2];
	uint64_t b_[2];
	uint64_t s[4] = {0};
	uint64_t u;
	uint64_t r[2];
	int i, j;

	a_[0] = a & 0xffffffff;
	b_[0] = b & 0xffffffff;
	a_[1] = a >> 32;
	b_[1] = b >> 32;

	for (i = 0; i < 2; i++) {
		u = 0;
		for (j = 0; j < 2; j++) {
			u = s[i + j] + a_[i] * b_[j] + u;
			s[i + j] = u & 0xffffffff;
			u >>= 32;
		}
		s[i + 2] = u;
	}

	for (i = 0; i < 2; i++) {
		r[i] = (s[2 * i + 1] << 32) | s[2 * i];
	}

	r[0] += c;
	if (r[0] < c) {
		r[1]++;
	}
	r[0] += d;
	if (r[0] < d) {
		r[1]++;
	}

	*w0 = r[0];
	*w1 = r[1];
}
#endif

													
// p = b640000002a3a6f1d603ab4ff58ec74521f2934b1a7aeedbe56f9b27e351457d
// p' = -p^(-1) mod 2^256 = afd2bac5558a13b3966a4b291522b137181ae39613c8dbaf892bc42c2f2ee42b
// sage: -(IntegerModRing(2^256)(p))^-1
const uint64_t SM9_Z256_P_PRIME[4] = {
	0x892bc42c2f2ee42b, 0x181ae39613c8dbaf, 0x966a4b291522b137, 0xafd2bac5558a13b3,
};


#ifndef ENABLE_SM9_Z256_ARMV8
// z = a*b
// c = (z + (z * p' mod 2^256) * p)/2^256
void sm9_z256_fp_mont_mul(uint64_t r[4], const uint64_t a[4], const uint64_t b[4])
{
	uint64_t z[8];
	uint64_t t[8];
	uint64_t c;

	// z = a * b
	sm9_z256_mul(z, a, b);

	// t = low(z) * p'
	sm9_z256_mul_low(t, z, SM9_Z256_P_PRIME);

	// t = low(t) * p
	sm9_z256_mul(t, t, SM9_Z256_P);

	// z = z + t
	c = sm9_z512_add(z, z, t);

	// r = high(r)
	sm9_z256_copy(r, z + 4);

	if (c) {
		sm9_z256_add(r, r, SM9_Z256_MODP_MONT_ONE);

	} else if (sm9_z256_cmp(r, SM9_Z256_P) >= 0) {
		(void)sm9_z256_sub(r, r, SM9_Z256_P);
	}
}
#endif

// TODO: NEON/SVE/SVE2 implementation
#if 0
void sm9_z256_fp_mont_mul_2way(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	sm9_z256_t d = {0}, e = {0};
	uint64_t q, t0, t1, p0, p1, tmp;
	uint64_t pre = SM9_Z256_MODP_MU * b[0];
	int i, j;

	for (j = 0; j < 4; j++) {
		q = pre * a[j] + SM9_Z256_MODP_MU * (d[0]-e[0]);

		sm9_u64_mul_add(&tmp, &t0, a[j], b[0], d[0], 0);
		sm9_u64_mul_add(&tmp, &t1, q, SM9_Z256_P[0], e[0], 0);

		for (i = 1; i < 4; i++) {
			sm9_u64_mul_add(&d[i-1], &t0, a[j], b[i], t0, d[i]);
			sm9_u64_mul_add(&e[i-1], &t1, q, SM9_Z256_P[i], t1, e[i]);
		}
		d[3] = t0;
		e[3] = t1;
	}

	if (sm9_z256_sub(r, d, e)) {
		sm9_z256_add(r, r, SM9_Z256_P);
	}
}
#endif


#ifndef ENABLE_SM9_Z256_ARMV8
void sm9_z256_fp_to_mont(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_fp_mont_mul(r, a, SM9_Z256_MODP_2e512);
}

void sm9_z256_fp_from_mont(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_fp_mont_mul(r, a, SM9_Z256_ONE);
}

void sm9_z256_fp_mont_sqr(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_fp_mont_mul(r, a, a);
}
#endif

// change args name to a_mont, r_mont
// 这个函数反复多次调用mont_mul，如果展开为asm，可以节约一些开销
// 也可以不展开这个函数，只是展开inv那个函数
// 所有256次调用可以完全展开，展开之后不需要判断w，直接就做计算就可以了，好处就是所有的初始化都不需要了
void sm9_z256_fp_pow(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t e)
{
	sm9_z256_t t;
	uint64_t w;
	int i, j;

	// t = mont(1) (mod p)
	sm9_z256_copy(t, SM9_Z256_MODP_MONT_ONE);

	for (i = 3; i >= 0; i--) {
		w = e[i];
		for (j = 0; j < 64; j++) {
			sm9_z256_fp_mont_sqr(t, t);
			if (w & 0x8000000000000000) {
				sm9_z256_fp_mont_mul(t, t, a);
			}
			w <<= 1;
		}
	}

	sm9_z256_copy(r, t);
}

void sm9_z256_fp_inv(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_fp_pow(r, a, SM9_Z256_P_MINUS_TWO);
}

int sm9_z256_fp_from_bytes(sm9_z256_t r, const uint8_t buf[32])
{
	sm9_z256_from_bytes(r, buf);
	sm9_z256_fp_to_mont(r, r);
	if (sm9_z256_cmp(r, SM9_Z256_P) >= 0) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp_to_bytes(const sm9_z256_t r, uint8_t out[32])
{
	sm9_z256_t t;
	sm9_z256_fp_from_mont(t, r);
	sm9_z256_to_bytes(t, out);
}

int sm9_z256_fp_from_hex(sm9_z256_t r, const char hex[64])
{
	if (sm9_z256_from_hex(r, hex) != 1) {
		error_print();
		return -1;
	}
	if (sm9_z256_cmp(r, SM9_Z256_P) >= 0) {
		error_print();
		return -1;
	}
	sm9_z256_fp_to_mont(r, r);
	return 1;
}

void sm9_z256_fp_to_hex(const sm9_z256_t r, char hex[64])
{
	sm9_z256_t t;
	sm9_z256_fp_from_mont(t, r);
	int i;
	for (i = 3; i >= 0; i--) {
		(void)sprintf(hex + 16*(3-i), "%016llx", t[i]);
	}
}


const sm9_z256_fp2 SM9_Z256_FP2_ZERO = {{0,0,0,0},{0,0,0,0}};
const sm9_z256_fp2 SM9_Z256_FP2_ONE = {{1,0,0,0},{0,0,0,0}};
const sm9_z256_fp2 SM9_Z256_FP2_U = {{0,0,0,0},{1,0,0,0}};
static const sm9_z256_fp2 SM9_Z256_FP2_MONT_5U = {{0,0,0,0},{0xb9f2c1e8c8c71995, 0x125df8f246a377fc, 0x25e650d049188d1c, 0x43fffffed866f63}};

void sm9_z256_fp2_set_one(sm9_z256_fp2 r)
{
	sm9_z256_fp_copy(r[0], SM9_Z256_MODP_MONT_ONE);
	sm9_z256_fp_set_zero(r[1]);
}

int sm9_z256_fp2_is_one(const sm9_z256_fp2 r)
{
	return sm9_z256_equ(r[0], SM9_Z256_MODP_MONT_ONE) && sm9_z256_is_zero(r[1]);
}

int sm9_z256_fp2_equ(const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	return sm9_z256_equ(a[0], b[0]) && sm9_z256_equ(a[1], b[1]);
}

void sm9_z256_fp2_copy(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_copy(r[0], a[0]);
	sm9_z256_copy(r[1], a[1]);
}

int sm9_z256_fp2_rand(sm9_z256_fp2 r)
{
	if (sm9_z256_fp_rand(r[0]) != 1
		|| sm9_z256_fp_rand(r[1]) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp2_to_bytes(const sm9_z256_fp2 a, uint8_t buf[64])
{
	sm9_z256_fp_to_bytes(a[1], buf);
	sm9_z256_fp_to_bytes(a[0], buf + 32);
}

int sm9_z256_fp2_from_bytes(sm9_z256_fp2 r, const uint8_t buf[64])
{
	if (sm9_z256_fp_from_bytes(r[1], buf) != 1
		|| sm9_z256_fp_from_bytes(r[0], buf + 32) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

int sm9_z256_fp2_from_hex(sm9_z256_fp2 r, const char hex[129])
{
	if (sm9_z256_fp_from_hex(r[1], hex) != 1
		|| sm9_z256_fp_from_hex(r[0], hex + 65) != 1) {
		error_print();
		return -1;
	}
	/*
	if (hex[64] != SM9_Z256_HEX_SEP) {
		error_print();
		return -1;
	}
	*/
	return 1;
}

void sm9_z256_fp2_to_hex(const sm9_z256_fp2 a, char hex[129])
{
	sm9_z256_fp_to_hex(a[1], hex);
	hex[64] = SM9_Z256_HEX_SEP;
	sm9_z256_fp_to_hex(a[0], hex + 65);
}

// TODO:
// fp2, fp4 函数可以粗粒度并行，或者调用 __sm9_z256_fp_add 来函数开始和结束的开销
// 是否需要给fp2提供独立的展开函数？还是直接展开fp4，提供armv8?

void sm9_z256_fp2_add(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	sm9_z256_fp_add(r[0], a[0], b[0]);
	sm9_z256_fp_add(r[1], a[1], b[1]);
}

void sm9_z256_fp2_dbl(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp_dbl(r[0], a[0]);
	sm9_z256_fp_dbl(r[1], a[1]);
}

void sm9_z256_fp2_tri(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp_tri(r[0], a[0]);
	sm9_z256_fp_tri(r[1], a[1]);
}

void sm9_z256_fp2_sub(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	sm9_z256_fp_sub(r[0], a[0], b[0]);
	sm9_z256_fp_sub(r[1], a[1], b[1]);
}

void sm9_z256_fp2_neg(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp_neg(r[0], a[0]);
	sm9_z256_fp_neg(r[1], a[1]);
}

void sm9_z256_fp2_a_mul_u(sm9_z256_fp2 r, sm9_z256_fp2 a)
{
	sm9_z256_t r0;

	sm9_z256_fp_dbl(r0, a[1]);
	sm9_z256_fp_neg(r0, r0);

	sm9_z256_fp_copy(r[1], a[0]);
	sm9_z256_fp_copy(r[0], r0);
}


void sm9_z256_fp2_mul(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	sm9_z256_t t0;
	sm9_z256_t t1;
	sm9_z256_t t2;

	// t2 = (a0 + a1) * (b0 + b1)
	sm9_z256_fp_add(t0, a[0], a[1]);
	sm9_z256_fp_add(t1, b[0], b[1]);
	sm9_z256_fp_mul(t2, t0, t1);

	// t0 = a0 * b0
	sm9_z256_fp_mul(t0, a[0], b[0]);

	// t1 = a1 * b1
	sm9_z256_fp_mul(t1, a[1], b[1]);

	// r1 = t2 - t0 - t1 = a0 * b1 + a1 * b0
	sm9_z256_fp_sub(t2, t2, t0);
	sm9_z256_fp_sub(t2, t2, t1);

	// r0 = t0 - 2*t1 = a0 * b0 - 2(a1 * b1)
	sm9_z256_fp_dbl(t1, t1);
	sm9_z256_fp_sub(t0, t0, t1);

	sm9_z256_fp_copy(r[0], t0);
	sm9_z256_fp_copy(r[1], t2);
}

void sm9_z256_fp2_mul_u(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	sm9_z256_t t0;
	sm9_z256_t t1;
	sm9_z256_t t2;

	// t2 = (a0 + a1) * (b0 + b1)
	sm9_z256_fp_add(t0, a[0], a[1]);
	sm9_z256_fp_add(t1, b[0], b[1]);
	sm9_z256_fp_mul(t2, t0, t1);

	// t0 = a0 * b0
	sm9_z256_fp_mul(t0, a[0], b[0]);

	// t1 = a1 * b1
	sm9_z256_fp_mul(t1, a[1], b[1]);

	// r0 = -2 *(t2 - t0 - t1) = -2 * (a0 * b1 + a1 * b0)
	sm9_z256_fp_sub(t2, t2, t0);
	sm9_z256_fp_sub(t2, t2, t1);
	sm9_z256_fp_dbl(t2, t2);
	sm9_z256_fp_neg(t2, t2);

	// r1 = t0 - 2*t1 = a0 * b0 - 2(a1 * b1)
	sm9_z256_fp_dbl(t1, t1);
	sm9_z256_fp_sub(t0, t0, t1);

	sm9_z256_fp_copy(r[0], t2);
	sm9_z256_fp_copy(r[1], t0);
}

void sm9_z256_fp2_mul_fp(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_t k)
{
	sm9_z256_fp_mul(r[0], a[0], k);
	sm9_z256_fp_mul(r[1], a[1], k);
}

void sm9_z256_fp2_sqr(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_t r0, r1, c0, c1;

	// r0 = (a0 + a1) * (a0 - 2a1) + a0 * a1
	sm9_z256_fp_mul(r1, a[0], a[1]);
	sm9_z256_fp_add(c0, a[0], a[1]);
	sm9_z256_fp_dbl(c1, a[1]);
	sm9_z256_fp_sub(c1, a[0], c1);
	sm9_z256_fp_mul(r0, c0, c1);
	sm9_z256_fp_add(r0, r0, r1);

	// r1 = 2 * a0 * a1
	sm9_z256_fp_dbl(r1, r1);

	sm9_z256_copy(r[0], r0);
	sm9_z256_copy(r[1], r1);
}

void sm9_z256_fp2_sqr_u(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_t t0;
	sm9_z256_t t1;
	sm9_z256_t t2;

	// t0 = a0 * a1
	sm9_z256_fp_mont_mul(t0, a[0], a[1]);

	// t1 = a0 + a1
	sm9_z256_fp_add(t1, a[0], a[1]);

	// t2 = a0 - 2*a
	sm9_z256_fp_sub(t2, a[0], a[1]);
	sm9_z256_fp_sub(t2, t2, a[1]);

	// r1 = t1 * t2 + t0
	sm9_z256_fp_mont_mul(t2, t2, t1);
	sm9_z256_fp_add(t2, t2, t0);

	// r0 = -4 * t0
	sm9_z256_fp_dbl(t0, t0);
	sm9_z256_fp_dbl(t0, t0);
	sm9_z256_fp_neg(t0, t0);

	sm9_z256_copy(r[0], t0);
	sm9_z256_copy(r[1], t2);
}

void sm9_z256_fp2_inv(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	if (sm9_z256_fp_is_zero(a[0])) {
		// r0 = 0
		sm9_z256_fp_set_zero(r[0]);
		// r1 = -(2 * a1)^-1
		sm9_z256_fp_dbl(r[1], a[1]);
		sm9_z256_fp_inv(r[1], r[1]);
		sm9_z256_fp_neg(r[1], r[1]);

	} else if (sm9_z256_fp_is_zero(a[1])) {
		/* r1 = 0 */
		sm9_z256_fp_set_zero(r[1]);
		/* r0 = a0^-1 */
		sm9_z256_fp_inv(r[0], a[0]);

	} else {
		sm9_z256_t k, t;

		// k = (a[0]^2 + 2 * a[1]^2)^-1
		sm9_z256_fp_sqr(k, a[0]);
		sm9_z256_fp_sqr(t, a[1]);
		sm9_z256_fp_dbl(t, t);
		sm9_z256_fp_add(k, k, t);
		sm9_z256_fp_inv(k, k);

		// r[0] = a[0] * k
		sm9_z256_fp_mul(r[0], a[0], k);

		// r[1] = -a[1] * k
		sm9_z256_fp_mul(r[1], a[1], k);
		sm9_z256_fp_neg(r[1], r[1]);
	}
}

void sm9_z256_fp2_div(sm9_z256_fp2 r, const sm9_z256_fp2 a, const sm9_z256_fp2 b)
{
	sm9_z256_fp2 t;
	sm9_z256_fp2_inv(t, b);
	sm9_z256_fp2_mul(r, a, t);
}

void sm9_z256_fp2_div2(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp_div2(r[0], a[0]);
	sm9_z256_fp_div2(r[1], a[1]);
}


const sm9_z256_fp4 SM9_Z256_FP4_ZERO = {{{0,0,0,0},{0,0,0,0}}, {{0,0,0,0},{0,0,0,0}}};
const sm9_z256_fp4 SM9_Z256_FP4_MONT_ONE = {{{0x1a9064d81caeba83, 0xde0d6cb4e5851124, 0x29fc54b00a7138ba, 0x49bffffffd5c590e},{0,0,0,0}}, {{0,0,0,0},{0,0,0,0}}};

int sm9_z256_fp4_equ(const sm9_z256_fp4 a, const sm9_z256_fp4 b)
{
	return sm9_z256_fp2_equ(a[0], b[0]) && sm9_z256_fp2_equ(a[1], b[1]);
}

int sm9_z256_fp4_rand(sm9_z256_fp4 r)
{
	if (sm9_z256_fp2_rand(r[1]) != 1
		|| sm9_z256_fp2_rand(r[0]) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp4_copy(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_copy(r[0], a[0]);
	sm9_z256_fp2_copy(r[1], a[1]);
}

void sm9_z256_fp4_to_bytes(const sm9_z256_fp4 a, uint8_t buf[128])
{
	sm9_z256_fp2_to_bytes(a[1], buf);
	sm9_z256_fp2_to_bytes(a[0], buf + 64);
}

int sm9_z256_fp4_from_bytes(sm9_z256_fp4 r, const uint8_t buf[128])
{
	if (sm9_z256_fp2_from_bytes(r[1], buf) != 1
		|| sm9_z256_fp2_from_bytes(r[0], buf + 64) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

int sm9_z256_fp4_from_hex(sm9_z256_fp4 r, const char hex[65 * 4])
{
	if (sm9_z256_fp2_from_hex(r[1], hex) != 1
		|| hex[129] != SM9_Z256_HEX_SEP
		|| sm9_z256_fp2_from_hex(r[0], hex + 130) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp4_to_hex(const sm9_z256_fp4 a, char hex[259])
{
	sm9_z256_fp2_to_hex(a[1], hex);
	hex[129] = SM9_Z256_HEX_SEP;
	sm9_z256_fp2_to_hex(a[0], hex + 130);
}

void sm9_z256_fp4_add(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_fp4 b)
{
	sm9_z256_fp2_add(r[0], a[0], b[0]);
	sm9_z256_fp2_add(r[1], a[1], b[1]);
}

void sm9_z256_fp4_dbl(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_dbl(r[0], a[0]);
	sm9_z256_fp2_dbl(r[1], a[1]);
}

void sm9_z256_fp4_sub(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_fp4 b)
{
	sm9_z256_fp2_sub(r[0], a[0], b[0]);
	sm9_z256_fp2_sub(r[1], a[1], b[1]);
}

void sm9_z256_fp4_neg(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_neg(r[0], a[0]);
	sm9_z256_fp2_neg(r[1], a[1]);
}

void sm9_z256_fp4_div2(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_div2(r[0], a[0]);
	sm9_z256_fp2_div2(r[1], a[1]);
}

void sm9_z256_fp4_a_mul_v(sm9_z256_fp4 r, sm9_z256_fp4 a)
{
	sm9_z256_fp2 r0;

	sm9_z256_fp2_a_mul_u(r0, a[1]);

	sm9_z256_fp2_copy(r[1], a[0]);
	sm9_z256_fp2_copy(r[0], r0);
}

void sm9_z256_fp4_mul(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_fp4 b)
{
	sm9_z256_fp2 r0, r1, t;

	// r0 = a0 + a1
	sm9_z256_fp2_add(r0, a[0], a[1]);

	// t = b0 + b1
	sm9_z256_fp2_add(t, b[0], b[1]);

	// r1 = (a0 + a1) * (b0 + b1)
	sm9_z256_fp2_mul(r1, t, r0);

	// r0 = a0 * b0
	sm9_z256_fp2_mul(r0, a[0], b[0]);

	// t = a1 * b1
	sm9_z256_fp2_mul(t, a[1], b[1]);

	// r1 = a0 * b1 + a1 * b0
	sm9_z256_fp2_sub(r1, r1, r0);
	sm9_z256_fp2_sub(r1, r1, t);

	// t = a1 * b1 * u
	sm9_z256_fp2_a_mul_u(t, t);

	// r0 = a0 * b0 + a1 * b1 * u
	sm9_z256_fp2_add(r0, r0, t);

	sm9_z256_fp2_copy(r[0], r0);
	sm9_z256_fp2_copy(r[1], r1);
}

void sm9_z256_fp4_mul_fp(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_t k)
{
	sm9_z256_fp2_mul_fp(r[0], a[0], k);
	sm9_z256_fp2_mul_fp(r[1], a[1], k);
}

void sm9_z256_fp4_mul_fp2(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_fp2 b0)
{
	sm9_z256_fp2_mul(r[0], a[0], b0);
	sm9_z256_fp2_mul(r[1], a[1], b0);
}

void sm9_z256_fp4_mul_v(sm9_z256_fp4 r, const sm9_z256_fp4 a, const sm9_z256_fp4 b)
{
	sm9_z256_fp2 r0, r1, t;

	sm9_z256_fp2_mul_u(r0, a[0], b[1]);
	sm9_z256_fp2_mul_u(t, a[1], b[0]);
	sm9_z256_fp2_add(r0, r0, t);

	sm9_z256_fp2_mul(r1, a[0], b[0]);
	sm9_z256_fp2_mul_u(t, a[1], b[1]);
	sm9_z256_fp2_add(r1, r1, t);

	sm9_z256_fp2_copy(r[0], r0);
	sm9_z256_fp2_copy(r[1], r1);
}

void sm9_z256_fp4_sqr(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2 r0, r1, t;

	sm9_z256_fp2_add(r1, a[0], a[1]);
	sm9_z256_fp2_sqr(r1, r1);

	sm9_z256_fp2_sqr(r0, a[0]);
	sm9_z256_fp2_sqr(t, a[1]);

	sm9_z256_fp2_sub(r1, r1, r0);
	sm9_z256_fp2_sub(r1, r1, t);

	sm9_z256_fp2_a_mul_u(t, t);
	sm9_z256_fp2_add(r0, r0, t);

	sm9_z256_fp2_copy(r[0], r0);
	sm9_z256_fp2_copy(r[1], r1);
}

void sm9_z256_fp4_sqr_v(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2 r0, r1, t;

	sm9_z256_fp2_mul_u(t, a[0], a[1]);
	sm9_z256_fp2_dbl(r0, t);

	sm9_z256_fp2_sqr(r1, a[0]);
	sm9_z256_fp2_sqr_u(t, a[1]);
	sm9_z256_fp2_add(r1, r1, t);

	sm9_z256_fp2_copy(r[0], r0);
	sm9_z256_fp2_copy(r[1], r1);
}

void sm9_z256_fp4_inv(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2 r0, r1, k;

	sm9_z256_fp2_sqr_u(k, a[1]);
	sm9_z256_fp2_sqr(r0, a[0]);
	sm9_z256_fp2_sub(k, k, r0);
	sm9_z256_fp2_inv(k, k);

	sm9_z256_fp2_mul(r0, a[0], k);
	sm9_z256_fp2_neg(r0, r0);

	sm9_z256_fp2_mul(r1, a[1], k);

	sm9_z256_fp2_copy(r[0], r0);
	sm9_z256_fp2_copy(r[1], r1);
}




void sm9_z256_fp12_copy(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	sm9_z256_fp4_copy(r[0], a[0]);
	sm9_z256_fp4_copy(r[1], a[1]);
	sm9_z256_fp4_copy(r[2], a[2]);
}

int sm9_z256_fp12_rand(sm9_z256_fp12 r)
{
	if (sm9_z256_fp4_rand(r[0]) != 1
		|| sm9_z256_fp4_rand(r[1]) != 1
		|| sm9_z256_fp4_rand(r[2]) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp12_set_zero(sm9_z256_fp12 r)
{
	sm9_z256_fp4_copy(r[0], SM9_Z256_FP4_ZERO);
	sm9_z256_fp4_copy(r[1], SM9_Z256_FP4_ZERO);
	sm9_z256_fp4_copy(r[2], SM9_Z256_FP4_ZERO);
}

void sm9_z256_fp12_set_one(sm9_z256_fp12 r)
{
	sm9_z256_fp4_copy(r[0], SM9_Z256_FP4_MONT_ONE);
	sm9_z256_fp4_copy(r[1], SM9_Z256_FP4_ZERO);
	sm9_z256_fp4_copy(r[2], SM9_Z256_FP4_ZERO);
}

int sm9_z256_fp12_from_hex(sm9_z256_fp12 r, const char hex[65 * 12 - 1])
{
	if (sm9_z256_fp4_from_hex(r[2], hex) != 1
		|| hex[65 * 4 - 1] != SM9_Z256_HEX_SEP
		|| sm9_z256_fp4_from_hex(r[1], hex + 65 * 4) != 1
		|| hex[65 * 4 - 1] != SM9_Z256_HEX_SEP
		|| sm9_z256_fp4_from_hex(r[0], hex + 65 * 8) != 1) {
		error_print();
		return -1;
	}
	return 1;
}

void sm9_z256_fp12_to_hex(const sm9_z256_fp12 a, char hex[65 * 12 - 1])
{
	sm9_z256_fp4_to_hex(a[2], hex);
	hex[65 * 4 - 1] = SM9_Z256_HEX_SEP;
	sm9_z256_fp4_to_hex(a[1], hex + 65 * 4);
	hex[65 * 8 - 1] = SM9_Z256_HEX_SEP;
	sm9_z256_fp4_to_hex(a[0], hex + 65 * 8);
}

void sm9_z256_fp12_to_bytes(const sm9_z256_fp12 a, uint8_t buf[32 * 12])
{
	sm9_z256_fp4_to_bytes(a[2], buf);
	sm9_z256_fp4_to_bytes(a[1], buf + 32 * 4);
	sm9_z256_fp4_to_bytes(a[0], buf + 32 * 8);
}

void sm9_z256_fp12_print(const char *prefix, const sm9_z256_fp12 a)
{
	char hex[65 * 12];
	sm9_z256_fp12_to_hex(a, hex);
	printf("%s\n%s\n", prefix, hex);
}

void sm9_z256_fp12_set(sm9_z256_fp12 r, const sm9_z256_fp4 a0, const sm9_z256_fp4 a1, const sm9_z256_fp4 a2)
{
	sm9_z256_fp4_copy(r[0], a0);
	sm9_z256_fp4_copy(r[1], a1);
	sm9_z256_fp4_copy(r[2], a2);
}

int sm9_z256_fp12_equ(const sm9_z256_fp12 a, const sm9_z256_fp12 b)
{
	return sm9_z256_fp4_equ(a[0], b[0])
		&& sm9_z256_fp4_equ(a[1], b[1])
		&& sm9_z256_fp4_equ(a[2], b[2]);
}

void sm9_z256_fp12_add(sm9_z256_fp12 r, const sm9_z256_fp12 a, const sm9_z256_fp12 b)
{
	sm9_z256_fp4_add(r[0], a[0], b[0]);
	sm9_z256_fp4_add(r[1], a[1], b[1]);
	sm9_z256_fp4_add(r[2], a[2], b[2]);
}

void sm9_z256_fp12_dbl(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	sm9_z256_fp4_dbl(r[0], a[0]);
	sm9_z256_fp4_dbl(r[1], a[1]);
	sm9_z256_fp4_dbl(r[2], a[2]);
}

void sm9_z256_fp12_tri(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	sm9_z256_fp12 t;
	sm9_z256_fp12_dbl(t, a);
	sm9_z256_fp12_add(r, t, a);
}

void sm9_z256_fp12_sub(sm9_z256_fp12 r, const sm9_z256_fp12 a, const sm9_z256_fp12 b)
{
	sm9_z256_fp4_sub(r[0], a[0], b[0]);
	sm9_z256_fp4_sub(r[1], a[1], b[1]);
	sm9_z256_fp4_sub(r[2], a[2], b[2]);
}

void sm9_z256_fp12_neg(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	sm9_z256_fp4_neg(r[0], a[0]);
	sm9_z256_fp4_neg(r[1], a[1]);
	sm9_z256_fp4_neg(r[2], a[2]);
}

void sm9_z256_fp12_mul(sm9_z256_fp12 r, const sm9_z256_fp12 a, const sm9_z256_fp12 b)
{
	sm9_z256_fp4 r0, r1, r2;
	sm9_z256_fp4 t,  k0, k1;
	sm9_z256_fp4 m0, m1, m2;

	sm9_z256_fp4_mul(m0, a[0], b[0]);
	sm9_z256_fp4_mul(m1, a[1], b[1]);
	sm9_z256_fp4_mul(m2, a[2], b[2]);

	sm9_z256_fp4_add(k0, a[1], a[2]);
	sm9_z256_fp4_add(k1, b[1], b[2]);
	sm9_z256_fp4_mul(t, k0, k1);
	sm9_z256_fp4_sub(t, t, m1);
	sm9_z256_fp4_sub(t, t, m2);
	sm9_z256_fp4_a_mul_v(t, t);
	sm9_z256_fp4_add(r0, t, m0);

	sm9_z256_fp4_add(k0, a[0], a[2]);
	sm9_z256_fp4_add(k1, b[0], b[2]);
	sm9_z256_fp4_mul(t, k0, k1);
	sm9_z256_fp4_sub(t, t, m0);
	sm9_z256_fp4_sub(t, t, m2);
	sm9_z256_fp4_add(r2, t, m1);

	sm9_z256_fp4_add(k0, a[0], a[1]);
	sm9_z256_fp4_add(k1, b[0], b[1]);
	sm9_z256_fp4_mul(t, k0, k1);
	sm9_z256_fp4_sub(t, t, m0);
	sm9_z256_fp4_sub(t, t, m1);
	sm9_z256_fp4_a_mul_v(m2, m2);
	sm9_z256_fp4_add(r1, t, m2);

	sm9_z256_fp4_copy(r[0], r0);
	sm9_z256_fp4_copy(r[1], r1);
	sm9_z256_fp4_copy(r[2], r2);
}

// this is slower than the version below
// void sm9_z256_fp12_sqr(sm9_z256_fp12 r, const sm9_z256_fp12 a)
// {
// 	sm9_z256_fp4 r0, r1, r2, t;

// 	sm9_z256_fp4_sqr(r0, a[0]);
// 	sm9_z256_fp4_mul_v(t, a[1], a[2]);
// 	sm9_z256_fp4_dbl(t, t);
// 	sm9_z256_fp4_add(r0, r0, t);

// 	sm9_z256_fp4_mul(r1, a[0], a[1]);
// 	sm9_z256_fp4_dbl(r1, r1);
// 	sm9_z256_fp4_sqr_v(t, a[2]);
// 	sm9_z256_fp4_add(r1, r1, t);

// 	sm9_z256_fp4_mul(r2, a[0], a[2]);
// 	sm9_z256_fp4_dbl(r2, r2);
// 	sm9_z256_fp4_sqr(t, a[1]);
// 	sm9_z256_fp4_add(r2, r2, t);

// 	sm9_z256_fp4_copy(r[0], r0);
// 	sm9_z256_fp4_copy(r[1], r1);
// 	sm9_z256_fp4_copy(r[2], r2);
// }

void sm9_z256_fp12_sqr(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	sm9_z256_fp4 h0, h1, h2, t;
	sm9_z256_fp4 s0, s1, s2, s3;

	sm9_z256_fp4_sqr(h0, a[0]);
	sm9_z256_fp4_sqr(h1, a[2]);
	sm9_z256_fp4_add(s0, a[2], a[0]);

	sm9_z256_fp4_sub(t, s0, a[1]);
	sm9_z256_fp4_sqr(s1, t);

	sm9_z256_fp4_add(t, s0, a[1]);
	sm9_z256_fp4_sqr(s0, t);

	sm9_z256_fp4_mul(s2, a[1], a[2]);
	sm9_z256_fp4_dbl(s2, s2);

	sm9_z256_fp4_add(s3, s0, s1);
	sm9_z256_fp4_div2(s3, s3);

	sm9_z256_fp4_sub(t, s3, h1);
	sm9_z256_fp4_sub(h2, t, h0);

	sm9_z256_fp4_a_mul_v(h1, h1);
	sm9_z256_fp4_add(h1, h1, s0);
	sm9_z256_fp4_sub(h1, h1, s2);
	sm9_z256_fp4_sub(h1, h1, s3);

	sm9_z256_fp4_a_mul_v(s2, s2);
	sm9_z256_fp4_add(h0, h0, s2);

	sm9_z256_fp4_copy(r[0], h0);
	sm9_z256_fp4_copy(r[1], h1);
	sm9_z256_fp4_copy(r[2], h2);
}

void sm9_z256_fp12_inv(sm9_z256_fp12 r, const sm9_z256_fp12 a)
{
	if (sm9_z256_fp4_is_zero(a[2])) {
		sm9_z256_fp4 k, t;

		sm9_z256_fp4_sqr(k, a[0]);
		sm9_z256_fp4_mul(k, k, a[0]);
		sm9_z256_fp4_sqr_v(t, a[1]);
		sm9_z256_fp4_mul(t, t, a[1]);
		sm9_z256_fp4_add(k, k, t);
		sm9_z256_fp4_inv(k, k);

		sm9_z256_fp4_sqr(r[2], a[1]);
		sm9_z256_fp4_mul(r[2], r[2], k);

		sm9_z256_fp4_mul(r[1], a[0], a[1]);
		sm9_z256_fp4_mul(r[1], r[1], k);
		sm9_z256_fp4_neg(r[1], r[1]);

		sm9_z256_fp4_sqr(r[0], a[0]);
		sm9_z256_fp4_mul(r[0], r[0], k);

	} else {
		sm9_z256_fp4 t0, t1, t2, t3;

		sm9_z256_fp4_sqr(t0, a[1]);
		sm9_z256_fp4_mul(t1, a[0], a[2]);
		sm9_z256_fp4_sub(t0, t0, t1);

		sm9_z256_fp4_mul(t1, a[0], a[1]);
		sm9_z256_fp4_sqr_v(t2, a[2]);
		sm9_z256_fp4_sub(t1, t1, t2);

		sm9_z256_fp4_sqr(t2, a[0]);
		sm9_z256_fp4_mul_v(t3, a[1], a[2]);
		sm9_z256_fp4_sub(t2, t2, t3);

		sm9_z256_fp4_sqr(t3, t1);
		sm9_z256_fp4_mul(r[0], t0, t2);
		sm9_z256_fp4_sub(t3, t3, r[0]);
		sm9_z256_fp4_inv(t3, t3);
		sm9_z256_fp4_mul(t3, a[2], t3);

		sm9_z256_fp4_mul(r[0], t2, t3);

		sm9_z256_fp4_mul(r[1], t1, t3);
		sm9_z256_fp4_neg(r[1], r[1]);

		sm9_z256_fp4_mul(r[2], t0, t3);
	}
}

void sm9_z256_fp12_pow(sm9_z256_fp12 r, const sm9_z256_fp12 a, const sm9_z256_t k)
{
	sm9_z256_fp12 t;
	uint64_t w;
	int i, j;

	assert(sm9_z256_cmp(k, SM9_Z256_N_MINUS_ONE) < 0);
	sm9_z256_fp12_set_one(t);

	for (i = 3; i >=0; i--) {
		w = k[i];
		for (j = 0; j < 64; j++) {
			sm9_z256_fp12_sqr(t, t);
			if (w & 0x8000000000000000) {
				sm9_z256_fp12_mul(t, t, a);
			}
			w <<= 1;
		}
	}
	sm9_z256_fp12_copy(r, t);
}

void sm9_z256_fp2_conjugate(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp_copy(r[0], a[0]);
	sm9_z256_fp_neg (r[1], a[1]);
}

void sm9_z256_fp2_frobenius(sm9_z256_fp2 r, const sm9_z256_fp2 a)
{
	sm9_z256_fp2_conjugate(r, a);
}

// beta   = 0x6c648de5dc0a3f2cf55acc93ee0baf159f9d411806dc5177f5b21fd3da24d011
// alpha1 = 0x3f23ea58e5720bdb843c6cfa9c08674947c5c86e0ddd04eda91d8354377b698b
// alpha2 = 0xf300000002a3a6f2780272354f8b78f4d5fc11967be65334
// alpha3 = 0x6c648de5dc0a3f2cf55acc93ee0baf159f9d411806dc5177f5b21fd3da24d011
// alpha4 = 0xf300000002a3a6f2780272354f8b78f4d5fc11967be65333
// alpha5 = 0x2d40a38cf6983351711e5f99520347cc57d778a9f8ff4c8a4c949c7fa2a96686

// mont version (mod p)
static const sm9_z256_fp2 SM9_MONT_BETA  = {{0x39b4ef0f3ee72529, 0xdb043bf508582782, 0xb8554ab054ac91e3, 0x9848eec25498cab5}, {0}};
static const sm9_z256_t SM9_MONT_ALPHA1   = {0x1a98dfbd4575299f, 0x9ec8547b245c54fd, 0xf51f5eac13df846c, 0x9ef74015d5a16393};
static const sm9_z256_t SM9_MONT_ALPHA2   = {0xb626197dce4736ca, 0x08296b3557ed0186, 0x9c705db2fd91512a, 0x1c753e748601c992};
static const sm9_z256_t SM9_MONT_ALPHA3   = {0x39b4ef0f3ee72529, 0xdb043bf508582782, 0xb8554ab054ac91e3, 0x9848eec25498cab5};
static const sm9_z256_t SM9_MONT_ALPHA4   = {0x81054fcd94e9c1c4, 0x4c0e91cb8ce2df3e, 0x4877b452e8aedfb4, 0x88f53e748b491776};
static const sm9_z256_t SM9_MONT_ALPHA5   = {0x048baa79dcc34107, 0x5e2e7ac4fe76c161, 0x99399754365bd4bc, 0xaf91aeac819b0e13};


void sm9_z256_fp4_frobenius(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_conjugate(r[0], a[0]);
	sm9_z256_fp2_conjugate(r[1], a[1]);
	sm9_z256_fp2_mul(r[1], r[1], SM9_MONT_BETA);
}

void sm9_z256_fp4_conjugate(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_copy(r[0], a[0]);
	sm9_z256_fp2_neg(r[1], a[1]);
}

void sm9_z256_fp4_frobenius2(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp4_conjugate(r, a);
}

void sm9_z256_fp4_frobenius3(sm9_z256_fp4 r, const sm9_z256_fp4 a)
{
	sm9_z256_fp2_conjugate(r[0], a[0]);
	sm9_z256_fp2_conjugate(r[1], a[1]);
	sm9_z256_fp2_mul(r[1], r[1], SM9_MONT_BETA);
	sm9_z256_fp2_neg(r[1], r[1]);
}

void sm9_z256_fp12_frobenius(sm9_z256_fp12 r, const sm9_z256_fp12 x)
{
	const sm9_z256_fp2 *xa = x[0];
	const sm9_z256_fp2 *xb = x[1];
	const sm9_z256_fp2 *xc = x[2];

	sm9_z256_fp4 ra;
	sm9_z256_fp4 rb;
	sm9_z256_fp4 rc;

	sm9_z256_fp2_conjugate(ra[0], xa[0]);
	sm9_z256_fp2_conjugate(ra[1], xa[1]);
	sm9_z256_fp2_mul_fp(ra[1], ra[1], SM9_MONT_ALPHA3);

	sm9_z256_fp2_conjugate(rb[0], xb[0]);
	sm9_z256_fp2_mul_fp(rb[0], rb[0], SM9_MONT_ALPHA1);
	sm9_z256_fp2_conjugate(rb[1], xb[1]);
	sm9_z256_fp2_mul_fp(rb[1], rb[1], SM9_MONT_ALPHA4);

	sm9_z256_fp2_conjugate(rc[0], xc[0]);
	sm9_z256_fp2_mul_fp(rc[0], rc[0], SM9_MONT_ALPHA2);
	sm9_z256_fp2_conjugate(rc[1], xc[1]);
	sm9_z256_fp2_mul_fp(rc[1], rc[1], SM9_MONT_ALPHA5);

	sm9_z256_fp12_set(r, ra, rb, rc);
}

void sm9_z256_fp12_frobenius2(sm9_z256_fp12 r, const sm9_z256_fp12 x)
{
	sm9_z256_fp4 a;
	sm9_z256_fp4 b;
	sm9_z256_fp4 c;

	sm9_z256_fp4_conjugate(a, x[0]);
	sm9_z256_fp4_conjugate(b, x[1]);
	sm9_z256_fp4_mul_fp(b, b, SM9_MONT_ALPHA2);
	sm9_z256_fp4_conjugate(c, x[2]);
	sm9_z256_fp4_mul_fp(c, c, SM9_MONT_ALPHA4);

	sm9_z256_fp4_copy(r[0], a);
	sm9_z256_fp4_copy(r[1], b);
	sm9_z256_fp4_copy(r[2], c);
}

void sm9_z256_fp12_frobenius3(sm9_z256_fp12 r, const sm9_z256_fp12 x)
{
	const sm9_z256_fp2 *xa = x[0];
	const sm9_z256_fp2 *xb = x[1];
	const sm9_z256_fp2 *xc = x[2];

	sm9_z256_fp4 ra;
	sm9_z256_fp4 rb;
	sm9_z256_fp4 rc;

	sm9_z256_fp2_conjugate(ra[0], xa[0]);
	sm9_z256_fp2_conjugate(ra[1], xa[1]);
	sm9_z256_fp2_mul(ra[1], ra[1], SM9_MONT_BETA);
	sm9_z256_fp2_neg(ra[1], ra[1]);

	sm9_z256_fp2_conjugate(rb[0], xb[0]);
	sm9_z256_fp2_mul(rb[0], rb[0], SM9_MONT_BETA);
	sm9_z256_fp2_conjugate(rb[1], xb[1]);

	sm9_z256_fp2_conjugate(rc[0], xc[0]);
	sm9_z256_fp2_neg(rc[0], rc[0]);
	sm9_z256_fp2_conjugate(rc[1], xc[1]);
	sm9_z256_fp2_mul(rc[1], rc[1], SM9_MONT_BETA);

	sm9_z256_fp4_copy(r[0], ra);
	sm9_z256_fp4_copy(r[1], rb);
	sm9_z256_fp4_copy(r[2], rc);
}

void sm9_z256_fp12_frobenius6(sm9_z256_fp12 r, const sm9_z256_fp12 x)
{
	sm9_z256_fp4 a;
	sm9_z256_fp4 b;
	sm9_z256_fp4 c;

	sm9_z256_fp4_copy(a, x[0]);
	sm9_z256_fp4_copy(b, x[1]);
	sm9_z256_fp4_copy(c, x[2]);

	sm9_z256_fp4_conjugate(a, a);
	sm9_z256_fp4_conjugate(b, b);
	sm9_z256_fp4_neg(b, b);
	sm9_z256_fp4_conjugate(c, c);

	sm9_z256_fp4_copy(r[0], a);
	sm9_z256_fp4_copy(r[1], b);
	sm9_z256_fp4_copy(r[2], c);
}



void sm9_z256_point_from_hex(SM9_Z256_POINT *R, const char hex[65 * 2])
{
	sm9_z256_fp_from_hex(R->X, hex);
	sm9_z256_fp_from_hex(R->Y, hex + 65);
	sm9_z256_copy(R->Z, SM9_Z256_MODP_MONT_ONE);
}

int sm9_z256_point_is_at_infinity(const SM9_Z256_POINT *P)
{
	return sm9_z256_fp_is_zero(P->Z);
}

void sm9_z256_point_set_infinity(SM9_Z256_POINT *R)
{
	sm9_z256_copy(R->X, SM9_Z256_MODP_MONT_ONE);
	sm9_z256_copy(R->Y, SM9_Z256_MODP_MONT_ONE);
	sm9_z256_fp_set_zero(R->Z);
}

void sm9_z256_point_copy(SM9_Z256_POINT *R, const SM9_Z256_POINT *P)
{
	*R = *P;
}

// xy from this function are still mont
void sm9_z256_point_get_xy(const SM9_Z256_POINT *P, sm9_z256_t x, sm9_z256_t y)
{
	sm9_z256_t z_inv;

	assert(!sm9_z256_fp_is_zero(P->Z));

	if (sm9_z256_fp_equ(P->Z, SM9_Z256_MODP_MONT_ONE)) {
		sm9_z256_fp_copy(x, P->X);
		sm9_z256_fp_copy(y, P->Y);
	}

	sm9_z256_fp_inv(z_inv, P->Z);
	if (y)
		sm9_z256_fp_mul(y, P->Y, z_inv);
	sm9_z256_fp_sqr(z_inv, z_inv);
	sm9_z256_fp_mul(x, P->X, z_inv);
	if (y)
		sm9_z256_fp_mul(y, y, z_inv);
}

int sm9_z256_point_equ(const SM9_Z256_POINT *P, const SM9_Z256_POINT *Q)
{
	sm9_z256_t t1, t2, t3, t4;
	sm9_z256_fp_sqr(t1, P->Z);
	sm9_z256_fp_sqr(t2, Q->Z);
	sm9_z256_fp_mul(t3, P->X, t2);
	sm9_z256_fp_mul(t4, Q->X, t1);
	if (!sm9_z256_fp_equ(t3, t4)) {
		return 0;
	}
	sm9_z256_fp_mul(t1, t1, P->Z);
	sm9_z256_fp_mul(t2, t2, Q->Z);
	sm9_z256_fp_mul(t3, P->Y, t2);
	sm9_z256_fp_mul(t4, Q->Y, t1);
	return sm9_z256_fp_equ(t3, t4);
}

int sm9_z256_point_is_on_curve(const SM9_Z256_POINT *P)
{
	sm9_z256_t t0, t1, t2;
	if (sm9_z256_fp_equ(P->Z, SM9_Z256_MODP_MONT_ONE)) {
		sm9_z256_fp_sqr(t0, P->Y);
		sm9_z256_fp_sqr(t1, P->X);
		sm9_z256_fp_mul(t1, t1, P->X);
		sm9_z256_fp_add(t1, t1, SM9_Z256_MODP_MONT_FIVE);
	} else {
		sm9_z256_fp_sqr(t0, P->X);
		sm9_z256_fp_mul(t0, t0, P->X);
		sm9_z256_fp_sqr(t1, P->Z);
		sm9_z256_fp_sqr(t2, t1);
		sm9_z256_fp_mul(t1, t1, t2);
		sm9_z256_fp_mul(t1, t1, SM9_Z256_MODP_MONT_FIVE);
		sm9_z256_fp_add(t1, t0, t1);
		sm9_z256_fp_sqr(t0, P->Y);
	}
	if (sm9_z256_fp_equ(t0, t1) != 1) {
		error_print();
		return 0;
	}
	return 1;
}

// E(F_p): y^2 = x^3 + b， 计算公式和SM2不同
void sm9_z256_point_dbl(SM9_Z256_POINT *R, const SM9_Z256_POINT *P)
{
	const uint64_t *X1 = P->X;
	const uint64_t *Y1 = P->Y;
	const uint64_t *Z1 = P->Z;
	sm9_z256_t X3, Y3, Z3, T1, T2, T3;

	if (sm9_z256_point_is_at_infinity(P)) {
		sm9_z256_point_copy(R, P);
		return;
	}

	sm9_z256_fp_sqr(T2, X1);
	sm9_z256_fp_tri(T2, T2);
	sm9_z256_fp_dbl(Y3, Y1);
	sm9_z256_fp_mul(Z3, Y3, Z1);
	sm9_z256_fp_sqr(Y3, Y3);
	sm9_z256_fp_mul(T3, Y3, X1);
	sm9_z256_fp_sqr(Y3, Y3);
	sm9_z256_fp_div2(Y3, Y3);
	sm9_z256_fp_sqr(X3, T2);
	sm9_z256_fp_dbl(T1, T3);
	sm9_z256_fp_sub(X3, X3, T1);
	sm9_z256_fp_sub(T1, T3, X3);
	sm9_z256_fp_mul(T1, T1, T2);
	sm9_z256_fp_sub(Y3, T1, Y3);

	sm9_z256_fp_copy(R->X, X3);
	sm9_z256_fp_copy(R->Y, Y3);
	sm9_z256_fp_copy(R->Z, Z3);
}

void sm9_z256_point_add(SM9_Z256_POINT *R, const SM9_Z256_POINT *P, const SM9_Z256_POINT *Q)
{
	sm9_z256_t x;
	sm9_z256_t y;
	sm9_z256_point_get_xy(Q, x, y);

	const uint64_t *X1 = P->X;
	const uint64_t *Y1 = P->Y;
	const uint64_t *Z1 = P->Z;
	const uint64_t *x2 = x;
	const uint64_t *y2 = y;
	sm9_z256_t X3, Y3, Z3, T1, T2, T3, T4;

	if (sm9_z256_point_is_at_infinity(Q)) {
		sm9_z256_point_copy(R, P);
		return;
	}
	if (sm9_z256_point_is_at_infinity(P)) {
		sm9_z256_point_copy(R, Q);
		return;
	}

	sm9_z256_fp_sqr(T1, Z1);
	sm9_z256_fp_mul(T2, T1, Z1);
	sm9_z256_fp_mul(T1, T1, x2);
	sm9_z256_fp_mul(T2, T2, y2);
	sm9_z256_fp_sub(T1, T1, X1);
	sm9_z256_fp_sub(T2, T2, Y1);

	if (sm9_z256_fp_is_zero(T1)) {
		if (sm9_z256_fp_is_zero(T2)) {
			sm9_z256_point_dbl(R, Q);
			return;
		} else {
			sm9_z256_point_set_infinity(R);
			return;
		}
	}

	sm9_z256_fp_mul(Z3, Z1, T1);
	sm9_z256_fp_sqr(T3, T1);
	sm9_z256_fp_mul(T4, T3, T1);
	sm9_z256_fp_mul(T3, T3, X1);
	sm9_z256_fp_dbl(T1, T3);
	sm9_z256_fp_sqr(X3, T2);
	sm9_z256_fp_sub(X3, X3, T1);
	sm9_z256_fp_sub(X3, X3, T4);
	sm9_z256_fp_sub(T3, T3, X3);
	sm9_z256_fp_mul(T3, T3, T2);
	sm9_z256_fp_mul(T4, T4, Y1);
	sm9_z256_fp_sub(Y3, T3, T4);

	sm9_z256_fp_copy(R->X, X3);
	sm9_z256_fp_copy(R->Y, Y3);
	sm9_z256_fp_copy(R->Z, Z3);
}

void sm9_z256_point_neg(SM9_Z256_POINT *R, const SM9_Z256_POINT *P)
{
	sm9_z256_fp_copy(R->X, P->X);
	sm9_z256_fp_neg(R->Y, P->Y);
	sm9_z256_fp_copy(R->Z, P->Z);
}

void sm9_z256_point_sub(SM9_Z256_POINT *R, const SM9_Z256_POINT *P, const SM9_Z256_POINT *Q)
{
	SM9_Z256_POINT _T, *T = &_T;
	sm9_z256_point_neg(T, Q);
	sm9_z256_point_add(R, P, T);
}

void sm9_z256_point_dbl_x5(SM9_Z256_POINT *R, const SM9_Z256_POINT *A)

{
	sm9_z256_point_dbl(R, A);
	sm9_z256_point_dbl(R, R);
	sm9_z256_point_dbl(R, R);
	sm9_z256_point_dbl(R, R);
	sm9_z256_point_dbl(R, R);
}

void sm9_z256_point_mul(SM9_Z256_POINT *R, const sm9_z256_t k, const SM9_Z256_POINT *P)
{
	uint64_t window_size = 5;
	SM9_Z256_POINT T[16];
	int R_infinity = 1;
	int n = (256 + window_size - 1)/window_size;
	int i;

	// T[i] = (i + 1) * P
	sm9_z256_point_copy(&T[0], P);

	sm9_z256_point_dbl(&T[2-1], &T[1-1]);
	sm9_z256_point_dbl(&T[4-1], &T[2-1]);
	sm9_z256_point_dbl(&T[8-1], &T[4-1]);
	sm9_z256_point_dbl(&T[16-1], &T[8-1]);
	sm9_z256_point_add(&T[3-1], &T[2-1], P);
	sm9_z256_point_dbl(&T[6-1], &T[3-1]);
	sm9_z256_point_dbl(&T[12-1], &T[6-1]);
	sm9_z256_point_add(&T[5-1], &T[3-1], &T[2-1]);
	sm9_z256_point_dbl(&T[10-1], &T[5-1]);
	sm9_z256_point_add(&T[7-1], &T[4-1], &T[3-1]);
	sm9_z256_point_dbl(&T[14-1], &T[7-1]);
	sm9_z256_point_add(&T[9-1], &T[4-1], &T[5-1]);
	sm9_z256_point_add(&T[11-1], &T[6-1], &T[5-1]);
	sm9_z256_point_add(&T[13-1], &T[7-1], &T[6-1]);
	sm9_z256_point_add(&T[15-1], &T[8-1], &T[7-1]);


	for (i = n - 1; i >= 0; i--) {
		int booth = sm9_z256_get_booth(k, window_size, i);

		if (R_infinity) {
			if (booth != 0) {
				*R = T[booth - 1];
				R_infinity = 0;
			}
		} else {
			sm9_z256_point_dbl_x5(R, R);

			if (booth > 0) {
				sm9_z256_point_add(R, R, &T[booth - 1]);
			} else if (booth < 0) {
				sm9_z256_point_sub(R, R, &T[-booth - 1]);
			}
		}
	}

	if (R_infinity) {
		memset(R, 0, sizeof(*R));
	}
}

typedef struct {
	uint64_t X[4];
	uint64_t Y[4];
} SM9_Z256_POINT_AFFINE;

void sm9_z256_point_copy_affine(SM9_Z256_POINT *R, const SM9_Z256_POINT_AFFINE *P)
{
	sm9_z256_copy(R->X, P->X);
	sm9_z256_copy(R->Y, P->Y);
	sm9_z256_copy(R->Z, SM9_Z256_MODP_MONT_ONE);
}

void sm9_z256_point_add_affine(SM9_Z256_POINT *R, const SM9_Z256_POINT *P, const SM9_Z256_POINT_AFFINE *Q)
{
	SM9_Z256_POINT _S, *S = &_S;
	sm9_z256_point_copy_affine(S, Q);
	sm9_z256_point_add(R, P, S);
}

void sm9_z256_point_sub_affine(SM9_Z256_POINT *R, const SM9_Z256_POINT *P, const SM9_Z256_POINT_AFFINE *Q)
{
	SM9_Z256_POINT _S, *S = &_S;
	sm9_z256_point_copy_affine(S, Q);
	sm9_z256_point_sub(R, P, S);
}

extern const uint64_t sm9_z256_pre_comp[37][64 * 4 * 2];
static SM9_Z256_POINT_AFFINE (*g_pre_comp)[64] = (SM9_Z256_POINT_AFFINE (*)[64])sm9_z256_pre_comp;

void sm9_z256_point_mul_generator(SM9_Z256_POINT *R, const sm9_z256_t k)
{
	size_t window_size = 7;
	int R_infinity = 1;
	int n = (256 + window_size - 1) / window_size;
	int i;

	for (i = n - 1; i >= 0; i--) {
		int booth = sm9_z256_get_booth(k, window_size, i);

		if (R_infinity) {
			if (booth != 0) {
				sm9_z256_point_copy_affine(R, &g_pre_comp[i][booth - 1]);
				R_infinity = 0;
			}
		} else {
			if (booth > 0) {
				sm9_z256_point_add_affine(R, R, &g_pre_comp[i][booth - 1]);
			} else if (booth < 0) {
				sm9_z256_point_sub_affine(R, R, &g_pre_comp[i][-booth - 1]);
			}
		}
	}

	if (R_infinity) {
		sm9_z256_point_set_infinity(R);
	}
}

int sm9_z256_point_print(FILE *fp, int fmt, int ind, const char *label, const SM9_Z256_POINT *P)
{
	uint8_t buf[65];
	sm9_z256_point_to_uncompressed_octets(P, buf);
	format_bytes(fp, fmt, ind, label, buf, sizeof(buf));
	return 1;
}

int sm9_z256_twist_point_print(FILE *fp, int fmt, int ind, const char *label, const SM9_Z256_TWIST_POINT *P)
{
	uint8_t buf[129];
	sm9_z256_twist_point_to_uncompressed_octets(P, buf);
	format_bytes(fp, fmt, ind, label, buf, sizeof(buf));
	return 1;
}

void sm9_z256_twist_point_from_hex(SM9_Z256_TWIST_POINT *R, const char hex[65 * 4])
{
	sm9_z256_fp2_from_hex(R->X, hex);
	sm9_z256_fp2_from_hex(R->Y, hex + 65 * 2);
	sm9_z256_fp2_set_one(R->Z);
}

int sm9_z256_twist_point_is_at_infinity(const SM9_Z256_TWIST_POINT *P)
{
	return sm9_z256_fp2_is_zero(P->Z);
}

void sm9_z256_twist_point_set_infinity(SM9_Z256_TWIST_POINT *R)
{
	sm9_z256_fp2_set_one(R->X);
	sm9_z256_fp2_set_one(R->Y);
	sm9_z256_fp2_set_zero(R->Z);
}

void sm9_z256_twist_point_get_xy(const SM9_Z256_TWIST_POINT *P, sm9_z256_fp2 x, sm9_z256_fp2 y)
{
	sm9_z256_fp2 z_inv;

	assert(!sm9_z256_fp2_is_zero(P->Z));

	if (sm9_z256_fp2_is_one(P->Z)) {
		sm9_z256_fp2_copy(x, P->X);
		sm9_z256_fp2_copy(y, P->Y);
	}

	sm9_z256_fp2_inv(z_inv, P->Z);
	if (y)
		sm9_z256_fp2_mul(y, P->Y, z_inv);
	sm9_z256_fp2_sqr(z_inv, z_inv);
	sm9_z256_fp2_mul(x, P->X, z_inv);
	if (y)
		sm9_z256_fp2_mul(y, y, z_inv);
}

int sm9_z256_twist_point_equ(const SM9_Z256_TWIST_POINT *P, const SM9_Z256_TWIST_POINT *Q)
{
	sm9_z256_fp2 t1, t2, t3, t4;

	sm9_z256_fp2_sqr(t1, P->Z);
	sm9_z256_fp2_sqr(t2, Q->Z);
	sm9_z256_fp2_mul(t3, P->X, t2);
	sm9_z256_fp2_mul(t4, Q->X, t1);
	if (!sm9_z256_fp2_equ(t3, t4)) {
		return 0;
	}
	sm9_z256_fp2_mul(t1, t1, P->Z);
	sm9_z256_fp2_mul(t2, t2, Q->Z);
	sm9_z256_fp2_mul(t3, P->Y, t2);
	sm9_z256_fp2_mul(t4, Q->Y, t1);
	return sm9_z256_fp2_equ(t3, t4);
}

int sm9_z256_twist_point_is_on_curve(const SM9_Z256_TWIST_POINT *P)
{
	sm9_z256_fp2 t0, t1, t2;

	if (sm9_z256_fp2_is_one(P->Z)) {
		sm9_z256_fp2_sqr(t0, P->Y);
		sm9_z256_fp2_sqr(t1, P->X);
		sm9_z256_fp2_mul(t1, t1, P->X);
		sm9_z256_fp2_add(t1, t1, SM9_Z256_FP2_MONT_5U);

	} else {
		sm9_z256_fp2_sqr(t0, P->X);
		sm9_z256_fp2_mul(t0, t0, P->X);
		sm9_z256_fp2_sqr(t1, P->Z);
		sm9_z256_fp2_sqr(t2, t1);
		sm9_z256_fp2_mul(t1, t1, t2);
		sm9_z256_fp2_mul(t1, t1, SM9_Z256_FP2_MONT_5U);
		sm9_z256_fp2_add(t1, t0, t1);
		sm9_z256_fp2_sqr(t0, P->Y);
	}

	return sm9_z256_fp2_equ(t0, t1);
}

void sm9_z256_twist_point_neg(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P)
{
	sm9_z256_fp2_copy(R->X, P->X);
	sm9_z256_fp2_neg(R->Y, P->Y);
	sm9_z256_fp2_copy(R->Z, P->Z);
}

// E(Fp^2)的计算也比较重要，但是fp2上的计算并不容易做2路并发

void sm9_z256_twist_point_dbl(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P)
{
	const sm9_z256_t *X1 = P->X;
	const sm9_z256_t *Y1 = P->Y;
	const sm9_z256_t *Z1 = P->Z;
	sm9_z256_fp2 X3, Y3, Z3, T1, T2, T3;

	if (sm9_z256_twist_point_is_at_infinity(P)) {
		sm9_z256_twist_point_copy(R, P);
		return;
	}
	sm9_z256_fp2_sqr(T2, X1);
	sm9_z256_fp2_tri(T2, T2);
	sm9_z256_fp2_dbl(Y3, Y1);
	sm9_z256_fp2_mul(Z3, Y3, Z1);
	sm9_z256_fp2_sqr(Y3, Y3);
	sm9_z256_fp2_mul(T3, Y3, X1);
	sm9_z256_fp2_sqr(Y3, Y3);
	sm9_z256_fp2_div2(Y3, Y3);
	sm9_z256_fp2_sqr(X3, T2);
	sm9_z256_fp2_dbl(T1, T3);
	sm9_z256_fp2_sub(X3, X3, T1);
	sm9_z256_fp2_sub(T1, T3, X3);
	sm9_z256_fp2_mul(T1, T1, T2);
	sm9_z256_fp2_sub(Y3, T1, Y3);

	sm9_z256_fp2_copy(R->X, X3);
	sm9_z256_fp2_copy(R->Y, Y3);
	sm9_z256_fp2_copy(R->Z, Z3);
}

void sm9_z256_twist_point_add(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P, const SM9_Z256_TWIST_POINT *Q)
{
	const sm9_z256_t *X1 = P->X;
	const sm9_z256_t *Y1 = P->Y;
	const sm9_z256_t *Z1 = P->Z;
	const sm9_z256_t *x2 = Q->X;
	const sm9_z256_t *y2 = Q->Y;
	sm9_z256_fp2 X3, Y3, Z3, T1, T2, T3, T4;

	if (sm9_z256_twist_point_is_at_infinity(Q)) {
		sm9_z256_twist_point_copy(R, P);
		return;
	}
	if (sm9_z256_twist_point_is_at_infinity(P)) {
		sm9_z256_twist_point_copy(R, Q);
		return;
	}

	sm9_z256_fp2_sqr(T1, Z1);
	sm9_z256_fp2_mul(T2, T1, Z1);
	sm9_z256_fp2_mul(T1, T1, x2);
	sm9_z256_fp2_mul(T2, T2, y2);
	sm9_z256_fp2_sub(T1, T1, X1);
	sm9_z256_fp2_sub(T2, T2, Y1);
	if (sm9_z256_fp2_is_zero(T1)) {
		if (sm9_z256_fp2_is_zero(T2)) {
			sm9_z256_twist_point_dbl(R, Q);
			return;
		} else {
			sm9_z256_twist_point_set_infinity(R);
			return;
		}
	}
	sm9_z256_fp2_mul(Z3, Z1, T1);
	sm9_z256_fp2_sqr(T3, T1);
	sm9_z256_fp2_mul(T4, T3, T1);
	sm9_z256_fp2_mul(T3, T3, X1);
	sm9_z256_fp2_dbl(T1, T3);
	sm9_z256_fp2_sqr(X3, T2);
	sm9_z256_fp2_sub(X3, X3, T1);
	sm9_z256_fp2_sub(X3, X3, T4);
	sm9_z256_fp2_sub(T3, T3, X3);
	sm9_z256_fp2_mul(T3, T3, T2);
	sm9_z256_fp2_mul(T4, T4, Y1);
	sm9_z256_fp2_sub(Y3, T3, T4);

	sm9_z256_fp2_copy(R->X, X3);
	sm9_z256_fp2_copy(R->Y, Y3);
	sm9_z256_fp2_copy(R->Z, Z3);
}

void sm9_z256_twist_point_sub(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P, const SM9_Z256_TWIST_POINT *Q)
{
	SM9_Z256_TWIST_POINT _T, *T = &_T;
	sm9_z256_twist_point_neg(T, Q);
	sm9_z256_twist_point_add_full(R, P, T);
}

void sm9_z256_twist_point_add_full(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P, const SM9_Z256_TWIST_POINT *Q)
{
	const sm9_z256_t *X1 = P->X;
	const sm9_z256_t *Y1 = P->Y;
	const sm9_z256_t *Z1 = P->Z;
	const sm9_z256_t *X2 = Q->X;
	const sm9_z256_t *Y2 = Q->Y;
	const sm9_z256_t *Z2 = Q->Z;
	sm9_z256_fp2 T1, T2, T3, T4, T5, T6, T7, T8;

	if (sm9_z256_twist_point_is_at_infinity(Q)) {
		sm9_z256_twist_point_copy(R, P);
		return;
	}
	if (sm9_z256_twist_point_is_at_infinity(P)) {
		sm9_z256_twist_point_copy(R, Q);
		return;
	}

	sm9_z256_fp2_sqr(T1, Z1);
	sm9_z256_fp2_sqr(T2, Z2);
	sm9_z256_fp2_mul(T3, X2, T1);
	sm9_z256_fp2_mul(T4, X1, T2);
	sm9_z256_fp2_add(T5, T3, T4);
	sm9_z256_fp2_sub(T3, T3, T4);
	sm9_z256_fp2_mul(T1, T1, Z1);
	sm9_z256_fp2_mul(T1, T1, Y2);
	sm9_z256_fp2_mul(T2, T2, Z2);
	sm9_z256_fp2_mul(T2, T2, Y1);
	sm9_z256_fp2_add(T6, T1, T2);
	sm9_z256_fp2_sub(T1, T1, T2);

	if (sm9_z256_fp2_is_zero(T1) && sm9_z256_fp2_is_zero(T3)) {
		sm9_z256_twist_point_dbl(R, P);
		return;
	}
	if (sm9_z256_fp2_is_zero(T1) && sm9_z256_fp2_is_zero(T6)) {
		sm9_z256_twist_point_set_infinity(R);
		return;
	}

	sm9_z256_fp2_sqr(T6, T1);
	sm9_z256_fp2_mul(T7, T3, Z1);
	sm9_z256_fp2_mul(T7, T7, Z2);
	sm9_z256_fp2_sqr(T8, T3);
	sm9_z256_fp2_mul(T5, T5, T8);
	sm9_z256_fp2_mul(T3, T3, T8);
	sm9_z256_fp2_mul(T4, T4, T8);
	sm9_z256_fp2_sub(T6, T6, T5);
	sm9_z256_fp2_sub(T4, T4, T6);
	sm9_z256_fp2_mul(T1, T1, T4);
	sm9_z256_fp2_mul(T2, T2, T3);
	sm9_z256_fp2_sub(T1, T1, T2);

	sm9_z256_fp2_copy(R->X, T6);
	sm9_z256_fp2_copy(R->Y, T1);
	sm9_z256_fp2_copy(R->Z, T7);
}

void sm9_z256_twist_point_mul(SM9_Z256_TWIST_POINT *R, const sm9_z256_t k, const SM9_Z256_TWIST_POINT *P)
{
	SM9_Z256_TWIST_POINT _Q, *Q = &_Q;
	char kbits[256];
	int i;

	sm9_z256_to_bits(k, kbits);
	sm9_z256_twist_point_set_infinity(Q);
	for (i = 0; i < 256; i++) {
		sm9_z256_twist_point_dbl(Q, Q);
		if (kbits[i] == '1') {
			sm9_z256_twist_point_add_full(Q, Q, P);
		}
	}
	sm9_z256_twist_point_copy(R, Q);
}

void sm9_z256_twist_point_mul_generator(SM9_Z256_TWIST_POINT *R, const sm9_z256_t k)
{
	sm9_z256_twist_point_mul(R, k, SM9_Z256_MONT_P2);
}

void sm9_z256_eval_g_tangent(sm9_z256_fp12 num, sm9_z256_fp12 den, const SM9_Z256_TWIST_POINT *P, const SM9_Z256_POINT *Q)
{
	sm9_z256_t x;
	sm9_z256_t y;
	sm9_z256_point_get_xy(Q, x, y);

	const sm9_z256_t *XP = P->X;
	const sm9_z256_t *YP = P->Y;
	const sm9_z256_t *ZP = P->Z;
	const uint64_t *xQ = x;
	const uint64_t *yQ = y;

	sm9_z256_t *a0 = num[0][0];
	sm9_z256_t *a1 = num[0][1];
	sm9_z256_t *a4 = num[2][0];
	sm9_z256_t *b1 = den[0][1];

	sm9_z256_fp2 t0;
	sm9_z256_fp2 t1;
	sm9_z256_fp2 t2;

	sm9_z256_fp12_set_zero(num);
	sm9_z256_fp12_set_zero(den);

	sm9_z256_fp2_sqr(t0, ZP);
	sm9_z256_fp2_mul(t1, t0, ZP);
	sm9_z256_fp2_mul(b1, t1, YP);

	sm9_z256_fp2_mul_fp(t2, b1, yQ);
	sm9_z256_fp2_neg(a1, t2);

	sm9_z256_fp2_sqr(t1, XP);
	sm9_z256_fp2_mul(t0, t0, t1);
	sm9_z256_fp2_mul_fp(t0, t0, xQ);
	sm9_z256_fp2_tri(t0, t0);
	sm9_z256_fp2_div2(a4, t0);

	sm9_z256_fp2_mul(t1, t1, XP);
	sm9_z256_fp2_tri(t1, t1);
	sm9_z256_fp2_div2(t1, t1);
	sm9_z256_fp2_sqr(t0, YP);
	sm9_z256_fp2_sub(a0, t0, t1);
}

void sm9_z256_eval_g_line(sm9_z256_fp12 num, sm9_z256_fp12 den, const SM9_Z256_TWIST_POINT *T, const SM9_Z256_TWIST_POINT *P, const SM9_Z256_POINT *Q)
{
	sm9_z256_t x;
	sm9_z256_t y;
	sm9_z256_point_get_xy(Q, x, y);

	const sm9_z256_t *XT = T->X;
	const sm9_z256_t *YT = T->Y;
	const sm9_z256_t *ZT = T->Z;
	const sm9_z256_t *XP = P->X;
	const sm9_z256_t *YP = P->Y;
	const sm9_z256_t *ZP = P->Z;
	const uint64_t *xQ = x;
	const uint64_t *yQ = y;

	sm9_z256_t *a0 = num[0][0];
	sm9_z256_t *a1 = num[0][1];
	sm9_z256_t *a4 = num[2][0];
	sm9_z256_t *b1 = den[0][1];

	sm9_z256_fp2 T0, T1, T2, T3, T4;


	sm9_z256_fp12_set_zero(num);
	sm9_z256_fp12_set_zero(den);

	sm9_z256_fp2_sqr(T0, ZP);
	sm9_z256_fp2_mul(T1, T0, XT);
	sm9_z256_fp2_mul(T0, T0, ZP);
	sm9_z256_fp2_sqr(T2, ZT);
	sm9_z256_fp2_mul(T3, T2, XP);
	sm9_z256_fp2_mul(T2, T2, ZT);
	sm9_z256_fp2_mul(T2, T2, YP);
	sm9_z256_fp2_sub(T1, T1, T3);
	sm9_z256_fp2_mul(T1, T1, ZT);
	sm9_z256_fp2_mul(T1, T1, ZP);
	sm9_z256_fp2_mul(T4, T1, T0);
	sm9_z256_fp2_copy(b1, T4);
	sm9_z256_fp2_mul(T1, T1, YP);
	sm9_z256_fp2_mul(T3, T0, YT);
	sm9_z256_fp2_sub(T3, T3, T2);
	sm9_z256_fp2_mul(T0, T0, T3);
	sm9_z256_fp2_mul_fp(T0, T0, xQ);
	sm9_z256_fp2_copy(a4, T0);
	sm9_z256_fp2_mul(T3, T3, XP);
	sm9_z256_fp2_mul(T3, T3, ZP);
	sm9_z256_fp2_sub(T1, T1, T3);
	sm9_z256_fp2_copy(a0, T1);
	sm9_z256_fp2_mul_fp(T2, T4, yQ);
	sm9_z256_fp2_neg(T2, T2);
	sm9_z256_fp2_copy(a1, T2);
}

void sm9_z256_twist_point_pi1(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P)
{
	// c = 0x3f23ea58e5720bdb843c6cfa9c08674947c5c86e0ddd04eda91d8354377b698b
	// mont version
	const sm9_z256_t c = {0x1a98dfbd4575299f, 0x9ec8547b245c54fd, 0xf51f5eac13df846c, 0x9ef74015d5a16393};
	sm9_z256_fp2_conjugate(R->X, P->X);
	sm9_z256_fp2_conjugate(R->Y, P->Y);
	sm9_z256_fp2_conjugate(R->Z, P->Z);
	sm9_z256_fp2_mul_fp(R->Z, R->Z, c);

}

void sm9_z256_twist_point_pi2(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P)
{
	// c = 0xf300000002a3a6f2780272354f8b78f4d5fc11967be65334
	// mont version
	const sm9_z256_t c = {0xb626197dce4736ca, 0x8296b3557ed0186, 0x9c705db2fd91512a, 0x1c753e748601c992};
	sm9_z256_fp2_copy(R->X, P->X);
	sm9_z256_fp2_copy(R->Y, P->Y);
	sm9_z256_fp2_mul_fp(R->Z, P->Z, c);
}

void sm9_z256_twist_point_neg_pi2(SM9_Z256_TWIST_POINT *R, const SM9_Z256_TWIST_POINT *P)
{
	// c = 0xf300000002a3a6f2780272354f8b78f4d5fc11967be65334
	// mont version
	const sm9_z256_t c = {0xb626197dce4736ca, 0x8296b3557ed0186, 0x9c705db2fd91512a, 0x1c753e748601c992};
	sm9_z256_fp2_copy(R->X, P->X);
	sm9_z256_fp2_neg(R->Y, P->Y);
	sm9_z256_fp2_mul_fp(R->Z, P->Z, c);
}


void sm9_z256_final_exponent_hard_part(sm9_z256_fp12 r, const sm9_z256_fp12 f)
{
	// a2 = 0xd8000000019062ed0000b98b0cb27659
	// a3 = 0x2400000000215d941
	const sm9_z256_t a2 = {0x0000b98b0cb27659, 0xd8000000019062ed, 0, 0};
	const sm9_z256_t a3 = {0x400000000215d941, 0x2, 0, 0};
	const sm9_z256_t nine = {9,0,0,0};
	sm9_z256_fp12 t0, t1, t2, t3;

	sm9_z256_fp12_pow(t0, f, a3);
	sm9_z256_fp12_inv(t0, t0);
	sm9_z256_fp12_frobenius(t1, t0);
	sm9_z256_fp12_mul(t1, t0, t1);

	sm9_z256_fp12_mul(t0, t0, t1);
	sm9_z256_fp12_frobenius(t2, f);
	sm9_z256_fp12_mul(t3, t2, f);
	sm9_z256_fp12_pow(t3, t3, nine);

	sm9_z256_fp12_mul(t0, t0, t3);
	sm9_z256_fp12_sqr(t3, f);
	sm9_z256_fp12_sqr(t3, t3);
	sm9_z256_fp12_mul(t0, t0, t3);
	sm9_z256_fp12_sqr(t2, t2);
	sm9_z256_fp12_mul(t2, t2, t1);
	sm9_z256_fp12_frobenius2(t1, f);
	sm9_z256_fp12_mul(t1, t1, t2);

	sm9_z256_fp12_pow(t2, t1, a2);
	sm9_z256_fp12_mul(t0, t2, t0);
	sm9_z256_fp12_frobenius3(t1, f);
	sm9_z256_fp12_mul(t1, t1, t0);

	sm9_z256_fp12_copy(r, t1);
}

void sm9_z256_final_exponent(sm9_z256_fp12 r, const sm9_z256_fp12 f)
{
	sm9_z256_fp12 t0;
	sm9_z256_fp12 t1;

	sm9_z256_fp12_frobenius6(t0, f);
	sm9_z256_fp12_inv(t1, f);
	sm9_z256_fp12_mul(t0, t0, t1);
	sm9_z256_fp12_frobenius2(t1, t0);
	sm9_z256_fp12_mul(t0, t0, t1);
	sm9_z256_final_exponent_hard_part(t0, t0);

	sm9_z256_fp12_copy(r, t0);
}

// 这个计算是否有更快速的算法
// 特别是主循环中的计算时否需要再Fp12上面
void sm9_z256_pairing(sm9_z256_fp12 r, const SM9_Z256_TWIST_POINT *Q, const SM9_Z256_POINT *P)
{
	const char *abits = "00100000000000000000000000000000000000010000101100020200101000020";

	SM9_Z256_TWIST_POINT _T, *T = &_T;
	SM9_Z256_TWIST_POINT _Q1, *Q1 = &_Q1;
	SM9_Z256_TWIST_POINT _Q2, *Q2 = &_Q2;

	sm9_z256_fp12 f_num;
	sm9_z256_fp12 f_den;
	sm9_z256_fp12 g_num;
	sm9_z256_fp12 g_den;
	int i;

	sm9_z256_twist_point_copy(T, Q);

	sm9_z256_fp12_set_one(f_num);
	sm9_z256_fp12_set_one(f_den);

	for (i = 0; i < strlen(abits); i++) {
		sm9_z256_fp12_sqr(f_num, f_num);
		sm9_z256_fp12_sqr(f_den, f_den);
		sm9_z256_eval_g_tangent(g_num, g_den, T, P);
		sm9_z256_fp12_mul(f_num, f_num, g_num);
		sm9_z256_fp12_mul(f_den, f_den, g_den);

		sm9_z256_twist_point_dbl(T, T);

		if (abits[i] == '1') {
			sm9_z256_eval_g_line(g_num, g_den, T, Q, P);
			sm9_z256_fp12_mul(f_num, f_num, g_num);
			sm9_z256_fp12_mul(f_den, f_den, g_den);
			sm9_z256_twist_point_add_full(T, T, Q);
		} else if (abits[i] == '2') {
			sm9_z256_twist_point_neg(Q1, Q);
			sm9_z256_eval_g_line(g_num, g_den, T, Q1, P);
			sm9_z256_fp12_mul(f_num, f_num, g_num);
			sm9_z256_fp12_mul(f_den, f_den, g_den);
			sm9_z256_twist_point_add_full(T, T, Q1);
		}
	}

	sm9_z256_twist_point_pi1(Q1, Q);
	sm9_z256_twist_point_neg_pi2(Q2, Q);

	sm9_z256_eval_g_line(g_num, g_den, T, Q1, P);
	sm9_z256_fp12_mul(f_num, f_num, g_num);
	sm9_z256_fp12_mul(f_den, f_den, g_den);
	sm9_z256_twist_point_add_full(T, T, Q1);

	sm9_z256_eval_g_line(g_num, g_den, T, Q2, P);
	sm9_z256_fp12_mul(f_num, f_num, g_num);
	sm9_z256_fp12_mul(f_den, f_den, g_den);
	sm9_z256_twist_point_add_full(T, T, Q2);

	sm9_z256_fp12_inv(f_den, f_den);
	sm9_z256_fp12_mul(r, f_num, f_den);

	sm9_z256_final_exponent(r, r);
}

// Mont was not used for mod N
void sm9_z256_fn_add(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t c;
	c = sm9_z256_add(r, a, b);

	if (c) {
		// a + b - n = (a + b - 2^256) + (2^256 - n)
		(void)sm9_z256_add(r, r, SM9_Z256_NEG_N);
		return;
	}
	if (sm9_z256_cmp(r, SM9_Z256_N) >= 0) {
		(void)sm9_z256_sub(r, r, SM9_Z256_N);
	}
}

void sm9_z256_fn_sub(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	uint64_t c;
	c = sm9_z256_sub(r, a, b);

	if (c) {
		// a - b + n = (a - b + 2^256) - (2^256 - n)
		(void)sm9_z256_sub(r, r, SM9_Z256_NEG_N);
	}
}

void sm9_z320_mul(uint64_t r[10], const uint64_t a[5], const uint64_t b[5])
{
	uint64_t a_[10];
	uint64_t b_[10];
	uint64_t s[20] = {0};
	uint64_t u;
	int i, j;

	for (i = 0; i < 5; i++) {
		a_[2 * i] = a[i] & 0xffffffff;
		b_[2 * i] = b[i] & 0xffffffff;
		a_[2 * i + 1] = a[i] >> 32;
		b_[2 * i + 1] = b[i] >> 32;
	}

	for (i = 0; i < 10; i++) {
		u = 0;
		for (j = 0; j < 10; j++) {
			u = s[i + j] + a_[i] * b_[j] + u;
			s[i + j] = u & 0xffffffff;
			u >>= 32;
		}
		s[i + 10] = u;
	}

	for (i = 0; i < 10; i++) {
		r[i] = (s[2 * i + 1] << 32) | s[2 * i];
	}
}

const uint64_t SM9_Z256_N_BARRETT_MU[5] = {0x74df4fd4dfc97c2f,
		0x9c95d85ec9c073b0, 0x55f73aebdcd1312c, 0x67980e0beb5759a6, 0x1};

void sm9_z256_fn_mul(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t b)
{
	sm9_z256_t x, y;
	uint64_t z[8], h[10], s[8];
	uint64_t t, c = 0;

	sm9_z256_mul(z, a, b);

	// (z // 2^192) = z[3-7]
	sm9_z320_mul(h, z + 3, SM9_Z256_N_BARRETT_MU);

	// (h // 2^320) = h[5-9]
	sm9_z256_mul(s, h + 5, SM9_Z256_N);

	// h[5-9] * N % 2^320 = (h[5-8]*N + 2^256 * (N[0]*h[9])%2^64) % 2^320
	s[4] += SM9_Z256_N[0] * h[9];

	// s[0-4] = z[0-4] - s[0-4] (% 2^320)
	t = z[0] - s[0];
	c = t > z[0];
	r[0] = t;

	t = z[1] - c;
	c = t > z[1];
	r[1] = t - s[1];
	c += r[1] > t;

	t = z[2] - c;
	c = t > z[2];
	r[2] = t - s[2];
	c += r[2] > t;

	t = z[3] - c;
	c = t > z[3];
	r[3] = t - s[3];
	c += r[3] > t;

	t = z[4] - c;
	s[4] = t - s[4]; // we put r[4] in s[4]

	if (s[4] > 0 || sm9_z256_cmp(r, SM9_Z256_N) >= 0) { // r >= N
		sm9_z256_sub(r, r, SM9_Z256_N);
	}
}

void sm9_z256_fn_pow(sm9_z256_t r, const sm9_z256_t a, const sm9_z256_t e)
{
	sm9_z256_t t;
	uint64_t w;
	int i, j;

	sm9_z256_copy(t, SM9_Z256_ONE);

	for (i = 3; i >= 0; i--) {
		w = e[i];
		for (j = 0; j < 64; j++) {
			sm9_z256_fn_mul(t, t, t);
			if (w & 0x8000000000000000) {
				sm9_z256_fn_mul(t, t, a);
			}
			w <<= 1;
		}
	}
	sm9_z256_copy(r, t);
}

void sm9_z256_fn_inv(sm9_z256_t r, const sm9_z256_t a)
{
	sm9_z256_t e;
	sm9_z256_sub(e, SM9_Z256_N, SM9_Z256_TWO);
	sm9_z256_fn_pow(r, a, e);
}

int sm9_z256_fn_from_bytes(sm9_z256_t a, const uint8_t in[32])
{
	sm9_z256_from_bytes(a, in);
	return 1;
}

const sm9_z256_t SM9_Z256_N_MINUS_ONE_BARRETT_MU = {0x74df4fd4dfc97c31,
		0x9c95d85ec9c073b0, 0x55f73aebdcd1312c, 0x67980e0beb5759a6}; // , 0x1};

void sm9_z256_fn_from_hash(sm9_z256_t h, const uint8_t Ha[40])
{
	int i;
	uint64_t z[8] = {0};
	uint64_t r[9] = {0};
	uint64_t c = 0, t = 0;

	for (i = 0; i < 5; i++) {
		z[4-i] = GETU64(Ha + (8*i));
	}

	// (z // 2^192) = z[3], z[4]
	sm9_z256_mul(r, z + 3, SM9_Z256_N_MINUS_ONE_BARRETT_MU); // most to r[5]
	// (r[4], r[5]) += (z[3], z[4])
	r[4] += z[3];
	c = r[4] < z[3];
	t = z[4] + c;
	c = t < z[4];
	r[5] += t;
	c += r[5] < t;
	r[6] = c;

	// (r // 2^320) = (r[5], r[6])
	sm9_z256_mul(r, r + 5, SM9_Z256_N_MINUS_ONE);
	sm9_z256_sub(h, z, r);

	sm9_z256_fn_add(h, h, SM9_Z256_ONE);
}

int sm9_z256_point_to_uncompressed_octets(const SM9_Z256_POINT *P, uint8_t octets[65])
{
	sm9_z256_t x;
	sm9_z256_t y;
	sm9_z256_point_get_xy(P, x, y);
	octets[0] = 0x04;
	sm9_z256_fp_to_bytes(x, octets + 1); // fp_to_bytes include from_mont
	sm9_z256_fp_to_bytes(y, octets + 32 + 1);
	return 1;
}

int sm9_z256_point_from_uncompressed_octets(SM9_Z256_POINT *P, const uint8_t octets[65])
{
	if (octets[0] != 0x04) {
		error_print();
		return -1;
	}
	memset(P, 0, sizeof(*P));
	sm9_z256_fp_from_bytes(P->X, octets + 1); // fp_from_bytes include to_mont
	sm9_z256_fp_from_bytes(P->Y, octets + 32 + 1);
	sm9_z256_fp_copy(P->Z, SM9_Z256_MODP_MONT_ONE);
	if (!sm9_z256_point_is_on_curve(P)) {
		error_print();
		return -1;
	}
	return 1;
}

int sm9_z256_twist_point_to_uncompressed_octets(const SM9_Z256_TWIST_POINT *P, uint8_t octets[129])
{
	octets[0] = 0x04;
	sm9_z256_fp2 x;
	sm9_z256_fp2 y;
	sm9_z256_twist_point_get_xy(P, x, y);
	sm9_z256_fp2_to_bytes(x, octets + 1);
	sm9_z256_fp2_to_bytes(y, octets + 32 * 2 + 1);
	return 1;
}

int sm9_z256_twist_point_from_uncompressed_octets(SM9_Z256_TWIST_POINT *P, const uint8_t octets[129])
{
	assert(octets[0] == 0x04);
	sm9_z256_fp2_from_bytes(P->X, octets + 1);
	sm9_z256_fp2_from_bytes(P->Y, octets + 32 * 2 + 1);
	sm9_z256_fp2_set_one(P->Z);
	if (!sm9_z256_twist_point_is_on_curve(P)) return -1;
	return 1;
}
