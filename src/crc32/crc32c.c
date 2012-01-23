/* 

taken from Colin Percival's kivaloo, licensed under the BSD License 
slightly modified to not use dynamic allocations by rofl0r

Copyright 2011 Colin Percival.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

*/

#include <stdint.h>
#include <stdlib.h>

#include "../../include/crc32c.h"

/**
 * CRC32C tables:
 * T[0][i] = reverse32(reverse8(i) * x^32 mod p(x) mod 2)
 * T[1][i] = reverse32(reverse8(i) * x^40 mod p(x) mod 2)
 * T[2][i] = reverse32(reverse8(i) * x^48 mod p(x) mod 2)
 * T[3][i] = reverse32(reverse8(i) * x^56 mod p(x) mod 2)
 */

/* Optimization: Precomputed value of T[0][0x80]. */
#define T_0_0x80 0x82f63b78
static uint32_t* T[4];
static uint32_t T0[256];
static uint32_t T1[256];
static uint32_t T2[256];
static uint32_t T3[256];
static int initdone = 0;
/**
 * reverse(x):
 * Return x with reversed bit-order.
 */
static uint32_t reverse(uint32_t x) {

	x = ((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16);
	x = ((x & 0xff00ff00) >> 8)  | ((x & 0x00ff00ff) << 8);
	x = ((x & 0xf0f0f0f0) >> 4)  | ((x & 0x0f0f0f0f) << 4);
	x = ((x & 0xcccccccc) >> 2)  | ((x & 0x33333333) << 2);
	x = ((x & 0xaaaaaaaa) >> 1)  | ((x & 0x55555555) << 1);

	return (x);
}


/**
 * init(void):
 * Initialize tables.
 */
static void init(void) {
	size_t i, j, k;
	uint32_t r;
	/* Fill in tables. */
	for (i = 0; i < 256; i++) {
		r = reverse(i);
		for (j = 0; j < 4; j++) {
			for (k = 0; k < 8; k++) {
				if (r & 0x80000000)
					r = (r << 1) ^ 0x1EDC6F41;
				else
					r = (r << 1);
			}
			T[j][i] = reverse(r);
		}
	}
}

/**
 * CRC32C_Init(ctx):
 * Initialize a CRC32C-computing context.  This function can only fail the
 * first time it is called.
 */
int
CRC32C_Init(CRC32C_CTX * ctx)
{
	
	if(!initdone) {
		T[0] = T0;
		T[1] = T1;
		T[2] = T2;
		T[3] = T3;
		
		init();
		initdone = 1;
	}

	/* Set state to the CRC of the implicit leading 1 bit. */
	ctx->state = T_0_0x80;

	/* Success! */
	return (0);
}

/**
 * CRC32C_Update(ctx, buf, len):
 * Feed ${len} bytes from the buffer ${buf} into the CRC32C being computed
 * via the context ${ctx}.
 */
void
CRC32C_Update(CRC32C_CTX * ctx, const uint8_t * buf, size_t len)
{

	/* Handle blocks of 4 bytes. */
	for (; len >= 4; len -= 4, buf += 4) {
		ctx->state =
		    T[0][((ctx->state >> 24) & 0xff) ^ buf[3]] ^
		    T[1][((ctx->state >> 16) & 0xff) ^ buf[2]] ^
		    T[2][((ctx->state >> 8)  & 0xff) ^ buf[1]] ^
		    T[3][((ctx->state)       & 0xff) ^ buf[0]];
	}

	/* Handle individual bytes. */
	for (; len > 0; len--, buf++) {
		ctx->state = (ctx->state >> 8) ^
		    T[0][((ctx->state) & 0xff) ^ buf[0]];
	}
}

/**
 * CRC32C_Final(cbuf, ctx):
 * Store in ${cbuf} a value such that 1[buf][buf]...[buf][cbuf], where each
 * buffer is interpreted as a bit sequence starting with the least
 * significant bit of the byte in the lowest address, is a product of the
 * Castagnoli polynomial.
 */
void
CRC32C_Final(uint8_t cbuf[4], CRC32C_CTX * ctx)
{

	/* Copy state out. */
	cbuf[0] = ctx->state & 0xff;
	cbuf[1] = (ctx->state >> 8) & 0xff;
	cbuf[2] = (ctx->state >> 16) & 0xff;
	cbuf[3] = (ctx->state >> 24) & 0xff;
}

