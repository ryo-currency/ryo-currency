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

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "hash-ops.h"
#include "initializer.h"
#include "random.h"

static void generate_system_random_bytes(size_t n, void *result);

#if defined(_WIN32)

#include <stdio.h>
#include <windows.h>
#include <wincrypt.h>

static void generate_system_random_bytes(size_t n, void *result)
{
	HCRYPTPROV prov;
#ifdef NDEBUG
#define must_succeed(x)                     \
	do                                      \
		if(!(x))                            \
		{                                   \
			fprintf(stderr, "Failed: " #x); \
			_exit(1);                       \
		}                                   \
	while(0)
#else
#define must_succeed(x) \
	do                  \
		if(!(x))        \
			abort();    \
	while(0)
#endif
	must_succeed(CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT));
	must_succeed(CryptGenRandom(prov, (DWORD)n, result));
	must_succeed(CryptReleaseContext(prov, 0));
#undef must_succeed
}

#else

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void generate_system_random_bytes(size_t n, void *result)
{
	int fd;
	if((fd = open("/dev/urandom", O_RDONLY | O_NOCTTY | O_CLOEXEC)) < 0)
	{
		err(EXIT_FAILURE, "open /dev/urandom");
	}
	for(;;)
	{
		ssize_t res = read(fd, result, n);
		if((size_t)res == n)
		{
			break;
		}
		if(res < 0)
		{
			if(errno != EINTR)
			{
				err(EXIT_FAILURE, "read /dev/urandom");
			}
		}
		else if(res == 0)
		{
			errx(EXIT_FAILURE, "read /dev/urandom: end of file");
		}
		else
		{
			result = padd(result, (size_t)res);
			n -= (size_t)res;
		}
	}
	if(close(fd) < 0)
	{
		err(EXIT_FAILURE, "close /dev/urandom");
	}
}

#endif

static union hash_state state;

#if !defined(NDEBUG)
static volatile int curstate; /* To catch thread safety problems. */
#endif

FINALIZER(deinit_random)
{
#if !defined(NDEBUG)
	assert(curstate == 1);
	curstate = 0;
#endif
	memset(&state, 0, sizeof(union hash_state));
}

INITIALIZER(init_random)
{
	generate_system_random_bytes(32, &state);
	REGISTER_FINALIZER(deinit_random);
#if !defined(NDEBUG)
	assert(curstate == 0);
	curstate = 1;
#endif
}

void generate_random_bytes_not_thread_safe(size_t n, void *result)
{
#if !defined(NDEBUG)
	assert(curstate == 1);
	curstate = 2;
#endif
	if(n == 0)
	{
#if !defined(NDEBUG)
		assert(curstate == 2);
		curstate = 1;
#endif
		return;
	}
	for(;;)
	{
		hash_permutation(&state);
		if(n <= HASH_DATA_AREA)
		{
			memcpy(result, &state, n);
#if !defined(NDEBUG)
			assert(curstate == 2);
			curstate = 1;
#endif
			return;
		}
		else
		{
			memcpy(result, &state, HASH_DATA_AREA);
			result = padd(result, HASH_DATA_AREA);
			n -= HASH_DATA_AREA;
		}
	}
}
