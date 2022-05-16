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
#include <bitcoin/network/error.hpp>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>

namespace libbitcoin {
namespace network {
namespace error {

DEFINE_ERROR_T_MESSAGE_MAP(error)
{
    { success, "success" },
    { unknown, "unknown error" },
    { bypassed, "start bypassed without failure" },

    // addresses
    { address_not_found, "address not found" },
    { seeding_unsuccessful, "seeding unsuccessful" },
        
    // file system
    { file_load, "failed to load file" },
    { file_save, "failed to save file" },
    { file_system, "file system error" },

    // general I/O failures
    { bad_stream, "bad data stream" },

    // incoming connection failures
    { listen_failed, "incoming connection failed" },
    { accept_failed, "connection acceptance failed" },
    { oversubscribed, "service oversubscribed" },
        
    // outgoing connection failures
    { address_blocked, "address blocked by policy" },
    { address_in_use, "address already in use" },
    { resolve_failed, "resolving hostname failed" },
    { connect_failed, "unable to reach remote host" },

    // heading read failures
    { invalid_heading, "invalid message heading" },
    { invalid_magic, "invalid message heading magic" },

    // payload read failures
    { oversized_payload, "oversize message payload" },
    { invalid_checksum, "invalid message checksum" },
    { invalid_message, "message failed to deserialize" },
    { unknown_message, "unknown message type" },

    // general failures
    { protocol_violation, "protocol violation" },
    { invalid_configuration, "invalid configuration" },
    { operation_timeout, "operation timed out" },
    { operation_canceled, "operation canceled" },
    { operation_failed, "operation failed" },

    // termination
    { channel_timeout, "connection timed out" },
    { channel_dropped, "channel dropped" },
    { channel_stopped, "channel stopped" },
    { service_stopped, "service stopped" },
    { subscriber_stopped, "subscriber stopped" },
};

DEFINE_ERROR_T_CATEGORY(error, "network", "network code")

bool asio_is_canceled(const error::boost_code& ec)
{
    // We test against the platform-independent condition (equivalence).
    // Boost documents that cancellation gives basic_errors::operation_aborted,
    // however that is the code (equality), not the condition (equivalence):
    ////operation_aborted = BOOST_ASIO_WIN_OR_POSIX(
    ////    BOOST_ASIO_NATIVE_ERROR(ERROR_OPERATION_ABORTED),
    ////    BOOST_ASIO_NATIVE_ERROR(ECANCELED))
    ////return ec == boost::asio::error::operation_aborted;
    return ec == boost_error_t::operation_canceled;
}

// This method is only invoked when asio returns an error that is not the
// result of cancellation of the call (boost_error_t::operation_canceled).
// Equivalence tests require equality operator override. The success and 
// connection_aborted codes are the only expected in normal operation, so these
// are first, to optimize the case where asio_is_canceled is not used.
code asio_to_error_code(const error::boost_code& ec)
{
    if (ec == boost_error_t::success)
        return error::success;

    // termination
    if (ec == boost_error_t::connection_aborted ||
        ec == boost_error_t::operation_canceled)
        return error::operation_canceled;

    // network
    if (ec == boost_error_t::connection_refused ||
        ec == boost_error_t::connection_reset ||
        ec == boost_error_t::not_connected ||
        ec == boost_error_t::not_connected ||
        ec == boost_error_t::operation_not_permitted ||
        ec == boost_error_t::operation_not_supported ||
        ec == boost_error_t::owner_dead ||
        ec == boost_error_t::permission_denied)
        return error::operation_failed;

    // connect-resolve
    if (ec == boost_error_t::address_family_not_supported ||
        ec == boost_error_t::address_not_available ||
        ec == boost_error_t::bad_address ||
        ec == boost_error_t::destination_address_required)
        return error::resolve_failed;

    // connect-resolve
    if (ec == boost_error_t::broken_pipe ||
        ec == boost_error_t::host_unreachable ||
        ec == boost_error_t::network_down ||
        ec == boost_error_t::network_reset ||
        ec == boost_error_t::network_unreachable ||
        ec == boost_error_t::no_link ||
        ec == boost_error_t::no_protocol_option ||
        ec == boost_error_t::no_such_file_or_directory ||
        ec == boost_error_t::no_such_file_or_directory ||
        ec == boost_error_t::not_a_socket ||
        ec == boost_error_t::protocol_not_supported ||
        ec == boost_error_t::wrong_protocol_type)
        return error::connect_failed;

    // connect-address
    if (ec == boost_error_t::address_in_use ||
        ec == boost_error_t::already_connected ||
        ec == boost_error_t::connection_already_in_progress ||
        ec == boost_error_t::operation_in_progress)
        return error::address_in_use;

    // I/O
    if (ec == boost_error_t::bad_message ||
        ec == boost_error_t::illegal_byte_sequence ||
        ec == boost_error_t::io_error ||
        ec == boost_error_t::message_size ||
        ec == boost_error_t::no_message_available ||
        ec == boost_error_t::no_message ||
        ec == boost_error_t::no_stream_resources ||
        ec == boost_error_t::not_a_stream ||
        ec == boost_error_t::protocol_error)
        return error::bad_stream;

    // timeout
    if (ec == boost_error_t::stream_timeout ||
        ec == boost_error_t::timed_out)
        return error::channel_timeout;

    // file system errors
    if (ec == boost_error_t::cross_device_link ||
        ec == boost_error_t::bad_file_descriptor ||
        ec == boost_error_t::device_or_resource_busy ||
        ec == boost_error_t::directory_not_empty ||
        ec == boost_error_t::executable_format_error ||
        ec == boost_error_t::file_exists ||
        ec == boost_error_t::file_too_large ||
        ec == boost_error_t::filename_too_long ||
        ec == boost_error_t::invalid_seek ||
        ec == boost_error_t::is_a_directory ||
        ec == boost_error_t::no_space_on_device ||
        ec == boost_error_t::no_such_device ||
        ec == boost_error_t::no_such_device_or_address ||
        ec == boost_error_t::read_only_file_system ||
        ec == boost_error_t::resource_unavailable_try_again ||
        ec == boost_error_t::text_file_busy ||
        ec == boost_error_t::too_many_files_open ||
        ec == boost_error_t::too_many_files_open_in_system ||
        ec == boost_error_t::too_many_links ||
        ec == boost_error_t::too_many_symbolic_link_levels)
        return error::file_system;

    ////// unknown
    ////if (ec == boost_error_t::argument_list_too_long ||
    ////    ec == boost_error_t::argument_out_of_domain ||
    ////    ec == boost_error_t::function_not_supported ||
    ////    ec == boost_error_t::identifier_removed ||
    ////    ec == boost_error_t::inappropriate_io_control_operation ||
    ////    ec == boost_error_t::interrupted ||
    ////    ec == boost_error_t::invalid_argument ||
    ////    ec == boost_error_t::no_buffer_space ||
    ////    ec == boost_error_t::no_child_process ||
    ////    ec == boost_error_t::no_lock_available ||
    ////    ec == boost_error_t::no_such_process ||
    ////    ec == boost_error_t::not_a_directory ||
    ////    ec == boost_error_t::not_enough_memory ||
    ////    ec == boost_error_t::operation_would_block ||
    ////    ec == boost_error_t::resource_deadlock_would_occur ||
    ////    ec == boost_error_t::result_out_of_range ||
    ////    ec == boost_error_t::state_not_recoverable ||
    ////    ec == boost_error_t::value_too_large)
    return error::unknown;
}

} // namespace error
} // namespace network
} // namespace libbitcoin

// Just for reference.
#ifdef BOOST_CODES_AND_CONDITIONS

// Boost: asio error enum (basic subset, platform specific codes).
enum boost::asio::error::basic_errors
{
  /// Permission denied.
  access_denied = BOOST_ASIO_SOCKET_ERROR(EACCES),

  /// Address family not supported by protocol.
  address_family_not_supported = BOOST_ASIO_SOCKET_ERROR(EAFNOSUPPORT),

  /// Address already in use.
  address_in_use = BOOST_ASIO_SOCKET_ERROR(EADDRINUSE),

  /// Transport endpoint is already connected.
  already_connected = BOOST_ASIO_SOCKET_ERROR(EISCONN),

  /// Operation already in progress.
  already_started = BOOST_ASIO_SOCKET_ERROR(EALREADY),

  /// Broken pipe.
  broken_pipe = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_BROKEN_PIPE),
      BOOST_ASIO_NATIVE_ERROR(EPIPE)),

  /// A connection has been aborted.
  connection_aborted = BOOST_ASIO_SOCKET_ERROR(ECONNABORTED),

  /// Connection refused.
  connection_refused = BOOST_ASIO_SOCKET_ERROR(ECONNREFUSED),

  /// Connection reset by peer.
  connection_reset = BOOST_ASIO_SOCKET_ERROR(ECONNRESET),

  /// Bad file descriptor.
  bad_descriptor = BOOST_ASIO_SOCKET_ERROR(EBADF),

  /// Bad address.
  fault = BOOST_ASIO_SOCKET_ERROR(EFAULT),

  /// No route to host.
  host_unreachable = BOOST_ASIO_SOCKET_ERROR(EHOSTUNREACH),

  /// Operation now in progress.
  in_progress = BOOST_ASIO_SOCKET_ERROR(EINPROGRESS),

  /// Interrupted system call.
  interrupted = BOOST_ASIO_SOCKET_ERROR(EINTR),

  /// Invalid argument.
  invalid_argument = BOOST_ASIO_SOCKET_ERROR(EINVAL),

  /// Message too long.
  message_size = BOOST_ASIO_SOCKET_ERROR(EMSGSIZE),

  /// The name was too long.
  name_too_long = BOOST_ASIO_SOCKET_ERROR(ENAMETOOLONG),

  /// Network is down.
  network_down = BOOST_ASIO_SOCKET_ERROR(ENETDOWN),

  /// Network dropped connection on reset.
  network_reset = BOOST_ASIO_SOCKET_ERROR(ENETRESET),

  /// Network is unreachable.
  network_unreachable = BOOST_ASIO_SOCKET_ERROR(ENETUNREACH),

  /// Too many open files.
  no_descriptors = BOOST_ASIO_SOCKET_ERROR(EMFILE),

  /// No buffer space available.
  no_buffer_space = BOOST_ASIO_SOCKET_ERROR(ENOBUFS),

  /// Cannot allocate memory.
  no_memory = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_OUTOFMEMORY),
      BOOST_ASIO_NATIVE_ERROR(ENOMEM)),

  /// Operation not permitted.
  no_permission = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_ACCESS_DENIED),
      BOOST_ASIO_NATIVE_ERROR(EPERM)),

  /// Protocol not available.
  no_protocol_option = BOOST_ASIO_SOCKET_ERROR(ENOPROTOOPT),

  /// No such device.
  no_such_device = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_BAD_UNIT),
      BOOST_ASIO_NATIVE_ERROR(ENODEV)),

  /// Transport endpoint is not connected.
  not_connected = BOOST_ASIO_SOCKET_ERROR(ENOTCONN),

  /// Socket operation on non-socket.
  not_socket = BOOST_ASIO_SOCKET_ERROR(ENOTSOCK),

  /// Operation canceled.
  operation_aborted = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_OPERATION_ABORTED),
      BOOST_ASIO_NATIVE_ERROR(ECANCELED)),

  /// Operation not supported.
  operation_not_supported = BOOST_ASIO_SOCKET_ERROR(EOPNOTSUPP),

  /// Cannot send after transport endpoint shutdown.
  shut_down = BOOST_ASIO_SOCKET_ERROR(ESHUTDOWN),

  /// Connection timed out.
  timed_out = BOOST_ASIO_SOCKET_ERROR(ETIMEDOUT),

  /// Resource temporarily unavailable.
  try_again = BOOST_ASIO_WIN_OR_POSIX(
      BOOST_ASIO_NATIVE_ERROR(ERROR_RETRY),
      BOOST_ASIO_NATIVE_ERROR(EAGAIN)),

  /// Socket is marked non-blocking and requested operation would block.
  would_block = BOOST_ASIO_SOCKET_ERROR(EWOULDBLOCK)
};

// Boost: system error enum for error_conditions (platform-independent codes).
enum boost::system::errc::errc_t
{
    success = 0,
    address_family_not_supported = EAFNOSUPPORT,
    address_in_use = EADDRINUSE,
    address_not_available = EADDRNOTAVAIL,
    already_connected = EISCONN,
    argument_list_too_long = E2BIG,
    argument_out_of_domain = EDOM,
    bad_address = EFAULT,
    bad_file_descriptor = EBADF,
    bad_message = EBADMSG,
    broken_pipe = EPIPE,
    connection_aborted = ECONNABORTED,
    connection_already_in_progress = EALREADY,
    connection_refused = ECONNREFUSED,
    connection_reset = ECONNRESET,
    cross_device_link = EXDEV,
    destination_address_required = EDESTADDRREQ,
    device_or_resource_busy = EBUSY,
    directory_not_empty = ENOTEMPTY,
    executable_format_error = ENOEXEC,
    file_exists = EEXIST,
    file_too_large = EFBIG,
    filename_too_long = ENAMETOOLONG,
    function_not_supported = ENOSYS,
    host_unreachable = EHOSTUNREACH,
    identifier_removed = EIDRM,
    illegal_byte_sequence = EILSEQ,
    inappropriate_io_control_operation = ENOTTY,
    interrupted = EINTR,
    invalid_argument = EINVAL,
    invalid_seek = ESPIPE,
    io_error = EIO,
    is_a_directory = EISDIR,
    message_size = EMSGSIZE,
    network_down = ENETDOWN,
    network_reset = ENETRESET,
    network_unreachable = ENETUNREACH,
    no_buffer_space = ENOBUFS,
    no_child_process = ECHILD,
    no_link = ENOLINK,
    no_lock_available = ENOLCK,
    no_message_available = ENODATA,
    no_message = ENOMSG,
    no_protocol_option = ENOPROTOOPT,
    no_space_on_device = ENOSPC,
    no_stream_resources = ENOSR,
    no_such_device_or_address = ENXIO,
    no_such_device = ENODEV,
    no_such_file_or_directory = ENOENT,
    no_such_process = ESRCH,
    not_a_directory = ENOTDIR,
    not_a_socket = ENOTSOCK,
    not_a_stream = ENOSTR,
    not_connected = ENOTCONN,
    not_enough_memory = ENOMEM,
    not_supported = ENOTSUP,
    operation_canceled = ECANCELED,
    operation_in_progress = EINPROGRESS,
    operation_not_permitted = EPERM,
    operation_not_supported = EOPNOTSUPP,
    operation_would_block = EWOULDBLOCK,
    owner_dead = EOWNERDEAD,
    permission_denied = EACCES,
    protocol_error = EPROTO,
    protocol_not_supported = EPROTONOSUPPORT,
    read_only_file_system = EROFS,
    resource_deadlock_would_occur = EDEADLK,
    resource_unavailable_try_again = EAGAIN,
    result_out_of_range = ERANGE,
    state_not_recoverable = ENOTRECOVERABLE,
    stream_timeout = ETIME,
    text_file_busy = ETXTBSY,
    timed_out = ETIMEDOUT,
    too_many_files_open_in_system = ENFILE,
    too_many_files_open = EMFILE,
    too_many_links = EMLINK,
    too_many_symbolic_link_levels = ELOOP,
    value_too_large = EOVERFLOW,
    wrong_protocol_type = EPROTOTYPE
};

#endif // BOOST_CODES_AND_CONDITIONS