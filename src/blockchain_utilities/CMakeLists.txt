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

set(blocksdat "")
if(PER_BLOCK_CHECKPOINT)
  if(APPLE)
    add_custom_command(OUTPUT blocksdat.o MAIN_DEPENDENCY ../blocks/checkpoints.dat COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && touch stub.c && ${CMAKE_C_COMPILER} -o stub.o -c stub.c COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && ${CMAKE_LINKER} ${LD_RAW_FLAGS} -r -sectcreate __DATA __blocks_dat ../blocks/checkpoints.dat -o ${CMAKE_CURRENT_BINARY_DIR}/blocksdat.o stub.o && rm -f stub.*)
  else()
    add_custom_command(OUTPUT blocksdat.o MAIN_DEPENDENCY ../blocks/checkpoints.dat COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && cp ../blocks/checkpoints.dat blocks.dat && ${CMAKE_LINKER} ${LD_RAW_FLAGS} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/blocksdat.o blocks.dat && rm -f blocks.dat)
  endif()
  set(blocksdat "blocksdat.o")
endif()

set(blockchain_import_sources
  blockchain_import.cpp
  bootstrap_file.cpp
  blocksdat_file.cpp
  )

set(blockchain_import_private_headers
  bootstrap_file.h
  blocksdat_file.h
  bootstrap_serialization.h
  )

ryo_private_headers(blockchain_import
	  ${blockchain_import_private_headers})

set(blockchain_export_sources
  blockchain_export.cpp
  bootstrap_file.cpp
  blocksdat_file.cpp
  )

set(blockchain_export_private_headers
  bootstrap_file.h
  blocksdat_file.h
  bootstrap_serialization.h
  )

ryo_private_headers(blockchain_export
	  ${blockchain_export_private_headers})


set(blockchain_blackball_sources
  blockchain_blackball.cpp
  )

set(blockchain_blackball_private_headers
  bootstrap_file.h
  blocksdat_file.h
  bootstrap_serialization.h
  )

ryo_private_headers(blockchain_blackball
	  ${blockchain_blackball_private_headers})

set(blockchain_usage_sources
  blockchain_usage.cpp
  )

set(blockchain_usage_private_headers)

ryo_private_headers(blockchain_usage
	  ${blockchain_usage_private_headers})



monero_add_executable(blockchain_import
  ${blockchain_import_sources}
  ${blockchain_import_private_headers}
  ${blocksdat})

target_link_libraries(blockchain_import
  PRIVATE
    cryptonote_core
    blockchain_db
    p2p
    version
    ccnconfig
    epee
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

if(ARCH_WIDTH)
  target_compile_definitions(blockchain_import
    PUBLIC -DARCH_WIDTH=${ARCH_WIDTH})
endif()

set_property(TARGET blockchain_import
	PROPERTY
	OUTPUT_NAME "ryo-blockchain-import")
install(TARGETS blockchain_import DESTINATION bin)

monero_add_executable(blockchain_export
  ${blockchain_export_sources}
  ${blockchain_export_private_headers})

target_link_libraries(blockchain_export
  PRIVATE
    cryptonote_core
    blockchain_db
    p2p
    version
    ccnconfig
    epee
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

set_property(TARGET blockchain_export
	PROPERTY
	OUTPUT_NAME "ryo-blockchain-export")
install(TARGETS blockchain_export DESTINATION bin)

monero_add_executable(blockchain_blackball
  ${blockchain_blackball_sources}
  ${blockchain_blackball_private_headers})

target_link_libraries(blockchain_blackball
  PRIVATE
    wallet
    cryptonote_core
    blockchain_db
    p2p
    version
    ccnconfig
    epee
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_LOCALE_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

set_property(TARGET blockchain_blackball
	PROPERTY
	OUTPUT_NAME "ryo-blockchain-blackball")
install(TARGETS blockchain_blackball DESTINATION bin)


monero_add_executable(blockchain_usage
  ${blockchain_usage_sources}
  ${blockchain_usage_private_headers})

target_link_libraries(blockchain_usage
  PRIVATE
    cryptonote_core
    blockchain_db
    p2p
    version
    ccnconfig
    epee
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

set_property(TARGET blockchain_usage
	PROPERTY
	OUTPUT_NAME "ryo-blockchain-usage")
install(TARGETS blockchain_usage DESTINATION bin)

