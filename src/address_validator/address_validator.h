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

#pragma once

#include "address_validator/writer.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "cryptonote_config.h"

#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>

/*!
 * \namespace cryptonote
 * \brief Holds cryptonote related classes and helpers.
 */
namespace cryptonote
{
/*! validate ryo/sumokoin addresses
   *
   * The validator is backward compatible to sumokoin addresses.
   * A sumokoin address will be shown as RYO representation.
   */
class address_validator
{
  public:
	address_validator() = default;

	/*! evaluate all addresses
     *
     * validate given addresses and query attributes
     */
	void evaluate();

	/*! print json output
     *
     * if m_filename is non empty the output will be printed into the file else
     * to the terminal
     */
	void print();

	/*! create command line options */
	void init_options(
		boost::program_options::options_description &desc,
		boost::program_options::positional_options_description &pos_option);

	/*! validate parsed command line options
     *
     * @return 0 if all options are valid else !=0
     */
	bool validate_options();

  private:
	/*! address attributes */
	struct address_attributes
	{
		address_parse_info info;
		bool is_valid = false;
		std::string network;
		network_type nettype;
	};

	/*! evaluate an address
     *
     * @param net_type network type which is used to evaluate the address
     * @param add_str address as string
     * @param attr[out] attributes of an address
     * @param return 0 if address is valid else !=0
     */
	bool evaluate_address_attributes(const std::string &net_type, const std::string &addr_str, address_attributes &attr);

	/*! print json output of an address
     *
     * @param out instance of a write
     * @param addr_str input address as string
     * @param attr address attributes
     * @param separator separator character, e.g '\n`, ' '
     */
	void print(writer &out, const std::string &addr_str, const address_attributes &attr, const char separator);

	/*! vector of input addresses */
	std::vector<std::string> m_address_strs;

	/*! vector with address attributes */
	std::vector<address_attributes> m_address_attributes;

	/*! user selected network type
     *
     * valid values: auto, mainnet, testnet, stagenet
     */
	std::string m_network;

	/*! user defined output file */
	std::string m_filename;

	/*! human readable output
     *
     * if false the output will have no line breaks or spaces
     */
	bool m_human;
};
}
