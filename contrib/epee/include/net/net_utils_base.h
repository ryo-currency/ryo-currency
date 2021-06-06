// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef _NET_UTILS_BASE_H_
#define _NET_UTILS_BASE_H_

#include "serialization/keyvalue_serialization.h"
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <type_traits>
#include <typeinfo>

#include "common/gulps.hpp"

#ifndef MAKE_IP
#define MAKE_IP(a1, a2, a3, a4) (a1 | (a2 << 8) | (a3 << 16) | (a4 << 24))
#endif

namespace epee
{
namespace net_utils
{

GULPS_CAT_MAJOR("epee_net");

class ipv4_network_address
{
	uint32_t m_ip;
	uint16_t m_port;

  public:
	constexpr ipv4_network_address(uint32_t ip, uint16_t port) noexcept
		: m_ip(ip),
		  m_port(port) {}

	bool equal(const ipv4_network_address &other) const noexcept;
	bool less(const ipv4_network_address &other) const noexcept;
	constexpr bool is_same_host(const ipv4_network_address &other) const noexcept
	{
		return ip() == other.ip();
	}

	constexpr uint32_t ip() const noexcept { return m_ip; }
	constexpr uint16_t port() const noexcept { return m_port; }
	std::string str() const;
	std::string host_str() const;
	bool is_loopback() const;
	bool is_local() const;
	static constexpr uint8_t get_type_id() noexcept { return ID; }

	static const uint8_t ID = 1;
	BEGIN_KV_SERIALIZE_MAP(ipv4_network_address)
	KV_SERIALIZE(m_ip)
	KV_SERIALIZE(m_port)
	END_KV_SERIALIZE_MAP()
};

inline bool operator==(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return lhs.equal(rhs);
}
inline bool operator!=(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return !lhs.equal(rhs);
}
inline bool operator<(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return lhs.less(rhs);
}
inline bool operator<=(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return !rhs.less(lhs);
}
inline bool operator>(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return rhs.less(lhs);
}
inline bool operator>=(const ipv4_network_address &lhs, const ipv4_network_address &rhs) noexcept
{
	return !lhs.less(rhs);
}

class network_address
{
	struct interface
	{
		virtual ~interface(){};

		virtual bool equal(const interface &) const = 0;
		virtual bool less(const interface &) const = 0;
		virtual bool is_same_host(const interface &) const = 0;

		virtual std::string str() const = 0;
		virtual std::string host_str() const = 0;
		virtual bool is_loopback() const = 0;
		virtual bool is_local() const = 0;
		virtual uint8_t get_type_id() const = 0;
	};

	template <typename T>
	struct implementation final : interface
	{
		T value;

		implementation(const T &src) : value(src) {}
		~implementation() = default;

		// Type-checks for cast are done in cpp
		static const T &cast(const interface &src) noexcept
		{
			return static_cast<const implementation<T> &>(src).value;
		}

		virtual bool equal(const interface &other) const override
		{
			return value.equal(cast(other));
		}

		virtual bool less(const interface &other) const override
		{
			return value.less(cast(other));
		}

		virtual bool is_same_host(const interface &other) const override
		{
			return value.is_same_host(cast(other));
		}

		virtual std::string str() const override { return value.str(); }
		virtual std::string host_str() const override { return value.host_str(); }
		virtual bool is_loopback() const override { return value.is_loopback(); }
		virtual bool is_local() const override { return value.is_local(); }
		virtual uint8_t get_type_id() const override { return value.get_type_id(); }
	};

	std::shared_ptr<interface> self = nullptr;

	template <typename Type>
	Type &as_mutable() const
	{
		// types `implmentation<Type>` and `implementation<const Type>` are unique
		using Type_ = typename std::remove_const<Type>::type;
		network_address::interface *const self_ = self.get(); // avoid clang warning in typeid
		if(!self_ || typeid(implementation<Type_>) != typeid(*self_))
			throw std::bad_cast{};
		return static_cast<implementation<Type_> *>(self_)->value;
	}

  public:
	template <typename T>
	network_address(const T &src)
		: self(std::make_shared<implementation<T>>(src)) {}
	bool equal(const network_address &other) const;
	bool less(const network_address &other) const;
	bool is_same_host(const network_address &other) const;
	std::string str() const { return self ? self->str() : "<none>"; }
	std::string host_str() const { return self ? self->host_str() : "<none>"; }
	bool is_loopback() const { return self ? self->is_loopback() : false; }
	bool is_local() const { return self ? self->is_local() : false; }
	uint8_t get_type_id() const { return self ? self->get_type_id() : 0; }
	template <typename Type>
	const Type &as() const { return as_mutable<const Type>(); }

	BEGIN_KV_SERIALIZE_MAP(network_address)
	uint8_t type = is_store ? this_ref.get_type_id() : 0;
	if(!epee::serialization::selector<is_store>::serialize(type, stg, hparent_section, "type"))
		return false;
	switch(type)
	{
	case ipv4_network_address::ID:
	{
		if(!is_store)
		{
			const_cast<network_address &>(this_ref) = ipv4_network_address{0, 0};
			auto &addr = this_ref.template as_mutable<ipv4_network_address>();
			if(epee::serialization::selector<is_store>::serialize(addr, stg, hparent_section, "addr"))
				GULPSF_LOG_L1("Found as addr: {}", this_ref.str());
			else if(epee::serialization::selector<is_store>::serialize(addr, stg, hparent_section, "template as<ipv4_network_address>()"))
				GULPSF_LOG_L1("Found as template as<ipv4_network_address>(): {}", this_ref.str());
			else if(epee::serialization::selector<is_store>::serialize(addr, stg, hparent_section, "template as_mutable<ipv4_network_address>()"))
				GULPSF_LOG_L1("Found as template as_mutable<ipv4_network_address>(): {}", this_ref.str());
			else
			{
				GULPS_WARN("Address not found");
				return false;
			}
		}
		else
		{
			auto &addr = this_ref.template as_mutable<ipv4_network_address>();
			if(!epee::serialization::selector<is_store>::serialize(addr, stg, hparent_section, "addr"))
				return false;
		}
		break;
	}
	default:
		GULPS_ERROR("Unsupported network address type: ", (unsigned)type);
		return false;
	}
	END_KV_SERIALIZE_MAP()
};

inline bool operator==(const network_address &lhs, const network_address &rhs)
{
	return lhs.equal(rhs);
}
inline bool operator!=(const network_address &lhs, const network_address &rhs)
{
	return !lhs.equal(rhs);
}
inline bool operator<(const network_address &lhs, const network_address &rhs)
{
	return lhs.less(rhs);
}
inline bool operator<=(const network_address &lhs, const network_address &rhs)
{
	return !rhs.less(lhs);
}
inline bool operator>(const network_address &lhs, const network_address &rhs)
{
	return rhs.less(lhs);
}
inline bool operator>=(const network_address &lhs, const network_address &rhs)
{
	return !lhs.less(rhs);
}

bool create_network_address(network_address &address, const std::string &string, uint16_t default_port = 0);

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct connection_context_base
{
	const boost::uuids::uuid m_connection_id;
	const network_address m_remote_address;
	const bool m_is_income;
	const time_t m_started;
	time_t m_last_recv;
	time_t m_last_send;
	uint64_t m_recv_cnt;
	uint64_t m_send_cnt;
	double m_current_speed_down;
	double m_current_speed_up;

	connection_context_base(boost::uuids::uuid connection_id,
							const network_address &remote_address, bool is_income,
							time_t last_recv = 0, time_t last_send = 0,
							uint64_t recv_cnt = 0, uint64_t send_cnt = 0) : m_connection_id(connection_id),
																			m_remote_address(remote_address),
																			m_is_income(is_income),
																			m_started(time(NULL)),
																			m_last_recv(last_recv),
																			m_last_send(last_send),
																			m_recv_cnt(recv_cnt),
																			m_send_cnt(send_cnt),
																			m_current_speed_down(0),
																			m_current_speed_up(0)
	{
	}

	connection_context_base() : m_connection_id(),
								m_remote_address(ipv4_network_address{0, 0}),
								m_is_income(false),
								m_started(time(NULL)),
								m_last_recv(0),
								m_last_send(0),
								m_recv_cnt(0),
								m_send_cnt(0),
								m_current_speed_down(0),
								m_current_speed_up(0)
	{
	}

	connection_context_base(const connection_context_base& a) : connection_context_base()
	{
		set_details(a.m_connection_id, a.m_remote_address, a.m_is_income);
	}

	connection_context_base &operator=(const connection_context_base &a)
	{
		set_details(a.m_connection_id, a.m_remote_address, a.m_is_income);
		return *this;
	}

  private:
	template <class t_protocol_handler>
	friend class connection;
	void set_details(boost::uuids::uuid connection_id, const network_address &remote_address, bool is_income)
	{
		this->~connection_context_base();
		new(this) connection_context_base(connection_id, remote_address, is_income);
	}
};

/************************************************************************/
/*                                                                      */
/************************************************************************/
struct i_service_endpoint
{
	virtual bool do_send(const void *ptr, size_t cb) = 0;
	virtual bool close() = 0;
	virtual bool call_run_once_service_io() = 0;
	virtual bool request_callback() = 0;
	virtual boost::asio::io_service &get_io_service() = 0;
	//protect from deletion connection object(with protocol instance) during external call "invoke"
	virtual bool add_ref() = 0;
	virtual bool release() = 0;

  protected:
	virtual ~i_service_endpoint() noexcept(false) {}
};

//some helpers

std::string print_connection_context(const connection_context_base &ctx);
std::string print_connection_context_short(const connection_context_base &ctx);

inline std::ostream &operator<<(std::ostream &os, const connection_context_base &ct)
{
	os << "[" << epee::net_utils::print_connection_context_short(ct).c_str() << "] ";
	return os;
}

}
}

#if BOOST_VERSION >= 107000
#define GET_IO_SERVICE(s) ((boost::asio::io_context&)(s).get_executor().context())
#else
#define GET_IO_SERVICE(s) ((s).get_io_service())
#endif
#endif //_NET_UTILS_BASE_H_
