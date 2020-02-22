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

#include <queue>
#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

template <typename T>
class thdq
{
public:

	bool pop(T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.empty() && !finish) { cond_.wait(mlock); }
		if(!queue_.empty())
		{
			item = std::move(queue_.front());
			queue_.pop();
			mlock.unlock();
			size_cond_.notify_all();
			return true;
		}
		return false;
	}

	std::unique_lock<std::mutex> get_lock()
	{
		return std::unique_lock<std::mutex>(mutex_, std::defer_lock);
	}

	bool wait_for_pop(std::unique_lock<std::mutex>& lck)
	{
		lck.lock();
		while (queue_.empty() && !finish) { cond_.wait(lck); }
		bool has_pop = !queue_.empty();
		if(!has_pop) { lck.unlock(); }
		return has_pop;
	}

	size_t wait_for_size(size_t q_size)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		while (queue_.size() > q_size) { size_cond_.wait(mlock); }
		return queue_.size();
	}

	T pop(std::unique_lock<std::mutex>& lck)
	{
		T item = std::move(queue_.front());
		queue_.pop();
		lck.unlock();
		size_cond_.notify_all();
		return item;
	}

	void push(const T& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		if(finish) return;
		queue_.push(item);
		mlock.unlock();
		cond_.notify_one();
	}

	void push(T&& item)
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		if(finish) return;
		queue_.push(std::move(item));
		mlock.unlock();
		cond_.notify_one();
	}

	void set_finish_flag()
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		finish = true;
		cond_.notify_all();
	}

private:
	std::queue<T> queue_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::condition_variable size_cond_;
	bool finish = false;
};
