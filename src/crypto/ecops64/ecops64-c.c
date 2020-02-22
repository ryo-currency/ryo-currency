// Copyright (c) 2020, Ryo Currency Project
//
// Portions of this file are available under BSD-3 license. Please see
// ORIGINAL-LICENSE for details All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This
// includes
//    posting the work online, or hosting copies of the modified work for
//    download.
//
// 4. Any derivative version of this work is also covered by this license,
// including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may
// be
//    used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in
// accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this
// licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2021
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Based on Dr Bernstein's public domain code.
// https://bench.cr.yp.to/supercop.html

#include "ecops64.h"

static inline void fe64_setint(fe64 r, unsigned int v)
{
	r[0] = v;
	r[1] = 0;
	r[2] = 0;
	r[3] = 0;
}

static inline void fe64_copy(fe64 t, const fe64 x)
{
	t[0] = x[0];
	t[1] = x[1];
	t[2] = x[2];
	t[3] = x[3];
}

static inline void fe64_neg(fe64 r, const fe64 x)
{
	fe64 t;
	fe64_setint(t, 0);
	fe64_sub(r, t, x);
}

static inline void fe64_unpack(fe64 r, const unsigned char x[32])
{
	r[0] = *(unsigned long long*)x;
	r[1] = *(((unsigned long long*)x) + 1);
	r[2] = *(((unsigned long long*)x) + 2);
	r[3] = *(((unsigned long long*)x) + 3);
	r[3] &= 0x7fffffffffffffffULL;
}

static inline void fe64_pack(unsigned char r[32], const fe64 x)
{
	fe64 t;
	fe64_copy(t, x);
	fe64_reduce(t);

	*(uint64_t*)r = t[0];
	*(((uint64_t*)r) + 1) = t[1];
	*(((uint64_t*)r) + 2) = t[2];
	*(((uint64_t*)r) + 3) = t[3];
}

static inline int fe64_iszero_vartime(const fe64 x)
{
	fe64 t;
	fe64_copy(t, x);
	fe64_reduce(t);

	if(t[0])
		return 0;
	if(t[1])
		return 0;
	if(t[2])
		return 0;
	if(t[3])
		return 0;

	return 1;
}

static inline int fe64_isnegative(const fe64 f)
{
	fe64 t;
	fe64_copy(t, f);
	fe64_reduce(t);

	return (unsigned char)t[0] & 1;
}

void fe64_divpowm1(fe64 r, const fe64 u, const fe64 v)
{
	fe64 v3, uv7, t0, t1, t2;
	size_t i;

	fe64_square(v3, v);
	fe64_mul(v3, v3, v); /* v3 = v^3 */
	fe64_square(uv7, v3);
	fe64_mul(uv7, uv7, v);
	fe64_mul(uv7, uv7, u); /* uv7 = uv^7 */

	/*fe_pow22523(uv7, uv7);*/
	/* From fe_pow22523.c */
	fe64_square(t0, uv7);
	fe64_square(t1, t0);
	fe64_square(t1, t1);
	fe64_mul(t1, uv7, t1);
	fe64_mul(t0, t0, t1);
	fe64_square(t0, t0);
	fe64_mul(t0, t1, t0);
	fe64_square(t1, t0);

	for(i = 0; i < 4; ++i)
		fe64_square(t1, t1);

	fe64_mul(t0, t1, t0);
	fe64_square(t1, t0);

	for(i = 0; i < 9; ++i)
		fe64_square(t1, t1);

	fe64_mul(t1, t1, t0);
	fe64_square(t2, t1);
	for(i = 0; i < 19; ++i)
		fe64_square(t2, t2);

	fe64_mul(t1, t2, t1);
	for(i = 0; i < 10; ++i)
		fe64_square(t1, t1);

	fe64_mul(t0, t1, t0);
	fe64_square(t1, t0);
	for(i = 0; i < 49; ++i)
		fe64_square(t1, t1);

	fe64_mul(t1, t1, t0);
	fe64_square(t2, t1);
	for(i = 0; i < 99; ++i)
		fe64_square(t2, t2);

	fe64_mul(t1, t2, t1);
	for(i = 0; i < 50; ++i)
		fe64_square(t1, t1);

	fe64_mul(t0, t1, t0);
	fe64_square(t0, t0);
	fe64_square(t0, t0);
	fe64_mul(t0, t0, uv7);

	/* End fe_pow22523.c */
	/* t0 = (uv^7)^((q-5)/8) */
	fe64_mul(t0, t0, v3);
	fe64_mul(r, t0, u); /* u^(m+1)v^(-(m+1)) */
}

void fe64_invert(fe64 out, const fe64 z)
{
	fe64 t0, t1, t2, t3;
	int i;

	fe64_square(t0, z);
	fe64_square(t1, t0);
	fe64_square(t1, t1);
	fe64_mul(t1, z, t1);
	fe64_mul(t0, t0, t1);
	fe64_square(t2, t0);
	fe64_mul(t1, t1, t2);
	fe64_square(t2, t1);

	for(i = 0; i < 4; ++i)
		fe64_square(t2, t2);

	fe64_mul(t1, t2, t1);
	fe64_square(t2, t1);
	for(i = 0; i < 9; ++i)
		fe64_square(t2, t2);

	fe64_mul(t2, t2, t1);
	fe64_square(t3, t2);
	for(i = 0; i < 19; ++i)
		fe64_square(t3, t3);

	fe64_mul(t2, t3, t2);
	fe64_square(t2, t2);
	for(i = 0; i < 9; ++i)
		fe64_square(t2, t2);

	fe64_mul(t1, t2, t1);
	fe64_square(t2, t1);
	for(i = 0; i < 49; ++i)
		fe64_square(t2, t2);

	fe64_mul(t2, t2, t1);
	fe64_square(t3, t2);
	for(i = 0; i < 99; ++i)
		fe64_square(t3, t3);

	fe64_mul(t2, t3, t2);
	fe64_square(t2, t2);
	for(i = 0; i < 49; ++i)
		fe64_square(t2, t2);

	fe64_mul(t1, t2, t1);
	fe64_square(t1, t1);

	for(i = 0; i < 4; ++i)
		fe64_square(t1, t1);

	fe64_mul(out, t1, t0);
}

void ge64_p3_to_p2(ge64_p2* r, const ge64_p3* p)
{
	fe64_copy(r->x, p->x);
	fe64_copy(r->y, p->y);
	fe64_copy(r->z, p->z);
}

void ge64_mul8(ge64_p1p1* r, const ge64_p2* t)
{
	ge64_p2 u;
	ge64_p2_dbl(r, t);
	ge64_p1p1_to_p2(&u, r);
	ge64_p2_dbl(r, &u);
	ge64_p1p1_to_p2(&u, r);
	ge64_p2_dbl(r, &u);
}

/* d */
extern const fe64 fe64_d;
extern const fe64 fe64_2d;
/* sqrt(-1) */
extern const fe64 fe64_sqrtm1;

/* return 0 on success, -1 otherwise */
int ge64_frombytes_vartime(ge64_p3* h, const unsigned char p[32])
{
	fe64 u, v, vxx, check;

	fe64_unpack(h->y, p);

	fe64_setint(h->z, 1);
	fe64_square(u, h->y);   /* x = y^2 */
	fe64_mul(v, u, fe64_d); /* den = dy^2 */
	fe64_sub(u, u, h->z);   /* x = y^2-1 */
	fe64_add(v, v, h->z);   /* den = dy^2+1 */

	fe64_divpowm1(h->x, u, v);

	fe64_square(vxx, h->x);
	fe64_mul(vxx, vxx, v);
	fe64_sub(check, vxx, u);

	if(!fe64_iszero_vartime(check))
	{
		fe64_add(check, vxx, u);
		if(!fe64_iszero_vartime(check))
			return -1;
		fe64_mul(h->x, h->x, fe64_sqrtm1);
	}

	if(fe64_isnegative(h->x) != (p[31] >> 7))
	{
		if(fe64_iszero_vartime(h->x))
			return -1;
		fe64_neg(h->x, h->x);
	}

	fe64_mul(h->t, h->x, h->y);
	return 0;
}

void ge64_tobytes(unsigned char s[32], const ge64_p2* h)
{
	fe64 recip, x, y;

	fe64_invert(recip, h->z);
	fe64_mul(x, h->x, recip);
	fe64_mul(y, h->y, recip);
	fe64_pack(s, y);

	s[31] ^= fe64_isnegative(x) << 7;
}

/* no overflow for this particular order */
extern const uint64_t sc64_reduce_order[16];

static inline uint64_t smaller(uint64_t a, uint64_t b)
{
	uint64_t atop = a >> 32;
	uint64_t abot = a & 4294967295;
	uint64_t btop = b >> 32;
	uint64_t bbot = b & 4294967295;
	uint64_t atopbelowbtop = (atop - btop) >> 63;
	uint64_t atopeqbtop = ((atop ^ btop) - 1) >> 63;
	uint64_t abotbelowbbot = (abot - bbot) >> 63;
	return atopbelowbtop | (atopeqbtop & abotbelowbbot);
}

void sc64_reduce32(unsigned char x[32])
{
	uint64_t rv0, rv1, rv2, rv3;
	int j;

	/* assuming little-endian */
	rv0 = *(uint64_t*)x;
	rv1 = *(((uint64_t*)x) + 1);
	rv2 = *(((uint64_t*)x) + 2);
	rv3 = *(((uint64_t*)x) + 3);

	for(j = 3; j >= 0; j--)
	{
		uint64_t t0, t1, t2, t3;
		uint64_t b = sc64_reduce_order[4 * j + 0];
		t0 = rv0 - b;
		b = smaller(rv0, b);

		b += sc64_reduce_order[4 * j + 1];
		t1 = rv1 - b;
		b = smaller(rv1, b);

		b += sc64_reduce_order[4 * j + 2];
		t2 = rv2 - b;
		b = smaller(rv2, b);

		b += sc64_reduce_order[4 * j + 3];
		t3 = rv3 - b;
		b = smaller(rv3, b);

		uint64_t mask = b - 1;
		rv0 ^= mask & (rv0 ^ t0);
		rv1 ^= mask & (rv1 ^ t1);
		rv2 ^= mask & (rv2 ^ t2);
		rv3 ^= mask & (rv3 ^ t3);
	}

	*(uint64_t*)x = rv0;
	*(((uint64_t*)x) + 1) = rv1;
	*(((uint64_t*)x) + 2) = rv2;
	*(((uint64_t*)x) + 3) = rv3;
}

static inline void mult_extract(signed char b[64], const unsigned char s[32])
{
	int carry, carry2;

	carry = 0; /* 0..1 */
	for(uint64_t i = 0; i < 31; i++)
	{
		carry += s[i];						  /* 0..256 */
		carry2 = (carry + 8) >> 4;			  /* 0..16 */
		b[2 * i] = carry - (carry2 << 4);	 /* -8..7 */
		carry = (carry2 + 8) >> 4;			  /* 0..1 */
		b[2 * i + 1] = carry2 - (carry << 4); /* -8..7 */
	}

	carry += s[31];				   /* 0..128 */
	carry2 = (carry + 8) >> 4;	 /* 0..8 */
	b[62] = carry - (carry2 << 4); /* -8..7 */
	b[63] = carry2;				   /* 0..8 */
}

/* Multiples of the base point in Niels' representation */
extern const ge64_niels ge64_base_multiples_niels[];
extern const fe64 ge64_base_mult0_x[];
extern const fe64 ge64_base_mult0_y[];
extern const fe64 ge64_base_mult0_t[];

void ge64_scalarmult_base(ge64_p3* r, const unsigned char s[32])
{
	signed char b[64];
	mult_extract(b, s);

	fe64_copy(r->x, ge64_base_mult0_x[b[0] + 8]);
	fe64_copy(r->y, ge64_base_mult0_y[b[0] + 8]);
	fe64_copy(r->t, ge64_base_mult0_t[b[0] + 8]);
	fe64_setint(r->z, 2);

	for(uint64_t i = 1; i < 64; i++)
	{
		uint64_t idx = (i - 1) * 17 + b[i] + 8;
		ge64_nielsadd2(r, &ge64_base_multiples_niels[idx]);
	}
}

static inline void ge64_p3_to_pniels(ge64_pniels* r, const ge64_p3* p)
{
	fe64_add(r->xaddy, p->x, p->y);
	fe64_sub(r->ysubx, p->y, p->x);
	fe64_copy(r->z, p->z);
	fe64_mul(r->t2d, p->t, fe64_2d);
}

/* Assumes that a[31] <= 127 */
void ge64_scalarmult(ge64_p2* r, const unsigned char a[32], const ge64_p3* A)
{
	signed char e[64];
	mult_extract(e, a);

	ge64_pniels Ai[17];

	fe64_setint(Ai[8].xaddy, 1);
	fe64_setint(Ai[8].ysubx, 1);
	fe64_setint(Ai[8].z, 1);
	fe64_setint(Ai[8].t2d, 0);

	ge64_p3_to_pniels(&Ai[9], A);

	ge64_p3 u;
	ge64_p1p1 t;
	for(uint64_t i = 9; i < 16; i++)
	{
		ge64_pnielsadd_p1p1(&t, A, &Ai[i]);
		ge64_p1p1_to_p3(&u, &t);
		ge64_p3_to_pniels(&Ai[i + 1], &u);
	}

	for(uint64_t i = 0; i < 8; i++)
	{
		fe64_copy(Ai[i].xaddy, Ai[16 - i].ysubx);
		fe64_copy(Ai[i].ysubx, Ai[16 - i].xaddy);
		fe64_copy(Ai[i].z, Ai[16 - i].z);
		fe64_neg(Ai[i].t2d, Ai[16 - i].t2d);
	}

	fe64_setint(r->x, 0);
	fe64_setint(r->y, 1);
	fe64_setint(r->z, 1);

	for(int64_t i = 63; i >= 0; i--)
	{
		ge64_p2_dbl(&t, r);
		ge64_p1p1_to_p2(r, &t);
		ge64_p2_dbl(&t, r);
		ge64_p1p1_to_p2(r, &t);
		ge64_p2_dbl(&t, r);
		ge64_p1p1_to_p2(r, &t);
		ge64_p2_dbl(&t, r);
		ge64_p1p1_to_p3(&u, &t);

		uint64_t idx = e[i] + 8;
		ge64_pnielsadd_p1p1(&t, &u, &Ai[idx]);
		ge64_p1p1_to_p2(r, &t);
	}
}

inline void ge64_sub(ge64_p1p1* r, const ge64_p3* p, const ge64_p3* q)
{
	fe64 t0, xaddy, ysubx, t2d;
	fe64_add(xaddy, q->x, q->y);
	fe64_sub(ysubx, q->y, q->x);
	fe64_mul(t2d, q->t, fe64_2d);

	fe64_add(r->x, p->y, p->x);
	fe64_sub(r->y, p->y, p->x);
	fe64_mul(r->z, r->x, ysubx);
	fe64_mul(r->y, r->y, xaddy);
	fe64_mul(r->t, t2d, p->t);
	fe64_mul(r->x, p->z, q->z);
	fe64_add(t0, r->x, r->x);
	fe64_sub(r->x, r->z, r->y);
	fe64_add(r->y, r->z, r->y);
	fe64_sub(r->z, t0, r->t);
	fe64_add(r->t, t0, r->t);
}
