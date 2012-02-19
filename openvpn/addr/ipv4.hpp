#ifndef OPENVPN_ADDR_IPV4_H
#define OPENVPN_ADDR_IPV4_H

#include <cstring> // for std::memcpy

#include <boost/cstdint.hpp> // for boost::uint32_t
#include <boost/asio.hpp>

#include <openvpn/common/exception.hpp>
#include <openvpn/common/ostream.hpp>

namespace openvpn {
  namespace IP {
    class Addr;
  }

  namespace IPv4 {

    OPENVPN_SIMPLE_EXCEPTION(ipv4_render_exception);
    OPENVPN_SIMPLE_EXCEPTION(ipv4_malformed_netmask);
    OPENVPN_SIMPLE_EXCEPTION(ipv4_bad_prefix_len);
    OPENVPN_EXCEPTION(ipv4_parse_exception);

    class Addr // NOTE: must be union-legal, so default constructor does not initialize
    {
      friend class IP::Addr;

    public:
      typedef boost::uint32_t base_type;

      static Addr from_uint32(const base_type addr)
      {
	Addr ret;
	ret.u.addr = addr;
	return ret;
      }

      static Addr from_bytes(const unsigned char *bytes)
      {
	Addr ret;
	std::memcpy(ret.u.bytes, bytes, 4);
	return ret;
      }

      static Addr from_zero()
      {
	Addr ret;
	ret.zero();
	return ret;
      }

      static Addr from_zero_complement()
      {
	Addr ret;
	ret.zero();
	ret.negate();
	return ret;
      }

      // build a netmask using given prefix_len
      static Addr netmask_from_prefix_len(const unsigned int prefix_len)
      {
	Addr ret;
	ret.u.addr = prefix_len_to_netmask(prefix_len);
	return ret;
      }

      static Addr from_string(const std::string& ipstr, const char *title = NULL)
      {
	boost::system::error_code ec;
	boost::asio::ip::address_v4 a = boost::asio::ip::address_v4::from_string(ipstr, ec);
	if (ec)
	  {
	    if (!title)
	      title = "";
	    OPENVPN_THROW(ipv4_parse_exception, "error parsing " << title << " IPv4 address '" << ipstr << "' : " << ec.message());
	  }
	return from_asio(a);
      }

      std::string to_string() const
      {
	const boost::asio::ip::address_v4 a = to_asio();
	boost::system::error_code ec;
	std::string ret = a.to_string(ec);
	if (ec)
	  throw ipv4_render_exception();
	return ret;
      }

      static Addr from_asio(const boost::asio::ip::address_v4& asio_addr)
      {
	Addr ret;
	ret.u.addr = asio_addr.to_ulong();
	return ret;
      }

      boost::asio::ip::address_v4 to_asio() const
      {
	return boost::asio::ip::address_v4(u.addr);
      }

      Addr operator&(const Addr& other) const {
	Addr ret;
	ret.u.addr = u.addr & other.u.addr;
	return ret;
      }

      Addr operator|(const Addr& other) const {
	Addr ret;
	ret.u.addr = u.addr | other.u.addr;
	return ret;
      }

      bool operator==(const Addr& other)
      {
	return u.addr == other.u.addr;
      }

      bool unspecified() const
      {
	return u.addr == 0;
      }

      // convert netmask in addr to prefix_len using binary search,
      // throws ipv4_malformed_netmask if addr is not a netmask
      unsigned int prefix_len() const
      {
	if (u.addr != ~0)
	  {
	    unsigned int high = 32;
	    unsigned int low = 1;
	    for (unsigned int i = 0; i < 5; ++i)
	      {
		const unsigned int mid = (high + low) / 2;
		const IPv4::Addr::base_type test = prefix_len_to_netmask_unchecked(mid);
		if (u.addr == test)
		  return mid;
		else if (u.addr > test)
		  low = mid;
		else
		  high = mid;
	      }
	    throw ipv4_malformed_netmask();
	  }
	else
	  return 32;
      }

      void negate()
      {
	u.addr = ~u.addr;
      }

      void zero()
      {
	u.addr = 0;
      }

    private:
      static base_type prefix_len_to_netmask_unchecked(const unsigned int prefix_len)
      {
	return ~((1 << (32 - prefix_len)) - 1);
      }

      static base_type prefix_len_to_netmask(const unsigned int prefix_len)
      {
	if (prefix_len >= 1 && prefix_len <= 32)
	  return prefix_len_to_netmask_unchecked(prefix_len);
	else
	  throw ipv4_bad_prefix_len();
      }

      union {
	base_type addr;
	unsigned char bytes[4];
      } u;
    };

    OPENVPN_OSTREAM(Addr, to_string)
  }
} // namespace openvpn

#endif // OPENVPN_ADDR_IPV4_H
