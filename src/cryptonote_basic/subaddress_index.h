// Copyright (c) 2019, Ryo Currency Project
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

#pragma once

#include "serialization/keyvalue_serialization.h"
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/version.hpp>
#include <ostream>

namespace cryptonote
{
struct subaddress_index
{
	uint32_t major;
	uint32_t minor;
	bool operator==(const subaddress_index &rhs) const { return !memcmp(this, &rhs, sizeof(subaddress_index)); }
	bool operator!=(const subaddress_index &rhs) const { return !(*this == rhs); }
	bool is_zero() const { return major == 0 && minor == 0; }

	BEGIN_SERIALIZE_OBJECT()
	FIELD(major)
	FIELD(minor)
	END_SERIALIZE()

	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(major)
	KV_SERIALIZE(minor)
	END_KV_SERIALIZE_MAP()
};
}

namespace cryptonote
{
inline std::ostream &operator<<(std::ostream &out, const cryptonote::subaddress_index &subaddr_index)
{
	return out << subaddr_index.major << '/' << subaddr_index.minor;
}
}

namespace std
{
template <>
struct hash<cryptonote::subaddress_index>
{
	size_t operator()(const cryptonote::subaddress_index &index) const
	{
		size_t res;
		if(sizeof(size_t) == 8)
		{
			res = ((uint64_t)index.major << 32) | index.minor;
		}
		else
		{
			// https://stackoverflow.com/a/17017281
			res = 17;
			res = res * 31 + hash<uint32_t>()(index.major);
			res = res * 31 + hash<uint32_t>()(index.minor);
		}
		return res;
	}
};
}

BOOST_CLASS_VERSION(cryptonote::subaddress_index, 0)

namespace boost
{
namespace serialization
{
template <class Archive>
inline void serialize(Archive &a, cryptonote::subaddress_index &x, const boost::serialization::version_type ver)
{
	a &x.major;
	a &x.minor;
}
}
}
