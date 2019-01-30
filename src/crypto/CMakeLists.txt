# Copyright (c) 2018, Ryo Currency Project
# Portions copyright (c) 2014-2018, The Monero Project
#
# Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
# All rights reserved.
# 
# Authors and copyright holders give permission for following:
#
# 1. Redistribution and use in source and binary forms WITHOUT modification.
#
# 2. Modification of the source form for your own personal use. 
# 
# As long as the following conditions are met:
# 
# 3. You must not distribute modified copies of the work to third parties. This includes 
#    posting the work online, or hosting copies of the modified work for download.
#
# 4. Any derivative version of this work is also covered by this license, including point 8.
#
# 5. Neither the name of the copyright holders nor the names of the authors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# 6. You agree that this licence is governed by and shall be construed in accordance 
#    with the laws of England and Wales.
#
# 7. You agree to submit all disputes arising out of or in connection with this licence 
#    to the exclusive jurisdiction of the Courts of England and Wales.
#
# Authors and copyright holders agree that:
#
# 8. This licence expires and the work covered by it is released into the 
#    public domain on 1st of February 2019
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set(crypto_sources
  chacha.c
  crypto-ops-data.c
  crypto-ops.c
  crypto.cpp
  hash.c
  keccak.c
  random.cpp
  tree-hash.c
  pow_hash/aux_hash.c
  pow_hash/cn_slow_hash_soft.cpp
  pow_hash/cn_slow_hash_hard_intel.cpp
  pow_hash/cn_slow_hash_intel_avx2.cpp
  pow_hash/cn_slow_hash_hard_arm.cpp)


if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	if (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
		set_source_files_properties(pow_hash/cn_slow_hash_intel_avx2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
		set_source_files_properties(pow_hash/cn_slow_hard_intel.cpp PROPERTIES COMPILE_FLAGS "-msse2 -maes")
	elseif (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
		set_source_files_properties(pow_hash/cn_slow_hash_hard_arm.cpp PROPERTIES COMPILE_FLAGS "-march=armv8-a+crypto")
	endif()
endif()

set(crypto_headers)

set(crypto_private_headers
  chacha.h
  crypto-ops.h
  crypto.h
  generic-ops.h
  hash-ops.h
  hash.h
  keccak.h
  random.hpp
  pow_hash/cn_slow_hash.hpp)

ryo_private_headers(cncrypto
  ${crypto_private_headers})
ryo_add_library(cncrypto
  ${crypto_sources}
  ${crypto_headers}
  ${crypto_private_headers})
target_link_libraries(cncrypto
  PUBLIC
    epee
    ${Boost_SYSTEM_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})

if(BUILD_TESTS)
  # if tests are enabled than build a special crypto library with a deterministic
  # random number generator, this lib is than used for the test `crypto`
  ryo_add_library(cncrypto_for_test
    ${crypto_sources}
    ${crypto_headers}
    ${crypto_private_headers})
  target_link_libraries(cncrypto_for_test
    PUBLIC
      epee
      ${Boost_SYSTEM_LIBRARY}
    PRIVATE
      ${EXTRA_LIBRARIES})
  target_compile_definitions(cncrypto_for_test
      PUBLIC
      -DCRYPTO_TEST_ONLY_FIXED_PRNG
  )
endif()
