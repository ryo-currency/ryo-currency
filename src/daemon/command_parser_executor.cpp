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

#include "daemon/command_parser_executor.h"
#include "common/dns_utils.h"
#include "version.h"

#include "common/gulps.hpp"

namespace daemonize
{

t_command_parser_executor::t_command_parser_executor(
	uint32_t ip, uint16_t port, const boost::optional<tools::login> &login, bool is_rpc, cryptonote::core_rpc_server *rpc_server) :
	m_executor(ip, port, login, is_rpc, rpc_server)
{
}

bool t_command_parser_executor::print_peer_list(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_peer_list();
}

bool t_command_parser_executor::print_peer_list_stats(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_peer_list_stats();
}

bool t_command_parser_executor::save_blockchain(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.save_blockchain();
}

bool t_command_parser_executor::show_hash_rate(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.show_hash_rate();
}

bool t_command_parser_executor::hide_hash_rate(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.hide_hash_rate();
}

bool t_command_parser_executor::show_difficulty(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.show_difficulty();
}

bool t_command_parser_executor::show_status(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.show_status();
}

bool t_command_parser_executor::print_connections(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_connections();
}

bool t_command_parser_executor::print_blockchain_info(const std::vector<std::string> &args)
{
	if(!args.size())
	{
		GULPS_PRINT("need block index parameter");
		return false;
	}
	uint64_t start_index = 0;
	uint64_t end_index = 0;
	if(!epee::string_tools::get_xtype_from_string(start_index, args[0]))
	{
		GULPS_PRINT("wrong starter block index parameter");
		return false;
	}
	if(args.size() > 1 && !epee::string_tools::get_xtype_from_string(end_index, args[1]))
	{
		GULPS_PRINT("wrong end block index parameter");
		return false;
	}

	return m_executor.print_blockchain_info(start_index, end_index);
}

bool t_command_parser_executor::set_log_level(const std::vector<std::string> &args)
{
	if(args.size() > 1)
	{
		GULPS_PRINT("use: set_log [<log_level_number_0-4> | <categories>]");
		return true;
	}

	if(args.empty())
	{
		return m_executor.set_log_categories("+");
	}

	uint16_t l = 0;
	if(epee::string_tools::get_xtype_from_string(l, args[0]))
	{
		if(4 < l)
		{
			GULPS_PRINT("wrong number range, use: set_log <log_level_number_0-4>");
			return true;
		}
		return m_executor.set_log_level(l);
	}
	else
	{
		return m_executor.set_log_categories(args.front());
	}
}

bool t_command_parser_executor::print_height(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_height();
}

bool t_command_parser_executor::print_block(const std::vector<std::string> &args)
{
	if(args.empty())
	{
		GULPS_PRINT("expected: print_block (<block_hash> | <block_height>)");
		return false;
	}

	const std::string &arg = args.front();

	bool is_hash = !(std::find_if(arg.begin(), arg.end(), [](char c) { return !std::isdigit(c); }) == arg.end());
	if(is_hash)
	{
		crypto::hash block_hash;
		if(parse_hash256(arg, block_hash))
		{
			return m_executor.print_block_by_hash(block_hash);
		}
	}
	else
	{
		try
		{
			uint64_t height = boost::lexical_cast<uint64_t>(arg);
			return m_executor.print_block_by_height(height);
		}
		catch(const boost::bad_lexical_cast &e)
		{
			return false;
		}
	}
	return false;
}

bool t_command_parser_executor::print_transaction(const std::vector<std::string> &args)
{
	bool include_hex = false;
	bool include_json = false;

	// Assumes that optional flags come after mandatory argument <transaction_hash>
	for(unsigned int i = 1; i < args.size(); ++i)
	{
		if(args[i] == "+hex")
			include_hex = true;
		else if(args[i] == "+json")
			include_json = true;
		else
		{
			GULPSF_PRINT("unexpected argument: {}", args[i]);
			return true;
		}
	}
	if(args.empty())
	{
		GULPS_PRINT("expected: print_tx <transaction_hash> [+hex] [+json]");
		return true;
	}

	const std::string &str_hash = args.front();
	crypto::hash tx_hash;
	if(parse_hash256(str_hash, tx_hash))
	{
		m_executor.print_transaction(tx_hash, include_hex, include_json);
	}

	return true;
}

bool t_command_parser_executor::is_key_image_spent(const std::vector<std::string> &args)
{
	if(args.empty())
	{
		GULPS_PRINT("expected: is_key_image_spent <key_image>");
		return true;
	}

	const std::string &str = args.front();
	crypto::key_image ki;
	crypto::hash hash;
	if(parse_hash256(str, hash))
	{
		memcpy(&ki, &hash, sizeof(ki));
		m_executor.is_key_image_spent(ki);
	}

	return true;
}

bool t_command_parser_executor::print_transaction_pool_long(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_transaction_pool_long();
}

bool t_command_parser_executor::print_transaction_pool_short(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_transaction_pool_short();
}

bool t_command_parser_executor::print_transaction_pool_stats(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_transaction_pool_stats();
}

bool t_command_parser_executor::start_mining(const std::vector<std::string> &args)
{
	if(!args.size())
	{
		GULPS_PRINT("Please specify a wallet address to mine for: start_mining <addr> [<threads>]");
		return true;
	}

	cryptonote::address_parse_info info;
	cryptonote::network_type nettype = cryptonote::MAINNET;
	if(!cryptonote::get_account_address_from_str(cryptonote::MAINNET, info, args.front()))
	{
		if(!cryptonote::get_account_address_from_str(cryptonote::TESTNET, info, args.front()))
		{
			if(!cryptonote::get_account_address_from_str(cryptonote::STAGENET, info, args.front()))
			{
				GULPS_PRINT("target account address has wrong format");
				return true;
			}
			else
			{
				nettype = cryptonote::STAGENET;
			}
		}
		else
		{
			nettype = cryptonote::TESTNET;
		}
	}

	if(info.is_subaddress)
	{
		tools::fail_msg_writer() << "subaddress for mining reward is not yet supported!" << std::endl;
		return true;
	}

	if(args.front() == cryptonote::common_config::DEV_FUND_ADDRESS)
	{
		tools::fail_msg_writer() << "Dev fund address is not mineable. If you would like to support the dev team please mine to " << cryptonote::common_config::RYO_DONATION_ADDR << std::endl;
		return true;
	}

	if(nettype != cryptonote::MAINNET)

		GULPSF_PRINT("Mining to a {} address, make sure this is intentional!", (nettype == cryptonote::TESTNET ? "testnet" : "stagenet"));
	uint64_t threads_count = 1;
	bool do_background_mining = false;
	bool ignore_battery = false;
	if(args.size() > 4)
	{
		return false;
	}

	if(args.size() == 4)
	{
		ignore_battery = args[3] == "true";
	}

	if(args.size() >= 3)
	{
		do_background_mining = args[2] == "true";
	}

	if(args.size() >= 2)
	{
		bool ok = epee::string_tools::get_xtype_from_string(threads_count, args[1]);
		threads_count = (ok && 0 < threads_count) ? threads_count : 1;
	}

	m_executor.start_mining(info.address, threads_count, nettype, do_background_mining, ignore_battery);

	return true;
}

bool t_command_parser_executor::stop_mining(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.stop_mining();
}

bool t_command_parser_executor::stop_daemon(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.stop_daemon();
}

bool t_command_parser_executor::print_status(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;

	return m_executor.print_status();
}

bool t_command_parser_executor::set_limit(const std::vector<std::string> &args)
{
	if(args.size() > 1)
		return false;
	if(args.size() == 0)
	{
		return m_executor.get_limit();
	}
	int64_t limit;
	try
	{
		limit = std::stoll(args[0]);
	}
	catch(const std::exception &ex)
	{
		GULPS_PRINT("failed to parse argument");
		return false;
	}

	return m_executor.set_limit(limit, limit);
}

bool t_command_parser_executor::set_limit_up(const std::vector<std::string> &args)
{
	if(args.size() > 1)
		return false;
	if(args.size() == 0)
	{
		return m_executor.get_limit_up();
	}
	int64_t limit;
	try
	{
		limit = std::stoll(args[0]);
	}
	catch(const std::exception &ex)
	{
		GULPS_PRINT("failed to parse argument");
		return false;
	}

	return m_executor.set_limit(0, limit);
}

bool t_command_parser_executor::set_limit_down(const std::vector<std::string> &args)
{
	if(args.size() > 1)
		return false;
	if(args.size() == 0)
	{
		return m_executor.get_limit_down();
	}
	int64_t limit;
	try
	{
		limit = std::stoll(args[0]);
	}
	catch(const std::exception &ex)
	{
		GULPS_PRINT("failed to parse argument");
		return false;
	}

	return m_executor.set_limit(limit, 0);
}

bool t_command_parser_executor::out_peers(const std::vector<std::string> &args)
{
	if(args.empty())
		return false;

	unsigned int limit;
	try
	{
		limit = std::stoi(args[0]);
	}

	catch(const std::exception &ex)
	{
		GULPS_ERROR("stoi exception");
		return false;
	}

	return m_executor.out_peers(limit);
}

bool t_command_parser_executor::in_peers(const std::vector<std::string> &args)
{
	if(args.empty())
		return false;

	unsigned int limit;
	try
	{
		limit = std::stoi(args[0]);
	}

	catch(const std::exception &ex)
	{
		GULPS_ERROR("stoi exception");
		return false;
	}

	return m_executor.in_peers(limit);
}

bool t_command_parser_executor::hard_fork_info(const std::vector<std::string> &args)
{
	int version;
	if(args.size() == 0)
	{
		version = 0;
	}
	else if(args.size() == 1)
	{
		try
		{
			version = std::stoi(args[0]);
		}
		catch(const std::exception &ex)
		{
			return false;
		}
		if(version <= 0 || version > 255)
			return false;
	}
	else
	{
		return false;
	}
	return m_executor.hard_fork_info(version);
}

bool t_command_parser_executor::show_bans(const std::vector<std::string> &args)
{
	if(!args.empty())
		return false;
	return m_executor.print_bans();
}

bool t_command_parser_executor::ban(const std::vector<std::string> &args)
{
	if(args.size() != 1 && args.size() != 2)
		return false;
	std::string ip = args[0];
	time_t seconds = P2P_IP_BLOCKTIME;
	if(args.size() > 1)
	{
		try
		{
			seconds = std::stoi(args[1]);
		}
		catch(const std::exception &e)
		{
			return false;
		}
		if(seconds == 0)
		{
			return false;
		}
	}
	return m_executor.ban(ip, seconds);
}

bool t_command_parser_executor::unban(const std::vector<std::string> &args)
{
	if(args.size() != 1)
		return false;
	std::string ip = args[0];
	return m_executor.unban(ip);
}

bool t_command_parser_executor::flush_txpool(const std::vector<std::string> &args)
{
	if(args.size() > 1)
		return false;

	std::string txid;
	if(args.size() == 1)
	{
		crypto::hash hash;
		if(!parse_hash256(args[0], hash))
		{
			GULPS_PRINT("failed to parse tx id");
			return true;
		}
		txid = args[0];
	}
	return m_executor.flush_txpool(txid);
}

bool t_command_parser_executor::output_histogram(const std::vector<std::string> &args)
{
	std::vector<uint64_t> amounts;
	uint64_t min_count = 3;
	uint64_t max_count = 0;
	size_t n_raw = 0;

	for(size_t n = 0; n < args.size(); ++n)
	{
		if(args[n][0] == '@')
		{
			amounts.push_back(boost::lexical_cast<uint64_t>(args[n].c_str() + 1));
		}
		else if(n_raw == 0)
		{
			min_count = boost::lexical_cast<uint64_t>(args[n]);
			n_raw++;
		}
		else if(n_raw == 1)
		{
			max_count = boost::lexical_cast<uint64_t>(args[n]);
			n_raw++;
		}
		else
		{
			GULPS_PRINT("Invalid syntax: more than two non-amount parameters");
			return true;
		}
	}
	return m_executor.output_histogram(amounts, min_count, max_count);
}

bool t_command_parser_executor::print_coinbase_tx_sum(const std::vector<std::string> &args)
{
	if(!args.size())
	{
		GULPS_PRINT("need block height parameter");
		return false;
	}
	uint64_t height = 0;
	uint64_t count = 0;
	if(!epee::string_tools::get_xtype_from_string(height, args[0]))
	{
		GULPS_PRINT("wrong starter block height parameter");
		return false;
	}
	if(args.size() > 1 && !epee::string_tools::get_xtype_from_string(count, args[1]))
	{
		GULPS_PRINT("wrong count parameter");
		return false;
	}

	return m_executor.print_coinbase_tx_sum(height, count);
}

bool t_command_parser_executor::alt_chain_info(const std::vector<std::string> &args)
{
	if(args.size())
	{
		GULPS_PRINT("No parameters allowed");
		return false;
	}

	return m_executor.alt_chain_info();
}

bool t_command_parser_executor::print_blockchain_dynamic_stats(const std::vector<std::string> &args)
{
	if(args.size() != 1)
	{
		GULPS_PRINT("Exactly one parameter is needed");
		return false;
	}

	uint64_t nblocks = 0;
	if(!epee::string_tools::get_xtype_from_string(nblocks, args[0]) || nblocks == 0)
	{
		GULPS_PRINT("wrong number of blocks");
		return false;
	}

	return m_executor.print_blockchain_dynamic_stats(nblocks);
}

bool t_command_parser_executor::update(const std::vector<std::string> &args)
{
	if(args.size() != 1)
	{
		GULPS_PRINT("Exactly one parameter is needed: check, download, or update");
		return false;
	}

	return m_executor.update(args.front());
}

bool t_command_parser_executor::relay_tx(const std::vector<std::string> &args)
{
	if(args.size() != 1)
		return false;

	std::string txid;
	crypto::hash hash;
	if(!parse_hash256(args[0], hash))
	{
		GULPS_PRINT("failed to parse tx id");
		return true;
	}
	txid = args[0];
	return m_executor.relay_tx(txid);
}

bool t_command_parser_executor::sync_info(const std::vector<std::string> &args)
{
	if(args.size() != 0)
		return false;

	return m_executor.sync_info();
}

bool t_command_parser_executor::version(const std::vector<std::string> &args)
{
	GULPSF_PRINT("Ryo '{}' ({})", RYO_RELEASE_NAME, RYO_VERSION_FULL);
	return true;
}

} // namespace daemonize
