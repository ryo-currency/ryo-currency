// Copyright (c) 2020, Ryo Currency Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Authors and copyright holders give permission for following:
//
// 1. Redistribution and use in source and binary forms WITHOUT modification.
//
// 2. Modification of the source form for your own personal use.
//
// As long as the following conditions are met:
//
// 3. You must not distribute modified copies of the work to third parties. This includes
//    posting the work online, or hosting copies of the modified work for download.
//
// 4. Any derivative version of this work is also covered by this license, including point 8.
//
// 5. Neither the name of the copyright holders nor the names of the authors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// 6. You agree that this licence is governed by and shall be construed in accordance
//    with the laws of England and Wales.
//
// 7. You agree to submit all disputes arising out of or in connection with this licence
//    to the exclusive jurisdiction of the Courts of England and Wales.
//
// Authors and copyright holders agree that:
//
// 8. This licence expires and the work covered by it is released into the
//    public domain on 1st of February 2021
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Based on Dr Bernstein's public domain code.
// https://bench.cr.yp.to/supercop.html

#pragma once

#include <inttypes.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	typedef uint64_t fe64[4];

	typedef struct
	{
		fe64 x;
		fe64 y;
		fe64 z;
		fe64 t;
	} ge64_p3;

	typedef struct
	{
		fe64 x;
		fe64 z;
		fe64 y;
		fe64 t;
	} ge64_p1p1;

	typedef struct
	{
		fe64 x;
		fe64 y;
		fe64 z;
	} ge64_p2;

	typedef struct
	{
		fe64 ysubx;
		fe64 xaddy;
		fe64 t2d;
	} ge64_niels;

	typedef struct
	{
		fe64 ysubx;
		fe64 xaddy;
		fe64 z;
		fe64 t2d;
	} ge64_pniels;

	///// EXTERN ASM
	void sV_fe64_mul(fe64 r, const fe64 x, const fe64 y);
	void sV_fe64_square(fe64 r, const fe64 x);
	void sV_fe64_add(fe64 r, const fe64 x, const fe64 y);
	void sV_fe64_sub(fe64 r, const fe64 x, const fe64 y);
	void sV_fe64_reduce(fe64 r);
	void sV_ge64_add(ge64_p1p1* r, const ge64_p3* p, const ge64_p3* q);
	void sV_ge64_p1p1_to_p2(ge64_p2* r, const ge64_p1p1* p);
	void sV_ge64_p1p1_to_p3(ge64_p3* r, const ge64_p1p1* p);
	void sV_ge64_nielsadd2(ge64_p3* r, const ge64_niels* q);
	void sV_ge64_pnielsadd_p1p1(ge64_p1p1* r, const ge64_p3* p,
		const ge64_pniels* q);
	void sV_ge64_p2_dbl(ge64_p1p1* r, const ge64_p2* p);

#ifdef _WIN32
	void win64_wrapper(void* f, ...);
#define fe64_mul(r, x, y) win64_wrapper(sV_fe64_mul, r, x, y)
#define fe64_square(r, x) win64_wrapper(sV_fe64_square, r, x)
#define fe64_add(r, x, y) win64_wrapper(sV_fe64_add, r, x, y)
#define fe64_sub(r, x, y) win64_wrapper(sV_fe64_sub, r, x, y)
#define fe64_reduce(r) win64_wrapper(sV_fe64_reduce, r)
#define ge64_add(r, p, q) win64_wrapper(sV_ge64_add, r, p, q)
#define ge64_p1p1_to_p2(r, p) win64_wrapper(sV_ge64_p1p1_to_p2, r, p)
#define ge64_p1p1_to_p3(r, p) win64_wrapper(sV_ge64_p1p1_to_p3, r, p)
#define ge64_nielsadd2(r, q) win64_wrapper(sV_ge64_nielsadd2, r, q)
#define ge64_pnielsadd_p1p1(r, p, q) \
	win64_wrapper(sV_ge64_pnielsadd_p1p1, r, p, q)
#define ge64_p2_dbl(r, p) win64_wrapper(sV_ge64_p2_dbl, r, p)
#else
#define fe64_mul sV_fe64_mul
#define fe64_square sV_fe64_square
#define fe64_add sV_fe64_add
#define fe64_sub sV_fe64_sub
#define fe64_reduce sV_fe64_reduce
#define ge64_add sV_ge64_add
#define ge64_p1p1_to_p2 sV_ge64_p1p1_to_p2
#define ge64_p1p1_to_p3 sV_ge64_p1p1_to_p3
#define ge64_nielsadd2 sV_ge64_nielsadd2
#define ge64_pnielsadd_p1p1 sV_ge64_pnielsadd_p1p1
#define ge64_p2_dbl sV_ge64_p2_dbl
#endif // !_WIN32

	///// C FUNCTIONS
	void ge64_p3_to_p2(ge64_p2* r, const ge64_p3* p);
	void ge64_mul8(ge64_p1p1* r, const ge64_p2* t);
	int ge64_frombytes_vartime(ge64_p3* h, const unsigned char p[32]);
	void ge64_tobytes(unsigned char s[32], const ge64_p2* h);
	void sc64_reduce32(unsigned char x[32]);
	void ge64_scalarmult_base(ge64_p3* r, const unsigned char s[32]);
	void ge64_scalarmult(ge64_p2* r, const unsigned char a[32], const ge64_p3* A);

	void ge64_sub(ge64_p1p1* r, const ge64_p3* p, const ge64_p3* q);

#ifdef __cplusplus
}
#endif // __cplusplus
