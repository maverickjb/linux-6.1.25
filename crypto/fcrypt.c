/* FCrypt encryption algorithm
 *
 * Copyright (C) 2006 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Based on code:
 *
 * Copyright (c) 1995 - 2000 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <asm/byteorder.h>
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/crypto.h>

#define ROUNDS 16

struct fcrypt_ctx {
	__be32 sched[ROUNDS];
};

/* Rotate right two 32 bit numbers as a 56 bit number */
#define ror56(hi, lo, n)					\
do {								\
	u32 t = lo & ((1 << n) - 1);				\
	lo = (lo >> n) | ((hi & ((1 << n) - 1)) << (32 - n));	\
	hi = (hi >> n) | (t << (24-n));				\
} while (0)

/* Rotate right one 64 bit number as a 56 bit number */
#define ror56_64(k, n) (k = (k >> n) | ((k & ((1 << n) - 1)) << (56 - n)))

/*
 * Sboxes for Feistel network derived from
 * /afs/transarc.com/public/afsps/afs.rel31b.export-src/rxkad/sboxes.h
 */
#undef Z
#define Z(x) cpu_to_be32(x << 3)
static const __be32 sbox0[256] = {
	Z(0xea), Z(0x7f), Z(0xb2), Z(0x64), Z(0x9d), Z(0xb0), Z(0xd9), Z(0x11),
	Z(0xcd), Z(0x86), Z(0x86), Z(0x91), Z(0x0a), Z(0xb2), Z(0x93), Z(0x06),
	Z(0x0e), Z(0x06), Z(0xd2), Z(0x65), Z(0x73), Z(0xc5), Z(0x28), Z(0x60),
	Z(0xf2), Z(0x20), Z(0xb5), Z(0x38), Z(0x7e), Z(0xda), Z(0x9f), Z(0xe3),
	Z(0xd2), Z(0xcf), Z(0xc4), Z(0x3c), Z(0x61), Z(0xff), Z(0x4a), Z(0x4a),
	Z(0x35), Z(0xac), Z(0xaa), Z(0x5f), Z(0x2b), Z(0xbb), Z(0xbc), Z(0x53),
	Z(0x4e), Z(0x9d), Z(0x78), Z(0xa3), Z(0xdc), Z(0x09), Z(0x32), Z(0x10),
	Z(0xc6), Z(0x6f), Z(0x66), Z(0xd6), Z(0xab), Z(0xa9), Z(0xaf), Z(0xfd),
	Z(0x3b), Z(0x95), Z(0xe8), Z(0x34), Z(0x9a), Z(0x81), Z(0x72), Z(0x80),
	Z(0x9c), Z(0xf3), Z(0xec), Z(0xda), Z(0x9f), Z(0x26), Z(0x76), Z(0x15),
	Z(0x3e), Z(0x55), Z(0x4d), Z(0xde), Z(0x84), Z(0xee), Z(0xad), Z(0xc7),
	Z(0xf1), Z(0x6b), Z(0x3d), Z(0xd3), Z(0x04), Z(0x49), Z(0xaa), Z(0x24),
	Z(0x0b), Z(0x8a), Z(0x83), Z(0xba), Z(0xfa), Z(0x85), Z(0xa0), Z(0xa8),
	Z(0xb1), Z(0xd4), Z(0x01), Z(0xd8), Z(0x70), Z(0x64), Z(0xf0), Z(0x51),
	Z(0xd2), Z(0xc3), Z(0xa7), Z(0x75), Z(0x8c), Z(0xa5), Z(0x64), Z(0xef),
	Z(0x10), Z(0x4e), Z(0xb7), Z(0xc6), Z(0x61), Z(0x03), Z(0xeb), Z(0x44),
	Z(0x3d), Z(0xe5), Z(0xb3), Z(0x5b), Z(0xae), Z(0xd5), Z(0xad), Z(0x1d),
	Z(0xfa), Z(0x5a), Z(0x1e), Z(0x33), Z(0xab), Z(0x93), Z(0xa2), Z(0xb7),
	Z(0xe7), Z(0xa8), Z(0x45), Z(0xa4), Z(0xcd), Z(0x29), Z(0x63), Z(0x44),
	Z(0xb6), Z(0x69), Z(0x7e), Z(0x2e), Z(0x62), Z(0x03), Z(0xc8), Z(0xe0),
	Z(0x17), Z(0xbb), Z(0xc7), Z(0xf3), Z(0x3f), Z(0x36), Z(0xba), Z(0x71),
	Z(0x8e), Z(0x97), Z(0x65), Z(0x60), Z(0x69), Z(0xb6), Z(0xf6), Z(0xe6),
	Z(0x6e), Z(0xe0), Z(0x81), Z(0x59), Z(0xe8), Z(0xaf), Z(0xdd), Z(0x95),
	Z(0x22), Z(0x99), Z(0xfd), Z(0x63), Z(0x19), Z(0x74), Z(0x61), Z(0xb1),
	Z(0xb6), Z(0x5b), Z(0xae), Z(0x54), Z(0xb3), Z(0x70), Z(0xff), Z(0xc6),
	Z(0x3b), Z(0x3e), Z(0xc1), Z(0xd7), Z(0xe1), Z(0x0e), Z(0x76), Z(0xe5),
	Z(0x36), Z(0x4f), Z(0x59), Z(0xc7), Z(0x08), Z(0x6e), Z(0x82), Z(0xa6),
	Z(0x93), Z(0xc4), Z(0xaa), Z(0x26), Z(0x49), Z(0xe0), Z(0x21), Z(0x64),
	Z(0x07), Z(0x9f), Z(0x64), Z(0x81), Z(0x9c), Z(0xbf), Z(0xf9), Z(0xd1),
	Z(0x43), Z(0xf8), Z(0xb6), Z(0xb9), Z(0xf1), Z(0x24), Z(0x75), Z(0x03),
	Z(0xe4), Z(0xb0), Z(0x99), Z(0x46), Z(0x3d), Z(0xf5), Z(0xd1), Z(0x39),
	Z(0x72), Z(0x12), Z(0xf6), Z(0xba), Z(0x0c), Z(0x0d), Z(0x42), Z(0x2e)
};

#undef Z
#define Z(x) cpu_to_be32(((x & 0x1f) << 27) | (x >> 5))
static const __be32 sbox1[256] = {
	Z(0x77), Z(0x14), Z(0xa6), Z(0xfe), Z(0xb2), Z(0x5e), Z(0x8c), Z(0x3e),
	Z(0x67), Z(0x6c), Z(0xa1), Z(0x0d), Z(0xc2), Z(0xa2), Z(0xc1), Z(0x85),
	Z(0x6c), Z(0x7b), Z(0x67), Z(0xc6), Z(0x23), Z(0xe3), Z(0xf2), Z(0x89),
	Z(0x50), Z(0x9c), Z(0x03), Z(0xb7), Z(0x73), Z(0xe6), Z(0xe1), Z(0x39),
	Z(0x31), Z(0x2c), Z(0x27), Z(0x9f), Z(0xa5), Z(0x69), Z(0x44), Z(0xd6),
	Z(0x23), Z(0x83), Z(0x98), Z(0x7d), Z(0x3c), Z(0xb4), Z(0x2d), Z(0x99),
	Z(0x1c), Z(0x1f), Z(0x8c), Z(0x20), Z(0x03), Z(0x7c), Z(0x5f), Z(0xad),
	Z(0xf4), Z(0xfa), Z(0x95), Z(0xca), Z(0x76), Z(0x44), Z(0xcd), Z(0xb6),
	Z(0xb8), Z(0xa1), Z(0xa1), Z(0xbe), Z(0x9e), Z(0x54), Z(0x8f), Z(0x0b),
	Z(0x16), Z(0x74), Z(0x31), Z(0x8a), Z(0x23), Z(0x17), Z(0x04), Z(0xfa),
	Z(0x79), Z(0x84), Z(0xb1), Z(0xf5), Z(0x13), Z(0xab), Z(0xb5), Z(0x2e),
	Z(0xaa), Z(0x0c), Z(0x60), Z(0x6b), Z(0x5b), Z(0xc4), Z(0x4b), Z(0xbc),
	Z(0xe2), Z(0xaf), Z(0x45), Z(0x73), Z(0xfa), Z(0xc9), Z(0x49), Z(0xcd),
	Z(0x00), Z(0x92), Z(0x7d), Z(0x97), Z(0x7a), Z(0x18), Z(0x60), Z(0x3d),
	Z(0xcf), Z(0x5b), Z(0xde), Z(0xc6), Z(0xe2), Z(0xe6), Z(0xbb), Z(0x8b),
	Z(0x06), Z(0xda), Z(0x08), Z(0x15), Z(0x1b), Z(0x88), Z(0x6a), Z(0x17),
	Z(0x89), Z(0xd0), Z(0xa9), Z(0xc1), Z(0xc9), Z(0x70), Z(0x6b), Z(0xe5),
	Z(0x43), Z(0xf4), Z(0x68), Z(0xc8), Z(0xd3), Z(0x84), Z(0x28), Z(0x0a),
	Z(0x52), Z(0x66), Z(0xa3), Z(0xca), Z(0xf2), Z(0xe3), Z(0x7f), Z(0x7a),
	Z(0x31), Z(0xf7), Z(0x88), Z(0x94), Z(0x5e), Z(0x9c), Z(0x63), Z(0xd5),
	Z(0x24), Z(0x66), Z(0xfc), Z(0xb3), Z(0x57), Z(0x25), Z(0xbe), Z(0x89),
	Z(0x44), Z(0xc4), Z(0xe0), Z(0x8f), Z(0x23), Z(0x3c), Z(0x12), Z(0x52),
	Z(0xf5), Z(0x1e), Z(0xf4), Z(0xcb), Z(0x18), Z(0x33), Z(0x1f), Z(0xf8),
	Z(0x69), Z(0x10), Z(0x9d), Z(0xd3), Z(0xf7), Z(0x28), Z(0xf8), Z(0x30),
	Z(0x05), Z(0x5e), Z(0x32), Z(0xc0), Z(0xd5), Z(0x19), Z(0xbd), Z(0x45),
	Z(0x8b), Z(0x5b), Z(0xfd), Z(0xbc), Z(0xe2), Z(0x5c), Z(0xa9), Z(0x96),
	Z(0xef), Z(0x70), Z(0xcf), Z(0xc2), Z(0x2a), Z(0xb3), Z(0x61), Z(0xad),
	Z(0x80), Z(0x48), Z(0x81), Z(0xb7), Z(0x1d), Z(0x43), Z(0xd9), Z(0xd7),
	Z(0x45), Z(0xf0), Z(0xd8), Z(0x8a), Z(0x59), Z(0x7c), Z(0x57), Z(0xc1),
	Z(0x79), Z(0xc7), Z(0x34), Z(0xd6), Z(0x43), Z(0xdf), Z(0xe4), Z(0x78),
	Z(0x16), Z(0x06), Z(0xda), Z(0x92), Z(0x76), Z(0x51), Z(0xe1), Z(0xd4),
	Z(0x70), Z(0x03), Z(0xe0), Z(0x2f), Z(0x96), Z(0x91), Z(0x82), Z(0x80)
};

#undef Z
#define Z(x) cpu_to_be32(x << 11)
static const __be32 sbox2[256] = {
	Z(0xf0), Z(0x37), Z(0x24), Z(0x53), Z(0x2a), Z(0x03), Z(0x83), Z(0x86),
	Z(0xd1), Z(0xec), Z(0x50), Z(0xf0), Z(0x42), Z(0x78), Z(0x2f), Z(0x6d),
	Z(0xbf), Z(0x80), Z(0x87), Z(0x27), Z(0x95), Z(0xe2), Z(0xc5), Z(0x5d),
	Z(0xf9), Z(0x6f), Z(0xdb), Z(0xb4), Z(0x65), Z(0x6e), Z(0xe7), Z(0x24),
	Z(0xc8), Z(0x1a), Z(0xbb), Z(0x49), Z(0xb5), Z(0x0a), Z(0x7d), Z(0xb9),
	Z(0xe8), Z(0xdc), Z(0xb7), Z(0xd9), Z(0x45), Z(0x20), Z(0x1b), Z(0xce),
	Z(0x59), Z(0x9d), Z(0x6b), Z(0xbd), Z(0x0e), Z(0x8f), Z(0xa3), Z(0xa9),
	Z(0xbc), Z(0x74), Z(0xa6), Z(0xf6), Z(0x7f), Z(0x5f), Z(0xb1), Z(0x68),
	Z(0x84), Z(0xbc), Z(0xa9), Z(0xfd), Z(0x55), Z(0x50), Z(0xe9), Z(0xb6),
	Z(0x13), Z(0x5e), Z(0x07), Z(0xb8), Z(0x95), Z(0x02), Z(0xc0), Z(0xd0),
	Z(0x6a), Z(0x1a), Z(0x85), Z(0xbd), Z(0xb6), Z(0xfd), Z(0xfe), Z(0x17),
	Z(0x3f), Z(0x09), Z(0xa3), Z(0x8d), Z(0xfb), Z(0xed), Z(0xda), Z(0x1d),
	Z(0x6d), Z(0x1c), Z(0x6c), Z(0x01), Z(0x5a), Z(0xe5), Z(0x71), Z(0x3e),
	Z(0x8b), Z(0x6b), Z(0xbe), Z(0x29), Z(0xeb), Z(0x12), Z(0x19), Z(0x34),
	Z(0xcd), Z(0xb3), Z(0xbd), Z(0x35), Z(0xea), Z(0x4b), Z(0xd5), Z(0xae),
	Z(0x2a), Z(0x79), Z(0x5a), Z(0xa5), Z(0x32), Z(0x12), Z(0x7b), Z(0xdc),
	Z(0x2c), Z(0xd0), Z(0x22), Z(0x4b), Z(0xb1), Z(0x85), Z(0x59), Z(0x80),
	Z(0xc0), Z(0x30), Z(0x9f), Z(0x73), Z(0xd3), Z(0x14), Z(0x48), Z(0x40),
	Z(0x07), Z(0x2d), Z(0x8f), Z(0x80), Z(0x0f), Z(0xce), Z(0x0b), Z(0x5e),
	Z(0xb7), Z(0x5e), Z(0xac), Z(0x24), Z(0x94), Z(0x4a), Z(0x18), Z(0x15),
	Z(0x05), Z(0xe8), Z(0x02), Z(0x77), Z(0xa9), Z(0xc7), Z(0x40), Z(0x45),
	Z(0x89), Z(0xd1), Z(0xea), Z(0xde), Z(0x0c), Z(0x79), Z(0x2a), Z(0x99),
	Z(0x6c), Z(0x3e), Z(0x95), Z(0xdd), Z(0x8c), Z(0x7d), Z(0xad), Z(0x6f),
	Z(0xdc), Z(0xff), Z(0xfd), Z(0x62), Z(0x47), Z(0xb3), Z(0x21), Z(0x8a),
	Z(0xec), Z(0x8e), Z(0x19), Z(0x18), Z(0xb4), Z(0x6e), Z(0x3d), Z(0xfd),
	Z(0x74), Z(0x54), Z(0x1e), Z(0x04), Z(0x85), Z(0xd8), Z(0xbc), Z(0x1f),
	Z(0x56), Z(0xe7), Z(0x3a), Z(0x56), Z(0x67), Z(0xd6), Z(0xc8), Z(0xa5),
	Z(0xf3), Z(0x8e), Z(0xde), Z(0xae), Z(0x37), Z(0x49), Z(0xb7), Z(0xfa),
	Z(0xc8), Z(0xf4), Z(0x1f), Z(0xe0), Z(0x2a), Z(0x9b), Z(0x15), Z(0xd1),
	Z(0x34), Z(0x0e), Z(0xb5), Z(0xe0), Z(0x44), Z(0x78), Z(0x84), Z(0x59),
	Z(0x56), Z(0x68), Z(0x77), Z(0xa5), Z(0x14), Z(0x06), Z(0xf5), Z(0x2f),
	Z(0x8c), Z(0x8a), Z(0x73), Z(0x80), Z(0x76), Z(0xb4), Z(0x10), Z(0x86)
};

#undef Z
#define Z(x) cpu_to_be32(x << 19)
static const __be32 sbox3[256] = {
	Z(0xa9), Z(0x2a), Z(0x48), Z(0x51), Z(0x84), Z(0x7e), Z(0x49), Z(0xe2),
	Z(0xb5), Z(0xb7), Z(0x42), Z(0x33), Z(0x7d), Z(0x5d), Z(0xa6), Z(0x12),
	Z(0x44), Z(0x48), Z(0x6d), Z(0x28), Z(0xaa), Z(0x20), Z(0x6d), Z(0x57),
	Z(0xd6), Z(0x6b), Z(0x5d), Z(0x72), Z(0xf0), Z(0x92), Z(0x5a), Z(0x1b),
	Z(0x53), Z(0x80), Z(0x24), Z(0x70), Z(0x9a), Z(0xcc), Z(0xa7), Z(0x66),
	Z(0xa1), Z(0x01), Z(0xa5), Z(0x41), Z(0x97), Z(0x41), Z(0x31), Z(0x82),
	Z(0xf1), Z(0x14), Z(0xcf), Z(0x53), Z(0x0d), Z(0xa0), Z(0x10), Z(0xcc),
	Z(0x2a), Z(0x7d), Z(0xd2), Z(0xbf), Z(0x4b), Z(0x1a), Z(0xdb), Z(0x16),
	Z(0x47), Z(0xf6), Z(0x51), Z(0x36), Z(0xed), Z(0xf3), Z(0xb9), Z(0x1a),
	Z(0xa7), Z(0xdf), Z(0x29), Z(0x43), Z(0x01), Z(0x54), Z(0x70), Z(0xa4),
	Z(0xbf), Z(0xd4), Z(0x0b), Z(0x53), Z(0x44), Z(0x60), Z(0x9e), Z(0x23),
	Z(0xa1), Z(0x18), Z(0x68), Z(0x4f), Z(0xf0), Z(0x2f), Z(0x82), Z(0xc2),
	Z(0x2a), Z(0x41), Z(0xb2), Z(0x42), Z(0x0c), Z(0xed), Z(0x0c), Z(0x1d),
	Z(0x13), Z(0x3a), Z(0x3c), Z(0x6e), Z(0x35), Z(0xdc), Z(0x60), Z(0x65),
	Z(0x85), Z(0xe9), Z(0x64), Z(0x02), Z(0x9a), Z(0x3f), Z(0x9f), Z(0x87),
	Z(0x96), Z(0xdf), Z(0xbe), Z(0xf2), Z(0xcb), Z(0xe5), Z(0x6c), Z(0xd4),
	Z(0x5a), Z(0x83), Z(0xbf), Z(0x92), Z(0x1b), Z(0x94), Z(0x00), Z(0x42),
	Z(0xcf), Z(0x4b), Z(0x00), Z(0x75), Z(0xba), Z(0x8f), Z(0x76), Z(0x5f),
	Z(0x5d), Z(0x3a), Z(0x4d), Z(0x09), Z(0x12), Z(0x08), Z(0x38), Z(0x95),
	Z(0x17), Z(0xe4), Z(0x01), Z(0x1d), Z(0x4c), Z(0xa9), Z(0xcc), Z(0x85),
	Z(0x82), Z(0x4c), Z(0x9d), Z(0x2f), Z(0x3b), Z(0x66), Z(0xa1), Z(0x34),
	Z(0x10), Z(0xcd), Z(0x59), Z(0x89), Z(0xa5), Z(0x31), Z(0xcf), Z(0x05),
	Z(0xc8), Z(0x84), Z(0xfa), Z(0xc7), Z(0xba), Z(0x4e), Z(0x8b), Z(0x1a),
	Z(0x19), Z(0xf1), Z(0xa1), Z(0x3b), Z(0x18), Z(0x12), Z(0x17), Z(0xb0),
	Z(0x98), Z(0x8d), Z(0x0b), Z(0x23), Z(0xc3), Z(0x3a), Z(0x2d), Z(0x20),
	Z(0xdf), Z(0x13), Z(0xa0), Z(0xa8), Z(0x4c), Z(0x0d), Z(0x6c), Z(0x2f),
	Z(0x47), Z(0x13), Z(0x13), Z(0x52), Z(0x1f), Z(0x2d), Z(0xf5), Z(0x79),
	Z(0x3d), Z(0xa2), Z(0x54), Z(0xbd), Z(0x69), Z(0xc8), Z(0x6b), Z(0xf3),
	Z(0x05), Z(0x28), Z(0xf1), Z(0x16), Z(0x46), Z(0x40), Z(0xb0), Z(0x11),
	Z(0xd3), Z(0xb7), Z(0x95), Z(0x49), Z(0xcf), Z(0xc3), Z(0x1d), Z(0x8f),
	Z(0xd8), Z(0xe1), Z(0x73), Z(0xdb), Z(0xad), Z(0xc8), Z(0xc9), Z(0xa9),
	Z(0xa1), Z(0xc2), Z(0xc5), Z(0xe3), Z(0xba), Z(0xfc), Z(0x0e), Z(0x25)
};

/*
 * This is a 16 round Feistel network with permutation F_ENCRYPT
 */
#define F_ENCRYPT(R, L, sched)						\
do {									\
	union lc4 { __be32 l; u8 c[4]; } u;				\
	u.l = sched ^ R;						\
	L ^= sbox0[u.c[0]] ^ sbox1[u.c[1]] ^ sbox2[u.c[2]] ^ sbox3[u.c[3]]; \
} while (0)

/*
 * encryptor
 */
static void fcrypt_encrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	const struct fcrypt_ctx *ctx = crypto_tfm_ctx(tfm);
	struct {
		__be32 l, r;
	} X;

	memcpy(&X, src, sizeof(X));

	F_ENCRYPT(X.r, X.l, ctx->sched[0x0]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x1]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x2]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x3]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x4]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x5]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x6]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x7]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x8]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x9]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xa]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0xb]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xc]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0xd]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xe]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0xf]);

	memcpy(dst, &X, sizeof(X));
}

/*
 * decryptor
 */
static void fcrypt_decrypt(struct crypto_tfm *tfm, u8 *dst, const u8 *src)
{
	const struct fcrypt_ctx *ctx = crypto_tfm_ctx(tfm);
	struct {
		__be32 l, r;
	} X;

	memcpy(&X, src, sizeof(X));

	F_ENCRYPT(X.l, X.r, ctx->sched[0xf]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xe]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0xd]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xc]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0xb]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0xa]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x9]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x8]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x7]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x6]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x5]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x4]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x3]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x2]);
	F_ENCRYPT(X.l, X.r, ctx->sched[0x1]);
	F_ENCRYPT(X.r, X.l, ctx->sched[0x0]);

	memcpy(dst, &X, sizeof(X));
}

/*
 * Generate a key schedule from key, the least significant bit in each key byte
 * is parity and shall be ignored. This leaves 56 significant bits in the key
 * to scatter over the 16 key schedules. For each schedule extract the low
 * order 32 bits and use as schedule, then rotate right by 11 bits.
 */
static int fcrypt_setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int keylen)
{
	struct fcrypt_ctx *ctx = crypto_tfm_ctx(tfm);

#if BITS_PER_LONG == 64  /* the 64-bit version can also be used for 32-bit
			  * kernels - it seems to be faster but the code is
			  * larger */

	u64 k;	/* k holds all 56 non-parity bits */

	/* discard the parity bits */
	k = (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key++) >> 1;
	k <<= 7;
	k |= (*key) >> 1;

	/* Use lower 32 bits for schedule, rotate by 11 each round (16 times) */
	ctx->sched[0x0] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x1] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x2] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x3] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x4] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x5] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x6] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x7] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x8] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0x9] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xa] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xb] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xc] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xd] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xe] = cpu_to_be32(k); ror56_64(k, 11);
	ctx->sched[0xf] = cpu_to_be32(k);

	return 0;
#else
	u32 hi, lo;		/* hi is upper 24 bits and lo lower 32, total 56 */

	/* discard the parity bits */
	lo = (*key++) >> 1;
	lo <<= 7;
	lo |= (*key++) >> 1;
	lo <<= 7;
	lo |= (*key++) >> 1;
	lo <<= 7;
	lo |= (*key++) >> 1;
	hi = lo >> 4;
	lo &= 0xf;
	lo <<= 7;
	lo |= (*key++) >> 1;
	lo <<= 7;
	lo |= (*key++) >> 1;
	lo <<= 7;
	lo |= (*key++) >> 1;
	lo <<= 7;
	lo |= (*key) >> 1;

	/* Use lower 32 bits for schedule, rotate by 11 each round (16 times) */
	ctx->sched[0x0] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x1] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x2] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x3] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x4] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x5] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x6] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x7] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x8] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0x9] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xa] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xb] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xc] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xd] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xe] = cpu_to_be32(lo); ror56(hi, lo, 11);
	ctx->sched[0xf] = cpu_to_be32(lo);
	return 0;
#endif
}

static struct crypto_alg fcrypt_alg = {
	.cra_name		=	"fcrypt",
	.cra_driver_name	=	"fcrypt-generic",
	.cra_flags		=	CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize		=	8,
	.cra_ctxsize		=	sizeof(struct fcrypt_ctx),
	.cra_module		=	THIS_MODULE,
	.cra_u			=	{ .cipher = {
	.cia_min_keysize	=	8,
	.cia_max_keysize	=	8,
	.cia_setkey		=	fcrypt_setkey,
	.cia_encrypt		=	fcrypt_encrypt,
	.cia_decrypt		=	fcrypt_decrypt } }
};

static int __init fcrypt_mod_init(void)
{
	return crypto_register_alg(&fcrypt_alg);
}

static void __exit fcrypt_mod_fini(void)
{
	crypto_unregister_alg(&fcrypt_alg);
}

subsys_initcall(fcrypt_mod_init);
module_exit(fcrypt_mod_fini);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("FCrypt Cipher Algorithm");
MODULE_AUTHOR("David Howells <dhowells@redhat.com>");
MODULE_ALIAS_CRYPTO("fcrypt");
