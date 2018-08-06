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

#define GULPS_CAT_MAJOR "basic_util"
#include "common/gulps.hpp"

#include "common/base58.h"
#include "common/dns_utils.h"
#include "common/int-util.h"
#include "crypto/hash.h"
#include "cryptonote_basic_impl.h"
#include "cryptonote_config.h"
#include "cryptonote_format_utils.h"
#include "misc_language.h"
#include "serialization/binary_utils.h"
#include "serialization/container.h"
#include "string_tools.h"

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "cn"

namespace cryptonote
{

struct integrated_address
{
	account_public_address adr;
	crypto::hash8 payment_id;

	BEGIN_SERIALIZE_OBJECT()
	FIELD(adr)
	FIELD(payment_id)
	END_SERIALIZE()

	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE(adr)
	KV_SERIALIZE(payment_id)
	END_KV_SERIALIZE_MAP()
};

struct kurz_address
{
	crypto::public_key m_public_key;

	BEGIN_SERIALIZE_OBJECT()
	FIELD(m_public_key)
	END_SERIALIZE()

	BEGIN_KV_SERIALIZE_MAP()
	KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(m_public_key)
	END_KV_SERIALIZE_MAP()
};

/************************************************************************/
/* Cryptonote helper functions                                          */
/************************************************************************/
//-----------------------------------------------------------------------------------------------
size_t get_min_block_size()
{
	return common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
}
//-----------------------------------------------------------------------------------------------
size_t get_max_tx_size()
{
	return common_config::TRANSACTION_SIZE_LIMIT;
}
//-----------------------------------------------------------------------------------------------
template <network_type NETTYPE>
bool get_dev_fund_amount(uint64_t height, uint64_t& amount)
{
	amount = 0;
	if(height < config<NETTYPE>::DEV_FUND_START)
		return false; // No dev fund output needed because the dev fund didn't start yet

	height -= config<NETTYPE>::DEV_FUND_START;

	if(height / config<NETTYPE>::DEV_FUND_PERIOD >= config<NETTYPE>::DEV_FUND_LENGTH)
		return false; // No dev fund output needed because the dev fund has ended

	if(height % config<NETTYPE>::DEV_FUND_PERIOD != 0)
		return false;  // No dev fund output needed because it isn't on the period

	amount = config<NETTYPE>::DEV_FUND_AMOUNT / config<NETTYPE>::DEV_FUND_LENGTH;
	return true;
}

template bool get_dev_fund_amount<MAINNET>(uint64_t height, uint64_t& amount);
template bool get_dev_fund_amount<TESTNET>(uint64_t height, uint64_t& amount);
template bool get_dev_fund_amount<STAGENET>(uint64_t height, uint64_t& amount);

//-----------------------------------------------------------------------------------------------
bool get_block_reward(network_type nettype, size_t median_size, size_t current_block_size, uint64_t already_generated_coins, uint64_t &reward, uint64_t height)
{
	uint64_t base_reward;
	uint64_t round_factor = 10000000; // 1 * pow(10, 7)
	if(height > 0)
	{
		if(height < (PEAK_COIN_EMISSION_HEIGHT + COIN_EMISSION_HEIGHT_INTERVAL))
		{
			uint64_t interval_num = height / COIN_EMISSION_HEIGHT_INTERVAL;
			double money_supply_pct = 0.1888 + interval_num * (0.023 + interval_num * 0.0032);
			base_reward = ((uint64_t)(MONEY_SUPPLY * money_supply_pct)) >> EMISSION_SPEED_FACTOR;
		}
		else
		{
			base_reward = (MONEY_SUPPLY - already_generated_coins) >> EMISSION_SPEED_FACTOR;
		}
	}
	else
	{
		base_reward = GENESIS_BLOCK_REWARD;
	}

	if(base_reward < FINAL_SUBSIDY)
	{
		if(MONEY_SUPPLY > already_generated_coins)
		{
			base_reward = FINAL_SUBSIDY;
		}
		else
		{
			base_reward = FINAL_SUBSIDY / 2;
		}
	}

	// rounding (floor) base reward
	base_reward = base_reward / round_factor * round_factor;

	//make it soft
	if(median_size < common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE)
	{
		median_size = common_config::CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE;
	}

	if(current_block_size > 2 * median_size)
	{
		MERROR("Block cumulative size is too big: " << current_block_size << ", expected less than " << 2 * median_size);
		return false;
	}

	if(current_block_size <= (median_size < common_config::BLOCK_SIZE_GROWTH_FAVORED_ZONE ? median_size * 110 / 100 : median_size))
	{
		reward = base_reward;
		return true;
	}

	assert(median_size < std::numeric_limits<uint32_t>::max());
	assert(current_block_size < std::numeric_limits<uint32_t>::max());

	uint64_t product_hi;
	// BUGFIX: 32-bit saturation bug (e.g. ARM7), the result was being
	// treated as 32-bit by default.
	uint64_t multiplicand = 2 * median_size - current_block_size;
	multiplicand *= current_block_size;
	uint64_t product_lo = mul128(base_reward, multiplicand, &product_hi);

	uint64_t reward_hi;
	uint64_t reward_lo;
	div128_32(product_hi, product_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
	div128_32(reward_hi, reward_lo, static_cast<uint32_t>(median_size), &reward_hi, &reward_lo);
	assert(0 == reward_hi);
	assert(reward_lo < base_reward);

	reward = reward_lo;
	return true;
}
//------------------------------------------------------------------------------------
uint8_t get_account_address_checksum(const public_address_outer_blob &bl)
{
	const unsigned char *pbuf = reinterpret_cast<const unsigned char *>(&bl);
	uint8_t summ = 0;
	for(size_t i = 0; i != sizeof(public_address_outer_blob) - 1; i++)
		summ += pbuf[i];

	return summ;
}
//------------------------------------------------------------------------------------
uint8_t get_account_integrated_address_checksum(const public_integrated_address_outer_blob &bl)
{
	const unsigned char *pbuf = reinterpret_cast<const unsigned char *>(&bl);
	uint8_t summ = 0;
	for(size_t i = 0; i != sizeof(public_integrated_address_outer_blob) - 1; i++)
		summ += pbuf[i];

	return summ;
}
//-----------------------------------------------------------------------
template <network_type NETTYPE>
std::string get_public_address_as_str(bool subaddress, account_public_address const &adr)
{
	uint64_t address_prefix;
	if(adr.m_spend_public_key == adr.m_view_public_key)
	{
		if(subaddress)
			address_prefix = config<NETTYPE>::RYO_KURZ_SUBADDRESS_BASE58_PREFIX;
		else
			address_prefix = config<NETTYPE>::RYO_KURZ_ADDRESS_BASE58_PREFIX;

		kurz_address kadr = {adr.m_spend_public_key};
		return tools::base58::encode_addr(address_prefix, t_serializable_object_to_blob(kadr));
	}
	else
	{
		if(subaddress)
			address_prefix = config<NETTYPE>::RYO_LONG_SUBADDRESS_BASE58_PREFIX;
		else
			address_prefix = config<NETTYPE>::RYO_LONG_ADDRESS_BASE58_PREFIX;

		return tools::base58::encode_addr(address_prefix, t_serializable_object_to_blob(adr));
	}
}

template std::string get_public_address_as_str<MAINNET>(bool subaddress, const account_public_address &adr);
template std::string get_public_address_as_str<TESTNET>(bool subaddress, const account_public_address &adr);
template std::string get_public_address_as_str<STAGENET>(bool subaddress, const account_public_address &adr);

//-----------------------------------------------------------------------
template <network_type NETTYPE>
std::string get_account_integrated_address_as_str(account_public_address const &adr, crypto::hash8 const &payment_id)
{
	uint64_t integrated_address_prefix = config<NETTYPE>::RYO_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX;

	integrated_address iadr = {adr, payment_id};
	return tools::base58::encode_addr(integrated_address_prefix, t_serializable_object_to_blob(iadr));
}

template std::string get_account_integrated_address_as_str<MAINNET>(account_public_address const &adr, crypto::hash8 const &payment_id);
template std::string get_account_integrated_address_as_str<TESTNET>(account_public_address const &adr, crypto::hash8 const &payment_id);
template std::string get_account_integrated_address_as_str<STAGENET>(account_public_address const &adr, crypto::hash8 const &payment_id);

//-----------------------------------------------------------------------
bool is_coinbase(const transaction &tx)
{
	if(tx.vin.size() != 1)
		return false;

	if(tx.vin[0].type() != typeid(txin_gen))
		return false;

	return true;
}
//-----------------------------------------------------------------------
template <network_type NETTYPE>
bool get_account_address_from_str(address_parse_info &info, std::string const &str)
{
	blobdata data;
	uint64_t prefix;
	if(!tools::base58::decode_addr(str, prefix, data))
	{
		GULPS_ERROR("Invalid address format");
		return false;
	}

	info.has_payment_id = false;
	info.is_subaddress = false;
	info.is_kurz = false;
	switch(prefix)
	{
	case config<NETTYPE>::RYO_LONG_ADDRESS_BASE58_PREFIX:
	case config<NETTYPE>::LEGACY_LONG_ADDRESS_BASE58_PREFIX:
		break;
	case config<NETTYPE>::RYO_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX:
	case config<NETTYPE>::LEGACY_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX:
		info.has_payment_id = true;
		break;
	case config<NETTYPE>::RYO_LONG_SUBADDRESS_BASE58_PREFIX:
	case config<NETTYPE>::LEGACY_LONG_SUBADDRESS_BASE58_PREFIX:
		info.is_subaddress = true;
		break;
	case config<NETTYPE>::RYO_KURZ_ADDRESS_BASE58_PREFIX:
		info.is_kurz = true;
		break;
/* Kurz subaddress are impossible to generate yet, so parsig them should error out
	case config<NETTYPE>::RYO_KURZ_SUBADDRESS_BASE58_PREFIX:
		info.is_subaddress = true;
		info.is_kurz = true;
		break;
*/
	default:
		GULPS_ERROR("Wrong address prefix: "_s + std::to_string(prefix));
		return false;
	}

	if(info.has_payment_id)
	{
		integrated_address iadr;
		if(!::serialization::parse_binary(data, iadr))
		{
			GULPS_ERROR("Account public address keys can't be parsed");
			return false;
		}
		info.address = iadr.adr;
		info.payment_id = iadr.payment_id;
	}
	else if(info.is_kurz)
	{
		kurz_address kadr;
		if(!::serialization::parse_binary(data, kadr))
		{
			GULPS_ERROR("Account public address keys can't be parsed");
			return false;
		}

		info.address.m_spend_public_key = kadr.m_public_key;
		info.address.m_view_public_key = kadr.m_public_key;
	}
	else
	{
		if(!::serialization::parse_binary(data, info.address))
		{
			GULPS_ERROR("Account public address keys can't be parsed");
			return false;
		}
	}

	if(!crypto::check_key(info.address.m_spend_public_key) || !crypto::check_key(info.address.m_view_public_key))
	{
		GULPS_ERROR("Failed to validate address keys");
		return false;
	}

	return true;
}

template bool get_account_address_from_str<MAINNET>(address_parse_info &, std::string const &, const bool);
template bool get_account_address_from_str<TESTNET>(address_parse_info &, std::string const &, const bool);
template bool get_account_address_from_str<STAGENET>(address_parse_info &, std::string const &, const bool);

//--------------------------------------------------------------------------------
bool operator==(const cryptonote::transaction &a, const cryptonote::transaction &b)
{
	return cryptonote::get_transaction_hash(a) == cryptonote::get_transaction_hash(b);
}

bool operator==(const cryptonote::block &a, const cryptonote::block &b)
{
	return cryptonote::get_block_hash(a) == cryptonote::get_block_hash(b);
}
}

//--------------------------------------------------------------------------------
bool parse_hash256(const std::string str_hash, crypto::hash &hash)
{
	std::string buf;
	bool res = epee::string_tools::parse_hexstr_to_binbuff(str_hash, buf);
	if(!res || buf.size() != sizeof(crypto::hash))
	{
		std::cout << "invalid hash format: <" << str_hash << '>' << std::endl;
		return false;
	}
	else
	{
		buf.copy(reinterpret_cast<char *>(&hash), sizeof(crypto::hash));
		return true;
	}
}
