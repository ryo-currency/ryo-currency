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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once

#if defined(__GNUC__)
#if defined(__sun) && defined(__SVR4)
#define INITIALIZER(name) __attribute__((constructor)) static void name(void)
#define FINALIZER(name) __attribute__((destructor)) static void name(void)
#else
#define INITIALIZER(name) __attribute__((constructor(101))) static void name(void)
#define FINALIZER(name) __attribute__((destructor(101))) static void name(void)
#endif
#define REGISTER_FINALIZER(name) ((void)0)

#elif defined(_MSC_VER)
#include <assert.h>
#include <stdlib.h>
// http://stackoverflow.com/questions/1113409/attribute-constructor-equivalent-in-vc
// http://msdn.microsoft.com/en-us/library/bb918180.aspx
#pragma section(".CRT$XCT", read)
#define INITIALIZER(name)                                                         \
	static void __cdecl name(void);                                               \
	__declspec(allocate(".CRT$XCT")) void(__cdecl * const _##name)(void) = &name; \
	static void __cdecl name(void)
#define FINALIZER(name) \
	static void __cdecl name(void)
#define REGISTER_FINALIZER(name) \
	do                           \
	{                            \
		int _res = atexit(name); \
		assert(_res == 0);       \
	} while(0);

#else
#error Unsupported compiler
#endif
