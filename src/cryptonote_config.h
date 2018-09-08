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

#pragma once

#include <assert.h>
#include <boost/uuid/uuid.hpp>
#include <string>

#define CRYPTONOTE_DNS_TIMEOUT_MS 20000

#define CRYPTONOTE_GETBLOCKTEMPLATE_MAX_BLOCK_SIZE 196608 //size of block (bytes) that is the maximum that miners will produce
#define CRYPTONOTE_PUBLIC_ADDRESS_TEXTBLOB_VER 0
#define CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW 60
#define CURRENT_TRANSACTION_VERSION 3
#define MIN_TRANSACTION_VERSION 2
#define MAX_TRANSACTION_VERSION 3
#define CRYPTONOTE_V2_POW_BLOCK_VERSION 3
#define CRYPTONOTE_DEFAULT_TX_SPENDABLE_AGE 10

// MONEY_SUPPLY - total number coins to be generated
#define MONEY_SUPPLY ((uint64_t)88888888000000000)
#define EMISSION_SPEED_FACTOR 19
#define FINAL_SUBSIDY ((uint64_t)4000000000)			  // 4 * pow(10, 9)
#define GENESIS_BLOCK_REWARD ((uint64_t)8800000000000000) // ~10% dev premine, now  mostly burned
#define EMISSION_SPEED_FACTOR_PER_MINUTE (20)

#define CRYPTONOTE_REWARD_BLOCKS_WINDOW 60

#define CRYPTONOTE_COINBASE_BLOB_RESERVED_SIZE 600
#define CRYPTONOTE_DISPLAY_DECIMAL_POINT 9

#define CRYPTONOTE_LOCKED_TX_ALLOWED_DELTA_BLOCKS 1

#define DIFFICULTY_BLOCKS_ESTIMATE_TIMESPAN common_config::DIFFICULTY_TARGET //just alias; used by tests

#define BLOCKS_IDS_SYNCHRONIZING_DEFAULT_COUNT 10000 //by default, blocks ids count in synchronizing
#define BLOCKS_SYNCHRONIZING_DEFAULT_COUNT 10		 //by default, blocks count in blocks downloading
#define CRYPTONOTE_PROTOCOL_HOP_RELAX_COUNT 3		 //value of hop, after which we use only announce of new block

#define CRYPTONOTE_MEMPOOL_TX_LIVETIME 86400				 //seconds, one day
#define CRYPTONOTE_MEMPOOL_TX_FROM_ALT_BLOCK_LIVETIME 604800 //seconds, one week

#define COMMAND_RPC_GET_BLOCKS_FAST_MAX_COUNT 250

#define P2P_LOCAL_WHITE_PEERLIST_LIMIT 1000
#define P2P_LOCAL_GRAY_PEERLIST_LIMIT 5000

#define P2P_DEFAULT_CONNECTIONS_COUNT 8
#define P2P_DEFAULT_HANDSHAKE_INTERVAL 60	//seconds
#define P2P_DEFAULT_PACKET_MAX_SIZE 50000000 //50000000 bytes maximum packet size
#define P2P_DEFAULT_PEERS_IN_HANDSHAKE 250
#define P2P_DEFAULT_CONNECTION_TIMEOUT 5000		  //5 seconds
#define P2P_DEFAULT_PING_CONNECTION_TIMEOUT 2000  //2 seconds
#define P2P_DEFAULT_INVOKE_TIMEOUT 60 * 2 * 1000  //2 minutes
#define P2P_DEFAULT_HANDSHAKE_INVOKE_TIMEOUT 5000 //5 seconds
#define P2P_DEFAULT_WHITELIST_CONNECTIONS_PERCENT 70
#define P2P_DEFAULT_ANCHOR_CONNECTIONS_COUNT 2

#define P2P_FAILED_ADDR_FORGET_SECONDS (60 * 60) //1 hour
#define P2P_IP_BLOCKTIME (60 * 60 * 24)			 //24 hour
#define P2P_IP_FAILS_BEFORE_BLOCK 10
#define P2P_IDLE_CONNECTION_KILL_INTERVAL (5 * 60) //5 minutes

#define P2P_SUPPORT_FLAG_FLUFFY_BLOCKS 0x01
#define P2P_SUPPORT_FLAGS P2P_SUPPORT_FLAG_FLUFFY_BLOCKS

#define ALLOW_DEBUG_COMMANDS

#define CRYPTONOTE_NAME "ryo"
#define CRYPTONOTE_POOLDATA_FILENAME "poolstate.bin"
#define CRYPTONOTE_BLOCKCHAINDATA_FILENAME "data.mdb"
#define CRYPTONOTE_BLOCKCHAINDATA_LOCK_FILENAME "lock.mdb"
#define P2P_NET_DATA_FILENAME "p2pstate.bin"
#define MINER_CONFIG_FILE_NAME "miner_conf.json"

#define THREAD_STACK_SIZE 5 * 1024 * 1024

#define DEFAULT_MIXIN 12 // default & minimum mixin allowed
#define MAX_MIXIN 240

#define PER_KB_FEE_QUANTIZATION_DECIMALS 8

#define HASH_OF_HASHES_STEP 256

#define DEFAULT_TXPOOL_MAX_SIZE 648000000ull // 3 days at 300000, in bytes

// coin emission change interval/speed configs
#define COIN_EMISSION_MONTH_INTERVAL 6																										// months to change emission speed
#define COIN_EMISSION_HEIGHT_INTERVAL ((uint64_t)(COIN_EMISSION_MONTH_INTERVAL * (30.4375 * 24 * 3600) / common_config::DIFFICULTY_TARGET)) // calculated to # of heights to change emission speed
#define PEAK_COIN_EMISSION_YEAR 4
#define PEAK_COIN_EMISSION_HEIGHT ((uint64_t)(((12 * 30.4375 * 24 * 3600) / common_config::DIFFICULTY_TARGET) * PEAK_COIN_EMISSION_YEAR)) // = (# of heights emmitted per year) * PEAK_COIN_EMISSION_YEAR

#define TX_FORK_ID_STR "ryo-currency"

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

// New constants are intended to go here
namespace cryptonote
{
enum network_type : uint8_t
{
	MAINNET = 0,
	TESTNET,
	STAGENET,
	FAKECHAIN,
	UNDEFINED = 255
};

enum hard_fork_feature
{
	FORK_V2_DIFFICULTY,
	FORK_V3_DIFFICULTY,
	FORK_FIXED_FEE,
	FORK_NEED_V3_TXES,
	FORK_BULLETPROOFS,
	FORK_STRICT_TX_SEMANTICS,
	FORK_DEV_FUND,
	FORK_FEE_V2
};

struct hardfork_conf
{
	static constexpr uint8_t FORK_ID_DISABLED = 0xff;

	hard_fork_feature ft;
	uint8_t mainnet;
	uint8_t testnet;
	uint8_t stagenet;
};

static constexpr hardfork_conf FORK_CONFIG[] = {
	{FORK_V2_DIFFICULTY, 2, 2, 1},
	{FORK_V3_DIFFICULTY, 4, 4, 1},
	{FORK_FIXED_FEE, 4, 4, 1},
	{FORK_NEED_V3_TXES, 4, 4, 1},
	{FORK_STRICT_TX_SEMANTICS, 5, 5, 1},
	{FORK_DEV_FUND, 5, 5, 1},
	{FORK_FEE_V2, 5, 6, 1},
	{FORK_BULLETPROOFS, hardfork_conf::FORK_ID_DISABLED, hardfork_conf::FORK_ID_DISABLED, 1}
};

// COIN - number of smallest units in one coin
inline constexpr uint64_t MK_COINS(uint64_t coins) { return coins * 1000000000ull; }  // pow(10, 9)

struct common_config
{
	static constexpr uint64_t POISSON_CHECK_TRIGGER = 10;  // Reorg size that triggers poisson timestamp check
	static constexpr uint64_t POISSON_CHECK_DEPTH = 60;   // Main-chain depth of the poisson check. The attacker will have to tamper 50% of those blocks
	static constexpr double POISSON_LOG_P_REJECT = -75.0; // Reject reorg if the probablity that the timestamps are genuine is below e^x, -75 = 10^-33

	static constexpr uint64_t DIFFICULTY_TARGET = 240; // 4 minutes

	/////////////// V1 difficulty constants
	static constexpr uint64_t DIFFICULTY_WINDOW_V1 = 720; // blocks
	static constexpr uint64_t DIFFICULTY_LAG_V1 = 15;	 // !!!
	static constexpr uint64_t DIFFICULTY_CUT_V1 = 60;	 // timestamps to cut after sorting
	static constexpr uint64_t DIFFICULTY_BLOCKS_COUNT_V1 = DIFFICULTY_WINDOW_V1 + DIFFICULTY_LAG_V1;
	static constexpr uint64_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V1 = 60;

	/////////////// V2 difficulty constants
	static constexpr uint64_t DIFFICULTY_WINDOW_V2 = 17;
	static constexpr uint64_t DIFFICULTY_CUT_V2 = 6;
	static constexpr uint64_t DIFFICULTY_BLOCKS_COUNT_V2 = DIFFICULTY_WINDOW_V2 + DIFFICULTY_CUT_V2 * 2;
	static constexpr uint64_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V2 = 12;
	static constexpr uint64_t BLOCK_FUTURE_TIME_LIMIT_V2 = 60 * 24;

	/////////////// V3 difficulty constants
	static constexpr uint64_t DIFFICULTY_WINDOW_V3 = 60 + 1;
	static constexpr uint64_t DIFFICULTY_BLOCKS_COUNT_V3 = DIFFICULTY_WINDOW_V3;
	static constexpr uint64_t BLOCKCHAIN_TIMESTAMP_CHECK_WINDOW_V3 = 11;
	static constexpr uint64_t BLOCK_FUTURE_TIME_LIMIT_V3 = DIFFICULTY_TARGET * 3;

	static constexpr uint64_t CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE = 240 * 1024; // 240 kB
	static constexpr uint64_t BLOCK_SIZE_GROWTH_FAVORED_ZONE = CRYPTONOTE_BLOCK_GRANTED_FULL_REWARD_ZONE * 4;
	static constexpr uint64_t TRANSACTION_SIZE_LIMIT = 300 * 1024;			// 300 kB
	static constexpr uint64_t BLOCK_SIZE_LIMIT_ABSOLUTE = 16 * 1024 * 1024; // 16 MB
	static constexpr uint64_t FEE_PER_KB = 500000;
	static constexpr uint64_t FEE_PER_RING_MEMBER = 500000;
	static constexpr uint64_t DYNAMIC_FEE_PER_KB_BASE_FEE = 500000;				  // 0.0005 * pow(10, 9)
	static constexpr uint64_t DYNAMIC_FEE_PER_KB_BASE_BLOCK_REWARD = 64000000000; // 64 * pow(10, 9)

	static constexpr uint64_t CRYPTONOTE_MAX_BLOCK_NUMBER = 500000000;

	///////////////// Dev fund constants
	// 2 out of 3 multisig address held by fireice, mosu, and psychocrypt
	static constexpr const char* DEV_FUND_ADDRESS = "RYoLshTYzNEaizPi9jWtRtNPtan5fAqc3TVbyzZDWghLY99eXivKD1XQMseVyJQ1kD5FXDvk4nHqpUXBTMCm5aqmQU8tHaN51Wc";
	// 34d6b7155d99da44c3a73424c60ecb0da53d228ed8da026df00ed275ea54e803
	static constexpr const char* DEV_FUND_VIEWKEY = "\x34\xd6\xb7\x15\x5d\x99\xda\x44\xc3\xa7\x34\x24\xc6\x0e\xcb\x0d\xa5\x3d\x22\x8e\xd8\xda\x02\x6d\xf0\x0e\xd2\x75\xea\x54\xe8\x03";
	// Exact number of coins burned in the premine burn, in atomic units
	static constexpr uint64_t PREMINE_BURN_AMOUNT = 8700051446427001;
	// Ryo donation address
	static constexpr const char* RYO_DONATION_ADDR = "RYoLshssqU9WvHMwAmt4j6dtpgRERDqwzSiHF4V9nEb5YWmQ5pLSkJC9QudNseKrxBacKtQuLWhpSQ6GLXgyDWjKAGjNXH72VDJ";
};

template <network_type type>
struct config : public common_config
{
};

template <>
struct config<MAINNET>
{
	static constexpr uint64_t LEGACY_LONG_ADDRESS_BASE58_PREFIX = 0x2bb39a;			   // Sumo
	static constexpr uint64_t LEGACY_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x29339a; //Sumi
	static constexpr uint64_t LEGACY_LONG_SUBADDRESS_BASE58_PREFIX = 0x8319a;		   // Subo

	static constexpr uint64_t RYO_KURZ_SUBADDRESS_BASE58_PREFIX = 0x3fe192;			// RYo
	static constexpr uint64_t RYO_KURZ_ADDRESS_BASE58_PREFIX = 0x2c6192;			// RYoK
	static constexpr uint64_t RYO_LONG_ADDRESS_BASE58_PREFIX = 0x2ce192;			// RYoL
	static constexpr uint64_t RYO_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x2de192; // RYoN
	static constexpr uint64_t RYO_LONG_SUBADDRESS_BASE58_PREFIX = 0x2fe192;			// RYoS

	static constexpr uint16_t P2P_DEFAULT_PORT = 12210;
	static constexpr uint16_t RPC_DEFAULT_PORT = 12211;
	static constexpr uint16_t ZMQ_RPC_DEFAULT_PORT = 12212;

	//Random UUID generated from radioactive cs-137 ( http://www.fourmilab.ch/hotbits/how3.html ) it gives me a nice warm feeling =) 
	static constexpr boost::uuids::uuid NETWORK_ID = { { 0xcd, 0xac, 0x50, 0x2e, 0xb3, 0x74, 0x8f, 0xf2, 0x0f, 0xb7, 0x72, 0x18, 0x0f, 0x73, 0x24, 0x13 } }; 

	static constexpr const char *GENESIS_TX =
		"023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
	static constexpr uint32_t GENESIS_NONCE = 10000;

	////////////////////// Dev fund constants
	// How ofen do we add the dev reward
	static constexpr uint64_t DEV_FUND_PERIOD = 15 * 24 * 7; // 1 week
	static constexpr uint64_t DEV_FUND_AMOUNT = MK_COINS(8000000);
	static constexpr uint64_t DEV_FUND_LENGTH = 52 * 6; // 6 years
	static constexpr uint64_t DEV_FUND_START  = 161500;
};

template <>
struct config<TESTNET>
{
	static constexpr uint64_t LEGACY_LONG_ADDRESS_BASE58_PREFIX = 0x37751a;			   // Suto
	static constexpr uint64_t LEGACY_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
	static constexpr uint64_t LEGACY_LONG_SUBADDRESS_BASE58_PREFIX = 0x1d351a;		   // Susu

	static constexpr uint64_t RYO_KURZ_SUBADDRESS_BASE58_PREFIX = 0x2ae192;			// RYoG
	static constexpr uint64_t RYO_KURZ_ADDRESS_BASE58_PREFIX = 0x2b6192;			// RYoH
	static constexpr uint64_t RYO_LONG_ADDRESS_BASE58_PREFIX = 0x306192;			// RYoT
	static constexpr uint64_t RYO_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x29e192; // RYoE
	static constexpr uint64_t RYO_LONG_SUBADDRESS_BASE58_PREFIX = 0x30e192;			// RYoU

	static constexpr uint16_t P2P_DEFAULT_PORT = 13310; 
	static constexpr uint16_t RPC_DEFAULT_PORT = 13311; 
	static constexpr uint16_t ZMQ_RPC_DEFAULT_PORT = 13312; 
 
	static constexpr boost::uuids::uuid NETWORK_ID = { { 0x6f, 0x81, 0x7d, 0x7e, 0xa2, 0x0b, 0x71, 0x77, 0x22, 0xc8, 0xd2, 0xff, 0x02, 0x5d, 0xe9, 0x92 } }; 

	static constexpr const char *GENESIS_TX =
		"023c01ff0001808098d0daf1d00f028be379aa57a70fa19c0ee5765fdc3d2aae0b1034158f4963e157d9042c24fbec21013402fc7071230f1f86f33099119105a7b1f64a898526060ab871e685059c223100";
	static constexpr uint32_t GENESIS_NONCE = 10001;
	
	////////////////////// Dev fund constants
	// How ofen do we add the dev reward
	static constexpr uint64_t DEV_FUND_PERIOD = 15 * 24; // 1 day
	static constexpr uint64_t DEV_FUND_AMOUNT = MK_COINS(8000000);
	static constexpr uint64_t DEV_FUND_LENGTH = 7 * 52 * 6; // 6 years (one day period)
	static constexpr uint64_t DEV_FUND_START  = 129750;
};

template <>
struct config<STAGENET>
{
	static constexpr uint64_t LEGACY_LONG_ADDRESS_BASE58_PREFIX = 0x37751a;			   // Suto
	static constexpr uint64_t LEGACY_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x34f51a; // Suti
	static constexpr uint64_t LEGACY_LONG_SUBADDRESS_BASE58_PREFIX = 0x1d351a;		   // Susu

	static constexpr uint64_t RYO_KURZ_SUBADDRESS_BASE58_PREFIX = 0xdc2192;			  // RYosG
	static constexpr uint64_t RYO_KURZ_ADDRESS_BASE58_PREFIX = 0x1fc2192;			  // RYosK
	static constexpr uint64_t RYO_LONG_ADDRESS_BASE58_PREFIX = 0xd1c2192;			  // RYosT
	static constexpr uint64_t RYO_LONG_INTEGRATED_ADDRESS_BASE58_PREFIX = 0x1fbbe192; // RYosE
	static constexpr uint64_t RYO_LONG_SUBADDRESS_BASE58_PREFIX = 0xe3c2192;		  // RYosU

	static constexpr uint16_t P2P_DEFAULT_PORT = 14410; 
	static constexpr uint16_t RPC_DEFAULT_PORT = 14411; 
	static constexpr uint16_t ZMQ_RPC_DEFAULT_PORT = 14412; 
 
	static constexpr boost::uuids::uuid NETWORK_ID = { { 0x15, 0x77, 0x3a, 0x26, 0x42, 0xa0, 0x3f, 0xf3, 0xe5, 0x79, 0x72, 0x8d, 0x4e, 0x5a, 0xf2, 0x98 } }; 

	static constexpr const char *GENESIS_TX =
		"013c01ff0001ffffffffffff0302df5d56da0c7d643ddd1ce61901c7bdc5fb1738bfe39fbe69c28a3a7032729c0f2101168d0c4ca86fb55a4cf6a36d31431be1c53a3bd7411bb24e8832410289fa6f3b";
	static constexpr uint32_t GENESIS_NONCE = 10002;
	
	////////////////////// Dev fund constants
	// How ofen do we add the dev reward
	static constexpr uint64_t DEV_FUND_PERIOD = 15 * 24; // 1 day
	static constexpr uint64_t DEV_FUND_AMOUNT = MK_COINS(8000000);
	static constexpr uint64_t DEV_FUND_LENGTH = 7 * 52 * 6; // 6 years (one day period)
	static constexpr uint64_t DEV_FUND_START  = 129750;
};

extern template struct config<MAINNET>;
extern template struct config<TESTNET>;
extern template struct config<STAGENET>;

inline uint16_t config_get_p2p_port(network_type type)
{
	switch(type)
	{
	case MAINNET:
		return config<MAINNET>::P2P_DEFAULT_PORT;
	case STAGENET:
		return config<STAGENET>::P2P_DEFAULT_PORT;
	case TESTNET:
		return config<TESTNET>::P2P_DEFAULT_PORT;
	default:
		return 0;
	}
}

inline uint16_t config_get_rpc_port(network_type type)
{
	switch(type)
	{
	case MAINNET:
		return config<MAINNET>::RPC_DEFAULT_PORT;
	case STAGENET:
		return config<STAGENET>::RPC_DEFAULT_PORT;
	case TESTNET:
		return config<TESTNET>::RPC_DEFAULT_PORT;
	default:
		return 0;
	}
}

inline uint8_t get_fork_v(network_type nt, hard_fork_feature ft)
{
	for(size_t i = 0; i < countof(FORK_CONFIG); i++)
	{
		if(FORK_CONFIG[i].ft == ft)
		{
			switch(nt)
			{
			case MAINNET:
				return FORK_CONFIG[i].mainnet;
			case TESTNET:
				return FORK_CONFIG[i].testnet;
			case STAGENET:
				return FORK_CONFIG[i].stagenet;
			default:
				assert(false);
			}
		}
	}

	assert(false);
	return hardfork_conf::FORK_ID_DISABLED;
}
}
