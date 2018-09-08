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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "include_base_utils.h"

using namespace epee;

#include "checkpoints.h"

#include "common/dns_utils.h"
#include "include_base_utils.h"
#include "serialization/keyvalue_serialization.h"
#include "storages/portable_storage_template_helper.h" // epee json include
#include "string_tools.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "checkpoints"

namespace cryptonote
{
/**
   * @brief struct for loading a checkpoint from json
   */
struct t_hashline
{
	uint64_t height;  //!< the height of the checkpoint
	std::string hash; //!< the hash for the checkpoint
	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(height)
	KV_SERIALIZE(hash)
	END_KV_SERIALIZE_MAP()
};

/**
   * @brief struct for loading many checkpoints from json
   */
struct t_hash_json
{
	std::vector<t_hashline> hashlines; //!< the checkpoint lines from the file
	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(hashlines)
	END_KV_SERIALIZE_MAP()
};

//---------------------------------------------------------------------------
checkpoints::checkpoints()
{
}
//---------------------------------------------------------------------------
bool checkpoints::add_checkpoint(uint64_t height, const std::string &hash_str)
{
	crypto::hash h = crypto::null_hash;
	bool r = epee::string_tools::parse_tpod_from_hex_string(hash_str, h);
	CHECK_AND_ASSERT_MES(r, false, "Failed to parse checkpoint hash string into binary representation!");

	// return false if adding at a height we already have AND the hash is different
	if(m_points.count(height))
	{
		CHECK_AND_ASSERT_MES(h == m_points[height], false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
	}
	m_points[height] = h;
	return true;
}
//---------------------------------------------------------------------------
bool checkpoints::is_in_checkpoint_zone(uint64_t height) const
{
	return !m_points.empty() && (height <= (--m_points.end())->first);
}
//---------------------------------------------------------------------------
bool checkpoints::check_block(uint64_t height, const crypto::hash &h, bool &is_a_checkpoint) const
{
	auto it = m_points.find(height);
	is_a_checkpoint = it != m_points.end();
	if(!is_a_checkpoint)
		return true;

	if(it->second == h)
	{
		MINFO("CHECKPOINT PASSED FOR HEIGHT " << height << " " << h);
		return true;
	}
	else
	{
		MWARNING("CHECKPOINT FAILED FOR HEIGHT " << height << ". EXPECTED HASH: " << it->second << ", FETCHED HASH: " << h);
		return false;
	}
}
//---------------------------------------------------------------------------
bool checkpoints::check_block(uint64_t height, const crypto::hash &h) const
{
	bool ignored;
	return check_block(height, h, ignored);
}
//---------------------------------------------------------------------------
//FIXME: is this the desired behavior?
bool checkpoints::is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const
{
	if(0 == block_height)
		return false;

	auto it = m_points.upper_bound(blockchain_height);
	// Is blockchain_height before the first checkpoint?
	if(it == m_points.begin())
		return true;

	--it;
	uint64_t checkpoint_height = it->first;
	return checkpoint_height < block_height;
}
//---------------------------------------------------------------------------
uint64_t checkpoints::get_max_height() const
{
	std::map<uint64_t, crypto::hash>::const_iterator highest =
		std::max_element(m_points.begin(), m_points.end(),
						 (boost::bind(&std::map<uint64_t, crypto::hash>::value_type::first, _1) <
						  boost::bind(&std::map<uint64_t, crypto::hash>::value_type::first, _2)));
	return highest->first;
}
//---------------------------------------------------------------------------
const std::map<uint64_t, crypto::hash> &checkpoints::get_points() const
{
	return m_points;
}

bool checkpoints::check_for_conflicts(const checkpoints &other) const
{
	for(auto &pt : other.get_points())
	{
		if(m_points.count(pt.first))
		{
			CHECK_AND_ASSERT_MES(pt.second == m_points.at(pt.first), false, "Checkpoint at given height already exists, and hash for new checkpoint was different!");
		}
	}
	return true;
}

bool checkpoints::init_default_checkpoints(network_type nettype)
{
	if(nettype == TESTNET)
	{
		ADD_CHECKPOINT(1,      "770309bd74bcebf90eb6f1197ad51601f917314088c5b1343316a4f581c51193");
		ADD_CHECKPOINT(10,     "d5f139ed1cdc14ff2dda722de9ee2236f4030d18670e4ef6b41ab74e57a0a816");
		ADD_CHECKPOINT(50000,  "3d251e3802cc5f6721babf8762bc13fdbc57df694c59759f3baba8d28b7d97c0");
		ADD_CHECKPOINT(122670, "6ba1f78526997681363b2800c942fc56d3a92ac462e3393ba27d0c5611614193"); // ryo
		ADD_CHECKPOINT(129749, "8393ff500fd806fd2364416076b7231af623d614dc080bfd1c7d59102c14c25a");
		return true;
	}
	if(nettype == STAGENET)
	{
		return true;
	}
	else
	{
		ADD_CHECKPOINT(1,      "82e8f378ea29d152146b6317903249751b809e97c0b6655f86e120b9de95c38a");
		ADD_CHECKPOINT(10,     "e097b62bba41e5fd583d3a68de074cddd77c85a6158b031d963232146494a2d6");
		ADD_CHECKPOINT(100,    "f3bd44c626cc12d449183ca84b58615d792523ba229385ff6717ab29a3e88926");
		ADD_CHECKPOINT(1000,   "d284c992cb570f86c2e0bcfaa552b1d73bd40417e1c2a40f82bc6432217f0873");
		ADD_CHECKPOINT(3000,   "81e040955b710dc5a5056668c4eaf3fbc4da2f72c0a63763250ede32a92e0f06");
		ADD_CHECKPOINT(5000,   "e838c077bc66356d9bb321d4eb60f0851ef766f0619ddc4c6568a0f149aacea0");
		ADD_CHECKPOINT(10000,  "360b96c3d0a5202c548672d550700d982ca15ad5627f70bce0a89dda840b3611");
		ADD_CHECKPOINT(20000,  "603a45b60dd92ef4524c80d58411d09480b4668c54bc08dd651d838832bd399e");
		ADD_CHECKPOINT(21300,  "d0a76e98ebb4d8e928d931a1807bba11a2fafdf544c119761b0ed6de4e1898cf"); // v2 fork
		ADD_CHECKPOINT(50000,  "ae36641cf06ed788375bfb32f0038cbcd98f1d7bfb09937148fb1a57c6b52dd8");
		ADD_CHECKPOINT(75000,  "b26f4e1225569da282b77659020bace52e5e89abbdee33e9e52266b1e71803a5");
		ADD_CHECKPOINT(100000, "ffe474fe8353f90700c8138ddea3547d5c1e4a6facb1df85897e7a6e4daab540");
		ADD_CHECKPOINT(116520, "da1cb8f30305cd5fad7d6c33b3ed88fede053e0926102d8e544b65e93e33a08b"); // v3 fork
		ADD_CHECKPOINT(137500, "86a5ccbf6ef872e074a63440b980f0543b3177745bf2c6fd268f93164a277e8b"); // ryo fork
		ADD_CHECKPOINT(137512, "008f4867478e8060352610b145055ad81b655a0f082f350505def8267275a1bd");
		ADD_CHECKPOINT(150000, "debc58ac64fc8dc7ae35b0b8782ae33a0fe5855dc85930bc52b8fec3c1aee94a"); // v4 fork
		ADD_CHECKPOINT(161500, "bd612bd580e3add2c41a2a3222bca09ff1d16755405c0d11e4c00dec637e574d"); // v5 fork
	}

	return true;
}

bool checkpoints::load_checkpoints_from_json(const std::string &json_hashfile_fullpath)
{
	boost::system::error_code errcode;
	if(!(boost::filesystem::exists(json_hashfile_fullpath, errcode)))
	{
		LOG_PRINT_L1("Blockchain checkpoints file not found");
		return true;
	}

	LOG_PRINT_L1("Adding checkpoints from blockchain hashfile");

	uint64_t prev_max_height = get_max_height();
	LOG_PRINT_L1("Hard-coded max checkpoint height is " << prev_max_height);
	t_hash_json hashes;
	if(!epee::serialization::load_t_from_json_file(hashes, json_hashfile_fullpath))
	{
		MERROR("Error loading checkpoints from " << json_hashfile_fullpath);
		return false;
	}
	for(std::vector<t_hashline>::const_iterator it = hashes.hashlines.begin(); it != hashes.hashlines.end();)
	{
		uint64_t height;
		height = it->height;
		if(height <= prev_max_height)
		{
			LOG_PRINT_L1("ignoring checkpoint height " << height);
		}
		else
		{
			std::string blockhash = it->hash;
			LOG_PRINT_L1("Adding checkpoint height " << height << ", hash=" << blockhash);
			ADD_CHECKPOINT(height, blockhash);
		}
		++it;
	}

	return true;
}

bool checkpoints::load_checkpoints_from_dns(network_type nettype)
{
	return true; //Temporarily disabled

	std::vector<std::string> records;

	// All four MoneroPulse domains have DNSSEC on and valid
	static const std::vector<std::string> dns_urls = {"checkpoints.moneropulse.se", "checkpoints.moneropulse.org", "checkpoints.moneropulse.net", "checkpoints.moneropulse.co"};

	static const std::vector<std::string> testnet_dns_urls = {"testpoints.moneropulse.se", "testpoints.moneropulse.org", "testpoints.moneropulse.net", "testpoints.moneropulse.co"};

	static const std::vector<std::string> stagenet_dns_urls = {"stagenetpoints.moneropulse.se", "stagenetpoints.moneropulse.org", "stagenetpoints.moneropulse.net", "stagenetpoints.moneropulse.co"};

	if(!tools::dns_utils::load_txt_records_from_dns(records, nettype == TESTNET ? testnet_dns_urls : nettype == STAGENET ? stagenet_dns_urls : dns_urls))
		return true; // why true ?

	for(const auto &record : records)
	{
		auto pos = record.find(":");
		if(pos != std::string::npos)
		{
			uint64_t height;
			crypto::hash hash;

			// parse the first part as uint64_t,
			// if this fails move on to the next record
			std::stringstream ss(record.substr(0, pos));
			if(!(ss >> height))
			{
				continue;
			}

			// parse the second part as crypto::hash,
			// if this fails move on to the next record
			std::string hashStr = record.substr(pos + 1);
			if(!epee::string_tools::parse_tpod_from_hex_string(hashStr, hash))
			{
				continue;
			}

			ADD_CHECKPOINT(height, hashStr);
		}
	}
	return true;
}

bool checkpoints::load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype, bool dns)
{
	bool result;

	result = load_checkpoints_from_json(json_hashfile_fullpath);
	if(dns)
	{
		result &= load_checkpoints_from_dns(nettype);
	}

	return result;
}
}
