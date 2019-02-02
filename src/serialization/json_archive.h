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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

/*! \file json_archive.h
 *
 * \brief JSON archive
 */

#pragma once

#include "serialization.h"
#include <cassert>
#include <iomanip>
#include <iostream>

/*! \struct json_archive_base
 *
 * \brief the base class of json archive type
 *
 * \detailed contains the basic logic for serializing a json archive
 */
template <class Stream, bool IsSaving>
struct json_archive_base
{
	typedef Stream stream_type;
	typedef json_archive_base<Stream, IsSaving> base_type;
	typedef boost::mpl::bool_<IsSaving> is_saving;

	typedef const char *variant_tag_type;

	json_archive_base(stream_type &s, bool indent = false)
		: stream_(s), indent_(indent), object_begin(false), depth_(0) {}

	void tag(const char *tag)
	{
		if(!object_begin)
			stream_ << ", ";
		make_indent();
		stream_ << '"' << tag << "\": ";
		object_begin = false;
	}

	void begin_object()
	{
		stream_ << "{";
		++depth_;
		object_begin = true;
	}

	void end_object()
	{
		--depth_;
		make_indent();
		stream_ << "}";
	}

	void begin_variant() { begin_object(); }
	void end_variant() { end_object(); }
	Stream &stream() { return stream_; }

  protected:
	void make_indent()
	{
		if(indent_)
		{
			stream_ << '\n'
					<< std::string(2 * depth_, ' ');
		}
	}

  protected:
	stream_type &stream_;
	bool indent_;
	bool object_begin;
	size_t depth_;
};

/*! \struct json_archive
 * 
 * \brief a archive using the JSON standard
 *
 * \detailed only supports being written to
 */
template <bool W>
struct json_archive;

template <>
struct json_archive<true> : public json_archive_base<std::ostream, true>
{
	json_archive(stream_type &s, bool indent = false) : base_type(s, indent) {}

	template <typename T>
	static auto promote_to_printable_integer_type(T v) -> decltype(+v)
	{
		// Unary operator '+' performs integral promotion on type T [expr.unary.op].
		// If T is signed or unsigned char, it's promoted to int and printed as number.
		return +v;
	}

	template <class T>
	void serialize_int(T v)
	{
		stream_ << std::dec << promote_to_printable_integer_type(v);
	}

	void serialize_blob(void *buf, size_t len, const char *delimiter = "\"")
	{
		begin_string(delimiter);
		for(size_t i = 0; i < len; i++)
		{
			unsigned char c = ((unsigned char *)buf)[i];
			stream_ << std::hex << std::setw(2) << std::setfill('0') << (int)c;
		}
		end_string(delimiter);
	}

	template <class T>
	void serialize_varint(T &v)
	{
		stream_ << std::dec << promote_to_printable_integer_type(v);
	}

	void begin_string(const char *delimiter = "\"")
	{
		stream_ << delimiter;
	}

	void end_string(const char *delimiter = "\"")
	{
		stream_ << delimiter;
	}

	void begin_array(size_t s = 0)
	{
		inner_array_size_ = s;
		++depth_;
		stream_ << "[ ";
	}

	void delimit_array()
	{
		stream_ << ", ";
	}

	void end_array()
	{
		--depth_;
		if(0 < inner_array_size_)
		{
			make_indent();
		}
		stream_ << "]";
	}

	void write_variant_tag(const char *t)
	{
		tag(t);
	}

  private:
	size_t inner_array_size_;
};
