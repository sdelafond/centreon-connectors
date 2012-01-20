/*
** Copyright 2011-2012 Merethis
**
** This file is part of Centreon Connector Perl.
**
** Centreon Connector Perl is free software: you can redistribute it
** and/or modify it under the terms of the GNU Affero General Public
** License as published by the Free Software Foundation, either version
** 3 of the License, or (at your option) any later version.
**
** Centreon Connector Perl is distributed in the hope that it will be
** useful, but WITHOUT ANY WARRANTY; without even the implied warranty
** of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public
** License along with Centreon Connector Perl. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <stdlib.h>
#include "com/centreon/connector/perl/checks/check.hh"
#include "com/centreon/connector/perl/checks/listener.hh"
#include "com/centreon/connector/perl/checks/result.hh"
#include "com/centreon/connector/perl/embedded_perl.hh"
#include "com/centreon/connector/perl/multiplexer.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon;
using namespace com::centreon::connector::perl::checks;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 */
check::check() : _cmd_id(0), _listnr(NULL) {}

/**
 *  Destructor.
 */
check::~check() throw () {
  try {
    result r;
    r.set_command_id(_cmd_id);
    _send_result_and_unregister(r);
  }
  catch (...) {}
}

/**
 *  Error occurred on one pipe.
 *
 *  @param[in] h Pipe.
 */
void check::error(handle& h) {
  (void)h;
  result r;
  r.set_command_id(_cmd_id);
  _send_result_and_unregister(r);
  return ;
}

/**
 *  Execute a Perl script.
 *
 *  @param[in] cmd_id Command ID.
 *  @param[in] cmd    Command line.
 *  @param[in] tmt    Timeout.
 *
 *  @return Process ID.
 */
pid_t check::execute(
               unsigned long long cmd_id,
               std::string const& cmd,
               time_t tmt) {
  // Run process.
  int fds[3];
  pid_t child(embedded_perl::instance().run(cmd, fds));
  ::close(fds[0]);
  _out.set_fd(fds[1]);
  _err.set_fd(fds[2]);

  // Store command ID.
  logging::debug(logging::low) << "check " << this
    << " has ID " << cmd_id;
  _cmd_id = cmd_id;

  // Register with multiplexer.
  multiplexer::instance().handle_manager::add(
    &_err,
    this);
  multiplexer::instance().handle_manager::add(
    &_out,
    this);

  // XXX: timeout

  return (child);
}

/**
 *  Listen the check.
 *
 *  @param[in] listnr New listener.
 */
void check::listen(listener* listnr) {
  logging::debug(logging::medium) << "check " << this
    << " is listened by " << listnr;
  _listnr = listnr;
  return ;
}

/**
 *  Read data from handle.
 *
 *  @param[in] h Handle.
 */
void check::read(handle& h) {
  char buffer[1024];
  unsigned long rb(h.read(buffer, sizeof(buffer)));
  if (&h == &_err)
    _stderr.append(buffer, rb);
  else
    _stdout.append(buffer, rb);
  return ;
}

/**
 *  Process termination callback.
 *
 *  @param[in] exit_code Process exit code.
 */
void check::terminated(int exit_code) {
  result r;
  r.set_command_id(_cmd_id);
  r.set_executed(true);
  r.set_exit_code(exit_code);
  r.set_error(_stderr);
  r.set_output(_stdout);
  _send_result_and_unregister(r);
  return ;
}

/**
 *  Unlisten the check.
 *
 *  @param[in] listnr Old listener.
 */
void check::unlisten(listener* listnr) {
  logging::debug(logging::medium) << "listener " << listnr
    << " stops listening check " << this;
  _listnr = NULL;
  return ;
}

/**
 *  Write callback.
 *
 *  @param[in] h Unused.
 */
void check::write(handle& h) {
  // This is an error, we shouldn't have been called.
  (void)h;
  result r;
  r.set_command_id(_cmd_id);
  _send_result_and_unregister(r);
  return ;
}

/**************************************
*                                     *
*           Private Methods           *
*                                     *
**************************************/

/**
 *  Copy constructor.
 *
 *  @param[in] c Unused.
 */
check::check(check const& c) : handle_listener(c) {
  _internal_copy(c);
}

/**
 *  Assignment operator.
 *
 *  @param[in] c Unused.
 *
 *  @return This object.
 */
check& check::operator=(check const& c) {
  _internal_copy(c);
  return (*this);
}

/**
 *  Copy internal data members.
 *
 *  @param[in] c Object to copy.
 */
void check::_internal_copy(check const& c) {
  (void)c;
  assert(!"check is not copyable");
  abort();
  return ;
}

/**
 *  Send check result and unregister.
 *
 *  @param[in] r Check result.
 */
void check::_send_result_and_unregister(result const& r) {
  // Check that we haven't already send a check result.
  if (_cmd_id) {
    // Unregister from multiplexer.
    multiplexer::instance().handle_manager::remove(this);

    // Reset command ID.
    _cmd_id = 0;

    // Send check result to listener.
    if (_listnr)
      _listnr->on_result(r);
  }

  return ;
}