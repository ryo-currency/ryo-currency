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

#define WALLET_RPC_ERROR_CODE_UNKNOWN_ERROR -1
#define WALLET_RPC_ERROR_CODE_WRONG_ADDRESS -2
#define WALLET_RPC_ERROR_CODE_DAEMON_IS_BUSY -3
#define WALLET_RPC_ERROR_CODE_GENERIC_TRANSFER_ERROR -4
#define WALLET_RPC_ERROR_CODE_WRONG_PAYMENT_ID -5
#define WALLET_RPC_ERROR_CODE_TRANSFER_TYPE -6
#define WALLET_RPC_ERROR_CODE_DENIED -7
#define WALLET_RPC_ERROR_CODE_WRONG_TXID -8
#define WALLET_RPC_ERROR_CODE_WRONG_SIGNATURE -9
#define WALLET_RPC_ERROR_CODE_WRONG_KEY_IMAGE -10
#define WALLET_RPC_ERROR_CODE_WRONG_URI -11
#define WALLET_RPC_ERROR_CODE_WRONG_INDEX -12
#define WALLET_RPC_ERROR_CODE_NOT_OPEN -13
#define WALLET_RPC_ERROR_CODE_ACCOUNT_INDEX_OUT_OF_BOUNDS -14
#define WALLET_RPC_ERROR_CODE_ADDRESS_INDEX_OUT_OF_BOUNDS -15
#define WALLET_RPC_ERROR_CODE_TX_NOT_POSSIBLE -16
#define WALLET_RPC_ERROR_CODE_NOT_ENOUGH_MONEY -17
#define WALLET_RPC_ERROR_CODE_TX_TOO_LARGE -18
#define WALLET_RPC_ERROR_CODE_NOT_ENOUGH_OUTS_TO_MIX -19
#define WALLET_RPC_ERROR_CODE_ZERO_DESTINATION -20
#define WALLET_RPC_ERROR_CODE_WALLET_ALREADY_EXISTS -21
#define WALLET_RPC_ERROR_CODE_INVALID_PASSWORD -22
#define WALLET_RPC_ERROR_CODE_NO_WALLET_DIR -23
#define WALLET_RPC_ERROR_CODE_NO_TXKEY -24
#define WALLET_RPC_ERROR_CODE_WRONG_KEY -25
#define WALLET_RPC_ERROR_CODE_BAD_HEX -26
#define WALLET_RPC_ERROR_CODE_BAD_TX_METADATA -27
#define WALLET_RPC_ERROR_CODE_ALREADY_MULTISIG -28
#define WALLET_RPC_ERROR_CODE_WATCH_ONLY -29
#define WALLET_RPC_ERROR_CODE_BAD_MULTISIG_INFO -30
#define WALLET_RPC_ERROR_CODE_NOT_MULTISIG -31
#define WALLET_RPC_ERROR_CODE_WRONG_LR -32
#define WALLET_RPC_ERROR_CODE_THRESHOLD_NOT_REACHED -33
#define WALLET_RPC_ERROR_CODE_BAD_MULTISIG_TX_DATA -34
#define WALLET_RPC_ERROR_CODE_MULTISIG_SIGNATURE -35
#define WALLET_RPC_ERROR_CODE_MULTISIG_SUBMISSION -36
#define WALLET_RPC_ERROR_CODE_NOT_ENOUGH_UNLOCKED_MONEY -37
#define WALLET_RPC_ERROR_CODE_NO_DAEMON_CONNECTION -38
