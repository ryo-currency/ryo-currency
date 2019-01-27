// Copyright (c) 2018, Ryo Currency Project
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

// Parts of this file are originally copyright (c) 2014-2018, The Monero Project
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include <cstddef>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

#include "string_tools.h"
#include "../io.h"
#include "crypto-tests.h"
#include "crypto/crypto.h"
#include "crypto/hash.h"
#include "warnings.h"

using namespace std;
using namespace crypto;
typedef crypto::hash chash;

bool operator!=(const ec_scalar &a, const ec_scalar &b)
{
	return 0 != memcmp(&a, &b, sizeof(ec_scalar));
}

bool operator!=(const ec_point &a, const ec_point &b)
{
	return 0 != memcmp(&a, &b, sizeof(ec_point));
}

bool operator!=(const key_derivation &a, const key_derivation &b)
{
	return 0 != memcmp(&a, &b, sizeof(key_derivation));
}

template<typename T>
inline std::string get_strval(const T& val)
{
	return epee::string_tools::pod_to_hex(val);
}

template<>
inline std::string get_strval<long unsigned int>(const long unsigned int& val)
{
	return std::to_string(val);
}

template<>
inline std::string get_strval<unsigned int>(const unsigned int& val)
{
	return std::to_string(val);
}

template<>
inline std::string get_strval<bool>(const bool& val)
{
	return val ? "true" : "false";
}

template<>
inline std::string get_strval<string>(const string& val)
{
	return val;
}

template<>
inline std::string get_strval<secret_key>(const secret_key& val)
{
	return epee::string_tools::pod_to_hex(val.data);
}

template<>
inline std::string get_strval<public_key>(const public_key& val)
{
	return epee::string_tools::pod_to_hex(val.data);
}

template<>
inline std::string get_strval<vector<char>>(const vector<char>& val)
{
	return epee::to_hex::string(epee::to_byte_span(epee::to_span(val)));
}

template<>
inline std::string get_strval<vector<public_key>>(const vector<public_key>& val)
{
	std::string ret;
	ret.reserve(val.size()*33);

	for(const public_key& s : val)
	{
		ret += epee::string_tools::pod_to_hex(s.data);
		ret += " ";
	}
	ret.pop_back();

	return ret;
}

template<>
inline std::string get_strval<vector<signature>>(const vector<signature>& val)
{
	std::string ret;
	ret.reserve(val.size()*64);

	for(const signature& s : val)
		ret += epee::string_tools::pod_to_hex(s);

	return ret;
}

bool error = false;
template<typename T>
inline bool report_result(const std::string& cmd, size_t test_id, const T& actual, const T& expected, size_t sub_test = 0)
{
	if(actual != expected)
	{
		if(sub_test == 0)
			cerr << "Wrong result for test " << cmd << " : " << test_id << endl;
		else
			cerr << "Wrong result for test " << cmd << " : " << test_id << " (" << sub_test << ")" << endl;

		cerr << "Expected " << get_strval(expected) << " got " << get_strval(actual);
		error = true;
		return false;
	}
	return true;
}

template <typename T>
struct has_iterator
{
    template <typename U>
    static char test(typename U::iterator* x);
 
    template <typename U>
    static long test(U* x);
 
    static const bool value = sizeof(test<T>(0)) == 1;
};

inline void print_result() {}

template <typename T, typename... TT> 
inline void print_result(T &res, TT &... resres)
{
	cout << get_strval(res);
	if(sizeof...(TT) > 0)
		cout << " ";
	else
		cout << endl;
	print_result(resres...);
}

DISABLE_GCC_WARNING(maybe-uninitialized)

int main(int argc, char *argv[])
{
	fstream input;
	string cmd;
	size_t test = 0;
	if(argc < 2 || argc > 3)
	{
		cerr << "invalid arguments" << endl;
		return 1;
	}

	bool regerate = false;
	if(argc == 3 && strcmp(argv[2], "regen") == 0)
		regerate = true;

	input.open(argv[1], ios_base::in);
	for(;;)
	{
		++test;
		input.exceptions(ios_base::badbit);
		if(!(input >> cmd))
		{
			break;
		}
		input.exceptions(ios_base::badbit | ios_base::failbit | ios_base::eofbit);
		if(cmd == "check_scalar")
		{
			ec_scalar scalar;
			bool expected = false, actual;
			get(input, scalar, expected);
			actual = check_scalar(scalar);
			if(regerate)
				print_result(cmd, scalar, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "hash_to_scalar")
		{
			vector<char> data;
			ec_scalar expected, actual;
			get(input, data, expected);
			crypto::hash_to_scalar(data.data(), data.size(), actual);
			if(regerate)
				print_result(cmd, data, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "generate_keys")
		{
			// this test uses a deterministic random number generator in the backend
			public_key expected1, actual1;
			secret_key expected2, actual2;
			get(input, expected1, expected2);
			generate_legacy_keys(actual1, actual2);
			if(regerate)
			{
				print_result(cmd, actual1, actual2);
			}
			else
			{
				report_result(cmd, test, actual1, expected1, 1);
				report_result(cmd, test, actual2, expected2, 2);
			}
		}
		else if(cmd == "check_key")
		{
			public_key key;
			bool expected, actual;
			get(input, key, expected);
			actual = check_key(key);
			if(regerate)
				print_result(cmd, key, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "secret_key_to_public_key")
		{
			secret_key sec;
			bool expected1, actual1;
			public_key expected2, actual2;
			get(input, sec, expected1);
			if(expected1)
			{
				get(input, expected2);
			}
			actual1 = secret_key_to_public_key(sec, actual2);
			if(regerate)
			{
				if(actual1)
					print_result(cmd, sec, actual1, actual2);
				else
					print_result(cmd, sec, actual1);
			}
			else
			{
				report_result(cmd, test, actual1, expected1, 1);
				if(expected1)
					report_result(cmd, test, actual2, expected2, 2);
			}
		}
		else if(cmd == "generate_key_derivation")
		{
			public_key key1;
			secret_key key2;
			bool expected1, actual1;
			key_derivation expected2, actual2;
			get(input, key1, key2, expected1);
			if(expected1)
			{
				get(input, expected2);
			}
			actual1 = generate_key_derivation(key1, key2, actual2);
			if(regerate)
			{
				if(actual1)
					print_result(cmd, key1, key2, actual1, actual2);
				else
					print_result(cmd, key1, key2, actual1);
			}
			else
			{
				report_result(cmd, test, actual1, expected1, 1);
				if(expected1)
					report_result(cmd, test, actual2, expected2, 2);
			}
		}
		else if(cmd == "derive_public_key")
		{
			key_derivation derivation;
			size_t output_index;
			public_key base;
			bool expected1, actual1;
			public_key expected2, actual2;
			get(input, derivation, output_index, base, expected1);
			if(expected1)
			{
				get(input, expected2);
			}
			actual1 = derive_public_key(derivation, output_index, base, actual2);
			if(regerate)
			{
				if(actual1)
					print_result(cmd, derivation, output_index, base, actual1, actual2);
				else
					print_result(cmd, derivation, output_index, base, actual1);
			}
			else
			{
				report_result(cmd, test, actual1, expected1, 1);
				if(expected1)
					report_result(cmd, test, actual2, expected2, 2);
			}
		}
		else if(cmd == "derive_secret_key")
		{
			key_derivation derivation;
			size_t output_index;
			secret_key base;
			secret_key expected, actual;
			get(input, derivation, output_index, base, expected);
			derive_secret_key(derivation, output_index, base, actual);
			if(regerate)
				print_result(cmd, derivation, output_index, base, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "generate_signature")
		{
			// this test uses a deterministic random number generator in the backend
			chash prefix_hash;
			public_key pub;
			secret_key sec;
			signature expected, actual;
			get(input, prefix_hash, pub, sec, expected);
			generate_signature(prefix_hash, pub, sec, actual);
			if(regerate)
				print_result(cmd, prefix_hash, pub, sec, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "check_signature")
		{
			chash prefix_hash;
			public_key pub;
			signature sig;
			bool expected, actual;
			get(input, prefix_hash, pub, sig, expected);
			actual = check_signature(prefix_hash, pub, sig);
			if(regerate)
				print_result(cmd, prefix_hash, pub, sig, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "hash_to_point")
		{
			chash h;
			ec_point expected, actual;
			get(input, h, expected);
			hash_to_point(h, actual);
			if(regerate)
				print_result(cmd, h, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "hash_to_ec")
		{
			public_key key;
			ec_point expected, actual;
			get(input, key, expected);
			hash_to_ec(key, actual);
			if(regerate)
				print_result(cmd, key, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "generate_key_image")
		{
			public_key pub;
			secret_key sec;
			key_image expected, actual;
			get(input, pub, sec, expected);
			generate_key_image(pub, sec, actual);
			if(regerate)
				print_result(cmd, pub, sec, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "generate_ring_signature")
		{
			// this test uses a deterministic random number generator in the backend  
			chash prefix_hash;
			key_image image;
			vector<public_key> vpubs;
			vector<const public_key *> pubs;
			size_t pubs_count;
			secret_key sec;
			size_t sec_index;
			vector<signature> expected, actual;
			size_t i;
			get(input, prefix_hash, image, pubs_count);
			vpubs.resize(pubs_count);
			pubs.resize(pubs_count);
			for(i = 0; i < pubs_count; i++)
			{
				get(input, vpubs[i]);
				pubs[i] = &vpubs[i];
			}
			get(input, sec, sec_index);
			expected.resize(pubs_count);
			getvar(input, pubs_count * sizeof(signature), expected.data());
			actual.resize(pubs_count);
			generate_ring_signature(prefix_hash, image, pubs.data(), pubs_count, sec, sec_index, actual.data());
			if(regerate)
				print_result(cmd, prefix_hash, image, pubs_count, vpubs, sec, sec_index, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else if(cmd == "check_ring_signature")
		{
			chash prefix_hash;
			key_image image;
			vector<public_key> vpubs;
			vector<const public_key *> pubs;
			size_t pubs_count;
			vector<signature> sigs;
			bool expected, actual;
			size_t i;
			get(input, prefix_hash, image, pubs_count);
			vpubs.resize(pubs_count);
			pubs.resize(pubs_count);
			for(i = 0; i < pubs_count; i++)
			{
				get(input, vpubs[i]);
				pubs[i] = &vpubs[i];
			}
			sigs.resize(pubs_count);
			getvar(input, pubs_count * sizeof(signature), sigs.data());
			get(input, expected);
			actual = check_ring_signature(prefix_hash, image, pubs.data(), pubs_count, sigs.data());
			if(regerate)
				print_result(cmd, prefix_hash, image, pubs_count, vpubs, sigs, actual);
			else
				report_result(cmd, test, actual, expected);
		}
		else
		{
			throw ios_base::failure("Unknown function: " + cmd);
		}
		continue;
	}
	return error ? 1 : 0;
}
