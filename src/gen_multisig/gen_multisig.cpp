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

/*!
 * \file gen_multisig.cpp
 * 
 * \brief Generates a set of multisig wallets
 */
#include "common/command_line.h"
#include "common/i18n.h"
#include "common/scoped_message_writer.h"
#include "common/util.h"
#include "crypto/crypto.h" // for crypto::secret_key definition
#include "include_base_utils.h"
#include "wallet/wallet2.h"
#include "wallet/wallet_args.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <sstream>

using namespace std;
using namespace epee;
using namespace cryptonote;
using boost::lexical_cast;
namespace po = boost::program_options;

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "wallet.gen_multisig"

namespace genms
{
const char *tr(const char *str)
{
	return i18n_translate(str, "tools::gen_multisig");
}
}

namespace
{
const command_line::arg_descriptor<std::string> arg_filename_base = {"filename-base", genms::tr("Base filename (-1, -2, etc suffixes will be appended as needed)"), ""};
const command_line::arg_descriptor<std::string> arg_scheme = {"scheme", genms::tr("Give threshold and participants at once as M/N"), ""};
const command_line::arg_descriptor<uint32_t> arg_participants = {"participants", genms::tr("How many participants will share parts of the multisig wallet"), 0};
const command_line::arg_descriptor<uint32_t> arg_threshold = {"threshold", genms::tr("How many signers are required to sign a valid transaction"), 0};
const command_line::arg_descriptor<bool, false> arg_testnet = {"testnet", genms::tr("Create testnet multisig wallets"), false};
const command_line::arg_descriptor<bool, false> arg_stagenet = {"stagenet", genms::tr("Create stagenet multisig wallets"), false};
const command_line::arg_descriptor<bool, false> arg_create_address_file = {"create-address-file", genms::tr("Create an address file for new wallets"), false};

const command_line::arg_descriptor<std::vector<std::string>> arg_command = {"command", ""};
}

static bool generate_multisig(uint32_t threshold, uint32_t total, const std::string &basename, network_type nettype, bool create_address_file)
{
	tools::msg_writer() << (boost::format(genms::tr("Generating %u %u/%u multisig wallets")) % total % threshold % total).str();

	const auto pwd_container = tools::password_container::prompt(true, "Enter password for new multisig wallets");

	try
	{
		// create M wallets first
		std::vector<boost::shared_ptr<tools::wallet2>> wallets(total);
		for(size_t n = 0; n < total; ++n)
		{
			std::string name = basename + "-" + std::to_string(n + 1);
			wallets[n].reset(new tools::wallet2(nettype));
			wallets[n]->init("");
			crypto::secret_key sk = rct::rct2sk(rct::skGen());
			wallets[n]->generate_legacy(name, pwd_container->password(), sk, create_address_file);
		}

		// gather the keys
		std::vector<crypto::secret_key> sk(total);
		std::vector<crypto::public_key> pk(total);
		for(size_t n = 0; n < total; ++n)
		{
			if(!tools::wallet2::verify_multisig_info(wallets[n]->get_multisig_info(), sk[n], pk[n]))
			{
				tools::fail_msg_writer() << tr("Failed to verify multisig info");
				return false;
			}
		}

		// make the wallets multisig
		std::vector<std::string> extra_info(total);
		std::stringstream ss;
		for(size_t n = 0; n < total; ++n)
		{
			std::string name = basename + "-" + std::to_string(n + 1);
			std::vector<crypto::secret_key> skn;
			std::vector<crypto::public_key> pkn;
			for(size_t k = 0; k < total; ++k)
			{
				if(k != n)
				{
					skn.push_back(sk[k]);
					pkn.push_back(pk[k]);
				}
			}
			extra_info[n] = wallets[n]->make_multisig(pwd_container->password(), skn, pkn, threshold);
			ss << "  " << name << std::endl;
		}

		// finalize step if needed
		if(!extra_info[0].empty())
		{
			std::unordered_set<crypto::public_key> pkeys;
			std::vector<crypto::public_key> signers(total);
			for(size_t n = 0; n < total; ++n)
			{
				if(!tools::wallet2::verify_extra_multisig_info(extra_info[n], pkeys, signers[n]))
				{
					tools::fail_msg_writer() << genms::tr("Error verifying multisig extra info");
					return false;
				}
			}
			for(size_t n = 0; n < total; ++n)
			{
				if(!wallets[n]->finalize_multisig(pwd_container->password(), pkeys, signers))
				{
					tools::fail_msg_writer() << genms::tr("Error finalizing multisig");
					return false;
				}
			}
		}

		std::string address = wallets[0]->get_account().get_public_address_str(wallets[0]->nettype());
		tools::success_msg_writer() << genms::tr("Generated multisig wallets for address ") << address << std::endl
									<< ss.str();
	}
	catch(const std::exception &e)
	{
		tools::fail_msg_writer() << genms::tr("Error creating multisig wallets: ") << e.what();
		return false;
	}

	return true;
}

int main(int argc, char *argv[])
{
	po::options_description desc_params(wallet_args::tr("Wallet options"));
	command_line::add_arg(desc_params, arg_filename_base);
	command_line::add_arg(desc_params, arg_scheme);
	command_line::add_arg(desc_params, arg_threshold);
	command_line::add_arg(desc_params, arg_participants);
	command_line::add_arg(desc_params, arg_testnet);
	command_line::add_arg(desc_params, arg_stagenet);
	command_line::add_arg(desc_params, arg_create_address_file);

	int vm_error_code = 1;
	const auto vm = wallet_args::main(
		argc, argv,
		"ryo-gen-multisig [(--testnet|--stagenet)] [--filename-base=<filename>] [--scheme=M/N] [--threshold=M] [--participants=N]",
		genms::tr("This program generates a set of multisig wallets - use this simpler scheme only if all the participants trust each other"),
		desc_params,
		boost::program_options::positional_options_description(),
		[](const std::string &s, bool emphasis) { tools::scoped_message_writer(emphasis ? epee::console_color_white : epee::console_color_default, true) << s; },
		"ryo-gen-multisig.log",
		vm_error_code);
	if(!vm)
		return vm_error_code;

	bool testnet, stagenet;
	uint32_t threshold = 0, total = 0;
	std::string basename;

	testnet = command_line::get_arg(*vm, arg_testnet);
	stagenet = command_line::get_arg(*vm, arg_stagenet);
	if(testnet && stagenet)
	{
		tools::fail_msg_writer() << genms::tr("Error: Can't specify more than one of --testnet and --stagenet");
		return 1;
	}
	if(command_line::has_arg(*vm, arg_scheme))
	{
		if(sscanf(command_line::get_arg(*vm, arg_scheme).c_str(), "%u/%u", &threshold, &total) != 2)
		{
			tools::fail_msg_writer() << genms::tr("Error: expected N/M, but got: ") << command_line::get_arg(*vm, arg_scheme);
			return 1;
		}
	}
	if(!(*vm)["threshold"].defaulted())
	{
		if(threshold)
		{
			tools::fail_msg_writer() << genms::tr("Error: either --scheme or both of --threshold and --participants may be given");
			return 1;
		}
		threshold = command_line::get_arg(*vm, arg_threshold);
	}
	if(!(*vm)["participants"].defaulted())
	{
		if(total)
		{
			tools::fail_msg_writer() << genms::tr("Error: either --scheme or both of --threshold and --participants may be given");
			return 1;
		}
		total = command_line::get_arg(*vm, arg_participants);
	}
	if(threshold <= 1 || threshold > total)
	{
		tools::fail_msg_writer() << (boost::format(genms::tr("Error: expected N > 1 and N <= M, but got N==%u and M==%d")) % threshold % total).str();
		return 1;
	}
	if(!(*vm)["filename-base"].defaulted() && !command_line::get_arg(*vm, arg_filename_base).empty())
	{
		basename = command_line::get_arg(*vm, arg_filename_base);
	}
	else
	{
		tools::fail_msg_writer() << genms::tr("Error: --filename-base is required");
		return 1;
	}

	if(threshold != total - 1 && threshold != total)
	{
		tools::fail_msg_writer() << genms::tr("Error: unsupported scheme: only N/N and N-1/N are supported");
		return 1;
	}
	bool create_address_file = command_line::get_arg(*vm, arg_create_address_file);
	if(!generate_multisig(threshold, total, basename, testnet ? TESTNET : stagenet ? STAGENET : MAINNET, create_address_file))
		return 1;

	return 0;
	//CATCH_ENTRY_L0("main", 1);
}
