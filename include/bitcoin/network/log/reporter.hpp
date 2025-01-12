/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_LOG_REPORTER_HPP
#define LIBBITCOIN_NETWORK_LOG_REPORTER_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/levels.hpp>
#include <bitcoin/network/log/logger.hpp>

namespace libbitcoin {
namespace network {
    
/// Thread safe loggable base class.
class BCT_API reporter
{
protected:
    reporter(const logger& logger) NOEXCEPT;

public:
    const logger& log;
    void fire(uint8_t event, size_t count=zero) const NOEXCEPT;
    void span(uint8_t event, const logger::time& started) const NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
