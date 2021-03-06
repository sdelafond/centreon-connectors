/*
** Copyright 2011-2013 Centreon
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** For more information : contact@centreon.com
*/

#include <sstream>
#include "com/centreon/connector/perl/checks/result.hh"
#include "com/centreon/connector/perl/reporter.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon::connector::perl;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 */
reporter::reporter() : _can_report(true), _reported(0) {}

/**
 *  Copy constructor.
 *
 *  @param[in] r Object to copy.
 */
reporter::reporter(reporter const& r) : com::centreon::handle_listener(r) {
  _copy(r);
}

/**
 *  Destructor.
 */
reporter::~reporter() throw () {
  log_info(logging::medium) << "connector reporter " << _reported
    << " check results to monitoring engine";
}

/**
 *  Assignment operator.
 *
 *  @param[in] r Object to copy.
 *
 *  @return This object.
 */
reporter& reporter::operator=(reporter const& r) {
  if (this != &r) {
    com::centreon::handle_listener::operator=(r);
    _copy(r);
  }
  return (*this);
}

/**
 *  Check if reporter can report.
 *
 *  @return true if reporter can report.
 */
bool reporter::can_report() const throw () {
  return (_can_report);
}

/**
 *  Error event on the handle.
 *
 *  @param[in] h Unused.
 */
void reporter::error(handle& h) {
  (void)h;
  _can_report = false;
  throw (basic_error() << "error detected on the handle used" \
              " to report to the monitoring engine");
  return ;
}

/**
 *  Get reporter's internal buffer.
 *
 *  @return Internal buffer.
 */
std::string const& reporter::get_buffer() const throw () {
  return (_buffer);
}

/**
 *  Report check result.
 *
 *  @param[in] r Check result.
 */
void reporter::send_result(checks::result const& r) {
  // Update statistics.
  ++_reported;
  log_debug(logging::high)
    << "reporting check result #" << _reported << " (check "
    << r.get_command_id() << ")";

  // Build packet.
  std::ostringstream oss;
  // Packet ID.
  oss << "3";
  oss.put('\0');
  // Command ID.
  oss << r.get_command_id();
  oss.put('\0');
  // Executed.
  oss << (r.get_executed() ? "1" : "0");
  oss.put('\0');
  // Exit code.
  oss << r.get_exit_code();
  oss.put('\0');
  // Error output.
  if (r.get_error().empty())
    oss.put(' ');
  else
    oss << r.get_error();
  oss.put('\0');
  // Standard output.
  if (r.get_output().empty())
    oss.put(' ');
  else
    oss << r.get_output();
  // Packet boundary.
  for (unsigned int i = 0; i < 4; ++i)
    oss.put('\0');
  // Append packet to write buffer.
  _buffer.append(oss.str());
  return ;
}

/**
 *  Send protocol version to monitoring engine.
 *
 *  @param[in] major Major protocol version.
 *  @param[in] minor Minor protocol version.
 */
void reporter::send_version(unsigned int major, unsigned int minor) {
  // Build packet.
  log_debug(logging::medium) << "sending protocol version "
    << major << "." << minor << " to monitoring engine";
  std::ostringstream oss;
  oss << "1";
  oss.put('\0');
  // Major.
  oss << major;
  oss.put('\0');
  // Minor.
  oss << minor;
  for (unsigned int i = 0; i < 4; ++i)
    oss.put('\0');

  // Send packet back to monitoring engine.
  _buffer.append(oss.str());

  return ;
}

/**
 *  Do we want to send something to the monitoring engine ?
 *
 *  @param[in] h Monitoring engine handle.
 */
bool reporter::want_write(handle& h) {
  (void)h;
  return (can_report() && !_buffer.empty());
}

/**
 *  Send data to the monitoring engine.
 *
 *  @param[in] h Handle.
 */
void reporter::write(handle& h) {
  unsigned long wb(h.write(_buffer.c_str(), _buffer.size()));
  _buffer.erase(0, wb);
  return ;
}

/**************************************
*                                     *
*           Private Methods           *
*                                     *
**************************************/

/**
 *  Copy internal data members.
 *
 *  @param[in] r Object to copy.
 */
void reporter::_copy(reporter const& r) {
  _buffer = r._buffer;
  _can_report = r._can_report;
  _reported = r._reported;
  return ;
}
