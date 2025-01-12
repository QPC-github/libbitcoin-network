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
#include <bitcoin/network/messages/headers.hpp>

#include <iterator>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>
#include <bitcoin/network/messages/enums/level.hpp>
#include <bitcoin/network/messages/enums/magic_numbers.hpp>
#include <bitcoin/network/messages/inventory_item.hpp>
#include <bitcoin/network/messages/message.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

using namespace system;
    
const std::string headers::command = "headers";
const identifier headers::id = identifier::headers;
const uint32_t headers::version_minimum = level::headers_protocol;
const uint32_t headers::version_maximum = level::maximum_protocol;

// Each header must trail a zero byte (yes, it's stoopid).
constexpr uint8_t trail = 0x00;

// static
typename headers::cptr headers::deserialize(uint32_t version,
    const data_chunk& data) NOEXCEPT
{
    read::bytes::copy reader(data);
    const auto message = to_shared(deserialize(version, reader));
    if (!reader)
        return nullptr;

    constexpr auto size = chain::header::serialized_size();
    auto hash = std::next(data.data(), size_variable(data.front()));

    for (const auto& header: message->header_ptrs)
    {
        header->set_hash(bitcoin_hash(size, hash));
        std::advance(hash, add1(size));
    }

    return message;
}

// static
headers headers::deserialize(uint32_t version, reader& source) NOEXCEPT
{
    if (version < version_minimum || version > version_maximum)
        source.invalidate();

    const auto size = source.read_size(max_get_headers);
    chain::header_cptrs header_ptrs{};
    header_ptrs.reserve(size);

    for (size_t header = 0; header < size; ++header)
    {
        BC_PUSH_WARNING(NO_NEW_OR_DELETE)
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        header_ptrs.emplace_back(new chain::header{ source });
        BC_POP_WARNING()
        BC_POP_WARNING()

        if (source.read_byte() != trail)
            source.invalidate();
    }

    return { header_ptrs };
}

bool headers::serialize(uint32_t version,
    const data_slab& data) const NOEXCEPT
{
    write::bytes::copy writer(data);
    serialize(version, writer);
    return writer;
}

void headers::serialize(uint32_t BC_DEBUG_ONLY(version),
    writer& sink) const NOEXCEPT
{
    BC_DEBUG_ONLY(const auto bytes = size(version);)
    BC_DEBUG_ONLY(const auto start = sink.get_write_position();)

    sink.write_variable(header_ptrs.size());

    for (const auto& header: header_ptrs)
    {
        header->to_data(sink);
        sink.write_byte(trail);
    }

    BC_ASSERT(sink&& sink.get_write_position() - start == bytes);
}

size_t headers::size(uint32_t) const NOEXCEPT
{
    return variable_size(header_ptrs.size()) +
        (header_ptrs.size() * chain::header::serialized_size() + sizeof(trail));
}

// TODO: This would benefit from block hash store/return as pointer.
bool headers::is_sequential() const NOEXCEPT
{
    if (header_ptrs.empty())
        return true;

    auto previous = header_ptrs.front()->hash();

    for (auto it = std::next(header_ptrs.begin()); it != header_ptrs.end(); ++it)
    {
        if ((*it)->previous_block_hash() != previous)
            return false;

        previous = (*it)->hash();
    }

    return true;
}

// TODO: This would benefit from hashes as list of pointers.
hashes headers::to_hashes() const NOEXCEPT
{
    hashes out;
    out.reserve(header_ptrs.size());

    for (const auto& header: header_ptrs)
        out.push_back(header->hash());

    return out;
}

// TODO: This would benefit from inventory_item hash pointers.
inventory_items headers::to_inventory(inventory::type_id type) const NOEXCEPT
{
    inventory_items out;
    out.reserve(header_ptrs.size());

    // msvc: emplace_back(type, header->hash()) does not compile.
    for (const auto& header: header_ptrs)
        out.push_back({ type, header->hash() });

    return out;
}

} // namespace messages
} // namespace network
} // namespace libbitcoin
