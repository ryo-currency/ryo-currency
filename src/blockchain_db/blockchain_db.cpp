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

#include <boost/range/adaptor/reversed.hpp>
#include <unordered_set>

#include "blockchain_db.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "profile_tools.h"
#include "ringct/rctOps.h"
#include "string_tools.h"

#include "lmdb/db_lmdb.h"
#ifdef BERKELEY_DB
#include "berkeleydb/db_bdb.h"
#endif

static const char *db_types[] = {
	"lmdb",
#ifdef BERKELEY_DB
	"berkeley",
#endif
	NULL};

//#undef RYO_DEFAULT_LOG_CATEGORY
//#define RYO_DEFAULT_LOG_CATEGORY "blockchain.db"

using epee::string_tools::pod_to_hex;

namespace cryptonote
{

bool blockchain_valid_db_type(const std::string &db_type)
{
	int i;
	for(i = 0; db_types[i]; i++)
	{
		if(db_types[i] == db_type)
			return true;
	}
	return false;
}

std::string blockchain_db_types(const std::string &sep)
{
	int i;
	std::string ret = "";
	for(i = 0; db_types[i]; i++)
	{
		if(i)
			ret += sep;
		ret += db_types[i];
	}
	return ret;
}

std::string arg_db_type_description = "Specify database type, available: " + cryptonote::blockchain_db_types(", ");
const command_line::arg_descriptor<std::string> arg_db_type = {
	"db-type", arg_db_type_description.c_str(), DEFAULT_DB_TYPE};
const command_line::arg_descriptor<std::string> arg_db_sync_mode = {
	"db-sync-mode", "Specify sync option, using format [safe|fast|fastest]:[sync|async]:[nblocks_per_sync].", "fast:async:1000"};
const command_line::arg_descriptor<bool> arg_db_salvage = {
	"db-salvage", "Try to salvage a blockchain database if it seems corrupted", false};

BlockchainDB *new_db(const std::string &db_type)
{
	if(db_type == "lmdb")
		return new BlockchainLMDB();
#if defined(BERKELEY_DB)
	if(db_type == "berkeley")
		return new BlockchainBDB();
#endif
	return NULL;
}

void BlockchainDB::init_options(boost::program_options::options_description &desc)
{
	command_line::add_arg(desc, arg_db_type);
	command_line::add_arg(desc, arg_db_sync_mode);
	command_line::add_arg(desc, arg_db_salvage);
}

void BlockchainDB::pop_block()
{
	block blk;
	std::vector<transaction> txs;
	pop_block(blk, txs);
}

void BlockchainDB::add_transaction(const crypto::hash &blk_hash, const transaction &tx, const crypto::hash *tx_hash_ptr)
{
	bool miner_tx = false;
	crypto::hash tx_hash;
	
	if(!tx_hash_ptr)
	{
		// should only need to compute hash for miner transactions
		tx_hash = get_transaction_hash(tx);
		LOG_PRINT_L3("null tx_hash_ptr - needed to compute: " << tx_hash);
	}
	else
	{
		tx_hash = *tx_hash_ptr;
	}

	for(const txin_v &tx_input : tx.vin)
	{
		if(tx_input.type() == typeid(txin_to_key))
		{
			add_spent_key(boost::get<txin_to_key>(tx_input).k_image);
		}
		else if(tx_input.type() == typeid(txin_gen))
		{
			/* nothing to do here */
			miner_tx = true;
		}
		else
		{
			LOG_PRINT_L1("Unsupported input type, removing key images and aborting transaction addition");
			for(const txin_v &tx_input : tx.vin)
			{
				if(tx_input.type() == typeid(txin_to_key))
				{
					remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
				}
			}
			return;
		}
	}

	uint64_t tx_id = add_transaction_data(blk_hash, tx, tx_hash);

	std::vector<uint64_t> amount_output_indices;

	// iterate tx.vout using indices instead of C++11 foreach syntax because
	// we need the index
	for(uint64_t i = 0; i < tx.vout.size(); ++i)
	{
		// miner v2 txes have their coinbase output in one single out to save space,
		// and we store them as rct outputs with an identity mask
		if(miner_tx)
		{
			cryptonote::tx_out vout = tx.vout[i];
			rct::key commitment = rct::zeroCommit(vout.amount);

			if(is_vout_bad(vout))
				continue;

			vout.amount = 0;
			amount_output_indices.push_back(add_output(tx_hash, vout, i, tx.unlock_time, &commitment));
		}
		else
		{
			if(tx.vout[i].amount != 0)
			{
				//Better be safe than sorry
				LOG_PRINT_L1("Unsupported index type, removing key images and aborting transaction addition");
				for(const txin_v &tx_input : tx.vin)
				{
					if(tx_input.type() == typeid(txin_to_key))
						remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
				}
				return;
			}

			amount_output_indices.push_back(add_output(tx_hash, tx.vout[i], i, tx.unlock_time, &tx.rct_signatures.outPk[i].mask));
		}
	}

	add_tx_amount_output_indices(tx_id, amount_output_indices);
}

uint64_t BlockchainDB::add_block(const block &blk, const size_t &block_size, const difficulty_type &cumulative_difficulty, const uint64_t &coins_generated, const std::vector<transaction> &txs)
{
	// sanity
	if(blk.tx_hashes.size() != txs.size())
		throw std::runtime_error("Inconsistent tx/hashes sizes");

	block_txn_start(false);

	TIME_MEASURE_START(time1);
	crypto::hash blk_hash = get_block_hash(blk);
	TIME_MEASURE_FINISH(time1);
	time_blk_hash += time1;

	uint64_t prev_height = height();

	// call out to add the transactions

	time1 = epee::misc_utils::get_tick_count();
	add_transaction(blk_hash, blk.miner_tx);
	int tx_i = 0;
	crypto::hash tx_hash = crypto::null_hash;
	for(const transaction &tx : txs)
	{
		tx_hash = blk.tx_hashes[tx_i];
		add_transaction(blk_hash, tx, &tx_hash);
		++tx_i;
	}
	TIME_MEASURE_FINISH(time1);
	time_add_transaction += time1;

	// call out to subclass implementation to add the block & metadata
	time1 = epee::misc_utils::get_tick_count();
	add_block(blk, block_size, cumulative_difficulty, coins_generated, blk_hash);
	TIME_MEASURE_FINISH(time1);
	time_add_block1 += time1;

	m_hardfork->add(blk, prev_height);

	block_txn_stop();

	++num_calls;

	return prev_height;
}

void BlockchainDB::set_hard_fork(HardFork *hf)
{
	m_hardfork = hf;
}

void BlockchainDB::pop_block(block &blk, std::vector<transaction> &txs)
{
	blk = get_top_block();

	remove_block();

	for(const auto &h : boost::adaptors::reverse(blk.tx_hashes))
	{
		txs.push_back(get_tx(h));
		remove_transaction(h);
	}
	remove_transaction(get_transaction_hash(blk.miner_tx));
}

bool BlockchainDB::is_open() const
{
	return m_open;
}

void BlockchainDB::remove_transaction(const crypto::hash &tx_hash)
{
	transaction tx = get_tx(tx_hash);

	for(const txin_v &tx_input : tx.vin)
	{
		if(tx_input.type() == typeid(txin_to_key))
		{
			remove_spent_key(boost::get<txin_to_key>(tx_input).k_image);
		}
	}

	// need tx as tx.vout has the tx outputs, and the output amounts are needed
	remove_transaction_data(tx_hash, tx);
}

block BlockchainDB::get_block_from_height(const uint64_t &height) const
{
	blobdata bd = get_block_blob_from_height(height);
	block b;
	if(!parse_and_validate_block_from_blob(bd, b))
		throw DB_ERROR("Failed to parse block from blob retrieved from the db");

	return b;
}

block BlockchainDB::get_block(const crypto::hash &h) const
{
	blobdata bd = get_block_blob(h);
	block b;
	if(!parse_and_validate_block_from_blob(bd, b))
		throw DB_ERROR("Failed to parse block from blob retrieved from the db");

	return b;
}

bool BlockchainDB::get_tx(const crypto::hash &h, cryptonote::transaction &tx) const
{
	blobdata bd;
	if(!get_tx_blob(h, bd))
		return false;
	if(!parse_and_validate_tx_from_blob(bd, tx))
		throw DB_ERROR("Failed to parse transaction from blob retrieved from the db");

	return true;
}

transaction BlockchainDB::get_tx(const crypto::hash &h) const
{
	transaction tx;
	if(!get_tx(h, tx))
		throw TX_DNE(std::string("tx with hash ").append(epee::string_tools::pod_to_hex(h)).append(" not found in db").c_str());
	return tx;
}

void BlockchainDB::reset_stats()
{
	num_calls = 0;
	time_blk_hash = 0;
	time_tx_exists = 0;
	time_add_block1 = 0;
	time_add_transaction = 0;
	time_commit1 = 0;
}

void BlockchainDB::show_stats()
{
	LOG_PRINT_L1(ENDL
				 << "*********************************"
				 << ENDL
				 << "num_calls: " << num_calls
				 << ENDL
				 << "time_blk_hash: " << time_blk_hash << "ms"
				 << ENDL
				 << "time_tx_exists: " << time_tx_exists << "ms"
				 << ENDL
				 << "time_add_block1: " << time_add_block1 << "ms"
				 << ENDL
				 << "time_add_transaction: " << time_add_transaction << "ms"
				 << ENDL
				 << "time_commit1: " << time_commit1 << "ms"
				 << ENDL
				 << "*********************************"
				 << ENDL);
}

void BlockchainDB::fixup()
{
	static const char* const bad_outs[] =
	{
		// MAINNET
		"cbf2af2d517fce9d4d4a34907541c21ce70ea5cfea5bad887022f790c39c93f1",
		"0a01eaba1c3e73dfe5fc0f70ed9241d06852aadeb5c7905c3fe08731774f5f56",
		"ba37354aaab679aaf670f76cc225605751a44dab88830016464eb530b4d3be7d",
		// TESTNET (can be removed after reset)
		"be7f7851bac9212b655eef88cd791fb6115165ea4b85f9300b2c32a6a428f86b",
		"d0fe21f7e295bc7ee19edc89fd62d56b280f29b0829be23c57e8f15b9587581f",
		"5f2d93718ae1d9993e4ab1d889eba20fe7ab4b14cf958f32d5245ac4204721a0",
		"332bca16211fd4461c4af01b5013174af57ddcf2a1fbe39fbfbf79f9b3802e39",
		"86824ee174575df356ff698f42bb140b07474abb752b7b327403436e0e172836",
		"486cb4f14d10523ed611e10a627a8a8d2197d07a35d21c915121405b76accdac",
		"debd70f178993b4944a235825805b088e3bd1de9bb37e2f0b01266fb66afcc9c",
		"04ebbfbc0d6b148e2967ba9cb0a03c550a385a9a91bff851174ae81a4dd0f56a",
		"7d442156e1f969efdf1047e9be6d4d010120143423e290b2be78cb7a4e4ef764",
		"10d8db43c785c6d42487d34ea2d80669bda52f8641dca04fbd16a89b76cbe200",
		"43954b27e53ae99c8d95c791191222a56b4198fb8869c14d0d4771d8ec84cd09",
		"56865019a362fac052e577bdf7aa6ef30ee24f818e7df87795a35ecd9bd4a61f",
		"5a4a755e42f2ce62b386f01c271c898cb101b5a0695c6aaba348f353e992609a",
		"25ff98493e4c6727c8cc493e3e51038c86b3e8958980b3f78812b8beb89d1786",
		"c69fc6cfe4f47045f118ba6155483252ad9bde2479f06067098d599f6174a183",
		"84cead1b24a45c883273129c5f9bcc5a76356f43ac810e5f4865519041083709",
		"d4d44e4c26f48571b58ec88504dcd7085ad229b3e398113a87b10002b6dfb958",
		"4dd12c5f366b89793241ee60650eaae27696a78a9db9f3b035ad3ca1f3b6aff3",
		"9ef2853efde2a2ccea3008022942a038485aa0f73c02796d733400bec6f8f5b8",
		"e1fddbb7719350109d7a4bc12d69aad4eba0aaf76c60ffabaef09aa6aed812dd",
		"085362d5dee8d582dbff35ed112d44bedcfe593ac077a50418fe976d47f804c9",
		"be5d563670f1abcb34725cb785cb74c8ce5bb731a42ba6a463d12dc60ec0e857",
		"acbf8571f64ac0a2ebfdeee143380178a47a65ca53da4e9cbdcc15ee669e3626",
		"4d341a37b72e8fdcf858b396e72922d3008cc6928bfcbb6ad46e58f21e5dbc45",
		"07ad7890342a4606ed173d77fa325601588f3e2a162186ce0863d79476775d10",
		"f6559054cffd7bf41e2c877bec2c291e1f506930664918b17d9031fbb0630b3a",
		"d591a395a3e064c061f3274cd13f1be69e4326cf8e981afc33a22da3926e47aa",
		"0c172a87e222101653b98791e144ed3e7743a449d41c7b806fbd2599df2be542",
		"d07c8564d4f20ac3cd84fb0d491b3be7600bd8a6b16e114d0fd32229bc3b134c",
		"7f60e247722794a2ee6b81fe74dcadd181f261271d83c7b6450b857d30628d93",
		"e908cb2b95b1d5b5c1c8806e5d367505e0edd08af9e46a3acd27e65de7ec1791",
		"f805eed90bbac948ebfed07936dd1adcd9aad2a89219190e0c581f1d2a17ac8b",
		"1bb0f4fc322f73195715fd3eb76c2ccec89b4552205b9d3073c75aaa9585a910",
		"2c71bc8265ccddf7052eca3d9c7285b645335268f79f0ece024252af79b6c370",
		"b0274f72ae29152cf14d6399db9ab5fb44804299faf864ca41de5c64adc6ba97",
		"7e5fe82fde054ccfa20e8b55e52257dbd1037f6e88003161b4a06641e0172ccd",
		"b07e101bda7498e182c077b59c6729744b9e5c786f3913e0d8822dac0067b823",
		"a06905dc4f8077170e92a40cdbf86aba151fc1dde13bddc06a0c05238df83875",
		"d1d8932a5424456ce46bd955e96dfa974943a82a9b942c573f92e1244bf7e0e2",
		"f5dd07996f178649de9a66a41b4100e1bd980aecf57fb5d5be6d6e5b206490e0",
		"dc3da93e180e7782cd173dfaf43f238853c71aa18947803016f7ba807a78fcd2",
		"c8c020fdc86bb67c9cc5a0f6ecd71b4ade82b8d447054531cb3b9aa779dce80f",
		"349cf03b67fbf7c0a85983e63be84d2fe522eff37b17d5af1efbdea2d1eb49e2",
		"2b434653c5de9272df701ef9d31753b24d38ffeee90ed4e6e367ba155b64bc2e",
		"3523991dceab0fba97944ecba677df7ee79296ae6313cb91172892a0e069046c",
		"034949e1add41d960b157b869680f21fb95fff185e52bf1d51c18ee99c4079d9",
		"8a3302b807c5e795326c6ebf66207bc6611239deab623ea77f31a43204530067",
		"4daf2699a1fe3cab54be13f9433d91535545b673c0a65e5553f12b7d59362b23",
		"5c660d08f0c99abce7b5527c06cc952da5c656528ffe228eccbb37496618af4f",
		"b0252f83be099a6ca01ae8c9ad44a311809607ef2f0409b7f9eee53b79dde1e1",
		"b768109a884de5b3f77a612294890b9a7829512fc9ee03f8588467a0b5758799",
		"100b4af79642718f3e9616b7750b0aa75ddb8ee1364738247fca6269515c049b",
		"98f6ef660bba7219ec0ace316cf5160d82367f19a79d5268aee1593d47916ecd",
		"21066d11dea909852386a35406f22723aa1278a6c6b92d23a7f91ec1dee8e6cf",
		"d6ec7585d01481632282f0e760b4fe70ddcf404eff014f7c0291858adb2c27b2",
		"60e88705bc3e321ac23bf43e075a959bc901d3c5f1d7eb670324e99a016859e2",
		"64c171d999b5ff54f38147c7a2d89ee766f3846d7d1fcec80448e3c26b19e600",
		"5dc39951f578a5abf5e2426c1702a688c4fdebea778d2d50c5f0a289284bf8d7",
		"6a9c6f90f638bd7b8ebf03f132a066e0a13f00aba00ac816eb74495f35b429e9",
		"a93f4b5d953e41c734792e1bcd07fec23ecb7aa811ca3f1cbd929361129bd2ab",
		"1d3ba142f5d0473c245e2a38b925f1bbfbc727d2092987489d1ecc305a0ec8d2",
		"f6bcf52f4c8343f233ab60f4ab052f5ca3d02ae3dfc42280c658f8bf88948f7b",
		"e6c15a9cdede6c834cb2beb749ec73ba4a70f1d7c80ff76b123302b2995aa0a5",
		"709ca76c5ca373c6d26c2439a8b455e9760d45a20ae8d28885af56208eef4dfe",
		"767131138ecf4a21c4559d47dc12dd8436654a623bce04441cbc86ef482e9006",
		"2403e8412ac0aa5ac373f1eeea3e91d6786570dfee50ff575e25a0d2361094b1",
		"f538740bbc0f9f747ccbb98f7a33c01def8d575c31dcd21ac989a69c30320a81",
		"c00a283b0f8a39fb4f13c1813e6a23f57c813c966c2506c2666e51e18f6c1c80",
		"9bb310e9851a1d5812f61bacc1f950543a6b0f315014b92af62b4bdaf88d2f02",
		"5c547633ffde4aac9f01e2f5755ea5c1825389e05bfe0f04e9c623dd868d0b15",
		"9f413087b24254113c15bfe42c11e54bec9b99dd005daef38bd94945ca090fe2",
		"a4f8b2b98d64b8260b34235827dc062eb7c3b12470b035a543b22474d512fa44",
		"5c4dce7971f2c870634dcf2a33737e9170ecccebc86fd8b606e47510dd331dff",
		"56f0b9070c57597aac761b731e3be1b11cca436a28d82720c5ad0e2fb31d10be",
		"5fdbe5b41bbb16d3c389e2623d6a059af309d2cb459675b545539fcdc1d9ad4f",
		"188095c0f40109d7c5be8fe26a334e8e9a2eb705632e9c80e7a236305123d1c1",
		"f4fec3f638538ab804cbe804b772e2488812f45dc1297d590b3c74df89b90df2",
		"a1ca4d514060ddd2eb653f4d713989779830bbdaa2c09bfcfd64f6db7f313838",
		"c192f61aa58d4afec1d3df3333400d013172a0fb5e7fcc5b24a72c2dec6a17c2",
		"61dadd48f85f3f257a73e265a5f4254fcfbcc381e0dd116a59fa4008f233e89f",
		"1ca1fc6e1b7d4bcd6491144f92d2e26b1e1c8e261bd2083e87097601042249fb",
		"09febbf13dc54262238390e78cd7bee7b87f3c5618b226e31f5bd1fc11582604",
		"ab79483ec89bb9510e47cff0ec06a86a0dd644eca17e677216d342f87d6d273e",
		"92184d7066234393f9e9b399d8f862502ac566aa0492e69236d6229939e6caa1",
		"7d1b294447301f680d5ec098367f6a00f621cb92b0e43d164ddd61acc945ded0",
		"348d133e930d7437db29cc1405ed4bc405b000dd3be04e2f272bbca687149939",
		"0004f6862cbc378cac77d545119fbbc7dd01c2262fb2d910fb121f0a8bfa7ade",
		"8adfea203a6a6dc156fbeec3ea2bf318d12d13f8a828b22e5c108415c19ed816",
		"f5dca52e48f99ef579dc2bd26e1251c444460c9d1ee0307a6d8979de7aefe787",
		"0e677ed6e6f50c9df26079815d64618a10780386ffd810f8c130673f84a9ac84",
		"ded76dd770425208e9de1d53bc7f5dd5c81e03c17da976344081ad4a28eae35e",
		"6b9c14ae5579dd42da258b0dd91b9e35c9d77e9705e489466c7b5b02756ea39a",
		"754a7934bdc59b58230caf725439075cc35f4347978b5ff7700e53533dcbdc65",
		"09091bd0e2400c771ab0531face28092707eb963e8a4b59c3a7cfbe15a885b46",
		"6497d71db7fd3d6a584501fefdae57f062f61217b955b44b417a1821c4bb9d83",
		"22e3f1d73312e97082d78f84d00192df3ab757f46a6e3010f9d95a44af5cd05b",
		"aa479a5b23d2957af65ec90de2fd68aa5cfd6ed926fd80c5e5b8f7779ccbd81b",
		"94f2888b7a6e58fe9afd0790fcf9ae5538441ef6c8701a8950bbd32184005d4f",
		"ca53d473691d485539144d6ac188ca7c8e0ff8ae41ad953426d21afd4bf50cbf",
		"af83c3b56b9ddb8b7001d935f706b31a823a8f41b32e1cbc4a461d7b7d705ccb",
		"7b29623c86a507eaec6be2aab037079216b0621e2f02c9f6292c2724e12be204",
		"f6d689bd9b415f9cf0e9d3297dc7d1c5bf9c583a642b0a1fcaad41c28b0480ba",
		"d4f6894e514b1900fd677aa9c769e47d878107d1876877c13ce0fe0bbaa3bc1c",
		"e1fdb26fa136364631c957d7df86cb66dbbbfd4066ebff7ae2a64b247290d74d",
		"fdb21d778cf92ac19a0bf20134777d521ad5f9acf0273f7451f917005d63ec33",
		"e7b9f891b6331fb2f7c9b9337978fc34fd22099a440dd5d38bb1be7a5d6159d1",
		"0da0a1c08cdee5086e28544f520e78a2361de00b6de241ebc4cb30dfa13fa36a",
		"6fb42d7bc2b81bd86e7b46575f1acf9233c613748404f14f627f60fe104641b8",
		"d8caeaedd9ea28fec32199439b8141a8b5739074c08f3c386e72ddced31e0f46",
		"4c6d6443209f95d05823375799010c7c483ecb128424740214571aeb78251fc7",
		"56c9ff6d0cdfe7dd351d4692ab5ea44c223cba08ecf81dcd1f437398d82f8a09",
		"99c2e9b06e5e6dfb31d4235ba4b6a9bc0c48d8c14119b64265c1bfa67877acce"
	};

	for(const auto &spk : bad_outs)
	{
		crypto::public_key pk;
		epee::string_tools::hex_to_pod(spk, pk);
		bad_outpks.insert(pk);
	}

	if(is_read_only())
	{
		LOG_PRINT_L1("Database is opened read only - skipping fixup check");
		return;
	}

	set_batch_transactions(true);
	batch_start();

	// premine key images 
	static const char* const premine_key_images[] =
	{
		"a42fa875b187e7f9e8d25ad158187458cdcce0db9582b5ccd02e9a5d99a79239", //txid 29b65a4ddd5ad2502f4a5e536651de577837fef2b62e1eeccf518806a5195d98
		"89018dae2d6283edecf4df800bfdc56f1768fef450034dc9868388ac83cd045b", //txid c06ef81adb318383fef0af64620b35a781a6489b8ab90b574290057a5decd245
		"7caa8fe70872c26069bf1375c24c610702607568f5bc767d58ce027a8aa5ce93", //txid 97c5a2f7aef725270ed86e2407958fb72bcd9845e5293bd8177504f0ec659f86
		"d06f99b903d2633a44bd660fb3482172d4c699dba4b152f2232f25cc4b89fc40", //txid 29b65a4ddd5ad2502f4a5e536651de577837fef2b62e1eeccf518806a5195d98
		"2b241e39ee08d1f4292083fd7cfb0021f285a511933a1eaae3172dbf43fcf60d", //txid c06ef81adb318383fef0af64620b35a781a6489b8ab90b574290057a5decd245
		"5f14a472fe32f1c9da214133f58da9df2926b815945effdbf6fadd00d171a51a", //txid 97c5a2f7aef725270ed86e2407958fb72bcd9845e5293bd8177504f0ec659f86
	};

	for(const auto &kis : premine_key_images)
	{
		crypto::key_image ki;
		epee::string_tools::hex_to_pod(kis, ki);
		if(!has_key_image(ki))
		{
			LOG_PRINT_L1("Fixup: adding missing spent key " << ki);
			add_spent_key(ki);
		}
	}
	batch_stop();
}

} // namespace cryptonote
