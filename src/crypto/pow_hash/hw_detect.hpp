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

#if defined(_WIN32) || defined(_WIN64)
#include <intrin.h>
#include <malloc.h>
#define HAS_WIN_INTRIN_API
#endif

// Note HAS_INTEL_HW and HAS_ARM_HW only mean we can emit the AES instructions
// check CPU support for the hardware AES encryption has to be done at runtime
#if defined(__x86_64__) || defined(__i386__) || defined(_M_X86) || defined(_M_X64)
#ifdef __GNUC__
#include <x86intrin.h>
#if !defined(HAS_WIN_INTRIN_API)
#include <cpuid.h>
#endif // !defined(HAS_WIN_INTRIN_API)
#endif // __GNUC__
#define HAS_INTEL_HW
#endif

#if defined(__arm__) || defined(__aarch64__)
#define HAS_ARM
#endif

#if defined(__aarch64__)
#include <asm/hwcap.h>
#include <sys/auxv.h>
#define HAS_ARM_HW
#endif

#if !defined(_LP64) && !defined(_WIN64)
#define BUILD32
#endif

#if defined(CN_ADD_TARGETS_AND_HEADERS)
#if defined(__aarch64__)
#ifndef __ARM_FEATURE_CRYPTO
#define __ARM_FEATURE_CRYPTO 1
#endif
#ifndef __clang__
#pragma GCC target("+crypto")
#endif
#include "arm8_neon.hpp"
#elif defined(__arm__)
#ifndef __clang__
#pragma GCC target("fpu=vfpv4")
#endif
#include "arm_vfp.hpp"
#elif defined(HAS_INTEL_HW) && defined(INTEL_AVX2)
#ifndef __clang__
#pragma GCC target("aes,avx2")
#endif
#elif defined(HAS_INTEL_HW)
#ifndef __clang__
#pragma GCC target("aes,sse2")
#endif
#endif
#endif
