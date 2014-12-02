// Copyright 2014 The Crashpad Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef CRASHPAD_UTIL_MACH_MACH_MESSAGE_H_
#define CRASHPAD_UTIL_MACH_MACH_MESSAGE_H_

#include <mach/mach.h>
#include <stdint.h>

#include <limits>

namespace crashpad {

//! \brief The time before which a MachMessageWithDeadline() call should
//!     complete.
//!
//! A value of this type may be one of the special constants
//! #kMachMessageNonblocking or #kMachMessageWaitIndefinitely. Any other values
//! should be produced by calling MachMessageDeadlineFromTimeout().
//!
//! Internally, these are currently specified on the same time base as
//! ClockMonotonicNanoseconds(), although this is an implementation detail.
using MachMessageDeadline = uint64_t;

//! \brief Special constants used as \ref MachMessageDeadline values.
enum : MachMessageDeadline {
  //! \brief MachMessageWithDeadline() should not block at all in its operation.
  kMachMessageNonblocking = 0,

  //! \brief MachMessageWithDeadline() should wait indefinitely for the
  //!     requested operation to complete.
  kMachMessageWaitIndefinitely =
      std::numeric_limits<MachMessageDeadline>::max(),
};

//! \brief Computes the deadline for a specified timeout value.
//!
//! While deadlines exist on an absolute time scale, timeouts are relative. This
//! function calculates the deadline as \a timeout_ms milliseconds after it
//! executes.
//!
//! If \a timeout_ms is `0`, this function will return #kMachMessageNonblocking.
MachMessageDeadline MachMessageDeadlineFromTimeout(
    mach_msg_timeout_t timeout_ms);

//! \brief Runs `mach_msg()` with a deadline, as opposed to a timeout.
//!
//! This function is similar to `mach_msg()`, with the following differences:
//!  - The `timeout` parameter has been replaced by \a deadline. The deadline
//!    applies uniformly to a call that is requested to both send and receive
//!    a message.
//!  - The `MACH_SEND_TIMEOUT` and `MACH_RCV_TIMEOUT` bits in \a options are
//!    not used. Timeouts are specified by the \a deadline argument.
//!  - The `send_size` parameter has been removed. Its value is implied by
//!    \a message when \a options contains `MACH_SEND_MSG`.
//!  - The \a run_even_if_expired parameter has been added.
//!
//! Like the `mach_msg()` wrapper in `libsyscall`, this function will retry
//! operations when experiencing `MACH_SEND_INTERRUPTED` and
//! `MACH_RCV_INTERRUPTED`, unless \a options contains `MACH_SEND_INTERRUPT` or
//! `MACH_RCV_INTERRUPT`. Unlike `mach_msg()`, which restarts the call with the
//! full timeout when this occurs, this function continues enforcing the
//! user-specified \a deadline.
//!
//! Except as noted, the parameters and return value are identical to those of
//! `mach_msg()`.
//!
//! \param[in] deadline The time by which this call should complete. If the
//!     deadline is exceeded, this call will return `MACH_SEND_TIMED_OUT` or
//!     `MACH_RCV_TIMED_OUT`.
//! \param[in] run_even_if_expired If `true`, a deadline that is expired when
//!     this function is called will be treated as though a deadline of
//!     #kMachMessageNonblocking had been specified. When `false`, an expired
//!     deadline will result in a `MACH_SEND_TIMED_OUT` or `MACH_RCV_TIMED_OUT`
//!     return value, even if the deadline is already expired when the function
//!     is called.
mach_msg_return_t MachMessageWithDeadline(mach_msg_header_t* message,
                                          mach_msg_option_t options,
                                          mach_msg_size_t receive_size,
                                          mach_port_name_t receive_port,
                                          MachMessageDeadline deadline,
                                          mach_port_name_t notify_port,
                                          bool run_even_if_expired);

//! \brief Initializes a reply message for a MIG server routine based on its
//!     corresponding request.
//!
//! If a request is handled by a server routine, it may be necessary to revise
//! some of the fields set by this function, such as `msgh_size` and any fields
//! defined in a routine’s reply structure type.
//!
//! \param[in] in_header The request message to base the reply on.
//! \param[out] out_header The reply message to initialize. \a out_header will
//!     be treated as a `mig_reply_error_t*` and all of its fields will be set
//!     except for `RetCode`, which must be set by SetMIGReplyError(). This
//!     argument is accepted as a `mach_msg_header_t*` instead of a
//!     `mig_reply_error_t*` because that is the type that callers are expected
//!     to possess in the C API.
void PrepareMIGReplyFromRequest(const mach_msg_header_t* in_header,
                                mach_msg_header_t* out_header);

//! \brief Sets the error code in a reply message for a MIG server routine.
//!
//! \param[inout] out_header The reply message to operate on. \a out_header will
//!     be treated as a `mig_reply_error_t*` and its `RetCode` field will be
//!     set. This argument is accepted as a `mach_msg_header_t*` instead of a
//!     `mig_reply_error_t*` because that is the type that callers are expected
//!     to possess in the C API.
//! \param[in] error The error code to store in \a out_header.
//!
//! \sa PrepareMIGReplyFromRequest()
void SetMIGReplyError(mach_msg_header_t* out_header, kern_return_t error);

//! \brief Returns a Mach message trailer for a message that has been received.
//!
//! This function must only be called on Mach messages that have been received
//! via the Mach messaging interface, such as `mach_msg()`. Messages constructed
//! for sending do not contain trailers.
//!
//! \param[in] header A pointer to a received Mach message.
//!
//! \return A pointer to the trailer following the received Mach message’s body.
//!     The contents of the trailer depend on the options provided to
//!     `mach_msg()` or a similar function when the message was received.
const mach_msg_trailer_t* MachMessageTrailerFromHeader(
    const mach_msg_header_t* header);

}  // namespace crashpad

#endif  // CRASHPAD_UTIL_MACH_MACH_MESSAGE_H_
