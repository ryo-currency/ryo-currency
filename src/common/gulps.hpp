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

#pragma once

#include "inttypes.h"
#include <iostream>
#include <fstream>
#include <ctime>
#include <thread>
#include <unordered_map>
#include <map>
#include <sstream>
#include <vector>
#include <atomic>
#include "thdq.hpp"
#include "string.hpp"
#include <fmt/format.h>
#include <fmt/time.h>
#include <boost/mpl/contains.hpp>
#include <boost/algorithm/string.hpp>
#include "../cryptonote_config.h"

#if defined(WIN32)
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

// Some syntatic sugar to ease macro work
// stream_writer::write(a,b,c) is equivarent to
// strigstream ss; ss << a << b << c; return ss.str();
class stream_writer
{
public:
	template<typename T, typename... Args>
	static inline std::string write(const T& v, Args... args)
	{
		std::stringstream ss;
		ss << v;
		do_op(ss, args...);
		return ss.str();
	}

	//Optimise trivial strings
	static inline std::string write(const char* arg)
	{
		return arg;
	}

private:
	static inline void do_op(std::stringstream& ss) { }

	template<typename T, typename... Args>
	static inline void do_op(std::stringstream& ss, const T& v, Args... args)
	{
		ss << v;
		do_op(ss, args...);
	}
};

class gulps
{
public:
	enum output_mode : uint32_t
	{
		TEXT_ONLY,
		TIMESTAMP_ONLY,
		TIMESTAMP_LOG
	};

	enum level : uint32_t
	{
		LEVEL_PRINT = 0,
		LEVEL_ERROR,
		LEVEL_WARN,
		LEVEL_INFO,
		LEVEL_DEBUG,
		LEVEL_TRACE,
		LEVEL_TRACE2
	};

	static inline const char* level_to_str(level lvl)
	{
		switch(lvl)
		{
		case LEVEL_PRINT: return "PRINT";
		case LEVEL_ERROR: return "ERROR";
		case LEVEL_WARN: return "WARN";
		case LEVEL_INFO: return "INFO";
		case LEVEL_DEBUG: return "DEBUG";
		case LEVEL_TRACE: return "TRACE";
		case LEVEL_TRACE2: return "TRACE2";
		default: return "UNKNOWN";
		}
	}

	// Those are output channels. First question is are we directing the message at
	// the screen (USER) or disk (LOG). However the lines can get blurry - in an RPC command
	// do we direct the output at the RPC app screen, or the user on the other side? Hence 3
	// separate the the end user application can direct separately
	enum output : uint32_t
	{
		OUT_USER_0,
		OUT_USER_1,
		OUT_USER_2,
		OUT_LOG_0,
		OUT_LOG_1,
		OUT_LOG_2
	};

	static inline const char* out_to_str(output out)
	{
		switch(out)
		{
		case OUT_USER_0: return "OUTPUT0";
		case OUT_USER_1: return "OUTPUT1";
		case OUT_USER_2: return "OUTPUT2";
		case OUT_LOG_0: return "LOG0";
		case OUT_LOG_1: return "LOG1";
		case OUT_LOG_2: return "LOG2";
		default: return "UNKNOWN";
		}
	}

	enum color : uint32_t
	{
		COLOR_WHITE = 1,
		COLOR_RED   = 2,
		COLOR_GREEN = 3,
		COLOR_BLUE  = 4,
		COLOR_CYAN  = 5,
		COLOR_MAGENTA = 6,
		COLOR_YELLOW = 7,
		COLOR_BOLD_WHITE = 9,
		COLOR_BOLD_RED   = 10,
		COLOR_BOLD_GREEN = 11,
		COLOR_BOLD_BLUE  = 12,
		COLOR_BOLD_CYAN  = 13,
		COLOR_BOLD_MAGENTA = 14,
		COLOR_BOLD_YELLOW = 15,
		COLOR_MASK = 7, // Masks color from brightness
		COLOR_BOLD = 8
	};

	//Object handling a single long message
	class message
	{
	public:
		std::time_t time;
		level lvl;
		output out;
		std::string cat_major;
		std::string cat_minor;
		std::string src_path;
		int64_t src_line;
		std::string thread_id;
		std::string text;
		color clr;
		bool printed = false;
		bool logged = false;

		message(output out, level lvl, const char* major, const char* minor, const char* path, int64_t line, std::string&& txt, color clr = COLOR_WHITE, bool add_newline = true) :
			time(std::time(nullptr)), lvl(lvl), out(out), cat_major(major), cat_minor(minor), src_path(path), src_line(line),
			thread_id(gulps::inst().get_thread_tag()), text(std::move(txt)), clr(clr)
		{
			const std::string& pre = gulps::inst().get_path_prefix();

			if(src_path.find(pre) == 0)
				src_path.erase(0, pre.size());

			if(add_newline && text.back() != '\n')
				text += '\n';
		}

		const std::string& print_message(std::string& header, output_mode mode = TIMESTAMP_ONLY) const
		{
			switch(mode)
			{
			case TIMESTAMP_LOG:
				header = fmt::format("{:%Y-%m-%d %H:%M:%S} [{}/{}] {} {}.{} {}:{} ",
					fmt::localtime(time), level_to_str(lvl), out_to_str(out), thread_id, cat_major, cat_minor, src_path, src_line);
				break;

			case TIMESTAMP_ONLY:
				header = fmt::format("{:%Y-%m-%d %H:%M:%S} ", fmt::localtime(time));
				break;

			case TEXT_ONLY:
				header = "";
				break;
			}

			return text;
		}

		message(message&& ) = default;
		message& operator=(message&& ) = default;
		message(const message& ) = default;
		message& operator=(const message& ) = default;
	};

	// Function pointers are mariginally more efficient than functors here
	// NB Lambdas must not capture to be convertible to a function ptr
	typedef bool (*filter_fun)(const message&, bool printed, bool logged);

	class gulps_output
	{
	protected:
		enum output_stream
		{
			STREAM_PRINT,
			STREAM_FILE,
			STREAM_NONE
		};

	public:
		gulps_output() = default;
		virtual ~gulps_output() = default;
		virtual void log_message(const message& msg) = 0;
		virtual output_stream get_stream() = 0;

		// This is filtered call
		bool log(const message& msg, bool& printed, bool& logged)
		{
			bool result = false;
			for(filter_fun filter : filters)
			{
				if(filter(msg, printed, logged))
				{
					result = true;
					break;
				}
			}

			if(result)
			{
				log_message(msg);

				switch(get_stream())
				{
				case STREAM_PRINT:
					printed = true;
					break;
				case STREAM_FILE:
					logged = true;
					break;
				default:
					break;
				}
			}

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
		gulps_print_output(color hdr_color, output_mode mode = TIMESTAMP_ONLY) : hdr_color(hdr_color), mode(mode) {}

		output_stream get_stream() override { return STREAM_PRINT; }

		void log_message(const message& msg) override
		{
			std::string header;
			const std::string& text = msg.print_message(header, mode);
			if(mode != TEXT_ONLY)
				print(header, hdr_color);
			print(text, msg.clr);
		}

	private:
		output_mode mode;

		static void print(const std::string& txt, color clr)
		{
			set_console_color(clr);
			std::fputs(txt.c_str(), stdout);
			reset_console_color();

#if defined(WIN32)
			std::cout.flush();
#endif
		}

		static bool is_stdout_a_tty()
		{
#if defined(WIN32)
			static std::atomic<bool> is_a_tty(_isatty(_fileno(stdout)) != 0);
#else
			static std::atomic<bool> is_a_tty(isatty(fileno(stdout)) != 0);
#endif
			return is_a_tty;
		}

		static void set_console_color(color clr)
		{
			if(!is_stdout_a_tty())
				return;

#ifdef WIN32
			WORD bright = (clr & COLOR_BOLD) ? FOREGROUND_INTENSITY : 0;
#else
			bool bright = (clr & COLOR_BOLD) != 0;
#endif
			switch(clr & COLOR_MASK)
			{
			default:
			case COLOR_WHITE:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | bright);
#else
				std::cout << (bright ? "\033[1;97m" : "\033[0;97m");
				std::cout.flush();
#endif
				break;
			case COLOR_RED:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | bright);
#else
				std::cout << (bright ? "\033[1;31m" : "\033[0;31m");
				std::cout.flush();
#endif
				break;
			case COLOR_GREEN:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | bright);
#else
				std::cout << (bright ? "\033[1;32m" : "\033[0;32m");
				std::cout.flush();
#endif
				break;
			case COLOR_BLUE:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | bright);
#else
				std::cout << (bright ? "\033[1;34m" : "\033[0;34m");
				std::cout.flush();
#endif
				break;
			case COLOR_CYAN:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | bright);
#else
				std::cout << (bright ? "\033[1;36m" : "\033[0;36m");
				std::cout.flush();
#endif
				break;
			case COLOR_MAGENTA:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_BLUE | FOREGROUND_RED | bright);
#else
				std::cout << (bright ? "\033[1;35m" : "\033[0;35m");
				std::cout.flush();
#endif
				break;
			case COLOR_YELLOW:
#ifdef WIN32
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_RED | bright);
#else
				std::cout << (bright ? "\033[1;33m" : "\033[0;33m");
				std::cout.flush();
#endif
				break;
			}
		}

		static void reset_console_color()
		{
			if(!is_stdout_a_tty())
				return;

#ifdef WIN32
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
			std::cout << "\033[0m";
			std::cout.flush();
#endif
		}

	protected:
		color hdr_color;
	};

	//Store messages to, for example, send them over the network
	class gulps_mem_output : public gulps_output
	{
	public:
		gulps_mem_output() {}

		output_stream get_stream() override { return STREAM_NONE; }

		void log_message(const message& msg) override
		{
			log.push_back(msg);
		}

		const std::vector<message>& get_log()
		{
			return log;
		}

	protected:
		std::vector<message> log;
	};

	//Class handling synchronous file logging.
	class gulps_file_output : public gulps_output
	{
	public:
		gulps_file_output(const std::string& name) : fname(name)
		{
			output_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
			output_file.open(fname, std::ios::out | std::ios::app | std::ios::binary); // may throw
		}

		output_stream get_stream() override { return STREAM_FILE; }

		void log_message(const message& msg) override
		{
			write_output(msg);
		}

		~gulps_file_output()
		{
			log_message(message(OUT_LOG_2, LEVEL_TRACE, "log_shutdown", fname.c_str(), __FILE__, __LINE__, "Log file shutting down."));
			output_file.close();
		}

	protected:
		inline void write_output(const message& msg)
		{
			try
			{
				std::string header;
				const std::string text = msg.print_message(header, TIMESTAMP_LOG);
				output_file << header << text;
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
			std::unique_lock<std::mutex> lck = msg_q.get_lock();
			while(msg_q.wait_for_pop(lck))
			{
				message msg = msg_q.pop(lck);
				write_output(msg);
			}
		}

		~gulps_async_file_output()
		{
			log_message(message(OUT_LOG_2, LEVEL_TRACE, "log_shutdown", fname.c_str(), __FILE__, __LINE__, "Log thread shutting down."));
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
				std::string header;
				const std::string text = msg.print_message(header, TIMESTAMP_LOG);
				output_file << header << text;
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

	//Returned value is a handle unique to that output. It can be used to remove it.
	uint64_t add_output(std::unique_ptr<gulps_output> output)
	{
		std::unique_lock<std::mutex> lck(gulps_global);
		outputs.insert(std::make_pair(next_handle, std::move(output)));
		return next_handle++;
	}

	void remove_output(uint64_t handle)
	{
		std::unique_lock<std::mutex> lck(gulps_global);
		outputs.erase(handle);
	}

	void log(message&& in_msg)
	{
		std::unique_lock<std::mutex> lck(gulps_global);
		message msg = std::move(in_msg);
		bool printed = false, logged = false;

		for(const auto& it : outputs)
			it.second->log(msg, printed, logged);
	}

	inline const std::string& get_path_prefix()
	{
		return path_prefix;
	}

private:
	gulps()
	{
		path_prefix = __FILE__;
		size_t pos;
		if((pos = path_prefix.find("src/common/gulps.hpp")) != std::string::npos)
			path_prefix.erase(pos);
		else if((pos = path_prefix.find("src\\common\\gulps.hpp")) != std::string::npos)
			path_prefix.erase(pos);
		else
			assert(false);
	}

	inline std::string& thread_tag()
	{
		static thread_local std::string thread_tag;
		return thread_tag;
	}

	std::string path_prefix;
	uint64_t next_handle = 0;
	std::mutex gulps_global;
	std::map<uint64_t, std::unique_ptr<gulps_output>> outputs;
};

class gulps_log_level
{
private:
	struct cat_pair
	{
		cat_pair(std::string&& cat, gulps::level lvl) : cat(std::move(cat)), level(lvl) {}

		cat_pair(cat_pair&& ) = default;
		cat_pair& operator=(cat_pair&& ) = default;
		cat_pair(const cat_pair& ) = default;
		cat_pair& operator=(const cat_pair& ) = default;

		std::string cat;
		gulps::level level;
	};

	std::mutex cat_mutex;
	std::vector<cat_pair> log_cats;
	gulps::level wildcard_level;
	std::string current_cat_str;
	bool active = false;

public:
	gulps_log_level() {}

	static const char* get_default_log_level()
	{
		return "*:WARN";
	}

	const std::string& get_current_cat_str() const
	{
		return current_cat_str;
	}

	bool parse_cat_string(const char* str)
	{
		std::unique_lock<std::mutex> lck(cat_mutex);

		if(strlen(str) == 0)
			str = get_default_log_level();
		current_cat_str = str;

		static const std::unordered_map<std::string, gulps::level> str_to_level = {
			{"PRINT", gulps::LEVEL_PRINT},
			{"ERROR", gulps::LEVEL_ERROR},
			{"WARN", gulps::LEVEL_WARN},
			{"INFO", gulps::LEVEL_INFO},
			{"DEBUG", gulps::LEVEL_DEBUG},
			{"TRACE", gulps::LEVEL_TRACE},
			{"TRACE2", gulps::LEVEL_TRACE2}
		};

		static const std::unordered_map<std::string, std::string> aliased_levels = {
			{"0", "*:PRINT"},
			{"1", "*:WARN"},
			{"2", "*:INFO"},
			{"3", "*:DEBUG"},
			{"4", "*:TRACE2"}
		};

		std::vector<std::string> vcats, vcat;
		boost::split(vcats, str, boost::is_any_of(","), boost::token_compress_on);

		std::vector<cat_pair> log_cats_tmp;

		for(const std::string& scat : vcats)
		{
			vcat.clear();

			auto it1 = aliased_levels.find(scat);
			if(it1 != aliased_levels.end())
				boost::split(vcat, it1->second, boost::is_any_of(":"), boost::token_compress_on);
			else
				boost::split(vcat, scat, boost::is_any_of(":"), boost::token_compress_on);

			if(vcat.size() != 2)
				return false;

			auto it = str_to_level.find(vcat[1]);
			if(it == str_to_level.end())
				return false;

			gulps::level level = it->second;
			if(vcat[0] != "*")
				log_cats_tmp.emplace_back(std::move(vcat[0]), level);
			else
				wildcard_level = level;
		}

		// replace old log lvl with new levels
		log_cats.swap(log_cats_tmp);
		active = true;
		return true;
	}

	bool is_active() const { return active; }

	bool match_msg(const gulps::message& msg)
	{
		std::unique_lock<std::mutex> lck(cat_mutex);

		for(const cat_pair& p : log_cats)
		{
			if(msg.cat_minor == p.cat || msg.cat_major == p.cat)
				return msg.lvl <= p.level;
		}

		return msg.lvl <= wildcard_level;
	}
};

namespace debug
{
	inline bool get_set_enable_assert(bool set = false, bool v = false)
	{
		static bool e = true;
		if(set)
			e = v;
		return e;
	};
};


#define GULPS_CSTR_TYPE(...) \
  union { static constexpr auto c_str() -> decltype(__VA_ARGS__) { return __VA_ARGS__; } }
#define GULPS_CSTR(...) \
  [] { using Cat = GULPS_CSTR_TYPE(__VA_ARGS__); return Cat{}.c_str(); }()

#define GULPS_CAT_MAJOR(...) using gulps_major_cat = GULPS_CSTR_TYPE(__VA_ARGS__)
#define GULPS_CAT_MINOR(...) using gulps_minor_cat = GULPS_CSTR_TYPE(__VA_ARGS__)

GULPS_CAT_MINOR("");

#define GULPS_OUTPUT(out, lvl, maj, min, clr, ...) gulps::inst().log(gulps::message(out, lvl, maj, min, __FILE__, __LINE__, stream_writer::write(__VA_ARGS__), clr))
#define GULPS_OUTPUTF(out, lvl, maj, min, clr, ...) gulps::inst().log(gulps::message(out, lvl, maj, min, __FILE__, __LINE__, fmt::format(__VA_ARGS__), clr))

#define GULPS_PRINT_NOLF(...) gulps::inst().log(gulps::message(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), "input_line",  \
		__FILE__, __LINE__, stream_writer::write(__VA_ARGS__), gulps::COLOR_WHITE, false));

#define GULPS_PRINT(...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_PRINT_CLR(clr, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT,  gulps_major_cat::c_str(), gulps_minor_cat::c_str(), clr, __VA_ARGS__)
#define GULPS_ERROR(...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_RED, __VA_ARGS__)
#define GULPS_WARN(...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_YELLOW, __VA_ARGS__)
#define GULPS_INFO(...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_INFO_CLR(clr, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_INFO,  gulps_major_cat::c_str(), gulps_minor_cat::c_str(), clr, __VA_ARGS__)


#define GULPS_DEBUG_PRINT(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_ERROR(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_WARN(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_L0(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_L1(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_L2(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_LOG_L3(...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE2, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)

#define GULPSF_PRINT(...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_PRINT_CLR(clr, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_PRINT,  gulps_major_cat::c_str(), gulps_minor_cat::c_str(), clr, __VA_ARGS__)
#define GULPSF_ERROR(...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_ERROR,  gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_RED, __VA_ARGS__)
#define GULPSF_WARN(...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_BOLD_YELLOW, __VA_ARGS__)
#define GULPSF_INFO(...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_INFO_CLR(clr, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_INFO,  gulps_major_cat::c_str(), gulps_minor_cat::c_str(), clr, __VA_ARGS__)


#define GULPSF_DEBUG_PRINT(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_ERROR(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_WARN(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_L0(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_L1(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_L2(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_TRACE, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_LOG_L3(...) GULPS_OUTPUTF(gulps::OUT_LOG_0, gulps::LEVEL_TRACE2, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), gulps::COLOR_WHITE, __VA_ARGS__)

#define GULPS_CAT_PRINT(min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_PRINT_CLR(clr, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT,  gulps_major_cat::c_str(), min, clr, __VA_ARGS__)
#define GULPS_CAT_ERROR(min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), min, gulps::COLOR_BOLD_RED, __VA_ARGS__)
#define GULPS_CAT_WARN(min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), min, gulps::COLOR_BOLD_YELLOW, __VA_ARGS__)
#define GULPS_CAT_INFO(min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_INFO_CLR(clr, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), min, clr, __VA_ARGS__)

#define GULPS_CAT_DEBUG_PRINT(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_ERROR(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_WARN(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_L0(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_L1(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_L2(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT_LOG_L3(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE2, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)

#define GULPSF_CAT_PRINT(min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_CAT_PRINT_CLR(clr, min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_PRINT, gulps_major_cat::c_str(), min, clr, __VA_ARGS__)
#define GULPSF_CAT_ERROR(min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_ERROR, gulps_major_cat::c_str(), min, gulps::COLOR_BOLD_RED, __VA_ARGS__)
#define GULPSF_CAT_WARN(min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_WARN, gulps_major_cat::c_str(), min, gulps::COLOR_BOLD_YELLOW, __VA_ARGS__)
#define GULPSF_CAT_INFO(min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPSF_CAT_INFO_CLR(clr, min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_INFO, gulps_major_cat::c_str(), min, clr, __VA_ARGS__)

#define GULPSF_CAT_LOG_L1(min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, gulps_major_cat::c_str(), min, gulps::COLOR_WHITE, __VA_ARGS__)


#define GULPS_CAT2_PRINT(maj, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_PRINT_CLR(clr, maj, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_PRINT,  maj, min, clr, __VA_ARGS__)
#define GULPS_CAT2_ERROR(maj, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_ERROR, maj, min, gulps::COLOR_BOLD_RED, __VA_ARGS__)
#define GULPS_CAT2_WARN(maj, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_WARN, maj, min, gulps::COLOR_BOLD_YELLOW, __VA_ARGS__)
#define GULPS_CAT2_INFO(maj, min, ...) GULPS_OUTPUT(gulps::OUT_USER_0, gulps::LEVEL_INFO, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)

#define GULPS_CAT2_DEBUG_PRINT(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_PRINT, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_ERROR(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_ERROR, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_WARN(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_WARN, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_L0(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_INFO, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_L1(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_DEBUG, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_L2(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)
#define GULPS_CAT2_LOG_L3(maj, min, ...) GULPS_OUTPUT(gulps::OUT_LOG_0, gulps::LEVEL_TRACE2, maj, min, gulps::COLOR_WHITE, __VA_ARGS__)

#define GULPSF_CAT2_LOG_ERROR(maj, min, ...) GULPS_OUTPUTF(gulps::OUT_USER_0, gulps::LEVEL_ERROR, maj, min, gulps::COLOR_BOLD_RED, __VA_ARGS__)

#define GULPS_VERIFY_ERR_TX(...) GULPS_CAT2_LOG_ERROR("verify", "tx", __VA_ARGS__)
#define GULPS_VERIFY_ERR_BLK(...) GULPS_CAT2_LOG_ERROR("verify", "block", __VA_ARGS__)

#define GULPSF_VERIFY_ERR_TX(...) GULPSF_CAT2_LOG_ERROR("verify", "tx", __VA_ARGS__)
#define GULPSF_VERIFY_ERR_BLK(...) GULPSF_CAT2_LOG_ERROR("verify", "block", __VA_ARGS__)

#define GULPS_GLOBAL_PRINT(...) GULPS_CAT_PRINT_CLR(gulps::COLOR_CYAN, "global", __VA_ARGS__)
#define GULPSF_GLOBAL_PRINT(...) GULPSF_CAT_PRINT_CLR(gulps::COLOR_CYAN, "global", __VA_ARGS__)

#define GULPS_GLOBAL_PRINT_CLR(clr, ...) GULPS_CAT_PRINT_CLR(clr, "global", __VA_ARGS__)
#define GULPSF_GLOBAL_PRINT_CLR(clr, ...) GULPSF_CAT_PRINT_CLR(clr, "global", __VA_ARGS__)

/*
#define GULPS_ERROR(...) GULPS_OUTPUT(gulps::LEVEL_ERROR, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::red, __VA_ARGS__)
#define GULPS_WARN(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_WARN, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::orange, fstr, __VA_ARGS__)
#define GULPS_INFO(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_INFO, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG0(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_LOG_0, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG1(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_1, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_DEBUG2(fstr, ...) GULPS_OUTPUT(gulps::LEVEL_DEBUG_2, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), fmt::color::white, fstr, __VA_ARGS__)
#define GULPS_PRINT(clr, fstr, ...) GULPS_OUTPUT(gulps::LEVEL_OUTPUT_0, gulps_major_cat::c_str(), gulps_minor_cat::c_str(), clr, fstr, __VA_ARGS__)*/

#define GULPS_SET_THREAD_NAME(x) gulps::inst().set_thread_tag(x)

#ifndef GULPS_LOCAL_ASSERT
#include <assert.h>
#if(defined _MSC_VER)
#define GULPS_LOCAL_ASSERT(expr)                       \
	{                                            \
		if(debug::get_set_enable_assert()) \
		{                                        \
			_ASSERTE(expr);                      \
		}                                        \
	}
#else
#define GULPS_LOCAL_ASSERT(expr)
#endif
#endif

#define GULPS_TRY_ENTRY() \
	try             \
	{
#define GULPS_CATCH_ENTRY(location, return_val)                                          \
	}                                                                              \
	catch(const std::exception &ex)                                                \
	{                                                                              \
		(void)(ex);                                                                \
		GULPS_LOG_ERROR("Exception at [", location, "], what=", ex.what());        \
		return return_val;                                                         \
	}                                                                              \
	catch(...)                                                                     \
	{                                                                              \
		GULPS_LOG_ERROR("Exception at [", location, "], generic exception \"...\""); \
		return return_val;                                                         \
	}

#define GULPS_CATCH_ENTRY_L0(lacation, return_val) GULPS_CATCH_ENTRY(lacation, return_val)
#define GULPS_CATCH_ENTRY_L1(lacation, return_val) GULPS_CATCH_ENTRY(lacation, return_val)
#define GULPS_CATCH_ENTRY_L2(lacation, return_val) GULPS_CATCH_ENTRY(lacation, return_val)
#define GULPS_CATCH_ENTRY_L3(lacation, return_val) GULPS_CATCH_ENTRY(lacation, return_val)
#define GULPS_CATCH_ENTRY_L4(lacation, return_val) GULPS_CATCH_ENTRY(lacation, return_val)

#define GULPS_ASSERT_MES_AND_THROW(...)       \
{                                       \
	std::string str;	\
	str = stream_writer::write(__VA_ARGS__);	\
	GULPS_LOG_ERROR(__VA_ARGS__);                 \
	throw std::runtime_error(str); \
}

#define GULPS_CHECK_AND_ASSERT_THROW_MES(expr, ...) \
do                                            \
{                                             \
	if(!(expr))                               \
		GULPS_ASSERT_MES_AND_THROW(__VA_ARGS__);        \
} while(0)

#define GULPS_CHECK_AND_ASSERT(expr, fail_ret_val) \
do                                       \
{                                        \
	if(!(expr))                          \
	{                                    \
		GULPS_LOCAL_ASSERT(expr);              \
		return fail_ret_val;             \
	};                                   \
} while(0)

#define GULPS_CHECK_AND_ASSERT_MES(expr, fail_ret_val, ...) \
do                                                    \
{                                                     \
	if(!(expr))                                       \
	{                                                 \
		GULPS_LOG_ERROR(__VA_ARGS__);                           \
		return fail_ret_val;                          \
	};                                                \
} while(0)

#define GULPS_CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, l, ...) \
do                                                            \
{                                                             \
	if(!(expr))                                               \
	{                                                         \
		GULPS_LOG_L##l(__VA_ARGS__); /*LOCAL_ASSERT(expr);*/      \
		return fail_ret_val;                                  \
	};                                                        \
} while(0)

#define GULPS_CHECK_AND_NO_ASSERT_MES(expr, fail_ret_val, ...) GULPS_CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, 0, __VA_ARGS__)

#define GULPS_CHECK_AND_NO_ASSERT_MES_L1(expr, fail_ret_val, ... ) GULPS_CHECK_AND_NO_ASSERT_MES_L(expr, fail_ret_val, 1, __VA_ARGS__)

#define GULPS_CHECK_AND_ASSERT_MES_NO_RET(expr, ...) \
do                                             \
{                                              \
	if(!(expr))                                \
	{                                          \
		GULPS_LOG_ERROR(__VA_ARGS__);                    \
		return;                                \
	};                                         \
} while(0)

#define GULPS_CHECK_AND_ASSERT_MES2(expr, ...)) \
	do                                       \
	{                                        \
		if(!(expr))                          \
		{                                    \
			GULPS_LOG_ERROR(__VA_ARGS__);              \
		};                                   \
	} while(0)

#define GULPS_CHECK_AND_ASSERT_MES_CONTEXT(condition, return_val, ...) GULPS_CHECK_AND_ASSERT_MES(condition, return_val, "[", epee::net_utils::print_connection_context_short(context), "] ", __VA_ARGS__)
