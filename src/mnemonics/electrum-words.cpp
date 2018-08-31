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
 * \file electrum-words.cpp
 * 
 * \brief Mnemonic seed generation and wallet restoration from them.
 * 
 * This file and its header file are for translating Electrum-style word lists
 * into their equivalent byte representations for cross-compatibility with
 * that method of "backing up" one's wallet keys.
 */

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/crc.hpp>
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>
#include "common/boost_locale.hpp"

#include "mnemonics/electrum-words.h"
#include "crypto/crypto.h" // for declaration of crypto::secret_key
#include <cassert>
#include <cstdint>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "chinese_simplified.h"
#include "dutch.h"
#include "english.h"
#include "esperanto.h"
#include "french.h"
#include "german.h"
#include "italian.h"
#include "japanese.h"
#include "language_base.h"
#include "lojban.h"
#include "portuguese.h"
#include "russian.h"
#include "singleton.h"
#include "spanish.h"

/*!
 * \namespace crypto
 * 
 * \brief crypto namespace.
 */
namespace crypto
{
// Polynomial chosen using https://users.ece.cmu.edu/~koopman/crc/
// Params width=12  poly=0x987  init=0x000  refin=false  refout=false  xorout=0x000  check=0xf1a  residue=0x000
const uint16_t crc12_table[256] = {
	0x000, 0x987, 0xa89, 0x30e, 0xc95, 0x512, 0x61c, 0xf9b, 0x0ad, 0x92a, 0xa24, 0x3a3, 0xc38, 0x5bf, 0x6b1, 0xf36,
	0x15a, 0x8dd, 0xbd3, 0x254, 0xdcf, 0x448, 0x746, 0xec1, 0x1f7, 0x870, 0xb7e, 0x2f9, 0xd62, 0x4e5, 0x7eb, 0xe6c,
	0x2b4, 0xb33, 0x83d, 0x1ba, 0xe21, 0x7a6, 0x4a8, 0xd2f, 0x219, 0xb9e, 0x890, 0x117, 0xe8c, 0x70b, 0x405, 0xd82,
	0x3ee, 0xa69, 0x967, 0x0e0, 0xf7b, 0x6fc, 0x5f2, 0xc75, 0x343, 0xac4, 0x9ca, 0x04d, 0xfd6, 0x651, 0x55f, 0xcd8,
	0x568, 0xcef, 0xfe1, 0x666, 0x9fd, 0x07a, 0x374, 0xaf3, 0x5c5, 0xc42, 0xf4c, 0x6cb, 0x950, 0x0d7, 0x3d9, 0xa5e,
	0x432, 0xdb5, 0xebb, 0x73c, 0x8a7, 0x120, 0x22e, 0xba9, 0x49f, 0xd18, 0xe16, 0x791, 0x80a, 0x18d, 0x283, 0xb04,
	0x7dc, 0xe5b, 0xd55, 0x4d2, 0xb49, 0x2ce, 0x1c0, 0x847, 0x771, 0xef6, 0xdf8, 0x47f, 0xbe4, 0x263, 0x16d, 0x8ea,
	0x686, 0xf01, 0xc0f, 0x588, 0xa13, 0x394, 0x09a, 0x91d, 0x62b, 0xfac, 0xca2, 0x525, 0xabe, 0x339, 0x037, 0x9b0,
	0xad0, 0x357, 0x059, 0x9de, 0x645, 0xfc2, 0xccc, 0x54b, 0xa7d, 0x3fa, 0x0f4, 0x973, 0x6e8, 0xf6f, 0xc61, 0x5e6,
	0xb8a, 0x20d, 0x103, 0x884, 0x71f, 0xe98, 0xd96, 0x411, 0xb27, 0x2a0, 0x1ae, 0x829, 0x7b2, 0xe35, 0xd3b, 0x4bc,
	0x864, 0x1e3, 0x2ed, 0xb6a, 0x4f1, 0xd76, 0xe78, 0x7ff, 0x8c9, 0x14e, 0x240, 0xbc7, 0x45c, 0xddb, 0xed5, 0x752,
	0x93e, 0x0b9, 0x3b7, 0xa30, 0x5ab, 0xc2c, 0xf22, 0x6a5, 0x993, 0x014, 0x31a, 0xa9d, 0x506, 0xc81, 0xf8f, 0x608,
	0xfb8, 0x63f, 0x531, 0xcb6, 0x32d, 0xaaa, 0x9a4, 0x023, 0xf15, 0x692, 0x59c, 0xc1b, 0x380, 0xa07, 0x909, 0x08e,
	0xee2, 0x765, 0x46b, 0xdec, 0x277, 0xbf0, 0x8fe, 0x179, 0xe4f, 0x7c8, 0x4c6, 0xd41, 0x2da, 0xb5d, 0x853, 0x1d4,
	0xd0c, 0x48b, 0x785, 0xe02, 0x199, 0x81e, 0xb10, 0x297, 0xda1, 0x426, 0x728, 0xeaf, 0x134, 0x8b3, 0xbbd, 0x23a,
	0xc56, 0x5d1, 0x6df, 0xf58, 0x0c3, 0x944, 0xa4a, 0x3cd, 0xcfb, 0x57c, 0x672, 0xff5, 0x06e, 0x9e9, 0xae7, 0x360};

inline void update_crc12(uint32_t &crcv, uint8_t data)
{
	uint8_t idx = uint8_t((crcv >> 4) ^ data);
	crcv = crc12_table[idx] ^ (crcv << 8);
}

uint32_t calc_crc12(const crypto::secret_key_16 &key, uint8_t extra)
{
	uint32_t crcv = 0;
	for(size_t i = 0; i < 16; i++)
		update_crc12(crcv, tools::unwrap(key).data[i]);
	update_crc12(crcv, extra);
	return crcv & 0xfff;
}

/*!
   * \namespace crypto::ElectrumWords
   * 
   * \brief Mnemonic seed word generation and wallet restoration helper functions.
   */
/*!
   * \brief Finds the word list that contains the seed words and puts the indices
   *        where matches occured in matched_indices.
   * \param  seed            List of words to match.
   * \param  has_checksum    The seed has a checksum word (maybe not checked).
   * \param  matched_indices The indices where the seed words were found are added to this.
   * \param  language        Language instance pointer to write to after it is found.
   * \return                 true if all the words were present in some language false if not.
   */
bool find_seed_language(const std::vector<std::string> &seed, Language::Base **language)
{
	// Yes, I don't like std::pair
	struct sort_pair
	{
		Language::Base *inst;
		size_t score;

		inline bool operator<(const sort_pair &oth) const { return score < oth.score; }
	};

	// If there's a new language added, add an instance of it here.
	std::vector<sort_pair> language_instances({{Language::Singleton<Language::Chinese_Simplified>::instance(), 0},
											   {Language::Singleton<Language::English>::instance(), 0},
											   {Language::Singleton<Language::Dutch>::instance(), 0},
											   {Language::Singleton<Language::French>::instance(), 0},
											   {Language::Singleton<Language::Spanish>::instance(), 0},
											   {Language::Singleton<Language::German>::instance(), 0},
											   {Language::Singleton<Language::Italian>::instance(), 0},
											   {Language::Singleton<Language::Portuguese>::instance(), 0},
											   {Language::Singleton<Language::Japanese>::instance(), 0},
											   {Language::Singleton<Language::Russian>::instance(), 0},
											   {Language::Singleton<Language::Esperanto>::instance(), 0},
											   {Language::Singleton<Language::Lojban>::instance(), 0}});

	// Assumption here is that the end user will spell more words correctly than get at random in a foreign lang
	for(sort_pair &pair : language_instances)
	{
		const std::unordered_map<std::string, uint32_t> &word_map = pair.inst->get_word_map();
		for(const std::string &word : seed)
		{
			if(word_map.count(word) > 0)
				pair.score++;
		}
	}

	std::sort(language_instances.begin(), language_instances.end());

	if(language_instances.back().score > 0)
	{
		*language = language_instances.back().inst;
		return true;
	}

	return false;
}

bool match_words(const Language::Base &lang, const std::vector<std::string> &seed, std::vector<uint32_t> &matched_indices)
{
	size_t trim_size = lang.get_unique_prefix_length();
	const std::unordered_map<std::string, uint32_t> &trimmed_word_map = lang.get_trimmed_word_map();
	matched_indices.reserve(seed.size());

	for(const std::string &word : seed)
	{
		std::string trimmed_word = Language::utf8prefix(word, trim_size);
		if(trimmed_word_map.count(trimmed_word) > 0)
			matched_indices.push_back(trimmed_word_map.at(trimmed_word));
		else
			return false;
	}
	return true;
}

namespace Electrum
{

/*!
     * \brief Gets a list of seed languages that are supported.
     * \param languages The vector is set to the list of languages.
     */
const std::vector<const Language::Base *> &get_language_list()
{
	static const std::vector<const Language::Base *> language_instances({Language::Singleton<Language::German>::instance(),
																		 Language::Singleton<Language::English>::instance(),
																		 Language::Singleton<Language::Spanish>::instance(),
																		 Language::Singleton<Language::French>::instance(),
																		 Language::Singleton<Language::Italian>::instance(),
																		 Language::Singleton<Language::Dutch>::instance(),
																		 Language::Singleton<Language::Portuguese>::instance(),
																		 Language::Singleton<Language::Russian>::instance(),
																		 Language::Singleton<Language::Japanese>::instance(),
																		 Language::Singleton<Language::Chinese_Simplified>::instance(),
																		 Language::Singleton<Language::Esperanto>::instance(),
																		 Language::Singleton<Language::Lojban>::instance()});
	return language_instances;
}

void get_language_list(std::vector<std::string> &languages, bool english)
{
	const std::vector<const Language::Base *> language_instances = get_language_list();
	for(std::vector<const Language::Base *>::const_iterator it = language_instances.begin();
		it != language_instances.end(); it++)
	{
		languages.push_back(english ? (*it)->get_english_language_name() : (*it)->get_language_name());
	}
}

std::string get_english_name_for(const std::string &name)
{
	const std::vector<const Language::Base *> language_instances = get_language_list();
	for(std::vector<const Language::Base *>::const_iterator it = language_instances.begin();
		it != language_instances.end(); it++)
	{
		if((*it)->get_language_name() == name)
			return (*it)->get_english_language_name();
	}
	return "<language not found>";
}

std::string verify_language_input(std::string input)
{
	boost_locale_init();
	input = boost::locale::to_title(input);

	for(const Language::Base* lng : get_language_list())
	{
		if(lng->get_language_name() == input)
			return lng->get_language_name();
		
		if(lng->get_english_language_name() == input)
			return lng->get_language_name();
	}
	return "";
}
}

namespace Electrum14Words
{
bool words_to_bytes(std::string words, crypto::secret_key_16 &dst, uint8_t &extra, std::string &language_name)
{
	std::vector<std::string> seed;

	boost::algorithm::trim(words);
	boost::split(seed, words, boost::is_any_of(" "), boost::token_compress_on);

	if(seed.size() != 14 && seed.size() != 12)
		return false;

	Language::Base *language;
	if(!find_seed_language(seed, &language))
		return false;

	std::vector<uint32_t> matched_indices;
	if(!match_words(*language, seed, matched_indices))
		return false;

	language_name = language->get_language_name();
	uint32_t word_list_length = language->get_word_list().size();

	uint32_t *out = (uint32_t *)tools::unwrap(dst).data;
	for(unsigned int i = 0; i < seed.size() / 3; i++)
	{
		uint32_t val;
		uint32_t w1, w2, w3;
		w1 = matched_indices[i * 3];
		w2 = matched_indices[i * 3 + 1];
		w3 = matched_indices[i * 3 + 2];

		val = w1 + word_list_length * (((word_list_length - w1) + w2) % word_list_length) +
			  word_list_length * word_list_length * (((word_list_length - w2) + w3) % word_list_length);

		if(!(val % word_list_length == w1))
			return false;
		out[i] = val;
	}

	if(seed.size() == 14)
	{
		uint32_t val;
		uint32_t w1, w2;
		w1 = matched_indices[12];
		w2 = matched_indices[13];

		val = w1 + word_list_length * (((word_list_length - w1) + w2) % word_list_length);
		val &= 0xfffff; // 20 bytes

		extra = uint8_t(val & 0xff);
		val >>= 8;

		if(calc_crc12(dst, extra) != val)
			return false;
	}

	return true;
}

bool bytes_to_words(const crypto::secret_key_16 &src, uint8_t extra, std::string &words, const std::string &language_name)
{
	const std::vector<const Language::Base *> &lng_insts = Electrum::get_language_list();
	const Language::Base *language = nullptr;
	for(const Language::Base *b : lng_insts)
	{
		if(b->get_language_name() == language_name || b->get_english_language_name() == language_name)
		{
			language = b;
			break;
		}
	}

	if(language == nullptr)
		return false;

	const std::vector<std::string> &word_list = language->get_word_list();

	uint32_t word_list_length = word_list.size();
	const uint32_t *psrc = (const uint32_t *)tools::unwrap(src).data;
	for(unsigned int i = 0; i < 4; i++, words += ' ')
	{
		uint32_t w1, w2, w3;
		uint32_t val = psrc[i];

		w1 = val % word_list_length;
		w2 = ((val / word_list_length) + w1) % word_list_length;
		w3 = (((val / word_list_length) / word_list_length) + w2) % word_list_length;

		words += word_list[w1];
		words += ' ';
		words += word_list[w2];
		words += ' ';
		words += word_list[w3];
	}

	uint32_t w1, w2;
	uint32_t checksum = calc_crc12(src, extra);
	checksum <<= 8;
	checksum |= extra;

	w1 = checksum % word_list_length;
	w2 = ((checksum / word_list_length) + w1) % word_list_length;
	words += word_list[w1];
	words += ' ';
	words += word_list[w2];

	return true;
}
}

namespace Electrum25Words
{
/*!
   * \brief Creates a checksum index in the word list array on the list of words.
   * \param  word_list            Vector of words
   * \param unique_prefix_length  the prefix length of each word to use for checksum
   * \return                      Checksum index
   */
uint32_t create_checksum_index(const std::vector<std::string> &word_list, uint32_t unique_prefix_length)
{
	std::string trimmed_words = "";

	for(std::vector<std::string>::const_iterator it = word_list.begin(); it != word_list.end(); it++)
	{
		if(it->length() > unique_prefix_length)
		{
			trimmed_words += Language::utf8prefix(*it, unique_prefix_length);
		}
		else
		{
			trimmed_words += *it;
		}
	}
	boost::crc_32_type result;
	result.process_bytes(trimmed_words.data(), trimmed_words.length());
	return result.checksum() % seed_length;
}

/*!
   * \brief Does the checksum test on the seed passed.
   * \param seed                  Vector of seed words
   * \param unique_prefix_length  the prefix length of each word to use for checksum
   * \return                      True if the test passed false if not.
   */
bool checksum_test(std::vector<std::string> seed, uint32_t unique_prefix_length)
{
	if(seed.empty())
		return false;
	// The last word is the checksum.
	std::string last_word = seed.back();
	seed.pop_back();

	std::string checksum = seed[create_checksum_index(seed, unique_prefix_length)];

	std::string trimmed_checksum = checksum.length() > unique_prefix_length ? Language::utf8prefix(checksum, unique_prefix_length) : checksum;
	std::string trimmed_last_word = last_word.length() > unique_prefix_length ? Language::utf8prefix(last_word, unique_prefix_length) : last_word;
	return trimmed_checksum == trimmed_last_word;
}

/*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret data restored from the words.
     * \param  len             The number of bytes to expect, 0 if unknown
     * \param  duplicate       If true and len is not zero, we accept half the data, and duplicate it
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
bool words_to_bytes(std::string words, std::string &dst, size_t len, std::string &language_name)
{
	std::vector<std::string> seed;

	boost::algorithm::trim(words);
	boost::split(seed, words, boost::is_any_of(" "), boost::token_compress_on);

	if(len % 4)
		return false;

	if(seed.size() == 26) //Last word is a funky sumo attempt at a second checksum, ignore it
		seed.pop_back();

	bool has_checksum = true;
	if(len)
	{
		// error on non-compliant word list
		const size_t expected = len * 8 * 3 / 32;
		if(seed.size() != expected / 2 && seed.size() != expected && seed.size() != expected + 1)
			return false;

		// If it is seed with a checksum.
		has_checksum = seed.size() == (expected + 1);
	}

	Language::Base *language;
	if(!find_seed_language(seed, &language))
		return false;

	std::vector<uint32_t> matched_indices;
	if(!match_words(*language, seed, matched_indices))
		return false;

	language_name = language->get_language_name();
	uint32_t word_list_length = language->get_word_list().size();

	if(has_checksum)
	{
		if(!checksum_test(seed, language->get_unique_prefix_length()))
		{
			// Checksum fail
			return false;
		}
		seed.pop_back();
	}

	for(unsigned int i = 0; i < seed.size() / 3; i++)
	{
		uint32_t val;
		uint32_t w1, w2, w3;
		w1 = matched_indices[i * 3];
		w2 = matched_indices[i * 3 + 1];
		w3 = matched_indices[i * 3 + 2];

		val = w1 + word_list_length * (((word_list_length - w1) + w2) % word_list_length) +
			  word_list_length * word_list_length * (((word_list_length - w2) + w3) % word_list_length);

		if(!(val % word_list_length == w1))
			return false;

		dst.append((const char *)&val, 4); // copy 4 bytes to position
	}

	return true;
}

/*!
     * \brief Converts seed words to bytes (secret key).
     * \param  words           String containing the words separated by spaces.
     * \param  dst             To put the secret key restored from the words.
     * \param  language_name   Language of the seed as found gets written here.
     * \return                 false if not a multiple of 3 words, or if word is not in the words list
     */
bool words_to_bytes(std::string words, crypto::secret_key &dst, std::string &language_name)
{
	std::string s;
	if(!words_to_bytes(words, s, sizeof(dst), language_name))
		return false;
	if(s.size() != sizeof(dst))
		return false;
	dst = *(const crypto::secret_key *)s.data();
	return true;
}

/*!
     * \brief Converts bytes (secret key) to seed words.
     * \param  src           Secret key
     * \param  words         Space delimited concatenated words get written here.
     * \param  language_name Seed language name
     * \return               true if successful false if not. Unsuccessful if wrong key size.
     */
bool bytes_to_words(const char *src, size_t len, std::string &words, const std::string &language_name)
{
	if(len % 4 != 0 || len == 0)
		return false;

	const std::vector<const Language::Base *> &lng_insts = Electrum::get_language_list();
	const Language::Base *language = nullptr;
	for(const Language::Base *b : lng_insts)
	{
		if(b->get_language_name() == language_name || b->get_english_language_name() == language_name)
		{
			language = b;
			break;
		}
	}

	if(language == nullptr)
		return false;

	const std::vector<std::string> &word_list = language->get_word_list();
	// To store the words for random access to add the checksum word later.
	std::vector<std::string> words_store;

	uint32_t word_list_length = word_list.size();
	// 4 bytes -> 3 words.  8 digits base 16 -> 3 digits base 1626
	for(unsigned int i = 0; i < len / 4; i++, words += ' ')
	{
		uint32_t w1, w2, w3;

		uint32_t val;

		memcpy(&val, src + (i * 4), 4);

		w1 = val % word_list_length;
		w2 = ((val / word_list_length) + w1) % word_list_length;
		w3 = (((val / word_list_length) / word_list_length) + w2) % word_list_length;

		words += word_list[w1];
		words += ' ';
		words += word_list[w2];
		words += ' ';
		words += word_list[w3];

		words_store.push_back(word_list[w1]);
		words_store.push_back(word_list[w2]);
		words_store.push_back(word_list[w3]);
	}

	words.pop_back();
	words += (' ' + words_store[create_checksum_index(words_store, language->get_unique_prefix_length())]);
	return true;
}

bool bytes_to_words(const crypto::secret_key &src, std::string &words, const std::string &language_name)
{
	return bytes_to_words(src.data, sizeof(src), words, language_name);
}
}
}
