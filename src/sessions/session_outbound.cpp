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
#include <bitcoin/network/sessions/session_outbound.hpp>

#include <cstddef>
#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_outbound

using namespace bc::system;
using namespace config;
using namespace std::placeholders;

session_outbound::session_outbound(p2p& network) noexcept
  : session(network),
    batch_(std::max(network.network_settings().connect_batch_size, 1u)),
    count_(zero)
{
}

bool session_outbound::inbound() const noexcept
{
    return false;
}

bool session_outbound::notify() const noexcept
{
    return true;
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_outbound::start(result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(settings().outbound_connections) || 
        is_zero(settings().host_pool_capacity))
    {
        handler(error::success);
        return;
    }

    if (is_zero(address_count()))
    {
        handler(error::address_not_found);
        return;
    }

    session::start(BIND2(handle_started, _1, handler));
}

void session_outbound::handle_started(const code& ec,
    result_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    for (size_t peer = 0; peer < settings().outbound_connections; ++peer)
    {
        const auto connectors = create_connectors(batch_);

        // Save each connector for stop.
        for (const auto& connector: *connectors)
            stop_subscriber_->subscribe([=](const code&)
            {
                connector->stop();
            });

        start_connect(connectors);
    }

    // This is the end of the start sequence.
    handler(error::success);
}

// Connnect cycle.
// ----------------------------------------------------------------------------

void session_outbound::start_connect(connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // BATCH CONNECT (wait)
    batch(connectors, BIND3(handle_connect, _1, _2, connectors));
}

void session_outbound::handle_connect(const code& ec, channel::ptr channel,
    connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::service_stopped)
    {
        BC_ASSERT_MSG(!channel, "unexpected channel instance");
        return;
    }

    // There was an error connecting the channel, so try again.
    if (ec)
    {
        timer_->start(BIND1(start_connect, connectors),
            settings().connect_timeout());
        return;
    }

    if (stopped())
    {
        channel->stop(error::service_stopped);
        return;
    }

    start_channel(channel,
        BIND2(handle_channel_start, _1, channel),
        BIND2(handle_channel_stop, _1, connectors));
}

void session_outbound::attach_handshake(const channel::ptr& channel,
    result_handler handler) const noexcept
{
    BC_ASSERT_MSG(channel->stranded(), "strand");

    const auto relay = settings().relay_transactions;
    const auto own_version = settings().protocol_maximum;
    const auto own_services = settings().services;
    const auto invalid_services = settings().invalid_services;
    const auto minimum_version = settings().protocol_minimum;

    // Require peer to serve network (and witness if configured on self).
    const auto min_service = (own_services & messages::service::node_witness) |
        messages::service::node_network;

    // Reject messages are not handled until bip61 (70002).
    // The negotiated_version is initialized to the configured maximum.
    if (channel->negotiated_version() >= messages::level::bip61)
        channel->do_attach<protocol_version_70002>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service, relay)
            ->start(handler);
    else
        channel->do_attach<protocol_version_31402>(*this, own_version,
            own_services, invalid_services, minimum_version, min_service)
            ->start(handler);
}

void session_outbound::handle_channel_start(const code& ec,
    channel::ptr) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        // The start failure is also caught by handle_channel_stop.
        ////channel->stop(ec);
    }
}

void session_outbound::attach_protocols(
    const channel::ptr& channel) const noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto version = channel->negotiated_version();
    const auto heartbeat = settings().channel_heartbeat();

    if (version >= messages::level::bip31)
        channel->do_attach<protocol_ping_60001>(*this, heartbeat)->start();
    else
        channel->do_attach<protocol_ping_31402>(*this, heartbeat)->start();

    if (version >= messages::level::bip61)
        channel->do_attach<protocol_reject_70002>(*this)->start();

    channel->do_attach<protocol_address_31402>(*this)->start();
}

void session_outbound::handle_channel_stop(const code&,
    connectors_ptr connectors) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    start_connect(connectors);
}

// Batch connect.
// ----------------------------------------------------------------------------

void session_outbound::batch(connectors_ptr connectors, 
    channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");


    channel_handler start =
        BIND4(handle_batch, _1, _2, connectors, std::move(handler));

    // Initialize batch of connectors.
    for (const auto& connector: *connectors)
        fetch(BIND4(start_batch, _1, _2, connector, start));
}

void session_outbound::start_batch(const code& ec, const authority& host,
    connector::ptr connector, channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This termination prevents a tight loop in the empty address pool case.
    if (ec)
    {
        handler(ec, nullptr);
        return;
    }

    // This creates a tight loop in the case of a small address pool.
    if (blacklisted(host))
    {
        handler(error::address_blocked, nullptr);
        return;
    }

    // CONNECT (wait)
    connector->connect(host, std::move(handler));
}

// Called once for each call to start_batch.
void session_outbound::handle_batch(const code& ec, channel::ptr channel,
    connectors_ptr connectors, channel_handler handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto finish = (++count_ == batch_);

    if (!ec)
    {
        // Clear unfinished connectors.
        if (!finish)
        {
            count_ = batch_;
            for (auto it = connectors->begin(); it != connectors->end(); ++it)
                (*it)->stop();
        }

        // Got a connection.
        handler(error::success, channel);
        return;
    }

    // Got no successful connection.
    if (finish)
        handler(error::connect_failed, nullptr);
}

} // namespace network
} // namespace libbitcoin
