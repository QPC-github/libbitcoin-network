/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../test.hpp"
#include <sstream>

BOOST_AUTO_TEST_SUITE(authority_tests)

using namespace network::config;
using namespace boost::program_options;

// tools.ietf.org/html/rfc4291#section-2.2
#define BC_AUTHORITY_IPV4_ADDRESS "1.2.240.1"
#define BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "::"
#define BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "2001:db8::2"
#define BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "::0102:f001"
#define BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "::1.2.240.1"
#define BC_AUTHORITY_IPV4_BOGUS_ADDRESS "0.0.0.57:256"
#define BC_AUTHORITY_IPV6_BOGUS_IPV4_ADDRESS "[::ffff:0:39]:256"

// tools.ietf.org/html/rfc4291#section-2.5.2
constexpr messages::ip_address test_unspecified_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// tools.ietf.org/html/rfc4291#section-2.5.3
constexpr messages::ip_address test_loopback_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

// tools.ietf.org/html/rfc4291#section-2.5.5.2
constexpr messages::ip_address test_mapped_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0x01, 0x02, 0xf0, 0x01
};

// tools.ietf.org/html/rfc4291#section-2.5.5.1
constexpr messages::ip_address test_compatible_ip_address =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0xf0, 0x01
};

constexpr messages::ip_address test_ipv6_address =
{
    0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
};

static bool ip_equal(const messages::ip_address& left,
    const messages::ip_address& right)
{
    return std::equal(left.begin(), left.end(), right.begin());
}

static bool net_equal(const messages::address_item& left,
    const messages::address_item& right)
{
    return
        (left.timestamp == right.timestamp) &&
        (left.services == right.services) &&
        ip_equal(left.ip, right.ip) &&
        (left.port == right.port);
}

// construct

BOOST_AUTO_TEST_CASE(authority__construct__bogus_ip__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(authority host("bogus"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(authority__construct__bogus_port__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(authority host("[::]:bogus"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(authority__construct__invalid_ipv4__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(authority host("999.999.999.999"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(authority__construct__invalid_ipv6__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(authority host("[:::]"), invalid_option_value);
}

BOOST_AUTO_TEST_CASE(authority__construct__invalid_port__throws_invalid_option)
{
    BOOST_REQUIRE_THROW(authority host("[::]:12345678901"), invalid_option_value);
}

// port

BOOST_AUTO_TEST_CASE(authority__port__default__zero)
{
    const authority host;
    BOOST_REQUIRE_EQUAL(host.port(), 0u);
}

BOOST_AUTO_TEST_CASE(authority__port__copy__expected)
{
    constexpr uint16_t expected_port = 42;
    const authority other(test_ipv6_address, expected_port);
    const authority host(other);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__ipv4_authority__expected)
{
    constexpr uint16_t expected_port = 42;
    std::stringstream address;
    address << BC_AUTHORITY_IPV4_ADDRESS ":" << expected_port;
    const authority host(address.str());
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__ipv6_authority__expected)
{
    constexpr uint16_t expected_port = 42;
    std::stringstream address;
    address << "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:" << expected_port;
    const authority host(address.str());
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__address_item__expected)
{
    constexpr uint16_t expected_port = 42;
    const messages::address_item address
    {
        0, 0, test_ipv6_address, expected_port
    };

    const authority host(address);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__ip_address__expected)
{
    constexpr uint16_t expected_port = 42;
    const authority host(test_ipv6_address, expected_port);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__hostname__expected)
{
    constexpr uint16_t expected_port = 42;
    const authority host(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS, expected_port);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__boost_address__expected)
{
    constexpr uint16_t expected_port = 42;
    const auto address = asio::address::from_string(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS);
    const authority host(address, expected_port);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

BOOST_AUTO_TEST_CASE(authority__port__boost_endpoint__expected)
{
    constexpr uint16_t expected_port = 42;
    const auto address = asio::address::from_string(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS);
    const asio::endpoint tcp_endpoint(address, expected_port);
    const authority host(tcp_endpoint);
    BOOST_REQUIRE_EQUAL(host.port(), expected_port);
}

// bool

BOOST_AUTO_TEST_CASE(authority__bool__default__false)
{
    const authority host{};
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(authority__bool__unspecified__false)
{
    const authority host{ messages::unspecified_ip_address, 42 };
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(authority__bool__unspecified_ip_port__false)
{
    const authority host{ test_ipv6_address, messages::unspecified_ip_port };
    BOOST_REQUIRE(!host);
}

BOOST_AUTO_TEST_CASE(authority__bool__loopback_nonzero_port__true)
{
    const authority host{ messages::loopback_ip_address, 42 };
    BOOST_REQUIRE(host);
}

// to_ip_address

BOOST_AUTO_TEST_CASE(authority__to_ip_address__default__unspecified)
{
    const authority host{};
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_unspecified_ip_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__copy__expected)
{
    const auto& expected_ip = test_ipv6_address;
    const authority other(expected_ip, 42);
    const authority host(other);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), expected_ip));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv4_authority__expected)
{
    const authority host(BC_AUTHORITY_IPV4_ADDRESS ":42");
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_mapped_ip_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv6_authority__expected)
{
    const authority host("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42");
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_ipv6_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv6_compatible_authority__expected)
{
    // BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS A|B variants are equivalent.
    const authority host("[" BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "]:42");
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_compatible_ip_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv6_compatible_alternative_authority__expected)
{
    // BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS A|B variants are equivalent.
    const authority host("[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]:42");
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_compatible_ip_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__address_item__expected)
{
    const auto& expected_ip = test_ipv6_address;
    const messages::address_item address
    {
        0, 0, test_ipv6_address, 42
    };

    const authority host(address);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), expected_ip));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ip_address__expected)
{
    const auto& expected_ip = test_ipv6_address;
    const authority host(expected_ip, 42);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), expected_ip));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv4_hostname__expected)
{
    const authority host(BC_AUTHORITY_IPV4_ADDRESS, 42);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_mapped_ip_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv6_host__expected)
{
    const authority host(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS, 42);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_ipv6_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__ipv6_hostname__expected)
{
    const authority host("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]", 42);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_ipv6_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__boost_address__expected)
{
    const auto address = asio::address::from_string(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS);
    const authority host(address, 42);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_ipv6_address));
}

BOOST_AUTO_TEST_CASE(authority__to_ip_address__boost_endpoint__expected)
{
    const auto address = asio::address::from_string(BC_AUTHORITY_IPV4_ADDRESS);
    asio::endpoint tcp_endpoint(address, 42);
    const authority host(tcp_endpoint);
    BOOST_REQUIRE(ip_equal(host.to_ip_address(), test_mapped_ip_address));
}

// to_host

BOOST_AUTO_TEST_CASE(authority__to_host__default__ipv6_unspecified)
{
    const authority host;
    BOOST_REQUIRE_EQUAL(host.to_host(), "[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]");
}

BOOST_AUTO_TEST_CASE(authority__to_host__ipv4_mapped_ip_address__ipv4)
{
    // A mapped ip address serializes as IPv4.
    const authority host(test_mapped_ip_address, 0);
    BOOST_REQUIRE_EQUAL(host.to_host(), BC_AUTHORITY_IPV4_ADDRESS);
}

BOOST_AUTO_TEST_CASE(authority__to_host__ipv4_compatible_ip_address__ipv6_alternative)
{
    // A compatible ip address serializes as alternative notation IPv6.
    const authority host(test_compatible_ip_address, 0);
    BOOST_REQUIRE_EQUAL(host.to_host(), "[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]");
}

BOOST_AUTO_TEST_CASE(authority__to_host__ipv6_address__ipv6_compressed)
{
    // An ipv6 address serializes using compression.
    const authority host(test_ipv6_address, 0);
    BOOST_REQUIRE_EQUAL(host.to_host(), "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
}

// to_address_item

BOOST_AUTO_TEST_CASE(authority__to_address_item1__default__ipv6_unspecified)
{
    const messages::address_item expected
    {
        0, 0, test_unspecified_ip_address, 0,
    };

    const authority host;
    BOOST_REQUIRE(net_equal(host.to_address_item(), expected));
}

BOOST_AUTO_TEST_CASE(authority__to_address_item1__ipv4_mapped_ip_address__ipv4)
{
    const messages::address_item expected
    {
        0, 0, test_mapped_ip_address, 42,
    };

    const authority host(expected.ip, expected.port);
    BOOST_REQUIRE(net_equal(host.to_address_item(), expected));
}

BOOST_AUTO_TEST_CASE(authority__to_address_item1__ipv4_compatible_ip_address__ipv6_alternative)
{
    const messages::address_item expected
    {
        0, 0, test_compatible_ip_address, 42,
    };

    const authority host(expected.ip, expected.port);
    BOOST_REQUIRE(net_equal(host.to_address_item(), expected));
}

BOOST_AUTO_TEST_CASE(authority__to_address_item1__ipv6_address__ipv6_compressed)
{
    const messages::address_item expected
    {
        0, 0, test_ipv6_address, 42,
    };

    const authority host(expected.ip, expected.port);
    BOOST_REQUIRE(net_equal(host.to_address_item(), expected));
}

BOOST_AUTO_TEST_CASE(authority__to_address_item2__parameters__expected)
{
    const messages::address_item expected
    {
        42, 24, test_ipv6_address, 42,
    };

    const authority host(expected.ip, expected.port);
    BOOST_REQUIRE(net_equal(host.to_address_item(expected.timestamp, expected.services), expected));
}

// to_string

BOOST_AUTO_TEST_CASE(authority__to_string__default__unspecified)
{
    const authority host;
    BOOST_REQUIRE_EQUAL(host.to_string(), "[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]");
}

BOOST_AUTO_TEST_CASE(authority__to_string__unspecified__unspecified)
{
    const auto line = "[" BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS "]";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

// These results vary by Boost version, so these tests are disabled.
////BOOST_AUTO_TEST_CASE(authority__to_string__bogus_ipv4__ipv4)
////{
////    authority host(BC_AUTHORITY_IPV4_BOGUS_ADDRESS);
////    BOOST_REQUIRE_EQUAL(host.to_string(), BC_AUTHORITY_IPV4_BOGUS_ADDRESS);
////}
////BOOST_AUTO_TEST_CASE(authority__to_string__bogus_ipv4__ipv6_compatible)
////{
////    authority host(BC_AUTHORITY_IPV4_BOGUS_ADDRESS);
////    BOOST_REQUIRE_EQUAL(host.to_string(), BC_AUTHORITY_IPV6_BOGUS_IPV4_ADDRESS);
////}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv4__expected)
{
    const auto line = BC_AUTHORITY_IPV4_ADDRESS;
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv4_port__expected)
{
    const auto line = BC_AUTHORITY_IPV4_ADDRESS ":42";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6__expected)
{
    const auto line = "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6_port__expected)
{
    const auto line = "[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]:42";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6_compatible__expected)
{
    // A compatible ip address serializes as alternative notation IPv6.
    const authority host("[" BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "]");
    BOOST_REQUIRE_EQUAL(host.to_string(), "[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]");
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6_alternative_compatible_port__expected)
{
    // A compatible ip address serializes as alternative notation IPv6.
    const authority host("[" BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "]:42");
    BOOST_REQUIRE_EQUAL(host.to_string(), "[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]:42");
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6_alternative_compatible__expected)
{
    const auto line = "[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

BOOST_AUTO_TEST_CASE(authority__to_string__ipv6_compatible_port__expected)
{
    const auto line = "[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]:42";
    const authority host(line);
    BOOST_REQUIRE_EQUAL(host.to_string(), line);
}

// equality

BOOST_AUTO_TEST_CASE(authority__equality__default_default__true)
{
    const authority host1{};
    const authority host2{};
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(authority__equality__default_unspecified_port__false)
{
    const authority host1{};
    const authority host2(BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS, 42);
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(authority__equality__ipv4_ipv4__true)
{
    const authority host1(BC_AUTHORITY_IPV4_ADDRESS);
    const authority host2(BC_AUTHORITY_IPV4_ADDRESS);
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(authority__equality__ipv4_ipv4_port__true)
{
    const authority host1(BC_AUTHORITY_IPV4_ADDRESS);
    const authority host2(BC_AUTHORITY_IPV4_ADDRESS, 42);
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(authority__equality__ipv4_ipv6__false)
{
    const authority host1(BC_AUTHORITY_IPV4_ADDRESS);
    const authority host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(authority__equality__ipv6_ipv6__true)
{
    const authority host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    const authority host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE(host1 == host2);
}

BOOST_AUTO_TEST_CASE(authority__equality__ipv6_ipv6_port__false)
{
    const authority host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    const authority host2(BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS, 42);
    BOOST_REQUIRE(!(host1 == host2));
}

BOOST_AUTO_TEST_CASE(authority__equality__compatible_alternative__true)
{
    // A compatible ip address is equivalent to its alternative addressing.
    const authority host1("[" BC_AUTHORITY_IPV6_COMPATIBLE_ADDRESS "]");
    const authority host2("[" BC_AUTHORITY_IPV6_ALTERNATIVE_COMPATIBLE_ADDRESS "]");
    BOOST_REQUIRE(host1 == host2);
}

// inequality

BOOST_AUTO_TEST_CASE(authority__inequality__default_default__false)
{
    const authority host1{};
    const authority host2{};
    BOOST_REQUIRE(!(host1 != host2));
}

BOOST_AUTO_TEST_CASE(authority__inequality__default_unspecified_port__true)
{
    const authority host1{};
    const authority host2(BC_AUTHORITY_IPV6_UNSPECIFIED_ADDRESS, 42);
    BOOST_REQUIRE(host1 != host2);
}

BOOST_AUTO_TEST_CASE(authority__inequality__ipv6_ipv6__false)
{
    const authority host1("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    const authority host2("[" BC_AUTHORITY_IPV6_COMPRESSED_ADDRESS "]");
    BOOST_REQUIRE(!(host1 != host2));
}

BOOST_AUTO_TEST_SUITE_END()
