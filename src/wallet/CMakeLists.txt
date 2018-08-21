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

# include (${PROJECT_SOURCE_DIR}/cmake/libutils.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(wallet_sources
  wallet2.cpp
  wallet_args.cpp
  ringdb.cpp
  node_rpc_proxy.cpp)

set(wallet_private_headers
  wallet2.h
  wallet_args.h
  wallet_errors.h
  wallet_rpc_server.h
  wallet_rpc_server_commands_defs.h
  wallet_rpc_server_error_codes.h
  ringdb.h
  node_rpc_proxy.h)

ryo_private_headers(wallet
  ${wallet_private_headers})
ryo_add_library(wallet
  ${wallet_sources}
  ${wallet_private_headers})
target_link_libraries(wallet
  PUBLIC
    multisig
    common
    cryptonote_core
    mnemonics
    ${LMDB_LIBRARY}
    ${Boost_CHRONO_LIBRARY}
    ${Boost_SERIALIZATION_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_THREAD_LIBRARY}
    ${Boost_REGEX_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})

set(wallet_rpc_sources
  wallet_rpc_server.cpp)

set(wallet_rpc_headers)

set(wallet_rpc_private_headers
  wallet_rpc_server.h)

ryo_private_headers(wallet_rpc_server
  ${wallet_rpc_private_headers})
monero_add_executable(wallet_rpc_server
  ${wallet_rpc_sources}
  ${wallet_rpc_headers}
  ${wallet_rpc_private_headers})

target_link_libraries(wallet_rpc_server
  PRIVATE
    wallet
    epee
    rpc_base
    cryptonote_core
    cncrypto
    common
    version
    ccnconfig
    ${Boost_CHRONO_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_LOCALE_LIBRARY}
    ${ICU_LIBRARIES}
    ${Boost_THREAD_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})
set_property(TARGET wallet_rpc_server
  PROPERTY
    OUTPUT_NAME "ryo-wallet-rpc")
install(TARGETS wallet_rpc_server DESTINATION bin)


# build and install libwallet_merged only if we building for GUI
if (BUILD_GUI_DEPS)
    set(libs_to_merge
            wallet_api
            wallet
            multisig
            cryptonote_core
            cryptonote_basic
            mnemonics
            common
            cncrypto
            device
            ringct
            ringct_basic
            checkpoints
            version)

    foreach(lib ${libs_to_merge})
        list(APPEND objlibs $<TARGET_OBJECTS:obj_${lib}>) # matches naming convention in src/CMakeLists.txt
    endforeach()
    add_library(wallet_merged STATIC ${objlibs})
    if(IOS)
        set(lib_folder lib-${ARCH})
    else()
        set(lib_folder lib)
    endif()
    install(TARGETS wallet_merged
        ARCHIVE DESTINATION ${lib_folder})
endif()

add_subdirectory(api)
