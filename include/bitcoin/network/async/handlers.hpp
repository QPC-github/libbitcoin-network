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
#ifndef LIBBITCOIN_NETWORK_ASYNC_HANDLERS_HPP
#define LIBBITCOIN_NETWORK_ASYNC_HANDLERS_HPP

#include <functional>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

/// Aliases for common handler definition.

////typedef std::function<void(const code&)> handle0;
////
////template <typename Type>
////using handle1 = std::function<void(const code&, const Type&)>;
////
////template <typename Type1, typename Type2>
////using handle2 = std::function<void(const code&, const Type1&,
////    const Type2&)>;
////
////template <typename Type1, typename Type2, typename Type3>
////using handle3 = std::function<void(const code&, const Type1&,
////    const Type2&, const Type3&)>;

// Utility to convert a const reference instance to moveable.
template <typename Type>
Type move_copy(const Type& instance)
{
    auto copy = instance;
    return copy;
}

} // namespace network
} // namespace libbitcoin

#endif