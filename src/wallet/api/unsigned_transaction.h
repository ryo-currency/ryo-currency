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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "wallet/api/wallet2_api.h"
#include "wallet/wallet2.h"

#include <string>
#include <vector>

namespace Ryo
{

class WalletImpl;
class UnsignedTransactionImpl : public UnsignedTransaction
{
  public:
	UnsignedTransactionImpl(WalletImpl &wallet);
	~UnsignedTransactionImpl();
	int status() const;
	std::string errorString() const;
	std::vector<uint64_t> amount() const;
	std::vector<uint64_t> dust() const;
	std::vector<uint64_t> fee() const;
	std::vector<uint64_t> mixin() const;
	std::vector<std::string> paymentId() const;
	std::vector<std::string> recipientAddress() const;
	uint64_t txCount() const;
	// sign txs and save to file
	bool sign(const std::string &signedFileName);
	std::string confirmationMessage() const { return m_confirmationMessage; }
	uint64_t minMixinCount() const;

  private:
	// Callback function to check all loaded tx's and generate confirmationMessage
	bool checkLoadedTx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data &(size_t)> &get_tx, const std::string &extra_message);

	friend class WalletImpl;
	WalletImpl &m_wallet;

	int m_status;
	std::string m_errorString;
	tools::wallet2::unsigned_tx_set m_unsigned_tx_set;
	std::string m_confirmationMessage;
};
}
