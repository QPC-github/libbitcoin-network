/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/sessions/session_inbound.hpp>

#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_inbound::session_inbound(p2p& network, size_t key) NOEXCEPT
  : session(network, key), tracker<session_inbound>(network.log())
{
}

bool session_inbound::inbound() const NOEXCEPT
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().inbound_enabled())
    {
        LOG("Not configured for inbound connections.");
        handler(error::bypassed);
        return;
    }

    session::start(BIND2(handle_started, _1, std::move(handler)));
}

void session_inbound::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        return;
    }

    // TODO: create one acceptor for each configured local endpoint.
    const auto acceptor = create_acceptor();
    const auto error_code = acceptor->start(settings().inbound_port);

    if (!error_code)
    {
        LOG("Accepting up to " << settings().inbound_connections
            << " connections on port " << settings().inbound_port << ".");
    }

    handler(error_code);

    if (!error_code)
    {
        subscribe_stop([=](const code&) NOEXCEPT
        {
            acceptor->stop();
            return false;
        });

        start_accept(error::success, acceptor);
    }
}

// Accept cycle.
// ----------------------------------------------------------------------------

void session_inbound::start_accept(const code& ec,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    ////LOG("Reset start accept.");

    if (ec)
    {
        LOG("Failed to start acceptor, " << ec.message());
        return;
    }

    acceptor->accept(BIND3(handle_accept, _1, _2, acceptor));
}

void session_inbound::handle_accept(const code& ec,
    const socket::ptr& socket, const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        return;
    }

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket, "unexpected socket instance");
        LOG("Failed to accept inbound connection, " << ec.message());
        defer(BIND2(start_accept, _1, acceptor));
        return;
    }

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    if (!whitelisted(socket->authority()))
    {
        ////LOG("Dropping not whitelisted connection [" << socket->authority() << "]");
        socket->stop();
        return;
    }

    if (blacklisted(socket->authority()))
    {
        ////LOG("Dropping blacklisted connection [" << socket->authority() << "]");
        socket->stop();
        return;
    }

    // Could instead stop listening when at limit, though this is simpler.
    if (inbound_channel_count() >= settings().inbound_connections)
    {
        LOG("Dropping oversubscribed connection [" << socket->authority() << "]");
        socket->stop();
        return;
    }

    const auto channel = create_channel(socket, false);

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, channel));
}

// Completion sequence.
// ----------------------------------------------------------------------------

void session_inbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Inbound does not require any node services (e.g. bitnodes.io is zero).
    constexpr auto minimum_services = messages::service::node_none;

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto maximum_version = settings().protocol_maximum;
    const auto maximum_services = settings().services_maximum;
    const auto extended_version = maximum_version >= messages::level::bip37;
    const auto enable_transaction = settings().enable_transaction;
    const auto enable_reject = settings().enable_reject &&
        maximum_version >= messages::level::bip61;

    // Protocol must pause the channel after receiving version and verack.

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_version_70002>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else if (extended_version)
        channel->attach<protocol_version_70001>(self, minimum_services,
            maximum_services, enable_transaction)->shake(std::move(handler));

    else
        channel->attach<protocol_version_31402>(self, minimum_services,
            maximum_services)->shake(std::move(handler));
}

void session_inbound::handle_channel_start(const code&,
    const channel::ptr&) NOEXCEPT
{
    // TOOD: nonce check here.

    BC_ASSERT_MSG(stranded(), "strand");
    ////LOG("Inbound channel start [" << channel->authority() << "] "
    ////    << ec.message());
}

void session_inbound::attach_protocols(
    const channel::ptr& channel) const NOEXCEPT
{
    session::attach_protocols(channel);
}

void session_inbound::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOG("Inbound channel stop [" << channel->authority() << "] "
        << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
