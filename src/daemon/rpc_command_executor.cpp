// Copyright (c) 2020, Ryo Currency Project
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
//    public domain on 1st of February 2021
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


#include "daemon/rpc_command_executor.h"
#include "common/password.h"
#include "common/scoped_message_writer.h"
#include "cryptonote_basic/hardfork.h"
#include "cryptonote_config.h"
#include "cryptonote_core/cryptonote_core.h"
#include "rpc/core_rpc_server_commands_defs.h"
#include "string_tools.h"
#include <boost/format.hpp>
#include <ctime>
#include <string>

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("rpc_cmd_exe");

#define GULPS_PRINT_FAIL(...) GULPS_OUTPUT(gulps::OUT_USER_1, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_RED, "Error: ", __VA_ARGS__)
#define GULPS_PRINT_SUCCESS(...) GULPS_OUTPUT(gulps::OUT_USER_1, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_GREEN, __VA_ARGS__)
#define GULPSF_PRINT_SUCCESS(...) GULPS_OUTPUTF(gulps::OUT_USER_1, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_GREEN, __VA_ARGS__)
#define GULPS_PRINT_OK(...) GULPS_OUTPUT(gulps::OUT_USER_1, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_PRINT_OK(...) GULPS_OUTPUTF(gulps::OUT_USER_1, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)

namespace daemonize
{

namespace
{
void print_peer(std::string const &prefix, cryptonote::peer const &peer)
{
	time_t now;
	time(&now);
	time_t last_seen = static_cast<time_t>(peer.last_seen);

	std::string id_str;
	std::string port_str;
	std::string elapsed = epee::misc_utils::get_time_interval_string(now - last_seen);
	std::string ip_str = epee::string_tools::get_ip_string_from_int32(peer.ip);
	std::stringstream peer_id_str;
	peer_id_str << std::hex << std::setw(16) << peer.id;
	peer_id_str >> id_str;
	epee::string_tools::xtype_to_string(peer.port, port_str);
	std::string addr_str = ip_str + ":" + port_str;
	GULPSF_PRINT_OK("{:<10} {:<25} {:<25} {:<}", prefix, id_str, addr_str, elapsed);
}

void print_block_header(cryptonote::block_header_response const &header)
{
	GULPS_PRINT_CLR(gulps::COLOR_GREEN,
		"timestamp: ", boost::lexical_cast<std::string>(header.timestamp),
		"\nprevious hash: ", header.prev_hash,
		"\nnonce: ", boost::lexical_cast<std::string>(header.nonce),
		"\nis orphan: ", header.orphan_status,
		"\nheight: ", boost::lexical_cast<std::string>(header.height),
		"\ndepth: ", boost::lexical_cast<std::string>(header.depth),
		"\nhash: ", header.hash,
		"\ndifficulty: ", boost::lexical_cast<std::string>(header.difficulty),
		"\nreward: ", boost::lexical_cast<std::string>(header.reward));
}

std::string get_human_time_ago(time_t t, time_t now)
{
	if(t == now)
		return "now";
	time_t dt = t > now ? t - now : now - t;
	std::string s;
	if(dt < 90)
		s = boost::lexical_cast<std::string>(dt) + " seconds";
	else if(dt < 90 * 60)
		s = boost::lexical_cast<std::string>(dt / 60) + " minutes";
	else if(dt < 36 * 3600)
		s = boost::lexical_cast<std::string>(dt / 3600) + " hours";
	else
		s = boost::lexical_cast<std::string>(dt / (3600 * 24)) + " days";
	return s + " " + (t > now ? "in the future" : "ago");
}

std::string get_time_hms(time_t t)
{
	unsigned int hours, minutes, seconds;
	char buffer[24];
	hours = t / 3600;
	t %= 3600;
	minutes = t / 60;
	t %= 60;
	seconds = t;
	snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
	return std::string(buffer);
}

std::string make_error(const std::string &base, const std::string &status)
{
	if(status == CORE_RPC_STATUS_OK)
		return base;
	return base + " -- " + status;
}
}

t_rpc_command_executor::t_rpc_command_executor(
	uint32_t ip, uint16_t port, const boost::optional<tools::login> &login, bool is_rpc, cryptonote::core_rpc_server *rpc_server)
	: m_rpc_client(NULL), m_rpc_server(rpc_server)
{
	if(is_rpc)
	{
		boost::optional<epee::net_utils::http::login> http_login{};
		if(login)
			http_login.emplace(login->username, login->password.password());
		m_rpc_client = new tools::t_rpc_client(ip, port, std::move(http_login));
	}
	else
	{
		if(rpc_server == NULL)
		{
			throw std::runtime_error("If not calling commands via RPC, rpc_server pointer must be non-null");
		}
	}

	m_is_rpc = is_rpc;
}

t_rpc_command_executor::~t_rpc_command_executor()
{
	if(m_rpc_client != NULL)
	{
		delete m_rpc_client;
	}
}

bool t_rpc_command_executor::print_peer_list()
{
	cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
	cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

	std::string failure_message = "Couldn't retrieve peer list";
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_peer_list", failure_message.c_str()))
		{
			return false;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_peer_list(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( failure_message);
			return false;
		}
	}

	for(auto &peer : res.white_list)
	{
		print_peer("white", peer);
	}

	for(auto &peer : res.gray_list)
	{
		print_peer("gray", peer);
	}

	return true;
}

bool t_rpc_command_executor::print_peer_list_stats()
{
	cryptonote::COMMAND_RPC_GET_PEER_LIST::request req;
	cryptonote::COMMAND_RPC_GET_PEER_LIST::response res;

	std::string failure_message = "Couldn't retrieve peer list";
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_peer_list", failure_message.c_str()))
		{
			return false;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_peer_list(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( failure_message);
			return false;
		}
	}

	GULPSF_PRINT_OK("White list size: {}/{} ({}%)\nGray list size: {}/{} ({}%)", res.white_list.size() , P2P_LOCAL_WHITE_PEERLIST_LIMIT , res.white_list.size() * 100.0 / P2P_LOCAL_WHITE_PEERLIST_LIMIT ,
									res.gray_list.size() , P2P_LOCAL_GRAY_PEERLIST_LIMIT , res.gray_list.size() * 100.0 / P2P_LOCAL_GRAY_PEERLIST_LIMIT );

	return true;
}

bool t_rpc_command_executor::save_blockchain()
{
	cryptonote::COMMAND_RPC_SAVE_BC::request req;
	cryptonote::COMMAND_RPC_SAVE_BC::response res;

	std::string fail_message = "Couldn't save blockchain";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/save_bc", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_save_bc(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Blockchain saved");

	return true;
}

bool t_rpc_command_executor::show_hash_rate()
{
	cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
	cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
	req.visible = true;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_log_hash_rate(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
		}
	}

	GULPS_PRINT_SUCCESS( "Hash rate logging is on");

	return true;
}

bool t_rpc_command_executor::hide_hash_rate()
{
	cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::request req;
	cryptonote::COMMAND_RPC_SET_LOG_HASH_RATE::response res;
	req.visible = false;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/set_log_hash_rate", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_log_hash_rate(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Hash rate logging is off");

	return true;
}

bool t_rpc_command_executor::show_difficulty()
{
	cryptonote::COMMAND_RPC_GET_INFO::request req;
	cryptonote::COMMAND_RPC_GET_INFO::response res;

	std::string fail_message = "Problem fetching info";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/getinfo", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_info(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message.c_str(), res.status));
			return true;
		}
	}

	GULPSF_PRINT_SUCCESS("BH: {}, TH: {}, DIFF: {}, HR: {} H/s", res.height,
								res.top_block_hash,
								res.difficulty,
								res.difficulty / res.target);

	return true;
}

static std::string get_mining_speed(uint64_t hr)
{
	if(hr > 1e9)
		return fmt::format("{:.2f} GH/s", (hr / 1.0e9));
	if(hr > 1e6)
		return fmt::format("{:.2f} MH/s", (hr / 1.0e6));
	if(hr > 1e3)
		return fmt::format("{:.2f} kH/s", (hr / 1.0e3));
	return fmt::format("{} H/s", hr);
}

static std::string get_fork_extra_info(uint64_t t, uint64_t now, uint64_t block_time)
{
	uint64_t blocks_per_day = 86400 / block_time;

	if(t == now)
		return " (forking now)";

	if(t > now)
	{
		uint64_t dblocks = t - now;
		if(dblocks <= 30)
			return fmt::format(" (next fork in {} blocks)", (unsigned)dblocks);
		if(dblocks <= blocks_per_day / 2)
			return fmt::format(" (next fork in {:.1f) hours)", (dblocks / (float)(blocks_per_day / 24)));
		if(dblocks <= blocks_per_day * 30)
			return fmt::format(" (next fork in {:.1f} days)", (dblocks / (float)blocks_per_day));
		return "";
	}
	return "";
}

static float get_sync_percentage(uint64_t height, uint64_t target_height)
{
	target_height = target_height ? target_height < height ? height : target_height : height;
	float pc = 100.0f * height / target_height;
	if(height < target_height && pc > 99.9f)
		return 99.9f; // to avoid 100% when not fully synced
	return pc;
}
static float get_sync_percentage(const cryptonote::COMMAND_RPC_GET_INFO::response &ires)
{
	return get_sync_percentage(ires.height, ires.target_height);
}

bool t_rpc_command_executor::show_status()
{
	cryptonote::COMMAND_RPC_GET_INFO::request ireq;
	cryptonote::COMMAND_RPC_GET_INFO::response ires;
	cryptonote::COMMAND_RPC_HARD_FORK_INFO::request hfreq;
	cryptonote::COMMAND_RPC_HARD_FORK_INFO::response hfres;
	cryptonote::COMMAND_RPC_MINING_STATUS::request mreq;
	cryptonote::COMMAND_RPC_MINING_STATUS::response mres;
	epee::json_rpc::error error_resp;
	bool has_mining_info = true;

	std::string fail_message = "Problem fetching info";

	hfreq.version = 0;
	bool mining_busy = false;
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
		{
			return true;
		}
		if(!m_rpc_client->json_rpc_request(hfreq, hfres, "hard_fork_info", fail_message.c_str()))
		{
			return true;
		}
		// mining info is only available non unrestricted RPC mode
		has_mining_info = m_rpc_client->rpc_request(mreq, mres, "/mining_status", fail_message.c_str());
	}
	else
	{
		if(!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, ires.status));
			return true;
		}
		if(!m_rpc_server->on_hard_fork_info(hfreq, hfres, error_resp) || hfres.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, hfres.status));
			return true;
		}
		if(!m_rpc_server->on_mining_status(mreq, mres))
		{
			GULPS_PRINT_FAIL( fail_message.c_str());
			return true;
		}

		if(mres.status == CORE_RPC_STATUS_BUSY)
		{
			mining_busy = true;
		}
		else if(mres.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, mres.status));
			return true;
		}
	}

	std::time_t uptime = std::time(nullptr) - ires.start_time;
	uint64_t net_height = ires.target_height > ires.height ? ires.target_height : ires.height;
	std::string bootstrap_msg;
	if(ires.was_bootstrap_ever_used)
	{
		bootstrap_msg = ", bootstrapping from " + ires.bootstrap_daemon_address;
		if(ires.untrusted)
		{
			bootstrap_msg += fmt::format(", local height: {} ({:.1f}%)", ires.height_without_bootstrap, get_sync_percentage(ires.height_without_bootstrap, net_height));
		}
		else
		{
			bootstrap_msg += " was used before";
		}
	}

	GULPSF_PRINT_SUCCESS("Height: {}/{} ({:.1f}) on {}{}, {}, net hash {}, v{}{}, {}, {}(out)+{}(in) connections, uptime {}d {}h {}m {}s",
								(unsigned long long)ires.height,
								(unsigned long long)net_height,
								get_sync_percentage(ires),
								(ires.testnet ? "testnet" : ires.stagenet ? "stagenet" : "mainnet"),
								bootstrap_msg,
								(!has_mining_info ? "mining info unavailable" : mining_busy ? "syncing" : mres.active ? ((mres.is_background_mining_enabled ? "smart " : "") + std::string("mining at ") + get_mining_speed(mres.speed)) : "not mining"),
								get_mining_speed(ires.difficulty / ires.target),
								(unsigned)hfres.version,
								get_fork_extra_info(hfres.earliest_height, net_height, ires.target),
								(hfres.state == cryptonote::HardFork::Ready ? "up to date" : hfres.state == cryptonote::HardFork::UpdateNeeded ? "update needed" : "out of date, likely forked"),
								(unsigned)ires.outgoing_connections_count,
								(unsigned)ires.incoming_connections_count,
								(unsigned int)floor(uptime / 60.0 / 60.0 / 24.0),
								(unsigned int)floor(fmod((uptime / 60.0 / 60.0), 24.0)),
								(unsigned int)floor(fmod((uptime / 60.0), 60.0)),
								(unsigned int)fmod(uptime, 60.0));

	return true;
}

bool t_rpc_command_executor::print_connections()
{
	cryptonote::COMMAND_RPC_GET_CONNECTIONS::request req;
	cryptonote::COMMAND_RPC_GET_CONNECTIONS::response res;
	epee::json_rpc::error error_resp;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "get_connections", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_connections(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

GULPSF_PRINT_OK("{:<30}{:<20}{:<20}{:<30}{:<25}{:<20}{:<12}{:<14}{:<10}{:<13}",
														"Remote Host",
														"Peer id",
														"Support Flags",
														"Recv/Sent (inactive,sec)",
														"State",
														"Livetime(sec)",
														"Down (kB/s)",
														"Down(now)",
														"Up (kB/s)",
														"Up(now)\n");

	for(auto &info : res.connections)
	{
		std::string address = info.incoming ? "INC " : "OUT ";
		address += info.ip + ":" + info.port;
		//std::string in_out = info.incoming ? "INC " : "OUT ";
		GULPSF_PRINT_OK("{:<30}{:<20}{:<20}{:<30}{:<25}{:<20}{:<12}{:<14}{:<10}{:<13}{:<}{:<}",
			//<< std::setw(30) << std::left << in_out
			address,
			epee::string_tools::pad_string(info.peer_id, 16, '0', true),
			info.support_flags,
			std::to_string(info.recv_count) + "(" + std::to_string(info.recv_idle_time) + ")/" + std::to_string(info.send_count) + "(" + std::to_string(info.send_idle_time) + ")",
			info.state,
			info.live_time,
			info.avg_download,
			info.current_download,
			info.avg_upload,
			info.current_upload,

			(info.localhost ? "[LOCALHOST]" : ""),
			(info.local_ip ? "[LAN]" : ""));
		//GULPSF_PRINT_OK("{}-25s peer_id: {}-25s {}", address, info.peer_id, in_out);
	}

	return true;
}

bool t_rpc_command_executor::print_net_stats()
{
	cryptonote::COMMAND_RPC_GET_NET_STATS::request req;
	cryptonote::COMMAND_RPC_GET_NET_STATS::response res;
	cryptonote::COMMAND_RPC_GET_LIMIT::request lreq;
	cryptonote::COMMAND_RPC_GET_LIMIT::response lres;

	std::string fail_message = "Unsuccessful";

	if (m_is_rpc)
	{
		if (!m_rpc_client->json_rpc_request(req, res, "get_net_stats", fail_message.c_str()))
		{
			return true;
		}
		if (!m_rpc_client->json_rpc_request(lreq, lres, "get_limit", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if (!m_rpc_server->on_get_net_stats(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
		if (!m_rpc_server->on_get_limit(lreq, lres) || lres.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, lres.status));
			return true;
		}
	}

	uint64_t seconds = (uint64_t)time(NULL) - res.start_time;
	uint64_t raverage = seconds > 0 ? res.total_bytes_in / seconds : 0;
	uint64_t saverage = seconds > 0 ? res.total_bytes_out / seconds : 0;
	uint64_t rlimit = lres.limit_down * 1024;   // convert to bytes, as limits are always kB/s
	uint64_t slimit = lres.limit_up * 1024;
	double rpercent = (double)raverage / (double)rlimit * 100.0;
	double spercent = (double)saverage / (double)slimit * 100.0;

	GULPSF_PRINT_SUCCESS("Received {} bytes ({}) in {} packets, average {}/s ({:.1f}%) of the limit of {}/s",
								res.total_bytes_in,
								tools::get_human_readable_bytes(res.total_bytes_in),
								res.total_packets_in,
								tools::get_human_readable_bytes(raverage),
								rpercent,
								tools::get_human_readable_bytes(rlimit));
  
	GULPSF_PRINT_SUCCESS("Sent {} bytes ({}) in {} packets, average {}/s ({:.1f}%) of the limit of {}/s",
								res.total_bytes_out,
								tools::get_human_readable_bytes(res.total_bytes_out),
								res.total_packets_out,
								tools::get_human_readable_bytes(saverage),
								spercent,
								tools::get_human_readable_bytes(slimit));

	return true;
}

bool t_rpc_command_executor::print_blockchain_info(uint64_t start_block_index, uint64_t end_block_index)
{
	cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request req;
	cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response res;
	epee::json_rpc::error error_resp;

	req.start_height = start_block_index;
	req.end_height = end_block_index;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "getblockheadersrange", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_block_headers_range(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	bool first = true;
	for(auto &header : res.headers)
	{
		if(!first)

			GULPSF_PRINT_OK("\nheight: {}, timestamp: {}, difficulty: {}, size: {}, transactions: {}\nmajor version: {}, minor version: {}\nblock id: {}, previous block id: {}\ndifficulty: {}, nonce {}, reward {}",
			header.height, header.timestamp, header.difficulty, header.block_size, header.num_txes, (unsigned)header.major_version, (unsigned)header.minor_version,
			header.hash, header.prev_hash, header.difficulty, header.nonce, cryptonote::print_money(header.reward));
		first = false;
	}

	return true;
}

bool t_rpc_command_executor::set_log_level(int8_t level)
{
	cryptonote::COMMAND_RPC_SET_LOG_LEVEL::request req;
	cryptonote::COMMAND_RPC_SET_LOG_LEVEL::response res;
	req.level = level;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/set_log_level", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_log_level(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_SUCCESS("Log level is now {}",  level);

	return true;
}

bool t_rpc_command_executor::set_log_categories(const std::string &categories)
{
	cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::request req;
	cryptonote::COMMAND_RPC_SET_LOG_CATEGORIES::response res;
	req.categories = categories;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/set_log_categories", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_log_categories(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_SUCCESS("Log categories are now {}",  res.categories);

	return true;
}

bool t_rpc_command_executor::print_height()
{
	cryptonote::COMMAND_RPC_GET_HEIGHT::request req;
	cryptonote::COMMAND_RPC_GET_HEIGHT::response res;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/getheight", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_height(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS(boost::lexical_cast<std::string>(res.height));

	return true;
}

bool t_rpc_command_executor::print_block_by_hash(crypto::hash block_hash)
{
	cryptonote::COMMAND_RPC_GET_BLOCK::request req;
	cryptonote::COMMAND_RPC_GET_BLOCK::response res;
	epee::json_rpc::error error_resp;

	req.hash = epee::string_tools::pod_to_hex(block_hash);

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "getblock", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_block(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	print_block_header(res.block_header);
	GULPS_PRINT_CLR( gulps::COLOR_GREEN,  res.json );

	return true;
}

bool t_rpc_command_executor::print_block_by_height(uint64_t height)
{
	cryptonote::COMMAND_RPC_GET_BLOCK::request req;
	cryptonote::COMMAND_RPC_GET_BLOCK::response res;
	epee::json_rpc::error error_resp;

	req.height = height;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "getblock", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_block(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	print_block_header(res.block_header);
	GULPS_PRINT_CLR( gulps::COLOR_GREEN,  res.json );

	return true;
}

bool t_rpc_command_executor::print_transaction(crypto::hash transaction_hash,
											   bool include_hex,
											   bool include_json)
{
	cryptonote::COMMAND_RPC_GET_TRANSACTIONS::request req;
	cryptonote::COMMAND_RPC_GET_TRANSACTIONS::response res;

	std::string fail_message = "Problem fetching transaction";

	req.txs_hashes.push_back(epee::string_tools::pod_to_hex(transaction_hash));
	req.decode_as_json = false;
	req.prune = false;
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/gettransactions", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_transactions(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	if(1 == res.txs.size() || 1 == res.txs_as_hex.size())
	{
		if(1 == res.txs.size())
		{
			// only available for new style answers
			if(res.txs.front().in_pool)
				GULPS_PRINT_SUCCESS( "Found in pool");
			else
				GULPSF_PRINT_SUCCESS("Found in blockchain at height {}",  res.txs.front().block_height);
		}

		const std::string &as_hex = (1 == res.txs.size()) ? res.txs.front().as_hex : res.txs_as_hex.front();
		// Print raw hex if requested
		if(include_hex)
			GULPS_PRINT_CLR( gulps::COLOR_GREEN,  as_hex );

		// Print json if requested
		if(include_json)
		{
			crypto::hash tx_hash, tx_prefix_hash;
			cryptonote::transaction tx;
			cryptonote::blobdata blob;
			if(!string_tools::parse_hexstr_to_binbuff(as_hex, blob))
			{
				GULPS_PRINT_FAIL( "Failed to parse tx to get json format");
			}
			else if(!cryptonote::parse_and_validate_tx_from_blob(blob, tx, tx_hash, tx_prefix_hash))
			{
				GULPS_PRINT_FAIL( "Failed to parse tx blob to get json format");
			}
			else
			{
				GULPSF_PRINT_CLR( gulps::COLOR_GREEN, cryptonote::obj_to_json_str(tx) );
			}
		}
	}
	else
	{
		GULPS_PRINT_FAIL( "Transaction wasn't found: ", transaction_hash);
	}

	return true;
}

bool t_rpc_command_executor::is_key_image_spent(const crypto::key_image &ki)
{
	cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::request req;
	cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::response res;

	std::string fail_message = "Problem checking key image";

	req.key_images.push_back(epee::string_tools::pod_to_hex(ki));
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/is_key_image_spent", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_is_key_image_spent(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	if(1 == res.spent_status.size())
	{
		GULPS_PRINT_SUCCESS(ki, ": ", (res.spent_status.front() ? "spent" : "unspent"),
							(res.spent_status.front() == cryptonote::COMMAND_RPC_IS_KEY_IMAGE_SPENT::SPENT_IN_POOL ? " (in pool)" : ""));
	}
	else
	{
		GULPS_PRINT_FAIL( "key image status could not be determined");
	}

	return true;
}

bool t_rpc_command_executor::print_transaction_pool_long()
{
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

	std::string fail_message = "Problem fetching transaction pool";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_transaction_pool(req, res, false) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	if(res.transactions.empty() && res.spent_key_images.empty())
	{
		GULPS_PRINT_OK("Pool is empty");
	}
	if(!res.transactions.empty())
	{
		const time_t now = time(NULL);
		GULPS_PRINT_OK( "Transactions: ");
		for(auto &tx_info : res.transactions)
		{
			GULPSF_PRINT_OK("id: {}\n{}\nblob_size: {}\nfee: {}\nfee/byte: {}\nreceive_time: {} ({})relayed: {}\ndo_not_relay: {}\nkept_by_block: {}\ndouble_spend_seen: {}\nmax_used_block_height: {}\nmax_used_block_id: {}\nlast_failed_height: {}\nlast_failed_id: {}"
										, tx_info.id_hash
										, tx_info.tx_json
										, tx_info.blob_size
										, cryptonote::print_money(tx_info.fee)
										, cryptonote::print_money(tx_info.fee / (double)tx_info.blob_size)
										, tx_info.receive_time
										, get_human_time_ago(tx_info.receive_time, now)
										, [&](const cryptonote::tx_info &tx_info) -> std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; }(tx_info) , (tx_info.do_not_relay ? 'T' : 'F')
										, (tx_info.kept_by_block ? 'T' : 'F')
										, (tx_info.double_spend_seen ? 'T' : 'F')
										, tx_info.max_used_block_height
										, tx_info.max_used_block_id_hash
										, tx_info.last_failed_height
										, tx_info.last_failed_id_hash);
		}
		if(res.spent_key_images.empty())
		{
			GULPS_PRINT_OK( "WARNING: Inconsistent pool state - no spent key images");
		}
	}
	if(!res.spent_key_images.empty())
	{
		GULPS_PRINT_OK( "\nSpent key images: ");
		for(const cryptonote::spent_key_image_info &kinfo : res.spent_key_images)
		{
			GULPSF_PRINT_OK("key image: {}", kinfo.id_hash);
			if(kinfo.txs_hashes.size() == 1)
			{
				GULPSF_PRINT_OK("  tx: {}", kinfo.txs_hashes[0]);
			}
			else if(kinfo.txs_hashes.size() == 0)
			{
				GULPS_PRINT_OK( "  WARNING: spent key image has no txs associated");
			}
			else
			{
				GULPSF_PRINT_OK("  NOTE: key image for multiple txs: {}", kinfo.txs_hashes.size());
				for(const std::string &tx_id : kinfo.txs_hashes)
				{
					GULPSF_PRINT_OK("  tx: {}", tx_id);
				}
			}
		}
		if(res.transactions.empty())
		{
			GULPS_PRINT_OK( "WARNING: Inconsistent pool state - no transactions");
		}
	}

	return true;
}

bool t_rpc_command_executor::print_transaction_pool_short()
{
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::request req;
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL::response res;

	std::string fail_message = "Problem fetching transaction pool";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_transaction_pool", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_transaction_pool(req, res, false) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	if(res.transactions.empty())
	{
		GULPS_PRINT_OK("Pool is empty");
	}
	else
	{
		const time_t now = time(NULL);
		for(auto &tx_info : res.transactions)
		{
				GULPSF_PRINT_OK("id: {}\n{}\nblob_size: {}\nfee: {}\nfee/byte: {}\nreceive_time: {} ({})relayed: {}\ndo_not_relay: {}\nkept_by_block: {}\ndouble_spend_seen: {}\nmax_used_block_height: {}\nmax_used_block_id: {}\nlast_failed_height: {}\nlast_failed_id: {}"
										, tx_info.id_hash
										, tx_info.tx_json
										, tx_info.blob_size
										, cryptonote::print_money(tx_info.fee)
										, cryptonote::print_money(tx_info.fee / (double)tx_info.blob_size)
										, tx_info.receive_time
										, get_human_time_ago(tx_info.receive_time, now)
										, [&](const cryptonote::tx_info &tx_info) -> std::string { if (!tx_info.relayed) return "no"; return boost::lexical_cast<std::string>(tx_info.last_relayed_time) + " (" + get_human_time_ago(tx_info.last_relayed_time, now) + ")"; }(tx_info) , (tx_info.do_not_relay ? 'T' : 'F')
										, (tx_info.kept_by_block ? 'T' : 'F')
										, (tx_info.double_spend_seen ? 'T' : 'F')
										, tx_info.max_used_block_height
										, tx_info.max_used_block_id_hash
										, tx_info.last_failed_height
										, tx_info.last_failed_id_hash);
		}
	}

	return true;
}

bool t_rpc_command_executor::print_transaction_pool_stats()
{
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::request req;
	cryptonote::COMMAND_RPC_GET_TRANSACTION_POOL_STATS::response res;
	cryptonote::COMMAND_RPC_GET_INFO::request ireq;
	cryptonote::COMMAND_RPC_GET_INFO::response ires;

	std::string fail_message = "Problem fetching transaction pool stats";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_transaction_pool_stats", fail_message.c_str()))
		{
			return true;
		}
		if(!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		res.pool_stats.clear();
		if(!m_rpc_server->on_get_transaction_pool_stats(req, res, false) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
		if(!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, ires.status));
			return true;
		}
	}

	size_t n_transactions = res.pool_stats.txs_total;
	const uint64_t now = time(NULL);
	size_t avg_bytes = n_transactions ? res.pool_stats.bytes_total / n_transactions : 0;

	std::string backlog_message;
	const uint64_t full_reward_zone = ires.block_size_limit / 2;
	if(res.pool_stats.bytes_total <= full_reward_zone)
	{
		backlog_message = "no backlog";
	}
	else
	{
		uint64_t backlog = (res.pool_stats.bytes_total + full_reward_zone - 1) / full_reward_zone;
		backlog_message = fmt::format("estimated {} block ({} minutes) backlog", backlog, (backlog * cryptonote::common_config::DIFFICULTY_TARGET / 60));
	}

	GULPSF_PRINT_OK("{} tx(es), {} bytes total (min {}, max {}, avg {}, median {})\nfees {} (avg {} per tx, {} per byte)\n{} double spends, {} not relayed, {} failing, {} older than 10 minutes (oldest {}), {}",
									n_transactions , res.pool_stats.bytes_total , res.pool_stats.bytes_min , res.pool_stats.bytes_max , avg_bytes , res.pool_stats.bytes_med ,
									cryptonote::print_money(res.pool_stats.fee_total) , cryptonote::print_money(n_transactions ? res.pool_stats.fee_total / n_transactions : 0) ,
									cryptonote::print_money(res.pool_stats.bytes_total ? res.pool_stats.fee_total / res.pool_stats.bytes_total : 0) , res.pool_stats.num_double_spends ,
									res.pool_stats.num_not_relayed , res.pool_stats.num_failing , res.pool_stats.num_10m , (res.pool_stats.oldest == 0 ? "-" : get_human_time_ago(res.pool_stats.oldest, now)) , backlog_message);

	if(n_transactions > 1 && res.pool_stats.histo.size())
	{
		std::vector<uint64_t> times;
		uint64_t numer;
		size_t i, n = res.pool_stats.histo.size(), denom;
		times.resize(n);
		if(res.pool_stats.histo_98pc)
		{
			numer = res.pool_stats.histo_98pc;
			denom = n - 1;
			for(i = 0; i < denom; i++)
				times[i] = i * numer / denom;
			times[i] = now - res.pool_stats.oldest;
		}
		else
		{
			numer = now - res.pool_stats.oldest;
			denom = n;
			for(i = 0; i < denom; i++)
				times[i] = i * numer / denom;
		}
		GULPS_PRINT_OK("   Age      Txes       Bytes");
		for(i = 0; i < n; i++)
		{
			GULPSF_PRINT_OK("{}{:>8}{:>12}", get_time_hms(times[i]), res.pool_stats.histo[i].txs, res.pool_stats.histo[i].bytes);
		}
	}

	return true;
}

bool t_rpc_command_executor::start_mining(cryptonote::account_public_address address, uint64_t num_threads, cryptonote::network_type nettype, bool do_background_mining, bool ignore_battery)
{
	cryptonote::COMMAND_RPC_START_MINING::request req;
	cryptonote::COMMAND_RPC_START_MINING::response res;
	req.miner_address = cryptonote::get_public_address_as_str(nettype, false, address);
	req.threads_count = num_threads;
	req.do_background_mining = do_background_mining;
	req.ignore_battery = ignore_battery;

	std::string fail_message = "Mining did not start";

	if(m_is_rpc)
	{
		if(m_rpc_client->rpc_request(req, res, "/start_mining", fail_message.c_str()))
		{
			GULPS_PRINT_SUCCESS( "Mining started");
		}
	}
	else
	{
		if(!m_rpc_server->on_start_mining(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	return true;
}

bool t_rpc_command_executor::stop_mining()
{
	cryptonote::COMMAND_RPC_STOP_MINING::request req;
	cryptonote::COMMAND_RPC_STOP_MINING::response res;

	std::string fail_message = "Mining did not stop";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/stop_mining", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_stop_mining(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Mining stopped");
	return true;
}

bool t_rpc_command_executor::stop_daemon()
{
	cryptonote::COMMAND_RPC_STOP_DAEMON::request req;
	cryptonote::COMMAND_RPC_STOP_DAEMON::response res;

	//# ifdef WIN32
	//    // Stop via service API
	//    // TODO - this is only temporary!  Get rid of hard-coded constants!
	//    bool ok = windows::stop_service("Ryo Daemon");
	//    ok = windows::uninstall_service("Ryo Daemon");
	//    //bool ok = windows::stop_service(SERVICE_NAME);
	//    //ok = windows::uninstall_service(SERVICE_NAME);
	//    if (ok)
	//    {
	//      return true;
	//    }
	//# endif

	// Stop via RPC
	std::string fail_message = "Daemon did not stop";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/stop_daemon", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_stop_daemon(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Stop signal sent");

	return true;
}

bool t_rpc_command_executor::print_status()
{
	if(!m_is_rpc)
	{
		GULPS_PRINT_SUCCESS( "print_status makes no sense in interactive mode");
		return true;
	}

	bool daemon_is_alive = m_rpc_client->check_connection();

	if(daemon_is_alive)
	{
		GULPS_PRINT_SUCCESS( "ryod is running");
	}
	else
	{
		GULPS_PRINT_FAIL( "ryod is NOT running");
	}

	return true;
}

bool t_rpc_command_executor::get_limit()
{
	cryptonote::COMMAND_RPC_GET_LIMIT::request req;
	cryptonote::COMMAND_RPC_GET_LIMIT::response res;

	std::string failure_message = "Couldn't get limit";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(failure_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("limit-down is {} kB/s", res.limit_down );
	GULPSF_PRINT_OK("limit-up is {} kB/s", res.limit_up );
	return true;
}

bool t_rpc_command_executor::set_limit(int64_t limit_down, int64_t limit_up)
{
	cryptonote::COMMAND_RPC_SET_LIMIT::request req;
	cryptonote::COMMAND_RPC_SET_LIMIT::response res;

	req.limit_down = limit_down;
	req.limit_up = limit_up;

	std::string failure_message = "Couldn't set limit";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/set_limit", failure_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(failure_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("Set limit-down to {} kB/s", res.limit_down );
	GULPSF_PRINT_OK("Set limit-up to {} kB/s", res.limit_up );
	return true;
}

bool t_rpc_command_executor::get_limit_up()
{
	cryptonote::COMMAND_RPC_GET_LIMIT::request req;
	cryptonote::COMMAND_RPC_GET_LIMIT::response res;

	std::string failure_message = "Couldn't get limit";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(failure_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("limit-up is {} kB/s", res.limit_up );
	return true;
}

bool t_rpc_command_executor::get_limit_down()
{
	cryptonote::COMMAND_RPC_GET_LIMIT::request req;
	cryptonote::COMMAND_RPC_GET_LIMIT::response res;

	std::string failure_message = "Couldn't get limit";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/get_limit", failure_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_limit(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(failure_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("limit-down is {} kB/s", res.limit_down );
	return true;
}

bool t_rpc_command_executor::out_peers(uint64_t limit)
{
	cryptonote::COMMAND_RPC_OUT_PEERS::request req;
	cryptonote::COMMAND_RPC_OUT_PEERS::response res;

	epee::json_rpc::error error_resp;

	req.out_peers = limit;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/out_peers", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_out_peers(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("Max number of out peers set to {}", limit);

	return true;
}

bool t_rpc_command_executor::in_peers(uint64_t limit)
{
	cryptonote::COMMAND_RPC_IN_PEERS::request req;
	cryptonote::COMMAND_RPC_IN_PEERS::response res;

	epee::json_rpc::error error_resp;

	req.in_peers = limit;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/in_peers", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_in_peers(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_OK("Max number of in peers set to ", limit);

	return true;
}

bool t_rpc_command_executor::hard_fork_info(uint8_t version)
{
	cryptonote::COMMAND_RPC_HARD_FORK_INFO::request req;
	cryptonote::COMMAND_RPC_HARD_FORK_INFO::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	req.version = version;

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "hard_fork_info", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_hard_fork_info(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	version = version > 0 ? version : res.voting;
	GULPSF_PRINT_OK("version {} {}, {}/{} votes, threshold {}", (uint32_t)version , (res.enabled ? "enabled" : "not enabled") , res.votes , res.window , res.threshold);
	GULPSF_PRINT_OK("current version {}, voting for version {}", (uint32_t)res.version , (uint32_t)res.voting);

	return true;
}

bool t_rpc_command_executor::print_bans()
{
	cryptonote::COMMAND_RPC_GETBANS::request req;
	cryptonote::COMMAND_RPC_GETBANS::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "get_bans", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	for(auto i = res.bans.begin(); i != res.bans.end(); ++i)
	{
		GULPSF_PRINT_OK("{} banned for {} seconds", epee::string_tools::get_ip_string_from_int32(i->ip), i->seconds);
	}

	return true;
}

bool t_rpc_command_executor::ban(const std::string &ip, time_t seconds)
{
	cryptonote::COMMAND_RPC_SETBANS::request req;
	cryptonote::COMMAND_RPC_SETBANS::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	cryptonote::COMMAND_RPC_SETBANS::ban_data ban;
	if(!epee::string_tools::get_ip_int32_from_string(ban.ip, ip))
	{
		GULPS_PRINT_FAIL( "Invalid IP");
		return true;
	}
	ban.ban = true;
	ban.seconds = seconds;
	req.bans.push_back(ban);

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "set_bans", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	return true;
}

bool t_rpc_command_executor::unban(const std::string &ip)
{
	cryptonote::COMMAND_RPC_SETBANS::request req;
	cryptonote::COMMAND_RPC_SETBANS::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	cryptonote::COMMAND_RPC_SETBANS::ban_data ban;
	if(!epee::string_tools::get_ip_int32_from_string(ban.ip, ip))
	{
		GULPS_PRINT_FAIL( "Invalid IP");
		return true;
	}
	ban.ban = false;
	ban.seconds = 0;
	req.bans.push_back(ban);

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "set_bans", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_set_bans(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	return true;
}

bool t_rpc_command_executor::flush_txpool(const std::string &txid)
{
	cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::request req;
	cryptonote::COMMAND_RPC_FLUSH_TRANSACTION_POOL::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	if(!txid.empty())
		req.txids.push_back(txid);

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "flush_txpool", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_flush_txpool(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Pool successfully flushed");
	return true;
}

bool t_rpc_command_executor::output_histogram(const std::vector<uint64_t> &amounts, uint64_t min_count, uint64_t max_count)
{
	cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::request req;
	cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	req.amounts = amounts;
	req.min_count = min_count;
	req.max_count = max_count;
	req.unlocked = false;
	req.recent_cutoff = 0;

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "get_output_histogram", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_output_histogram(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	std::sort(res.histogram.begin(), res.histogram.end(),
			  [](const cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry &e1, const cryptonote::COMMAND_RPC_GET_OUTPUT_HISTOGRAM::entry &e2) -> bool { return e1.total_instances < e2.total_instances; });
	for(const auto &e : res.histogram)
	{
		GULPSF_PRINT_OK("{} {}", e.total_instances, cryptonote::print_money(e.amount));
	}

	return true;
}

bool t_rpc_command_executor::print_coinbase_tx_sum(uint64_t height, uint64_t count)
{
	cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::request req;
	cryptonote::COMMAND_RPC_GET_COINBASE_TX_SUM::response res;
	epee::json_rpc::error error_resp;

	req.height = height;
	req.count = count;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "get_coinbase_tx_sum", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_coinbase_tx_sum(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("Sum of coinbase transactions between block heights [{}, {}) is {} consisting of {} in emissions, and {} in fees", height , (height + count) , cryptonote::print_money(res.emission_amount + res.fee_amount)
						, cryptonote::print_money(res.emission_amount)
						, cryptonote::print_money(res.fee_amount) );
	return true;
}

bool t_rpc_command_executor::alt_chain_info()
{
	cryptonote::COMMAND_RPC_GET_INFO::request ireq;
	cryptonote::COMMAND_RPC_GET_INFO::response ires;
	cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::request req;
	cryptonote::COMMAND_RPC_GET_ALTERNATE_CHAINS::response res;
	epee::json_rpc::error error_resp;

	std::string fail_message = "Unsuccessful";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
		{
			return true;
		}
		if(!m_rpc_client->json_rpc_request(req, res, "get_alternate_chains", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, ires.status));
			return true;
		}
		if(!m_rpc_server->on_get_alternate_chains(req, res, error_resp))
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_OK(boost::lexical_cast<std::string>(res.chains.size()), " alternate chains found:");
	for(const auto &chain : res.chains)
	{
		uint64_t start_height = (chain.height - chain.length + 1);
		GULPSF_PRINT_OK("{} blocks long, from height {} ({} deep), diff {}: {}", chain.length , start_height , (ires.height - start_height - 1)
							, chain.difficulty , chain.block_hash);
	}
	return true;
}

bool t_rpc_command_executor::print_blockchain_dynamic_stats(uint64_t nblocks)
{
	cryptonote::COMMAND_RPC_GET_INFO::request ireq;
	cryptonote::COMMAND_RPC_GET_INFO::response ires;
	cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::request bhreq;
	cryptonote::COMMAND_RPC_GET_BLOCK_HEADERS_RANGE::response bhres;
	epee::json_rpc::error error_resp;

	std::string fail_message = "Problem fetching info";

	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(ireq, ires, "/getinfo", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_get_info(ireq, ires) || ires.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, ires.status));
			return true;
		}
	}

	GULPSF_PRINT_OK("Height: {}, diff {}, cum. diff {}, target {} sec", ires.height , ires.difficulty , ires.cumulative_difficulty
						, ires.target);

	if(nblocks > 0)
	{
		if(nblocks > ires.height)
			nblocks = ires.height;

		bhreq.start_height = ires.height - nblocks;
		bhreq.end_height = ires.height - 1;
		if(m_is_rpc)
		{
			if(!m_rpc_client->json_rpc_request(bhreq, bhres, "getblockheadersrange", fail_message.c_str()))
			{
				return true;
			}
		}
		else
		{
			if(!m_rpc_server->on_get_block_headers_range(bhreq, bhres, error_resp) || bhres.status != CORE_RPC_STATUS_OK)
			{
				GULPS_PRINT_FAIL( make_error(fail_message, bhres.status));
				return true;
			}
		}

		double avgdiff = 0;
		double avgnumtxes = 0;
		double avgreward = 0;
		std::vector<uint64_t> sizes;
		sizes.reserve(nblocks);
		uint64_t earliest = std::numeric_limits<uint64_t>::max(), latest = 0;
		std::vector<unsigned> major_versions(256, 0), minor_versions(256, 0);
		for(const auto &bhr : bhres.headers)
		{
			avgdiff += bhr.difficulty;
			avgnumtxes += bhr.num_txes;
			avgreward += bhr.reward;
			sizes.push_back(bhr.block_size);
			static_assert(sizeof(bhr.major_version) == 1, "major_version expected to be uint8_t");
			static_assert(sizeof(bhr.minor_version) == 1, "major_version expected to be uint8_t");
			major_versions[(unsigned)bhr.major_version]++;
			minor_versions[(unsigned)bhr.minor_version]++;
			earliest = std::min(earliest, bhr.timestamp);
			latest = std::max(latest, bhr.timestamp);
		}
		avgdiff /= nblocks;
		avgnumtxes /= nblocks;
		avgreward /= nblocks;
		uint64_t median_block_size = epee::misc_utils::median(sizes);
		GULPSF_PRINT_OK("Last {}: avg. diff {}, {} avg sec/block, avg num txes {}, avg. reward {}, median block size {}", nblocks , (uint64_t)avgdiff , (latest - earliest) / nblocks , avgnumtxes
							, cryptonote::print_money(avgreward) , median_block_size);

		unsigned int max_major = 256, max_minor = 256;
		while(max_major > 0 && !major_versions[--max_major])
			;
		while(max_minor > 0 && !minor_versions[--max_minor])
			;
		std::string s = "";
		for(unsigned n = 0; n <= max_major; ++n)
			if(major_versions[n])
				s += (s.empty() ? "" : ", ") + boost::lexical_cast<std::string>(major_versions[n]) + std::string(" v") + boost::lexical_cast<std::string>(n);
		GULPSF_PRINT_OK("Block versions: {}", s);
		s = "";
		for(unsigned n = 0; n <= max_minor; ++n)
			if(minor_versions[n])
				s += (s.empty() ? "" : ", ") + boost::lexical_cast<std::string>(minor_versions[n]) + std::string(" v") + boost::lexical_cast<std::string>(n);
		GULPSF_PRINT_OK("Voting for: {}", s);
	}
	return true;
}

bool t_rpc_command_executor::update(const std::string &command)
{
	cryptonote::COMMAND_RPC_UPDATE::request req;
	cryptonote::COMMAND_RPC_UPDATE::response res;
	epee::json_rpc::error error_resp;

	std::string fail_message = "Problem fetching info";

	req.command = command;
	if(m_is_rpc)
	{
		if(!m_rpc_client->rpc_request(req, res, "/update", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_update(req, res) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	if(!res.update)
	{
		GULPS_PRINT_OK( "No update available");
		return true;
	}

	GULPSF_PRINT_OK("Update available: v{}: {}, hash {}", res.version , res.user_uri , res.hash);
	if(command == "check")
		return true;

	if(!res.path.empty())
		GULPSF_PRINT_OK("Update downloaded to: {}", res.path);
	else
		GULPSF_PRINT_OK("Update download failed: {}", res.status);
	if(command == "download")
		return true;

	GULPS_PRINT_OK( "'update' not implemented yet");

	return true;
}

bool t_rpc_command_executor::relay_tx(const std::string &txid)
{
	cryptonote::COMMAND_RPC_RELAY_TX::request req;
	cryptonote::COMMAND_RPC_RELAY_TX::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	req.txids.push_back(txid);

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "relay_tx", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_relay_tx(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	GULPS_PRINT_SUCCESS( "Transaction successfully relayed");
	return true;
}

bool t_rpc_command_executor::sync_info()
{
	cryptonote::COMMAND_RPC_SYNC_INFO::request req;
	cryptonote::COMMAND_RPC_SYNC_INFO::response res;
	std::string fail_message = "Unsuccessful";
	epee::json_rpc::error error_resp;

	if(m_is_rpc)
	{
		if(!m_rpc_client->json_rpc_request(req, res, "sync_info", fail_message.c_str()))
		{
			return true;
		}
	}
	else
	{
		if(!m_rpc_server->on_sync_info(req, res, error_resp) || res.status != CORE_RPC_STATUS_OK)
		{
			GULPS_PRINT_FAIL( make_error(fail_message, res.status));
			return true;
		}
	}

	uint64_t target = res.target_height < res.height ? res.height : res.target_height;
	GULPSF_PRINT_SUCCESS("Height: {}, target: {} ({}%)", res.height, target, (100.0 * res.height / target));
	uint64_t current_download = 0;
	for(const auto &p : res.peers)
		current_download += p.info.current_download;
	GULPS_PRINT_CLR(gulps::COLOR_GREEN,"Downloading at ", current_download, " kB/s");

	GULPS_PRINT_SUCCESS(std::to_string(res.peers.size()), " peers");
	for(const auto &p : res.peers)
	{
		std::string address = epee::string_tools::pad_string(p.info.address, 24);
		uint64_t nblocks = 0, size = 0;
		for(const auto &s : res.spans)
			if(s.rate > 0.0f && s.connection_id == p.info.connection_id)
				nblocks += s.nblocks, size += s.size;
		GULPSF_PRINT_SUCCESS("{} {} {} {} kB/s, {} blocks / {} MB queued", address, epee::string_tools::pad_string(p.info.peer_id, 16, '0', true), p.info.height, p.info.current_download, nblocks, size / 1e6);
	}

	uint64_t total_size = 0;
	for(const auto &s : res.spans)
		total_size += s.size;
	GULPSF_PRINT_SUCCESS("{} spand, {}  MB", std::to_string(res.spans.size()), total_size / 1e6);
	for(const auto &s : res.spans)
	{
		std::string address = epee::string_tools::pad_string(s.remote_address, 24);
		if(s.size == 0)
		{
			GULPSF_PRINT_SUCCESS("{} {} ({} - {}) -", address, s.nblocks, s.start_block_height, (s.start_block_height + s.nblocks - 1));
		}
		else
		{
			GULPSF_PRINT_SUCCESS("{} {} ({} - {}, {}kB) {} kB/s ({})", address, s.nblocks, s.start_block_height, (s.start_block_height + s.nblocks - 1), (uint64_t)(s.size / 1e3), (unsigned)(s.rate / 1e3), s.speed / 100.0f);
		}
	}

	return true;
}

} // namespace daemonize
