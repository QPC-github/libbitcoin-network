/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/log/capture.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {
    
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

capture::capture(std::istream& input) NOEXCEPT
  : input_(input)
{
}

capture::~capture() NOEXCEPT
{
    BC_ASSERT_MSG(stopped_, "~capture not stopped");
    pool_.stop();
    BC_DEBUG_ONLY(const auto result =) pool_.join();
    BC_ASSERT_MSG(result, "capture::join");
}

bool capture::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

// start
// ----------------------------------------------------------------------------

void capture::start() NOEXCEPT
{
    stopped_ = false;
    boost::asio::post(pool_.service(),
        std::bind(&capture::do_start, this));
}

// Unstranded, owns one of the two capture threads.
void capture::do_start() NOEXCEPT
{
    std::string line{};

    // <ctrl-c> invalidates input, causing normal termination.
    // getline blocks (if input is valid) until receiving a "line" of input.
    // External input stream invalidation does not unblock getline.
    while (!stopped_ && std::getline(input_, line))
        notify(error::success, move_copy(line));

    // In case input was invalidated.
    stop();
}

// stop
// ----------------------------------------------------------------------------

void capture::stop() NOEXCEPT
{
    // Signal listener stop (must also receive input to terminate).
    stopped_ = true;

    // Protect pool and subscriber (idempotent but not thread safe).
    // This buffers the handler if getline is still blocking and there is only
    // one thread in the pool. Providing a second thread allows stop to proceed
    // and the buffer to clear immediately, despite shutdown remaining blocked
    // on getline completion.
    boost::asio::post(strand_,
        std::bind(&capture::do_stop, this));
}

// private
void capture::do_stop() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Stop accepting work.
    pool_.stop();

    // Subscriber asserts if stopped with a success code.
    subscriber_.stop_default(error::service_stopped);
}

// lines
// ----------------------------------------------------------------------------

// protected
void capture::notify(const code& ec, std::string&& line) const NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&capture::do_notify, this, ec, std::move(line)));
}

// private
void capture::do_notify(const code& ec,
    const std::string& line) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    subscriber_.notify(ec, line);
}

void capture::subscribe(notifier&& handler, result_handler&& complete) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&capture::do_subscribe,
            this, std::move(handler), std::move(complete)));
}

// private
void capture::do_subscribe(const notifier& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    complete(subscriber_.subscribe(move_copy(handler)));
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
