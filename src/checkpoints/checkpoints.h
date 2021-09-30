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

#pragma once
#include "crypto/hash.h"
#include "cryptonote_config.h"
#include "common/gulps.hpp"
#include <map>
#include <vector>

#include <boost/bind/placeholders.hpp>

using boost::placeholders::_1;
using boost::placeholders::_2;

#define ADD_CHECKPOINT(h, hash) GULPS_CHECK_AND_ASSERT(add_checkpoint(h, hash), false);
#define JSON_HASH_FILE_NAME "checkpoints.json"

namespace cryptonote
{
/**
   * @brief A container for blockchain checkpoints
   *
   * A checkpoint is a pre-defined hash for the block at a given height.
   * Some of these are compiled-in, while others can be loaded at runtime
   * either from a json file or via DNS from a checkpoint-hosting server.
   */
class checkpoints
{
  public:
	/**
     * @brief default constructor
     */
	checkpoints();

	/**
     * @brief adds a checkpoint to the container
     *
     * @param height the height of the block the checkpoint is for
     * @param hash_str the hash of the block, as a string
     *
     * @return false if parsing the hash fails, or if the height is a duplicate
     *         AND the existing checkpoint hash does not match the new one,
     *         otherwise returns true
     */
	bool add_checkpoint(uint64_t height, const std::string &hash_str);

	/**
     * @brief checks if there is a checkpoint in the future
     *
     * This function checks if the height passed is lower than the highest
     * checkpoint.
     *
     * @param height the height to check against
     *
     * @return false if no checkpoints, otherwise returns whether or not
     *         the height passed is lower than the highest checkpoint.
     */
	bool is_in_checkpoint_zone(uint64_t height) const;

	/**
     * @brief checks if the given height and hash agree with the checkpoints
     *
     * This function checks if the given height and hash exist in the
     * checkpoints container.  If so, it returns whether or not the passed
     * parameters match the stored values.
     *
     * @param height the height to be checked
     * @param h the hash to be checked
     * @param is_a_checkpoint return-by-reference if there is a checkpoint at the given height
     *
     * @return true if there is no checkpoint at the given height,
     *         true if the passed parameters match the stored checkpoint,
     *         false otherwise
     */
	bool check_block(uint64_t height, const crypto::hash &h, bool &is_a_checkpoint) const;

	/**
     * @overload
     */
	bool check_block(uint64_t height, const crypto::hash &h) const;

	/**
     * @brief checks if alternate chain blocks should be kept for a given height
     *
     * this basically says if the blockchain is smaller than the first
     * checkpoint then alternate blocks are allowed.  Alternatively, if the
     * last checkpoint *before* the end of the current chain is also before
     * the block to be added, then this is fine.
     *
     * @param blockchain_height the current blockchain height
     * @param block_height the height of the block to be added as alternate
     *
     * @return true if alternate blocks are allowed given the parameters,
     *         otherwise false
     */
	bool is_alternative_block_allowed(uint64_t blockchain_height, uint64_t block_height) const;

	/**
     * @brief gets the highest checkpoint height
     *
     * @return the height of the highest checkpoint
     */
	uint64_t get_max_height() const;

	/**
     * @brief gets the checkpoints container
     *
     * @return a const reference to the checkpoints container
     */
	const std::map<uint64_t, crypto::hash> &get_points() const;

	/**
     * @brief checks if our checkpoints container conflicts with another
     *
     * A conflict refers to a case where both checkpoint sets have a checkpoint
     * for a specific height but their hashes for that height do not match.
     *
     * @param other the other checkpoints instance to check against
     *
     * @return false if any conflict is found, otherwise true
     */
	bool check_for_conflicts(const checkpoints &other) const;

	/**
     * @brief loads the default main chain checkpoints
     * @param nettype network type
     *
     * @return true unless adding a checkpoint fails
     */
	bool init_default_checkpoints(network_type nettype);

	/**
     * @brief load new checkpoints
     *
     * Loads new checkpoints from the specified json file, as well as
     * (optionally) from DNS.
     *
     * @param json_hashfile_fullpath path to the json checkpoints file
     * @param nettype network type
     * @param dns whether or not to load DNS checkpoints
     *
     * @return true if loading successful and no conflicts
     */
	bool load_new_checkpoints(const std::string &json_hashfile_fullpath, network_type nettype = MAINNET, bool dns = true);

	/**
     * @brief load new checkpoints from json
     *
     * @param json_hashfile_fullpath path to the json checkpoints file
     *
     * @return true if loading successful and no conflicts
     */
	bool load_checkpoints_from_json(const std::string &json_hashfile_fullpath);

	/**
     * @brief load new checkpoints from DNS
     *
     * @param nettype network type
     *
     * @return true if loading successful and no conflicts
     */
	bool load_checkpoints_from_dns(network_type nettype = MAINNET);

  private:
	std::map<uint64_t, crypto::hash> m_points; //!< the checkpoints container
};
}
