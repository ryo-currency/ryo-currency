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
#
# Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

if (WIN32 OR STATIC)
  add_definitions(-DSTATICLIB)
  # miniupnp changed their static define
  add_definitions(-DMINIUPNP_STATICLIB)
endif ()

# warnings are cleared only for GCC on Linux
if (NOT (MINGW OR APPLE OR FREEBSD OR OPENBSD OR DRAGONFLY))
  add_compile_options("${WARNINGS_AS_ERRORS_FLAG}") # applies only to targets that follow
endif()

function (ryo_private_headers group)
  source_group("${group}\\Private"
    FILES
      ${ARGN})
endfunction ()

function (monero_install_headers subdir)
  install(
    FILES       ${ARGN}
    DESTINATION "include/${subdir}"
    COMPONENT   development)
endfunction ()

function (enable_stack_trace target)
  if(STACK_TRACE)
    set_property(TARGET ${target}
      APPEND PROPERTY COMPILE_DEFINITIONS "STACK_TRACE")
    if (STATIC)
      set_property(TARGET "${target}"
        APPEND PROPERTY LINK_FLAGS "-Wl,--wrap=__cxa_throw")
    endif()
  endif()
endfunction()

function (monero_add_executable name)
  source_group("${name}"
    FILES
      ${ARGN})

  add_executable("${name}"
    ${ARGN})
  target_link_libraries("${name}"
    PRIVATE
      ${EXTRA_LIBRARIES})
  set_property(TARGET "${name}"
    PROPERTY
      FOLDER "prog")
  set_property(TARGET "${name}"
    PROPERTY
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  enable_stack_trace("${name}")
endfunction ()

function (ryo_add_library name)
    ryo_add_library_with_deps(NAME "${name}" SOURCES ${ARGN})
endfunction()

function (ryo_add_library_with_deps)
  cmake_parse_arguments(RYO_ADD_LIBRARY "" "NAME" "DEPENDS;SOURCES" ${ARGN})
  source_group("${RYO_ADD_LIBRARY_NAME}" FILES ${RYO_ADD_LIBRARY_SOURCES})

  # Define a ("virtual") object library and an actual library that links those
  # objects together. The virtual libraries can be arbitrarily combined to link
  # any subset of objects into one library archive. This is used for releasing
  # libwallet, which combines multiple components.
  set(objlib obj_${RYO_ADD_LIBRARY_NAME})
  add_library(${objlib} OBJECT ${RYO_ADD_LIBRARY_SOURCES})
  add_library("${RYO_ADD_LIBRARY_NAME}" $<TARGET_OBJECTS:${objlib}>)
  if (RYO_ADD_LIBRARY_DEPENDS)
    add_dependencies(${objlib} ${RYO_ADD_LIBRARY_DEPENDS})
  endif()
  set_property(TARGET "${RYO_ADD_LIBRARY_NAME}" PROPERTY FOLDER "libs")
  target_compile_definitions(${objlib}
    PRIVATE $<TARGET_PROPERTY:${RYO_ADD_LIBRARY_NAME},INTERFACE_COMPILE_DEFINITIONS>)
  target_include_directories(${objlib}
    PRIVATE $<TARGET_PROPERTY:${RYO_ADD_LIBRARY_NAME},INTERFACE_INCLUDE_DIRECTORIES>)
endfunction ()

include(Version)
ryo_add_library(version SOURCES ${CMAKE_BINARY_DIR}/version.cpp DEPENDS genversion)
ryo_add_library(ccnconfig SOURCES ${CMAKE_SOURCE_DIR}/src/cryptonote_config.cpp)

add_subdirectory(common)
add_subdirectory(crypto)
add_subdirectory(ringct)
add_subdirectory(checkpoints)
add_subdirectory(cryptonote_basic)
add_subdirectory(cryptonote_core)
add_subdirectory(multisig)
add_subdirectory(address_validator)
if(NOT IOS)
  add_subdirectory(blockchain_db)
endif()
add_subdirectory(mnemonics)
if(NOT IOS)
  add_subdirectory(rpc)
  add_subdirectory(serialization)
endif()
add_subdirectory(wallet)
if(NOT IOS)
  add_subdirectory(p2p)
endif()
add_subdirectory(cryptonote_protocol)
if(NOT IOS)
  add_subdirectory(simplewallet)
  add_subdirectory(gen_multisig)
  add_subdirectory(daemonizer)
  add_subdirectory(daemon)
  add_subdirectory(blockchain_utilities)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_subdirectory(debug_utilities)
endif()

if(PER_BLOCK_CHECKPOINT)
  add_subdirectory(blocks)
endif()

add_subdirectory(device)
