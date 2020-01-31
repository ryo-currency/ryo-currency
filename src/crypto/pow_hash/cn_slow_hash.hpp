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

#include "hw_detect.hpp"
#include <assert.h>
#include <boost/align/aligned_alloc.hpp>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// Macros are for template instantiations
// Cryptonight
#define cn_v1_hash_t cn_slow_hash<2 * 1024 * 1024, 0x80000, 0>
// Cryptonight-heavy
#define cn_v2_hash_t cn_slow_hash<4 * 1024 * 1024, 0x40000, 1>
// Cryptonight-GPU
#define cn_v3_hash_t cn_slow_hash<2 * 1024 * 1024, 0xC000, 2>

// Use the types below
template <size_t MEMORY, size_t ITER, size_t VERSION>
class cn_slow_hash;
using cn_pow_hash_v1 = cn_v1_hash_t;
using cn_pow_hash_v2 = cn_v2_hash_t;
using cn_pow_hash_v3 = cn_v3_hash_t;

#ifdef HAS_INTEL_HW
inline void cpuid(uint32_t eax, int32_t ecx, int32_t val[4])
{
	val[0] = 0;
	val[1] = 0;
	val[2] = 0;
	val[3] = 0;

#if defined(HAS_WIN_INTRIN_API)
	__cpuidex(val, eax, ecx);
#else
	__cpuid_count(eax, ecx, val[0], val[1], val[2], val[3]);
#endif
}

inline bool hw_check_aes()
{
	int32_t cpu_info[4];
	cpuid(1, 0, cpu_info);
	return (cpu_info[2] & (1 << 25)) != 0;
}

inline bool check_avx2()
{
	int32_t cpu_info[4];
	cpuid(7, 0, cpu_info);
	const bool has_avx2 = (cpu_info[1] & (1 << 5)) != 0;
	cpuid(1, 0, cpu_info);
	const bool osxsave = (cpu_info[2] & (1 << 27)) != 0;
	return has_avx2 && osxsave;
}
#endif

#ifdef HAS_ARM_HW
inline bool hw_check_aes()
{
	return (getauxval(AT_HWCAP) & HWCAP_AES) != 0;
}
#endif

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
inline bool hw_check_aes()
{
	return false;
}
#endif

// This cruft avoids casting-galore and allows us not to worry about sizeof(void*)
class cn_sptr
{
  public:
	cn_sptr() :
		base_ptr(nullptr) {}
	cn_sptr(uint64_t *ptr) { base_ptr = ptr; }
	cn_sptr(uint32_t *ptr) { base_ptr = ptr; }
	cn_sptr(uint8_t *ptr) { base_ptr = ptr; }

	inline void set(void *ptr)
	{
		base_ptr = ptr;
	}
	inline cn_sptr offset(size_t i) { return reinterpret_cast<uint8_t *>(base_ptr) + i; }
	inline const cn_sptr offset(size_t i) const { return reinterpret_cast<uint8_t *>(base_ptr) + i; }

	inline void *as_void() { return base_ptr; }
	inline uint8_t &as_byte(size_t i) { return *(reinterpret_cast<uint8_t *>(base_ptr) + i); }
	inline uint8_t *as_byte() { return reinterpret_cast<uint8_t *>(base_ptr); }
	inline uint64_t &as_uqword(size_t i) { return *(reinterpret_cast<uint64_t *>(base_ptr) + i); }
	inline const uint64_t &as_uqword(size_t i) const { return *(reinterpret_cast<uint64_t *>(base_ptr) + i); }
	inline uint64_t *as_uqword() { return reinterpret_cast<uint64_t *>(base_ptr); }
	inline const uint64_t *as_uqword() const { return reinterpret_cast<uint64_t *>(base_ptr); }
	inline int64_t &as_qword(size_t i) { return *(reinterpret_cast<int64_t *>(base_ptr) + i); }
	inline int32_t &as_dword(size_t i) { return *(reinterpret_cast<int32_t *>(base_ptr) + i); }
	inline uint32_t &as_udword(size_t i) { return *(reinterpret_cast<uint32_t *>(base_ptr) + i); }
	inline const uint32_t &as_udword(size_t i) const { return *(reinterpret_cast<uint32_t *>(base_ptr) + i); }

	template <typename cast_t>
	inline cast_t *as_ptr() { return reinterpret_cast<cast_t *>(base_ptr); }

  private:
	void *base_ptr;
};

template <size_t MEMORY, size_t ITER, size_t VERSION>
class cn_slow_hash
{
  public:
	cn_slow_hash() :
		borrowed_pad(false)
	{
		lpad.set(boost::alignment::aligned_alloc(4096, MEMORY));
		spad.set(boost::alignment::aligned_alloc(4096, 4096));
	}

	cn_slow_hash(cn_slow_hash &&other) noexcept :
		lpad(other.lpad.as_byte()), spad(other.spad.as_byte()), borrowed_pad(other.borrowed_pad)
	{
		other.lpad.set(nullptr);
		other.spad.set(nullptr);
	}

	// Factory function enabling to temporaliy turn v2 object into v1
	// It is caller's responsibility to ensure that v2 object is not hashing at the same time!!
	static cn_pow_hash_v1 make_borrowed(cn_pow_hash_v2 &t)
	{
		return cn_pow_hash_v1(t.lpad.as_void(), t.spad.as_void());
	}

	static cn_pow_hash_v3 make_borrowed_v3(cn_pow_hash_v2 &t)
	{
		return cn_pow_hash_v3(t.lpad.as_void(), t.spad.as_void());
	}

	cn_slow_hash &operator=(cn_slow_hash &&other) noexcept
	{
		if(this == &other)
			return *this;

		free_mem();
		lpad.set(other.lpad.as_void());
		spad.set(other.spad.as_void());
		borrowed_pad = other.borrowed_pad;
		return *this;
	}

	// Copying is going to be really inefficient
	cn_slow_hash(const cn_slow_hash &other) = delete;
	cn_slow_hash &operator=(const cn_slow_hash &other) = delete;

	~cn_slow_hash()
	{
		free_mem();
	}

	void hash(const void *in, size_t len, void *out)
	{
		if(VERSION <= 1)
		{
			if(hw_check_aes() && !check_override())
				hardware_hash(in, len, out);
			else
				software_hash(in, len, out);
		}
		else
		{
			if(hw_check_aes() && !check_override())
				hardware_hash_3(in, len, out);
			else
				software_hash_3(in, len, out);
		}
	}

	void software_hash(const void *in, size_t len, void *out);
	void software_hash_3(const void *in, size_t len, void *pout);

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
	inline void hardware_hash(const void *in, size_t len, void *out)
	{
		assert(false);
	}
	inline void hardware_hash_3(const void *in, size_t len, void *out) { assert(false); }
#else
	void hardware_hash(const void *in, size_t len, void *out);
	void hardware_hash_3(const void *in, size_t len, void *pout);
#endif

  private:
	static constexpr size_t MASK = VERSION <= 1 ? ((MEMORY - 1) >> 4) << 4 : ((MEMORY - 1) >> 6) << 6;

	friend cn_pow_hash_v1;
	friend cn_pow_hash_v2;
	friend cn_pow_hash_v3;

	// Constructor enabling v1 hash to borrow v2's buffer
	cn_slow_hash(void *lptr, void *sptr)
	{
		lpad.set(lptr);
		spad.set(sptr);
		borrowed_pad = true;
	}

	inline bool check_override()
	{
		const char *env = getenv("RYO_USE_SOFTWARE_AES");
		if(!env)
		{
			return false;
		}
		else if(!strcmp(env, "0") || !strcmp(env, "no"))
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	inline void free_mem()
	{
		if(!borrowed_pad)
		{
			if(lpad.as_void() != nullptr)
				boost::alignment::aligned_free(lpad.as_void());
			if(lpad.as_void() != nullptr)
				boost::alignment::aligned_free(spad.as_void());
		}

		lpad.set(nullptr);
		spad.set(nullptr);
	}

	inline cn_sptr scratchpad_ptr(uint32_t idx) { return lpad.as_byte() + (idx & MASK); }
	inline cn_sptr scratchpad_ptr(uint32_t idx, size_t n) { return lpad.as_byte() + (idx & MASK) + n * 16; }

#if !defined(HAS_INTEL_HW) && !defined(HAS_ARM_HW)
	inline void explode_scratchpad_hard()
	{
		assert(false);
	}
	inline void implode_scratchpad_hard() { assert(false); }
#else
	void explode_scratchpad_hard();
	void implode_scratchpad_hard();
#endif

	void explode_scratchpad_3();
	void explode_scratchpad_soft();
	void implode_scratchpad_soft();

	void inner_hash_3();
	void inner_hash_3_avx();

	cn_sptr lpad;
	cn_sptr spad;
	bool borrowed_pad;
};

extern template class cn_v1_hash_t;
extern template class cn_v2_hash_t;
extern template class cn_v3_hash_t;
