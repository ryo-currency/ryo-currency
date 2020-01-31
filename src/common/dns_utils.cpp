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

#include "common/dns_utils.h"
// check local first (in the event of static or in-source compilation of libunbound)
#include "unbound.h"

#include "include_base_utils.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <random>
#include <stdlib.h>
namespace bf = boost::filesystem;

#include "common/gulps.hpp"

GULPS_CAT_MAJOR("dns_utils");

static const char *DEFAULT_DNS_PUBLIC_ADDR[] =
	{
		"194.150.168.168", // CCC (Germany)
		"80.67.169.40", // FDN (France)
		"89.233.43.71", // http://censurfridns.dk (Denmark)
		"109.69.8.51", // punCAT (Spain)
		"77.109.148.137", // Xiala.net (Switzerland)
		"193.58.251.251", // SkyDNS (Russia)
};

static boost::mutex instance_lock;

namespace
{

/*
 * The following two functions were taken from unbound-anchor.c, from
 * the unbound library packaged with this source.  The license and source
 * can be found in $PROJECT_ROOT/external/unbound
 */

/* Cert builtin commented out until it's used, as the compiler complains

// return the built in root update certificate
static const char*
get_builtin_cert(void)
{
	return
// The ICANN CA fetched at 24 Sep 2010.  Valid to 2028
"-----BEGIN CERTIFICATE-----"
"MIIDdzCCAl+gAwIBAgIBATANBgkqhkiG9w0BAQsFADBdMQ4wDAYDVQQKEwVJQ0FO"
"TjEmMCQGA1UECxMdSUNBTk4gQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkxFjAUBgNV"
"BAMTDUlDQU5OIFJvb3QgQ0ExCzAJBgNVBAYTAlVTMB4XDTA5MTIyMzA0MTkxMloX"
"DTI5MTIxODA0MTkxMlowXTEOMAwGA1UEChMFSUNBTk4xJjAkBgNVBAsTHUlDQU5O"
"IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MRYwFAYDVQQDEw1JQ0FOTiBSb290IENB"
"MQswCQYDVQQGEwJVUzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAKDb"
"cLhPNNqc1NB+u+oVvOnJESofYS9qub0/PXagmgr37pNublVThIzyLPGCJ8gPms9S"
"G1TaKNIsMI7d+5IgMy3WyPEOECGIcfqEIktdR1YWfJufXcMReZwU4v/AdKzdOdfg"
"ONiwc6r70duEr1IiqPbVm5T05l1e6D+HkAvHGnf1LtOPGs4CHQdpIUcy2kauAEy2"
"paKcOcHASvbTHK7TbbvHGPB+7faAztABLoneErruEcumetcNfPMIjXKdv1V1E3C7"
"MSJKy+jAqqQJqjZoQGB0necZgUMiUv7JK1IPQRM2CXJllcyJrm9WFxY0c1KjBO29"
"iIKK69fcglKcBuFShUECAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8B"
"Af8EBAMCAf4wHQYDVR0OBBYEFLpS6UmDJIZSL8eZzfyNa2kITcBQMA0GCSqGSIb3"
"DQEBCwUAA4IBAQAP8emCogqHny2UYFqywEuhLys7R9UKmYY4suzGO4nkbgfPFMfH"
"6M+Zj6owwxlwueZt1j/IaCayoKU3QsrYYoDRolpILh+FPwx7wseUEV8ZKpWsoDoD"
"2JFbLg2cfB8u/OlE4RYmcxxFSmXBg0yQ8/IoQt/bxOcEEhhiQ168H2yE5rxJMt9h"
"15nu5JBSewrCkYqYYmaxyOC3WrVGfHZxVI7MpIFcGdvSb2a1uyuua8l0BKgk3ujF"
"0/wsHNeP22qNyVO+XVBzrM8fk8BSUFuiT/6tZTYXRtEt5aKQZgXbKU5dUF3jT9qg"
"j/Br5BZw3X/zd325TvnswzMC1+ljLzHnQGGk"
"-----END CERTIFICATE-----"
		;
}
*/

/** return the built in root DS trust anchor */
static const char *const *
get_builtin_ds(void)
{
	static const char *const ds[] =
		{
			". IN DS 19036 8 2 49AAC11D7B6F6446702E54A1607371607A1A41855200FD2CE1CDDE32F24E8FB5\n",
			". IN DS 20326 8 2 E06D44B80B8F1D39A95C0B0D7C65D08458E880409BBC683457104237C7F8EC8D\n",
			nullptr // mark the last entry
		};
	return ds;
}

/************************************************************
 ************************************************************
 ***********************************************************/

} // anonymous namespace

namespace tools
{

// fuck it, I'm tired of dealing with getnameinfo()/inet_ntop/etc
std::string ipv4_to_string(const char *src, size_t len)
{
	assert(len >= 4);

	std::stringstream ss;
	unsigned int bytes[4];
	for(int i = 0; i < 4; i++)
	{
		unsigned char a = src[i];
		bytes[i] = a;
	}
	ss << bytes[0] << "."
	   << bytes[1] << "."
	   << bytes[2] << "."
	   << bytes[3];
	return ss.str();
}

// this obviously will need to change, but is here to reflect the above
// stop-gap measure and to make the tests pass at least...
std::string ipv6_to_string(const char *src, size_t len)
{
	assert(len >= 8);

	std::stringstream ss;
	unsigned int bytes[8];
	for(int i = 0; i < 8; i++)
	{
		unsigned char a = src[i];
		bytes[i] = a;
	}
	ss << bytes[0] << ":"
	   << bytes[1] << ":"
	   << bytes[2] << ":"
	   << bytes[3] << ":"
	   << bytes[4] << ":"
	   << bytes[5] << ":"
	   << bytes[6] << ":"
	   << bytes[7];
	return ss.str();
}

std::string txt_to_string(const char *src, size_t len)
{
	return std::string(src + 1, len - 1);
}

// custom smart pointer.
// TODO: see if std::auto_ptr and the like support custom destructors
template <typename type, void (*freefunc)(type *)>
class scoped_ptr
{
  public:
	scoped_ptr() :
		ptr(nullptr)
	{
	}
	scoped_ptr(type *p) :
		ptr(p)
	{
	}
	~scoped_ptr()
	{
		freefunc(ptr);
	}
	operator type *() { return ptr; }
	type **operator&() { return &ptr; }
	type *operator->() { return ptr; }
	operator const type *() const { return &ptr; }

  private:
	type *ptr;
};

typedef class scoped_ptr<ub_result, ub_resolve_free> ub_result_ptr;

struct DNSResolverData
{
	ub_ctx *m_ub_context;
};

// work around for bug https://www.nlnetlabs.nl/bugs-script/show_bug.cgi?id=515 needed for it to compile on e.g. Debian 7
class string_copy
{
  public:
	string_copy(const char *s) :
		str(strdup(s)) {}
	~string_copy() { free(str); }
	operator char *() { return str; }

  public:
	char *str;
};

DNSResolver::DNSResolver() :
	m_data(new DNSResolverData())
{
	int use_dns_public = 0;
	std::vector<std::string> dns_public_addr;
	if(auto res = getenv("DNS_PUBLIC"))
	{
		dns_public_addr = tools::dns_utils::parse_dns_public(res);
		if(!dns_public_addr.empty())
		{
			GULPSF_INFO("Using public DNS server(s): {} (TCP)", boost::join(dns_public_addr, ", "));
			use_dns_public = 1;
		}
		else
		{
			GULPS_ERROR("Failed to parse DNS_PUBLIC");
		}
	}

	// init libunbound context
	m_data->m_ub_context = ub_ctx_create();

	if(use_dns_public)
	{
		for(const auto &ip : dns_public_addr)
			ub_ctx_set_fwd(m_data->m_ub_context, string_copy(ip.c_str()));
		ub_ctx_set_option(m_data->m_ub_context, string_copy("do-udp:"), string_copy("no"));
		ub_ctx_set_option(m_data->m_ub_context, string_copy("do-tcp:"), string_copy("yes"));
	}
	else
	{
		// look for "/etc/resolv.conf" and "/etc/hosts" or platform equivalent
		ub_ctx_resolvconf(m_data->m_ub_context, NULL);
		ub_ctx_hosts(m_data->m_ub_context, NULL);
	}

	const char *const *ds = ::get_builtin_ds();
	while(*ds)
	{
		GULPSF_INFO("adding trust anchor: {}", *ds);
		ub_ctx_add_ta(m_data->m_ub_context, string_copy(*ds++));
	}
}

DNSResolver::~DNSResolver()
{
	if(m_data)
	{
		if(m_data->m_ub_context != NULL)
		{
			ub_ctx_delete(m_data->m_ub_context);
		}
		delete m_data;
	}
}

std::vector<std::string> DNSResolver::get_record(const std::string &url, int record_type, std::string (*reader)(const char *, size_t), bool &dnssec_available, bool &dnssec_valid)
{
	std::vector<std::string> addresses;
	dnssec_available = false;
	dnssec_valid = false;

	if(!check_address_syntax(url.c_str()))
	{
		return addresses;
	}

	// destructor takes care of cleanup
	ub_result_ptr result;

	// call DNS resolver, blocking.  if return value not zero, something went wrong
	if(!ub_resolve(m_data->m_ub_context, string_copy(url.c_str()), record_type, DNS_CLASS_IN, &result))
	{
		dnssec_available = (result->secure || (!result->secure && result->bogus));
		dnssec_valid = result->secure && !result->bogus;
		if(result->havedata)
		{
			for(size_t i = 0; result->data[i] != NULL; i++)
			{
				addresses.push_back((*reader)(result->data[i], result->len[i]));
			}
		}
	}

	return addresses;
}

std::vector<std::string> DNSResolver::get_ipv4(const std::string &url, bool &dnssec_available, bool &dnssec_valid)
{
	return get_record(url, DNS_TYPE_A, ipv4_to_string, dnssec_available, dnssec_valid);
}

std::vector<std::string> DNSResolver::get_ipv6(const std::string &url, bool &dnssec_available, bool &dnssec_valid)
{
	return get_record(url, DNS_TYPE_AAAA, ipv6_to_string, dnssec_available, dnssec_valid);
}

std::vector<std::string> DNSResolver::get_txt_record(const std::string &url, bool &dnssec_available, bool &dnssec_valid)
{
	return get_record(url, DNS_TYPE_TXT, txt_to_string, dnssec_available, dnssec_valid);
}

std::string DNSResolver::get_dns_format_from_oa_address(const std::string &oa_addr)
{
	std::string addr(oa_addr);
	auto first_at = addr.find("@");
	if(first_at == std::string::npos)
		return addr;

	// convert name@domain.tld to name.domain.tld
	addr.replace(first_at, 1, ".");

	return addr;
}

DNSResolver &DNSResolver::instance()
{
	boost::lock_guard<boost::mutex> lock(instance_lock);

	static DNSResolver staticInstance;
	return staticInstance;
}

DNSResolver DNSResolver::create()
{
	return DNSResolver();
}

bool DNSResolver::check_address_syntax(const char *addr) const
{
	// if string doesn't contain a dot, we won't consider it a url for now.
	if(strchr(addr, '.') == NULL)
	{
		return false;
	}
	return true;
}

namespace dns_utils
{

namespace
{
bool dns_records_match(const std::vector<std::string> &a, const std::vector<std::string> &b)
{
	if(a.size() != b.size())
		return false;

	for(const auto &record_in_a : a)
	{
		bool ok = false;
		for(const auto &record_in_b : b)
		{
			if(record_in_a == record_in_b)
			{
				ok = true;
				break;
			}
		}
		if(!ok)
			return false;
	}

	return true;
}
} // namespace

bool load_txt_records_from_dns(std::vector<std::string> &good_records, const std::vector<std::string> &dns_urls)
{
	// Prevent infinite recursion when distributing
	if(dns_urls.empty())
		return false;

	std::vector<std::vector<std::string>> records;
	records.resize(dns_urls.size());

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dis(0, dns_urls.size() - 1);
	size_t first_index = dis(gen);

	// send all requests in parallel
	std::vector<boost::thread> threads(dns_urls.size());
	std::deque<bool> avail(dns_urls.size(), false), valid(dns_urls.size(), false);
	for(size_t n = 0; n < dns_urls.size(); ++n)
	{
		threads[n] = boost::thread([n, dns_urls, &records, &avail, &valid]() {
			records[n] = tools::DNSResolver::instance().get_txt_record(dns_urls[n], avail[n], valid[n]);
		});
	}
	for(size_t n = 0; n < dns_urls.size(); ++n)
		threads[n].join();

	size_t cur_index = first_index;
	do
	{
		const std::string &url = dns_urls[cur_index];
		if(!avail[cur_index])
		{
			records[cur_index].clear();
			GULPSF_LOG_L2("DNSSEC not available for checkpoint update at URL: {}, skipping.", url);
		}
		if(!valid[cur_index])
		{
			records[cur_index].clear();
			GULPSF_LOG_L2("DNSSEC validation failed for checkpoint update at URL: {}, skipping.", url);
		}

		cur_index++;
		if(cur_index == dns_urls.size())
		{
			cur_index = 0;
		}
	} while(cur_index != first_index);

	size_t num_valid_records = 0;

	for(const auto &record_set : records)
	{
		if(record_set.size() != 0)
		{
			num_valid_records++;
		}
	}

	if(num_valid_records < 2)
	{
		//GULPS_LOG_L0("WARNING: no two valid MoneroPulse DNS checkpoint records were received");
		return false;
	}

	int good_records_index = -1;
	for(size_t i = 0; i < records.size() - 1; ++i)
	{
		if(records[i].size() == 0)
			continue;

		for(size_t j = i + 1; j < records.size(); ++j)
		{
			if(dns_records_match(records[i], records[j]))
			{
				good_records_index = i;
				break;
			}
		}
		if(good_records_index >= 0)
			break;
	}

	if(good_records_index < 0)
	{
		//GULPS_LOG_L0("WARNING: no two MoneroPulse DNS checkpoint records matched");
		return false;
	}

	good_records = records[good_records_index];
	return true;
}

std::vector<std::string> parse_dns_public(const char *s)
{
	unsigned ip0, ip1, ip2, ip3;
	char c;
	std::vector<std::string> dns_public_addr;
	if(!strcmp(s, "tcp"))
	{
		for(size_t i = 0; i < sizeof(DEFAULT_DNS_PUBLIC_ADDR) / sizeof(DEFAULT_DNS_PUBLIC_ADDR[0]); ++i)
			dns_public_addr.push_back(DEFAULT_DNS_PUBLIC_ADDR[i]);
		GULPSF_LOG_L0("Using default public DNS server(s):{}  (TCP)", boost::join(dns_public_addr, ", "));
	}
	else if(sscanf(s, "tcp://%u.%u.%u.%u%c", &ip0, &ip1, &ip2, &ip3, &c) == 4)
	{
		if(ip0 > 255 || ip1 > 255 || ip2 > 255 || ip3 > 255)
		{
			GULPSF_ERROR("Invalid IP: {}, using default", s);
		}
		else
		{
			dns_public_addr.push_back(std::string(s + strlen("tcp://")));
		}
	}
	else
	{
		GULPS_ERROR("Invalid DNS_PUBLIC contents, ignored");
	}
	return dns_public_addr;
}

} // namespace dns_utils

} // namespace tools
