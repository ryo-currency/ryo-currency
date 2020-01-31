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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#include "net/net_utils_base.h"
#include "p2p/p2p_protocol_defs.h"

namespace boost
{
namespace serialization
{
template <class T, class Archive>
inline void do_serialize(Archive &a, epee::net_utils::network_address &na, T local)
{
	if(typename Archive::is_saving())
		local = na.as<T>();
	a &local;
	if(!typename Archive::is_saving())
		na = local;
}
template <class Archive, class ver_type>
inline void serialize(Archive &a, epee::net_utils::network_address &na, const ver_type ver)
{
	uint8_t type;
	if(typename Archive::is_saving())
		type = na.get_type_id();
	a &type;
	switch(type)
	{
	case epee::net_utils::ipv4_network_address::ID:
		do_serialize(a, na, epee::net_utils::ipv4_network_address{0, 0});
		break;
	default:
		throw std::runtime_error("Unsupported network address type");
	}
}
template <class Archive, class ver_type>
inline void serialize(Archive &a, epee::net_utils::ipv4_network_address &na, const ver_type ver)
{
	uint32_t ip{na.ip()};
	uint16_t port{na.port()};
	a &ip;
	a &port;
	if(!typename Archive::is_saving())
		na = epee::net_utils::ipv4_network_address{ip, port};
}

template <class Archive, class ver_type>
inline void serialize(Archive &a, nodetool::peerlist_entry &pl, const ver_type ver)
{
	a &pl.adr;
	a &pl.id;
	a &pl.last_seen;
}

template <class Archive, class ver_type>
inline void serialize(Archive &a, nodetool::anchor_peerlist_entry &pl, const ver_type ver)
{
	a &pl.adr;
	a &pl.id;
	a &pl.first_seen;
}
} // namespace serialization
} // namespace boost
