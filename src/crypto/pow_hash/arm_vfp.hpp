// Copyright (c) 2019, Ryo Currency Project
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
//    public domain on 1st of February 2020
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
//
// Parts of this file are originally copyright (c) 2014-2017, SUMOKOIN
// Parts of this file are originally copyright (c) 2014-2017, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013, The Cryptonote developers

#pragma once
#include <inttypes.h>
#include <stddef.h>

// This header emulates NEON instructions in IEEE 754 compilant way
// Don't expect to use this code for anything other than verificaiton

struct float32x4_t
{
	float v[4];
};

struct int32x4_t
{
	int32_t v[4];
};

struct uint32x4_t
{
	uint32_t v[4];
};

inline int32x4_t vld1q_s32(int32_t *v)
{
	int32x4_t r;
	r.v[0] = v[0];
	r.v[1] = v[1];
	r.v[2] = v[2];
	r.v[3] = v[3];
	return r;
}

inline void vst1q_s32(int32_t *v, const int32x4_t &a)
{
	v[0] = a.v[0];
	v[1] = a.v[1];
	v[2] = a.v[2];
	v[3] = a.v[3];
}

inline float32x4_t vdupq_n_f32(float a)
{
	float32x4_t r;
	r.v[0] = a;
	r.v[1] = a;
	r.v[2] = a;
	r.v[3] = a;
	return r;
}

inline int32x4_t vdupq_n_s32(int32_t a)
{
	int32x4_t r;
	r.v[0] = a;
	r.v[1] = a;
	r.v[2] = a;
	r.v[3] = a;
	return r;
}

inline uint32x4_t vdupq_n_u32(uint32_t a)
{
	uint32x4_t r;
	r.v[0] = a;
	r.v[1] = a;
	r.v[2] = a;
	r.v[3] = a;
	return r;
}

template <size_t v>
inline void vrot_si32(int32x4_t &r)
{
	uint8_t tmp[v];
	uint8_t *vt = (uint8_t *)&r;
	for(size_t i = 0; i < v; i++)
		tmp[i] = vt[i];
	for(size_t i = 0; i < 16 - v; i++)
		vt[i] = vt[i + v];
	size_t e = 16 - v;
	for(size_t i = e; i < 16; i++)
		vt[i] = tmp[i - e];
}

template <>
inline void vrot_si32<0>(int32x4_t &r)
{
}

inline float32x4_t vcvtq_f32_s32(const int32x4_t &v)
{
	float32x4_t r;
	r.v[0] = v.v[0];
	r.v[1] = v.v[1];
	r.v[2] = v.v[2];
	r.v[3] = v.v[3];
	return r;
}

inline int32x4_t vcvtq_s32_f32(const float32x4_t &v)
{
	int32x4_t r;
	r.v[0] = v.v[0];
	r.v[1] = v.v[1];
	r.v[2] = v.v[2];
	r.v[3] = v.v[3];
	return r;
}

inline int32x4_t vorrq_s32(const int32x4_t &a, const int32x4_t &b)
{
	int32x4_t r;
	r.v[0] = a.v[0] | b.v[0];
	r.v[1] = a.v[1] | b.v[1];
	r.v[2] = a.v[2] | b.v[2];
	r.v[3] = a.v[3] | b.v[3];
	return r;
}

inline int32x4_t veorq_s32(const int32x4_t &a, const int32x4_t &b)
{
	int32x4_t r;
	r.v[0] = a.v[0] ^ b.v[0];
	r.v[1] = a.v[1] ^ b.v[1];
	r.v[2] = a.v[2] ^ b.v[2];
	r.v[3] = a.v[3] ^ b.v[3];
	return r;
}

inline float32x4_t vaddq_f32(const float32x4_t &a, const float32x4_t &b)
{
	float32x4_t r;
	r.v[0] = a.v[0] + b.v[0];
	r.v[1] = a.v[1] + b.v[1];
	r.v[2] = a.v[2] + b.v[2];
	r.v[3] = a.v[3] + b.v[3];
	return r;
}

inline float32x4_t vsubq_f32(const float32x4_t &a, const float32x4_t &b)
{
	float32x4_t r;
	r.v[0] = a.v[0] - b.v[0];
	r.v[1] = a.v[1] - b.v[1];
	r.v[2] = a.v[2] - b.v[2];
	r.v[3] = a.v[3] - b.v[3];
	return r;
}

inline float32x4_t vmulq_f32(const float32x4_t &a, const float32x4_t &b)
{
	float32x4_t r;
	r.v[0] = a.v[0] * b.v[0];
	r.v[1] = a.v[1] * b.v[1];
	r.v[2] = a.v[2] * b.v[2];
	r.v[3] = a.v[3] * b.v[3];
	return r;
}

inline float32x4_t vdivq_f32(const float32x4_t &a, const float32x4_t &b)
{
	float32x4_t r;
	r.v[0] = a.v[0] / b.v[0];
	r.v[1] = a.v[1] / b.v[1];
	r.v[2] = a.v[2] / b.v[2];
	r.v[3] = a.v[3] / b.v[3];
	return r;
}

inline void vandq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t *vt = (uint32x4_t *)&v;
	vt->v[0] &= v2;
	vt->v[1] &= v2;
	vt->v[2] &= v2;
	vt->v[3] &= v2;
}

inline void vorq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t *vt = (uint32x4_t *)&v;
	vt->v[0] |= v2;
	vt->v[1] |= v2;
	vt->v[2] |= v2;
	vt->v[3] |= v2;
}

inline void veorq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t *vt = (uint32x4_t *)&v;
	vt->v[0] ^= v2;
	vt->v[1] ^= v2;
	vt->v[2] ^= v2;
	vt->v[3] ^= v2;
}

inline uint32_t vheor_s32(const int32x4_t &a)
{
	int32_t r;
	r = a.v[0] ^ a.v[1] ^ a.v[2] ^ a.v[3];
	return (uint32_t)r;
}
