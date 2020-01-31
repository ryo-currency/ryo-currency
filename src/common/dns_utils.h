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
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace tools
{

// RFC defines for record types and classes for DNS, gleaned from ldns source
const static int DNS_CLASS_IN = 1;
const static int DNS_TYPE_A = 1;
const static int DNS_TYPE_TXT = 16;
const static int DNS_TYPE_AAAA = 8;

struct DNSResolverData;

/**
 * @brief Provides high-level access to DNS resolution
 *
 * This class is designed to provide a high-level abstraction to DNS resolution
 * functionality, including access to TXT records and such.  It will also
 * handle DNSSEC validation of the results.
 */
class DNSResolver
{
  private:
	/**
   * @brief Constructs an instance of DNSResolver
   *
   * Constructs a class instance and does setup stuff for the backend resolver.
   */
	DNSResolver();

  public:
	/**
   * @brief takes care of freeing C pointers and such
   */
	~DNSResolver();

	/**
   * @brief gets ipv4 addresses from DNS query of a URL
   *
   * returns a vector of all IPv4 "A" records for given URL.
   * If no "A" records found, returns an empty vector.
   *
   * @param url A string containing a URL to query for
   *
   * @param dnssec_available 
   *
   * @return vector of strings containing ipv4 addresses
   */
	std::vector<std::string> get_ipv4(const std::string &url, bool &dnssec_available, bool &dnssec_valid);

	/**
   * @brief gets ipv6 addresses from DNS query
   *
   * returns a vector of all IPv6 "A" records for given URL.
   * If no "A" records found, returns an empty vector.
   *
   * @param url A string containing a URL to query for
   *
   * @return vector of strings containing ipv6 addresses
   */
	std::vector<std::string> get_ipv6(const std::string &url, bool &dnssec_available, bool &dnssec_valid);

	/**
   * @brief gets all TXT records from a DNS query for the supplied URL;
   * if no TXT record present returns an empty vector.
   *
   * @param url A string containing a URL to query for
   *
   * @return A vector of strings containing a TXT record; or an empty vector
   */
	// TODO: modify this to accommodate DNSSEC
	std::vector<std::string> get_txt_record(const std::string &url, bool &dnssec_available, bool &dnssec_valid);

	/**
   * @brief Gets a DNS address from OpenAlias format
   *
   * If the address looks good, but contains one @ symbol, replace that with a .
   * e.g. donate@getmonero.org becomes donate.getmonero.org
   *
   * @param oa_addr  OpenAlias address
   *
   * @return dns_addr  DNS address
   */
	std::string get_dns_format_from_oa_address(const std::string &oa_addr);

	/**
   * @brief Gets the singleton instance of DNSResolver
   *
   * @return returns a pointer to the singleton
   */
	static DNSResolver &instance();

	/**
   * @brief Gets a new instance of DNSResolver
   *
   * @return returns a pointer to the new object
   */
	static DNSResolver create();

  private:
	/**
   * @brief gets all records of a given type from a DNS query for the supplied URL;
   * if no such record is present returns an empty vector.
   *
   * @param url A string containing a URL to query for
   * @param record_type the record type to retrieve (DNS_TYPE_A, etc)
   * @param reader a function that converts a record data to a string
   *
   * @return A vector of strings containing the requested record; or an empty vector
   */
	// TODO: modify this to accommodate DNSSEC
	std::vector<std::string> get_record(const std::string &url, int record_type, std::string (*reader)(const char *, size_t), bool &dnssec_available, bool &dnssec_valid);

	/**
   * @brief Checks a string to see if it looks like a URL
   *
   * @param addr the string to be checked
   *
   * @return true if it looks enough like a URL, false if not
   */
	bool check_address_syntax(const char *addr) const;

	DNSResolverData *m_data;
}; // class DNSResolver

namespace dns_utils
{

bool load_txt_records_from_dns(std::vector<std::string> &records, const std::vector<std::string> &dns_urls);

std::vector<std::string> parse_dns_public(const char *s);

} // namespace dns_utils

} // namespace tools
