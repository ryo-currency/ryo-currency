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

#pragma once

// the loose definitions of types in this file are, well, loose.
//
// these helpers aren't here for absolute type certainty at compile-time,
// but rather to help with templated functions telling types apart.

#include <string>
#include <type_traits>

namespace sfinae
{

typedef char true_type;

struct false_type
{
	true_type a[2];
};

template <typename T>
struct is_not_container
{
  private:
	// does not have const iterator
	template <typename C>
	static false_type c_iter(typename C::const_iterator *);
	template <typename C>
	static true_type c_iter(...);

	// does not have value_type
	template <typename C>
	static false_type v_type(typename C::value_type *);
	template <typename C>
	static true_type v_type(...);

	// does not have key_type
	template <typename C>
	static false_type k_type(typename C::key_type *);
	template <typename C>
	static true_type k_type(...);

	// does not have mapped_type
	template <typename C>
	static false_type m_type(typename C::mapped_type *);
	template <typename C>
	static true_type m_type(...);

  public:
	static const bool value = ((
								   sizeof(c_iter<T>(0)) == sizeof(true_type) &&
								   sizeof(v_type<T>(0)) == sizeof(true_type) &&
								   sizeof(k_type<T>(0)) == sizeof(true_type) &&
								   sizeof(m_type<T>(0)) == sizeof(true_type)) ||
		std::is_same<T, std::string>::value);

	typedef T type;
};

template <typename T>
struct is_vector_like
{
  private:
	// has const iterator
	template <typename C>
	static true_type c_iter(typename C::const_iterator *);
	template <typename C>
	static false_type c_iter(...);

	// has value_type
	template <typename C>
	static true_type v_type(typename C::value_type *);
	template <typename C>
	static false_type v_type(...);

	// does not have key_type
	template <typename C>
	static false_type k_type(typename C::key_type *);
	template <typename C>
	static true_type k_type(...);

	// does not have mapped_type
	template <typename C>
	static false_type m_type(typename C::mapped_type *);
	template <typename C>
	static true_type m_type(...);

  public:
	static const bool value = (sizeof(c_iter<T>(0)) == sizeof(true_type) &&
		sizeof(v_type<T>(0)) == sizeof(true_type) &&
		sizeof(k_type<T>(0)) == sizeof(true_type) &&
		sizeof(m_type<T>(0)) == sizeof(true_type) &&
		!std::is_same<T, std::string>::value);

	typedef T type;
};

template <typename T>
struct is_map_like
{
  private:
	// has const iterator
	template <typename C>
	static true_type c_iter(typename C::const_iterator *);
	template <typename C>
	static false_type c_iter(...);

	// has value_type
	template <typename C>
	static true_type v_type(typename C::value_type *);
	template <typename C>
	static false_type v_type(...);

	// has key_type
	template <typename C>
	static true_type k_type(typename C::key_type *);
	template <typename C>
	static false_type k_type(...);

	// has mapped_type
	template <typename C>
	static true_type m_type(typename C::mapped_type *);
	template <typename C>
	static false_type m_type(...);

  public:
	static const bool value = (sizeof(c_iter<T>(0)) == sizeof(true_type) &&
		sizeof(v_type<T>(0)) == sizeof(true_type) &&
		sizeof(k_type<T>(0)) == sizeof(true_type) &&
		sizeof(m_type<T>(0)) == sizeof(true_type) &&
		!std::is_same<T, std::string>::value);

	typedef T type;
};

} // namespace sfinae
