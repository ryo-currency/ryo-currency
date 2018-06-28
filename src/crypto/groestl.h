// Copyright (c) 2018, Ryo Currency Project
// Portions copyright (c) 2014-2018, The Monero Project
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
//    public domain on 1st of February 2019
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

#ifndef __hash_h
#define __hash_h
/*
#include "crypto_hash.h" 
#include "crypto_uint32.h"
#include "crypto_uint64.h"
#include "crypto_uint8.h"

typedef crypto_uint8 uint8_t; 
typedef crypto_uint32 uint32_t; 
typedef crypto_uint64 uint64_t;
*/
#include <stdint.h>

/* some sizes (number of bytes) */
#define ROWS 8
#define LENGTHFIELDLEN ROWS
#define COLS512 8

#define SIZE512 (ROWS * COLS512)

#define ROUNDS512 10
#define HASH_BIT_LEN 256

#define ROTL32(v, n) ((((v) << (n)) | ((v) >> (32 - (n)))) & li_32(ffffffff))

#define li_32(h) 0x##h##u
#define EXT_BYTE(var, n) ((uint8_t)((uint32_t)(var) >> (8 * n)))
#define u32BIG(a)                       \
	((ROTL32(a, 8) & li_32(00FF00FF)) | \
	 (ROTL32(a, 24) & li_32(FF00FF00)))

/* NIST API begin */
typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef struct
{
	uint32_t chaining[SIZE512 / sizeof(uint32_t)]; /* actual state */
	uint32_t block_counter1,
		block_counter2;			 /* message block counter(s) */
	BitSequence buffer[SIZE512]; /* data buffer */
	int buf_ptr;				 /* data buffer pointer */
	int bits_in_last_byte;		 /* no. of message bits in last byte of
			       data buffer */
} hashState;

/*void Init(hashState*);
void Update(hashState*, const BitSequence*, DataLength);
void Final(hashState*, BitSequence*); */
void groestl(const BitSequence *, DataLength, BitSequence *);
/* NIST API end   */

/*
int crypto_hash(unsigned char *out,
		const unsigned char *in,
		unsigned long long len);
*/

#endif /* __hash_h */
