// Copyright (c) 2019, Ryo Currency Project
//
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

#include <iostream>
#include <string.h>

#include "keccak.h"
#include "random.hpp"

#if defined(_WIN32)

#include <stdio.h>
#include <wincrypt.h>
#include <windows.h>

#else

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#endif

extern "C" void *memwipe(void *src, size_t n);

struct prng_handle
{
#if defined(CRYPTO_TEST_ONLY_FIXED_PRNG)
	uint64_t count = 0;
#elif defined(_WIN32)
	HCRYPTPROV prov;
#else
	int fd;
#endif
};

prng::~prng()
{
	if(hnd == nullptr)
		return;

#if !defined(CRYPTO_TEST_ONLY_FIXED_PRNG)
#if defined(_WIN32)
	if(!CryptReleaseContext(hnd->prov, 0))
	{
		GULPS_ERROR("CryptReleaseContext");
		std::abort();
	}
#else
	if(close(hnd->fd) < 0)
	{
		GULPS_ERROR("Exit Failure :: close /dev/urandom ");
		std::abort();
	}
#endif
#endif
	delete hnd;
}

void prng::start()
{
	hnd = new prng_handle;
#if defined(CRYPTO_TEST_ONLY_FIXED_PRNG)
	GULPS_ERROR("WARNING!!! Fixed PRNG is active! This should be done in tests only!");
	return;
#elif defined(_WIN32)
	if(!CryptAcquireContext(&hnd->prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
	{
		GULPS_ERROR("CryptAcquireContext Failed ");
		std::abort();
	}
#else
	if((hnd->fd = open("/dev/urandom", O_RDONLY | O_NOCTTY | O_CLOEXEC)) < 0)
	{
		GULPS_ERROR("Exit Failure :: open /dev/urandom");
		std::abort();
	}
#endif
	uint64_t test[2] = {0};
	generate_system_random_bytes(reinterpret_cast<uint8_t *>(test), sizeof(test));

	if(test[0] == 0 && test[1] == 0)
	{
		GULPS_ERROR("PRNG self-check failed!");
		std::abort();
	}
}

void prng::generate_random(uint8_t *output, size_t size_bytes)
{
	if(hnd == nullptr)
		start();

	if(size_bytes <= 32)
	{
		generate_system_random_bytes(output, size_bytes);
	}
	else
	{
		uint64_t buffer[5];
		buffer[0] = 0;
		generate_system_random_bytes(reinterpret_cast<uint8_t *>(buffer + 1), sizeof(buffer) - sizeof(uint64_t));

		while(size_bytes > 200)
		{
			buffer[0]++;
			keccak(reinterpret_cast<uint8_t *>(buffer), sizeof(buffer), output, 200);
			output += 200;
			size_bytes -= 200;
		}

		if(size_bytes > 0)
		{
			uint8_t last[200];
			buffer[0]++;
			keccak(reinterpret_cast<uint8_t *>(buffer), sizeof(buffer), last, 200);
			memcpy(output, last, size_bytes);
			memwipe(last, sizeof(last));
		}

		memwipe(buffer, sizeof(buffer));
	}
}

void prng::generate_system_random_bytes(uint8_t *result, size_t n)
{
#if defined(CRYPTO_TEST_ONLY_FIXED_PRNG)
	uint8_t buf[200];
	keccak(reinterpret_cast<uint8_t *>(&hnd->count), sizeof(hnd->count), buf, 200);
	memcpy(result, buf, n);
	hnd->count++;
#elif defined(_WIN32)
	if(!CryptGenRandom(hnd->prov, (DWORD)n, result))
	{
		GULPS_ERROR("CryptGenRandom Failed ");
		std::abort();
	}
#else
	while(true)
	{
		ssize_t res = read(hnd->fd, result, n);
		if((size_t)res == n)
		{
			break;
		}
		if(res < 0)
		{
			if(errno != EINTR)
			{
				GULPS_ERROR("EXIT_FAILURE :: read /dev/urandom");
				std::abort();
			}
		}
		else if(res == 0)
		{
			GULPS_ERROR("EXIT_FAILURE :: read /dev/urandom: end of file ");
			std::abort();
		}
		else
		{
			result += res;
			n -= (size_t)res;
		}
	}
#endif
}
