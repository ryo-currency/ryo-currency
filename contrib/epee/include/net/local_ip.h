// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#pragma once
#include "cryptonote_config.h"

namespace epee
{
namespace net_utils
{
struct ipv4_net_range
{
	uint32_t address;
	uint32_t netmask;
};

inline uint32_t bits_to_mask(uint32_t bits)
{
	// 2^x-1 or in other words, give me x ones
	uint32_t netmask = (1u << bits) - 1u;
	// Now move those bits to the high end
	netmask <<= 32u - bits;
	// And finally swap endianness to little endian
	return __builtin_bswap32(netmask);
}

inline bool is_ip_local(uint32_t ip)
{
	//Note - we are working in litte endian here
	const ipv4_net_range local_ranges[] = {
		{ 0x0000000a, bits_to_mask(8)  }, // 10.0.0.0/8 
		{ 0x0000a8c0, bits_to_mask(16) }, // 192.168.0.0/16
		{ 0x000010ac, bits_to_mask(12) }  // 172.16.0.0/12
	};

	for(size_t i=0; i < countof(local_ranges); i++)
	{
		if((ip & local_ranges[i].netmask) == local_ranges[i].address)
			return true;
	}

	return false;
}

inline bool is_ip_loopback(uint32_t ip)
{
	// 127.0.0.0/8
	return (ip & 0x000000ff) == 0x0000007f;
}
}
}
