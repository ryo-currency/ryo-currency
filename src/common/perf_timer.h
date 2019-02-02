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

#include "misc_log_ex.h"
#include <memory>
#include <stdio.h>
#include <string>

namespace tools
{

class PerformanceTimer;

extern el::Level performance_timer_log_level;

uint64_t get_tick_count();
uint64_t get_ticks_per_ns();
uint64_t ticks_to_ns(uint64_t ticks);

class PerformanceTimer
{
  public:
	PerformanceTimer(bool paused = false);
	~PerformanceTimer();
	void pause();
	void resume();

	uint64_t value() const { return ticks; }

  protected:
	uint64_t ticks;
	bool started;
	bool paused;
};

class LoggingPerformanceTimer : public PerformanceTimer
{
  public:
	LoggingPerformanceTimer(const std::string &s, const std::string &cat, uint64_t unit, el::Level l = el::Level::Debug);
	~LoggingPerformanceTimer();

  private:
	std::string name;
	std::string cat;
	uint64_t unit;
	el::Level level;
};

void set_performance_timer_log_level(el::Level level);

#define PERF_TIMER_UNIT(name, unit) tools::LoggingPerformanceTimer pt_##name(#name, "perf.oldlog", unit, tools::performance_timer_log_level)
#define PERF_TIMER_UNIT_L(name, unit, l) tools::LoggingPerformanceTimer pt_##name(#name, "perf.oldlog", unit, l)
#define PERF_TIMER(name) PERF_TIMER_UNIT(name, 1000000)
#define PERF_TIMER_L(name, l) PERF_TIMER_UNIT_L(name, 1000000, l)
#define PERF_TIMER_START_UNIT(name, unit) std::unique_ptr<tools::LoggingPerformanceTimer> pt_##name(new tools::LoggingPerformanceTimer(#name, "perf.oldlog", unit, el::Level::Info))
#define PERF_TIMER_START(name) PERF_TIMER_START_UNIT(name, 1000000)
#define PERF_TIMER_STOP(name)  \
	do                         \
	{                          \
		pt_##name.reset(NULL); \
	} while(0)
#define PERF_TIMER_PAUSE(name) pt_##name->pause()
#define PERF_TIMER_RESUME(name) pt_##name->resume()
}
