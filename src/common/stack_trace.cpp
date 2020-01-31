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

#if !defined __GNUC__ || defined __MINGW32__ || defined __MINGW64__ || defined __ANDROID__
#define USE_UNWIND
#else
#define ELPP_FEATURE_CRASH_LOG 1
#endif

#include <stdexcept>
#ifdef USE_UNWIND
#define UNW_LOCAL_ONLY
#include <cxxabi.h>
#include <libunwind.h>
#endif
#ifndef STATICLIB
#include <dlfcn.h>
#endif
#include "common/stack_trace.h"
#include <boost/algorithm/string.hpp>

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("stk_trace");

// from http://stackoverflow.com/questions/11665829/how-can-i-print-stack-trace-for-caught-exceptions-in-c-code-injection-in-c

// The decl of __cxa_throw in /usr/include/.../cxxabi.h uses
// 'std::type_info *', but GCC's built-in protype uses 'void *'.
#ifdef __clang__
#define CXA_THROW_INFO_T std::type_info
#else // !__clang__
#define CXA_THROW_INFO_T void
#endif // !__clang__

#ifdef STATICLIB
#define CXA_THROW __wrap___cxa_throw
extern "C"
	__attribute__((noreturn)) void
	__real___cxa_throw(void *ex, CXA_THROW_INFO_T *info, void (*dest)(void *));
#else // !STATICLIB
#define CXA_THROW __cxa_throw
extern "C" typedef
#ifdef __clang__ // only clang, not GCC, lets apply the attr in typedef
	__attribute__((noreturn))
#endif // __clang__
	void(cxa_throw_t)(void *ex, CXA_THROW_INFO_T *info, void (*dest)(void *));
#endif // !STATICLIB

extern "C"
	__attribute__((noreturn)) void
	CXA_THROW(void *ex, CXA_THROW_INFO_T *info, void (*dest)(void *))
{

	int status;
	char *dsym = abi::__cxa_demangle(((const std::type_info *)info)->name(), NULL, NULL, &status);
	tools::log_stack_trace((std::string("Exception: ") + ((!status && dsym) ? dsym : (const char *)info)).c_str());
	free(dsym);

#ifndef STATICLIB
#ifndef __clang__ // for GCC the attr can't be applied in typedef like for clang
	__attribute__((noreturn))
#endif // !__clang__
	cxa_throw_t *__real___cxa_throw = (cxa_throw_t *)dlsym(RTLD_NEXT, "__cxa_throw");
#endif // !STATICLIB
	__real___cxa_throw(ex, info, dest);
}

namespace
{
std::string stack_trace_log;
}

namespace tools
{

void set_stack_trace_log(const std::string &log)
{
	stack_trace_log = log;
}

void log_stack_trace(const char *msg)
{
#ifdef USE_UNWIND
	unw_context_t ctx;
	unw_cursor_t cur;
	unw_word_t ip, off;
	unsigned level;
	char sym[512], *dsym;
	int status;
	const char *log = stack_trace_log.empty() ? NULL : stack_trace_log.c_str();
#endif

	if(msg)
		GULPS_LOG_L0(msg);
	GULPS_LOG_L0("Unwound call stack:");

#ifdef USE_UNWIND
	if(unw_getcontext(&ctx) < 0)
	{
		GULPS_LOG_L0("Failed to create unwind context");
		return;
	}
	if(unw_init_local(&cur, &ctx) < 0)
	{
		GULPS_LOG_L0("Failed to find the first unwind frame");
		return;
	}
	for(level = 1; level < 999; ++level)
	{ // 999 for safety
		int ret = unw_step(&cur);
		if(ret < 0)
		{
			GULPS_LOG_L0("Failed to find the next frame");
			return;
		}
		if(ret == 0)
			break;
		if(unw_get_reg(&cur, UNW_REG_IP, &ip) < 0)
		{
			GULPSF_LOG_L0("{:<4}", level);
			continue;
		}
		if(unw_get_proc_name(&cur, sym, sizeof(sym), &off) < 0)
		{
			GULPSF_LOG_L0("{:<4}{:<20#0.x}", level, ip);
			continue;
		}
		dsym = abi::__cxa_demangle(sym, NULL, NULL, &status);
		GULPSF_LOG_L0("{:<4}{:<20#0.x} {#0.x} + {#0.x}", level, ip, (!status && dsym ? dsym : sym), off);
		free(dsym);
	}
#else
	//TODO PRINTING STACK
	GULPS_LOG_L0("");
#endif
}

} // namespace tools
