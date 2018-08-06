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

#pragma once

#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include <list>
#include <vector>
#include "thdq.hpp"
#include <fmt/format.h>
#include <fmt/time.h>

class gulps
{
public:
	enum level
	{
		LEVEL_OUTPUT_0,
		LEVEL_OUTPUT_1,
		LEVEL_OUTPUT_2,
		LEVEL_ERROR,
		LEVEL_WARN,
		LEVEL_INFO,
		LEVEL_DEBUG_0,
		LEVEL_DEBUG_1,
		LEVEL_DEBUG_2
	};
	
	static inline const char* level_to_str(level lvl)
	{
		switch(lvl)
		{
		case LEVEL_OUTPUT_0: return "OUTPUT0";
		case LEVEL_OUTPUT_1: return "OUTPUT1";
		case LEVEL_OUTPUT_2: return "OUTPUT2";
		case LEVEL_ERROR: return "ERROR";
		case LEVEL_WARN: return "WARN";
		case LEVEL_INFO: return "INFO";
		case LEVEL_DEBUG_0: return "DEBUG0";
		case LEVEL_DEBUG_1: return "DEBUG1";
		case LEVEL_DEBUG_2: return "DEBUG2";
		default: return "UNKNOWN";
		}
	}
	
	//Object handling a single long message
	class message
	{
	public:
		std::time_t time;
		level lvl;
		std::string cat_major;
		std::string cat_minor;
		std::string src_path;
		int64_t src_line;
		std::string thread_id;
		std::string text;
		fmt::color clr;

		message(level lvl, const char* major, const char* minor, const char* path, int64_t line, std::string&& text, fmt::color clr = fmt::color::white) : 
			time(std::time(nullptr)), lvl(lvl), cat_major(major), cat_minor(minor), src_path(path), src_line(line),
			thread_id(gulps::inst().get_thread_tag()), text(std::move(text)), clr(clr)
		{
		}

		std::string print_message(bool inc_text = true) const
		{
			std::string out = fmt::format("{:%Y-%m-%d %H:%M:%S} {} [{}] {}.{} {}:{} ", fmt::localtime(time), 
					level_to_str(lvl), thread_id, cat_major, cat_minor, src_path, src_line);

			if(inc_text)
				out += print_text();

			return out;
		}

		std::string print_text() const
		{
			std::string out = text;
			if(text.back() != '\n')
				out += '\n';
			return out;
		}

		message(message&& ) = default;
		message& operator=(message&& ) = default;
		message(const message& ) = default;
		message& operator=(const message& ) = default;
	};

	// Function pointers are mariginally more efficient than functors here
	// NB Lambdas must not capture to be convertible to a function ptr
	typedef bool (*filter_fun)(const message&);

	class gulps_output
	{
	public:
		gulps_output() = default;
		virtual ~gulps_output() = default;
		virtual void log_message(const message& msg) = 0;

		// This is filtered call
		bool log(const message& msg)
		{
			bool result = false;
			for(filter_fun filter : filters)
			{
				if(filter(msg))
				{
					result = true;
					break;
				}
			}

			if(result)
				log_message(msg);

			return result;
		}

		void add_filter(filter_fun filter) { filters.push_back(filter); };

	private:
		gulps_output(gulps_output&& ) = delete;
		gulps_output& operator=(gulps_output&& ) = delete;
		gulps_output(gulps_output& ) = delete;
		gulps_output& operator=(gulps_output& ) = delete;
		
		std::vector<filter_fun> filters;
	};

	class gulps_print_output : public gulps_output
	{
	public:
		gulps_print_output(bool text_only, fmt::color hdr_color) : text_only(text_only), hdr_color(hdr_color) {}

		void log_message(const message& msg) override
		{
			if(!text_only)
				fmt::print(hdr_color, msg.print_message(false));
			fmt::print(msg.clr, msg.print_text());
		}

	protected:
		bool text_only;
		fmt::color hdr_color;
	};

	//Class handling synchronous file logging.
	class gulps_file_output : public gulps_output
	{
	public:
		gulps_file_output(const std::string& name) : fname(name)
		{
			output_file.open(fname, std::ios::out | std::ios::app | std::ios::binary); // may throw
		}

		void log_message(const message& msg) override
		{
			write_output(msg);
		}
	
		~gulps_file_output()
		{
			log_message(message(LEVEL_DEBUG_2, "log_shutdown", fname.c_str(), __FILE__, __LINE__, "Log file shutting down."));
			output_file.close();
		}

	protected:
		inline void write_output(const message& msg)
		{
			try
			{
				output_file << msg.print_message(true);
				output_file.flush();
			}
			catch(const std::exception& ex)
			{
				std::cout << "!!! PANIC, error writing to a log file! Your disk is probably full! " << ex.what() << std::endl;
			}
		}

		std::string fname;
		std::ofstream output_file;
	};

	//Class handling async logging. Async logging is thread-safe and preserves temporal order
	class gulps_async_file_output : public gulps_file_output
	{
	public:
		gulps_async_file_output(const std::string& name) : 
			gulps_file_output(name), thd(&gulps_async_file_output::output_main, this)
		{
		}

		void log_message(const message& msg) override
		{
			msg_q.push(msg);
		}

		void output_main()
		{
			while(msg_q.wait_for_pop())
			{
				message msg = msg_q.pop();
				write_output(msg);
			}
		}

		~gulps_async_file_output()
		{
			log_message(message(LEVEL_DEBUG_2, "log_shutdown", fname.c_str(), __FILE__, __LINE__, "Log thread shutting down."));
			if(thd.joinable())
			{
				msg_q.set_finish_flag();
				thd.join();
			}
		}

	private:
		inline void write_output(const message& msg)
		{
			try
			{
				output_file << msg.print_message(true);
				output_file.flush();
			}
			catch(const std::exception& ex)
			{
				std::cerr << "!!! PANIC, error writing to a log file! Your disk is probably full! " << ex.what() << std::endl;
			}
		}

		thdq<message> msg_q;
		std::thread thd;
	};

	gulps() {}

	inline const std::string& get_thread_tag() { return thread_tag(); }
	inline void set_thread_tag(const std::string& tag) { thread_tag() = tag; }

	inline static gulps& inst()
	{
		static gulps inst;
		return inst;
	}

	inline static const gulps& cinst()
	{
		static const gulps& inst = gulps::inst();
		return inst;
	}

	void add_output(std::unique_ptr<gulps_output> output) { outputs.push_back(std::move(output)); };

	void log(message&& in_msg)
	{
		message msg = std::move(in_msg);
		
		for(std::unique_ptr<gulps_output>& out : outputs)
			out->log(msg);
	}

private:
	inline std::string& thread_tag()
	{
		static thread_local std::string thread_tag;
		return thread_tag;
	}

	std::list<std::unique_ptr<gulps_output>> outputs;
};

#define GULPS_OUTPUT(lvl, maj, min, clr, ...) gulps::inst().log(gulps::message(lvl, maj, min, __FILE__, __LINE__, fmt::format(__VA_ARGS__), clr))
#define GULPS_CAT_ERROR(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_ERROR, maj, min, fmt::color::red, fstr, __VA_ARGS__)
#define GULPS_CAT_WARN(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_WARN, maj, min, fmt::color::orange, fstr, __VA_ARGS__)
#define GULPS_CAT_INFO(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_INFO, maj, min, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_CAT_DEBUG0(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_0, maj, min, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_CAT_DEBUG1(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_1, maj, min, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_CAT_DEBUG2(maj, min, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_2, maj, min, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_CAT_PRINT(maj, min, clr, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_OUTPUT_0, maj, min, clr, fstr, __VA_ARGS__)

#ifndef GULPS_CAT_MAJOR
#define GULPS_CAT_MAJOR "default"
#endif

#ifndef GULPS_CAT_MINOR
#define GULPS_CAT_MINOR ""
#endif

#define GULPS_ERROR(...) GULPS_OUTPUT(gulps::LEVEL_ERROR, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::red, __VA_ARGS__)
#define GULPS_WARN(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_WARN, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::orange, fstr, __VA_ARGS__)
#define GULPS_INFO(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_INFO, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG0(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_0, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG1(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_1, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG2(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_2, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_PRINT(clr, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_OUTPUT_0, GULPS_CAT_MAJOR, GULPS_CAT_MINOR, clr, fstr, __VA_ARGS__)
