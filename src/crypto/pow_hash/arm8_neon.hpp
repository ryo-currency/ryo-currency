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
#include <arm_neon.h>

inline void vandq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t vc = vdupq_n_u32(v2);
	v = (float32x4_t)vandq_u32((uint32x4_t)v, vc);
}

inline void vorq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t vc = vdupq_n_u32(v2);
	v = (float32x4_t)vorrq_u32((uint32x4_t)v, vc);
}

inline void veorq_f32(float32x4_t &v, uint32_t v2)
{
	uint32x4_t vc = vdupq_n_u32(v2);
	v = (float32x4_t)veorq_u32((uint32x4_t)v, vc);
}

template <size_t v>
inline void vrot_si32(int32x4_t &r)
{
	r = (int32x4_t)vextq_s8((int8x16_t)r, (int8x16_t)r, v);
}

template <>
inline void vrot_si32<0>(int32x4_t &r)
{
}

inline uint32_t vheor_s32(const int32x4_t &v)
{
	int32x4_t v0 = veorq_s32(v, vrev64q_s32(v));
	int32x2_t vf = veor_s32(vget_high_s32(v0), vget_low_s32(v0));
	return (uint32_t)vget_lane_s32(vf, 0);
}
