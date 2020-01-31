// Copyright (c) 2019, Ryo Currency Project
// Portions copyright (c) 2016, Monero Research Labs
//
// Author: Shen Noether <shen.noether@gmx.com>
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

#pragma once

//#define DBG

#ifndef RCTSIGS_H
#define RCTSIGS_H

#include <cstddef>
#include <tuple>
#include <vector>

#include "crypto/generic-ops.h"
#include "crypto/random.hpp"
#include "cryptonote_config.h"

extern "C"
{
#include "crypto/keccak.h"
}
#include "crypto/crypto.h"

#include "rctOps.h"
#include "rctTypes.h"

//Define this flag when debugging to get additional info on the console
#ifdef DEBUG_BP_PRINT
#define DP(x) dp(x)
#else
#define DP(x)
#endif

namespace hw
{
class device;
}

namespace rct
{

boroSig genBorromean(const key64 x, const key64 P1, const key64 P2, const bits indices);
bool verifyBorromean(const boroSig &bb, const key64 P1, const key64 P2);

//Multilayered Spontaneous Anonymous Group Signatures (MLSAG signatures)
//These are aka MG signatutes in earlier drafts of the ring ct paper
// c.f. http://eprint.iacr.org/2015/1098 section 2.
// Gen creates a signature which proves that for some column in the keymatrix "pk"
//   the signer knows a secret key for each row in that column
// Ver verifies that the MG sig was created correctly
mgSig MLSAG_Gen(const key &message, const keyM &pk, const keyV &xx, const multisig_kLRki *kLRki, key *mscout, const unsigned int index, size_t dsRows, hw::device &hwdev);
bool MLSAG_Ver(const key &message, const keyM &pk, const mgSig &sig, size_t dsRows);
//mgSig MLSAG_Gen_Old(const keyM & pk, const keyV & xx, const int index);

//proveRange and verRange
//proveRange gives C, and mask such that \sumCi = C
//   c.f. http://eprint.iacr.org/2015/1098 section 5.1
//   and Ci is a commitment to either 0 or 2^i, i=0,...,63
//   thus this proves that "amount" is in [0, 2^64]
//   mask is a such that C = aG + bH, and b = amount
//verRange verifies that \sum Ci = C and that each Ci is a commitment to 0 or 2^i
rangeSig proveRange(key &C, key &mask, const ryo_amount &amount);
bool verRange(const key &C, const rangeSig &as);

//Ring-ct MG sigs
//Prove:
//   c.f. http://eprint.iacr.org/2015/1098 section 4. definition 10.
//   This does the MG sig on the "dest" part of the given key matrix, and
//   the last row is the sum of input commitments from that column - sum output commitments
//   this shows that sum inputs = sum outputs
//Ver:
//   verifies the above sig is created corretly
mgSig proveRctMG(const ctkeyM &pubs, const ctkeyV &inSk, const keyV &outMasks, const ctkeyV &outPk, const multisig_kLRki *kLRki, key *mscout, unsigned int index, key txnFee, const key &message, hw::device &hwdev);
mgSig proveRctMGSimple(const key &message, const ctkeyV &pubs, const ctkey &inSk, const key &a, const key &Cout, const multisig_kLRki *kLRki, key *mscout, unsigned int index, hw::device &hwdev);
bool verRctMG(const mgSig &mg, const ctkeyM &pubs, const ctkeyV &outPk, key txnFee, const key &message);
bool verRctMGSimple(const key &message, const mgSig &mg, const ctkeyV &pubs, const key &C);

//These functions get keys from blockchain
//replace these when connecting blockchain
//getKeyFromBlockchain grabs a key from the blockchain at "reference_index" to mix with
//populateFromBlockchain creates a keymatrix with "mixin" columns and one of the columns is inPk
//   the return value are the key matrix, and the index where inPk was put (random).
void getKeyFromBlockchain(ctkey &a, size_t reference_index);
std::tuple<ctkeyM, ryo_amount> populateFromBlockchain(ctkeyV inPk, int mixin);

//RingCT protocol
//genRct:
//   creates an rctSig with all data necessary to verify the rangeProofs and that the signer owns one of the
//   columns that are claimed as inputs, and that the sum of inputs  = sum of outputs.
//   Also contains masked "amount" and "mask" so the receiver can see how much they received
//verRct:
//   verifies that all signatures (rangeProogs, MG sig, sum inputs = outputs) are correct
//decodeRct: (c.f. http://eprint.iacr.org/2015/1098 section 5.1.1)
//   uses the attached ecdh info to find the amounts represented by each output commitment
//   must know the destination private key to find the correct amount, else will return a random number
rctSig genRct(const key &message, const ctkeyV &inSk, const keyV &destinations, const std::vector<ryo_amount> &amounts, const ctkeyM &mixRing, const keyV &amount_keys, const multisig_kLRki *kLRki, multisig_out *msout, unsigned int index, ctkeyV &outSk, hw::device &hwdev);
rctSig genRct(const key &message, const ctkeyV &inSk, const ctkeyV &inPk, const keyV &destinations, const std::vector<ryo_amount> &amounts, const keyV &amount_keys, const multisig_kLRki *kLRki, multisig_out *msout, const int mixin, hw::device &hwdev);
rctSig genRctSimple(const key &message, const ctkeyV &inSk, const ctkeyV &inPk, const keyV &destinations, const std::vector<ryo_amount> &inamounts, const std::vector<ryo_amount> &outamounts, const keyV &amount_keys, const std::vector<multisig_kLRki> *kLRki, multisig_out *msout, ryo_amount txnFee, unsigned int mixin, hw::device &hwdev);
rctSig genRctSimple(const key &message, const ctkeyV &inSk, const keyV &destinations, const std::vector<ryo_amount> &inamounts, const std::vector<ryo_amount> &outamounts, ryo_amount txnFee, const ctkeyM &mixRing, const keyV &amount_keys, const std::vector<multisig_kLRki> *kLRki, multisig_out *msout, const std::vector<unsigned int> &index, ctkeyV &outSk, bool bulletproof, hw::device &hwdev);
bool verRct(const rctSig &rv, bool semantics);
bool verRctSemanticsSimple(const rctSig &rv);
bool verRctSemanticsSimple(const std::vector<const rctSig *> &rv);
bool verRctNonSemanticsSimple(const rctSig &rv);
ryo_amount decodeRct(const rctSig &rv, const key &sk, unsigned int i, key &mask, hw::device &hwdev);
ryo_amount decodeRct(const rctSig &rv, const key &sk, unsigned int i, hw::device &hwdev);
ryo_amount decodeRctSimple(const rctSig &rv, const key &sk, unsigned int i, key &mask, hw::device &hwdev);
ryo_amount decodeRctSimple(const rctSig &rv, const key &sk, unsigned int i, hw::device &hwdev);

bool signMultisig(rctSig &rv, const std::vector<unsigned int> &indices, const keyV &k, const multisig_out &msout, const key &secret_key);

inline bool is_canonical_bulletproof_layout(const std::vector<rct::Bulletproof> &proofs)
{
	if(proofs.size() != 1)
		return false;
	const size_t sz = proofs[0].V.size();
	if(sz == 0 || sz > cryptonote::common_config::BULLETPROOF_MAX_OUTPUTS)
		return false;
	return true;
}

} // namespace rct
#endif /* RCTSIGS_H */
