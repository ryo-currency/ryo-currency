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

/*!
 * \file electrum-words.h
 * 
 * \brief Mnemonic seed generation and wallet restoration from them.
 * 
 * This file and its cpp file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#ifndef ELECTRUM_WORDS_H
#define ELECTRUM_WORDS_H

#include "crypto/crypto.h" // for declaration of crypto::secret_key
#include <cstdint>
#include <map>
#include <string>

/*!
 * \namespace crypto
 * 
 * \brief crypto namespace.
 */
namespace crypto
{
/*!
   * \namespace crypto::ElectrumWords
   * 
   * \brief Mnemonic seed word generation and wallet restoration helper functions.
   */
namespace Electrum
{
/*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages A vector is set to the list of languages.
     * \param english whether to get the names in English or the language language
     */
void get_language_list(std::vector<std::string> &languages, bool english = false);

/*!
     * \brief Verify and standardise user's input
     * \param input Whatever user gave us
     * \return lanaguage list entry or empty if we fail to match
     */
std::string verify_language_input(std::string input);

/*!
     * \brief Returns the name of a language in English
     * \param  name the name of the language in its own language
     * \return      the name of the language in English
     */
std::string get_english_name_for(const std::string &name);
}

namespace Electrum14Words
{
bool words_to_bytes(std::string words, crypto::secret_key_16 &dst, uint8_t &extra, std::string &language_name);
bool bytes_to_words(const crypto::secret_key_16 &dst, uint8_t extra, std::string &words, const std::string &language_name);
}

namespace Electrum25Words
{

constexpr int seed_length = 24;
/*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret data restored from the words.
     * \param  len             The number of bytes to expect, 0 if unknown
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
bool words_to_bytes(std::string words, std::string &dst, size_t len, std::string &language_name);
/*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret key restored from the words.
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
bool words_to_bytes(std::string words, crypto::secret_key &dst, std::string &language_name);

/*!
     * \brief Converts bytes to seed words.
     * \param  src           Secret data
     * \param  len           Secret data length in bytes (positive multiples of 4 only)
     * \param  words         Space delimited concatenated words get written here.
     * \param  language_name Seed language name
     * \return               true if successful false if not. Unsuccessful if wrong key size.
     */
bool bytes_to_words(const char *src, size_t len, std::string &words, const std::string &language_name);

/*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src           Secret key
     * \param  words         Space delimited concatenated words get written here.
     * \param  language_name Seed language name
     * \return               true if successful false if not. Unsuccessful if wrong key size.
     */
bool bytes_to_words(const crypto::secret_key &src, std::string &words, const std::string &language_name);
}
}

#endif
