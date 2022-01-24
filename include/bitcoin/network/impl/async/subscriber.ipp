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
#ifndef LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP
#define LIBBITCOIN_NETWORK_ASYNC_SUBSCRIBER_IPP

#include <boost/asio.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
subscriber<Args...>::subscriber(asio::strand& strand) noexcept
  : strand_(strand),
    stopped_(false)
{
}

template <typename... Args>
subscriber<Args...>::~subscriber() noexcept
{
    BC_ASSERT_MSG(stopped_, "subscriber is not stopped");
}

template <typename... Args>
void subscriber<Args...>::subscribe(handler&& notify) noexcept
{
    if (!stopped_)
        queue_.push_back(std::move(notify));
}

template <typename... Args>
void subscriber<Args...>::notify(const Args&... args) const noexcept
{
    if (stopped_)
        return;

    // std::bind copies handler and args for each post (including refs).
    // Post all to strand, non-blocking, cannot internally execute handler.
    for (const auto& handler: queue_)
        boost::asio::post(strand_, std::bind(handler, args...));
}

template <typename... Args>
void subscriber<Args...>::stop(const Args&... args) noexcept
{
    if (stopped_)
        return;

    stopped_ = true;
    notify(args...);
    queue_.clear();
}


} // namespace network
} // namespace libbitcoin

#endif
