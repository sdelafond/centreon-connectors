/*
** Copyright 2011-2012 Merethis
**
** This file is part of Centreon Connector SSH.
**
** Centreon Connector SSH is free software: you can redistribute it
** and/or modify it under the terms of the GNU Affero General Public
** License as published by the Free Software Foundation, either version
** 3 of the License, or (at your option) any later version.
**
** Centreon Connector SSH is distributed in the hope that it will be
** useful, but WITHOUT ANY WARRANTY; without even the implied warranty
** of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public
** License along with Centreon Connector SSH. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include "com/centreon/connector/ssh/checks/check.hh"
#include "com/centreon/connector/ssh/checks/result.hh"
#include "com/centreon/connector/ssh/checks/timeout.hh"
#include "com/centreon/connector/ssh/multiplexer.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon::connector::ssh::checks;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 *
 *  @param[in] cmd_id  Command ID.
 *  @param[in] cmd     Command to execute.
 *  @param[in] tmt     Command timeout.
 */
check::check(
         unsigned long long cmd_id,
         std::string const& cmd,
         time_t tmt)
  : _channel(NULL),
    _cmd(cmd),
    _cmd_id(cmd_id),
    _listnr(NULL),
    _session(NULL),
    _step(chan_open),
    _timeout(0) {
  // Check parameters.
  if (!cmd_id)
    throw (basic_error() << "check " << this
           << " got invalid command ID 0");

  // Log message.
  logging::debug(logging::low) << "check "
    << this << " has ID " << cmd_id;

  // Register timeout.
  logging::info(logging::high) << "registering timeout of check "
    << _cmd_id << " to execute at " << tmt;
  std::auto_ptr<timeout> t(new timeout(this));
  _timeout = multiplexer::instance().com::centreon::task_manager::add(
    t.get(),
    tmt,
    false,
    true);
  t.release();  
}

/**
 *  Destructor.
 */
check::~check() throw () {
  try {
    // Send result if we haven't already done so.
    result r;
    r.set_command_id(_cmd_id);
    _send_result_and_unregister(r);

    if (_channel) {
      // Close channel.
      logging::debug(logging::medium)
        << "closing channel of check " << this;
      ssh_channel_close(_channel);

      // Free channel.
      ssh_channel_free(_channel);
    }
  }
  catch (...) {}
}

/**
 *  Start executing a check.
 *
 *  @param[in] sess Session on which a channel will be opened.
 */
void check::execute(sessions::session& sess) {
  // Check before starting execution.
  if (!sess.is_connected())
    throw (basic_error() << "cannot run check " << _cmd_id
           << " on unconnected session " << &sess);
  if (_channel || !_cmd_id)
    throw (basic_error() << "attempt to run check " << _cmd_id
           << " which already executed");

  // Store session.
  _session = &sess;

  // Create channel.
  _channel = ssh_channel_new(_session->get_libssh_session());
  if (!_channel)
    throw (basic_error() << "cannot create new channel on session "
           << &sess << " (out of memory ?)");

  // Run check.
  logging::info(logging::high)
    << "manually launching check " << _cmd_id;
  run();

  return ;
}

/**
 *  Listen the check.
 *
 *  @param[in] listnr Listener.
 */
void check::listen(checks::listener* listnr) {
  logging::debug(logging::medium) << "check "
    << this << " is listened by " << listnr;
  _listnr = listnr;
  return ;
}

/**
 *  Perform action on channel.
 */
void check::run() {
  try {
    switch (_step) {
    case chan_open:
      logging::info(logging::high)
        << "attempting to open channel for check " << _cmd_id;
      if (!_open()) {
        logging::info(logging::high) << "check " << _cmd_id
          << " channel was successfully opened";
        _step = chan_exec;
        run();
      }
      break ;
    case chan_exec:
      logging::info(logging::high)
        << "attempting to execute check " << _cmd_id;
      if (!_exec()) {
        logging::info(logging::high)
          << "check " << _cmd_id << " was successfully executed";
        _step = chan_read;
        run();
      }
      break ;
    case chan_read:
      logging::info(logging::high)
        << "reading check " << _cmd_id << " result from channel";
      if (!_read()) {
        logging::info(logging::high) << "result of check "
          << _cmd_id << " was successfully fetched";
        _step = chan_close;
        run();
      }
      break ;
    case chan_close:
      {
        unsigned long long cmd_id(_cmd_id);
        logging::info(logging::high) << "attempting to close check "
          << cmd_id << " channel";
        if (!_close())
          logging::info(logging::medium) << "channel of check "
            << cmd_id << " successfully closed";
      }
      break ;
    default:
      throw (basic_error()
             << "channel requested to run at invalid step");
    }
  }
  catch (std::exception const& e) {
    logging::error(logging::low)
      << "error occured while executing check " << _cmd_id
      << " on session " << _session->get_credentials().get_user() << "@"
      << _session->get_credentials().get_host() << ": " << e.what();
    result r;
    r.set_command_id(_cmd_id);
    _send_result_and_unregister(r);
  }
  catch (...) {
    logging::error(logging::low)
      << "unknown error occured while executing check " << _cmd_id
      << " on session " << _session->get_credentials().get_user() << "@"
      << _session->get_credentials().get_host();
    result r;
    r.set_command_id(_cmd_id);
    _send_result_and_unregister(r);
  }
  return ;
}

/**
 *  Called when check timeout occurs.
 */
void check::on_timeout() {
  // Log message.
  logging::error(logging::low) << "check " << _cmd_id
    << " reached timeout";

  // Reset timeout task ID.
  _timeout = 0;

  // Send check result.
  result r;
  r.set_command_id(_cmd_id);
  _send_result_and_unregister(r);

  return ;
}

/**
 *  Stop listening to the check.
 *
 *  @param[in] listnr Listener.
 */
void check::unlisten(checks::listener* listnr) {
  logging::debug(logging::medium) << "listener " << listnr
    << " stops listening check " << this;
  _listnr = NULL;
  return ;
}

/**
 *  Check whether or not channel needs to read.
 *
 *  @return true if channel needs to read.
 */
bool check::want_read() {
  return (true);
}

/**
 *  Check whether or not channel needs to write.
 *
 *  @return true if channel needs to write.
 */
bool check::want_write() {
  return (_step != chan_read);
}

/**************************************
*                                     *
*           Private Methods           *
*                                     *
**************************************/

/**
 *  @brief Copy constructor.
 *
 *  Any call to this constructor will result in a call to abort().
 *
 *  @param[in] c Unused.
 */
check::check(check const& c) {
  (void)c;
  assert(!"check is not copyable");
  abort();
}

/**
 *  @brief Assignment operator.
 *
 *  Any call to this method will result in a call to abort().
 *
 *  @param[in] c Unused.
 *
 *  @return This object.
 */
check& check::operator=(check const& c) {
  (void)c;
  assert(!"check is not copyable");
  abort();
  return (*this);
}

/**
 *  Attempt to close channel.
 *
 *  @return true while channel was not closed properly.
 */
bool check::_close() {
  bool retval;

  // Check that channel was opened.
  if (_channel) {
    // Attempt to close channel.
    int ret(ssh_channel_close(_channel));
    if (ret) {
      if (ret != SSH_AGAIN) {
        char const* msg(ssh_get_error(_session->get_libssh_session()));
        throw (basic_error() << "could not close channel: " << msg);
      }
      retval = true;
    }
    // Close succeeded.
    else {
      // Get exit status.
      int exitcode(ssh_channel_get_exit_status(_channel));

      // Free channel.
      ssh_channel_free(_channel);
      _channel = NULL;

      // Method should not be called again.
      retval = false;

      // Send results to parent process.
      result r;
      r.set_command_id(_cmd_id);
      r.set_error(_stderr);
      r.set_executed(true);
      r.set_exit_code(exitcode);
      r.set_output(_stdout);
      _send_result_and_unregister(r);
    }
  }
  // Attempt to close a closed channel.
  else
    throw (basic_error()
           << "channel requested to close whereas it wasn't opened");

  return (retval);
}

/**
 *  Attempt to execute the command.
 *
 *  @return true while the command was not successfully executed.
 */
bool check::_exec() {
  // Attempt to execute command.
  int ret(ssh_channel_request_exec(_channel, _cmd.c_str()));

  // Check that we can try again later.
  if (ret && (ret != SSH_AGAIN)) {
    char const* msg(ssh_get_error(_session->get_libssh_session()));
    throw (basic_error()
             << "could not execute command on SSH channel: " << msg);
  }

  // Check whether command succeeded or if we can try again later.
  return (ret == SSH_AGAIN);
}

/**
 *  Attempt to open a channel.
 *
 *  @return true while the channel was not successfully opened.
 */
bool check::_open() {
  // Return value.
  bool retval;

  // Attempt to open channel.
  int ret(ssh_channel_open_session(_channel));
  if (ret == SSH_OK)
    retval = false;
  // Channel creation failed, check that we can try again later.
  else {
    char const* msg(ssh_get_error(_session->get_libssh_session()));
    if (ret != SSH_AGAIN)
      throw (basic_error() << "could not open SSH channel: " << msg);
    else
      retval = true;
  }

  return (retval);
}

/**
 *  Attempt to read command output.
 *
 *  @return true while command output can be read again.
 */
bool check::_read() {
  // Read command's stdout.
  char buffer[BUFSIZ];
  int orb(ssh_channel_read_nonblocking(
            _channel,
            buffer,
            sizeof(buffer),
            0));

  // Error occured.
  if (orb < 0) {
    char const* msg(ssh_get_error(_session->get_libssh_session()));
    throw (basic_error() << "failed to read command output: " << msg);
  }
  // Append data.
  else
    _stdout.append(buffer, orb);

  // Read command's stderr.
  int erb(ssh_channel_read_nonblocking(
            _channel,
            buffer,
            sizeof(buffer),
            1));
  if (erb > 0)
    _stderr.append(buffer, erb);

  // Should we read again ?
  return (ssh_channel_is_eof(_channel));
}

/**
 *  Send check result and unregister from session.
 *
 *  @param[in] r Check result.
 */
void check::_send_result_and_unregister(result const& r) {
  // Remove timeout task.
  if (_timeout) {
    try {
      multiplexer::instance().com::centreon::task_manager::remove(
        _timeout);
    }
    catch (...) {}
    _timeout = 0;
  }

  // Remove session.
  _session = NULL;

  // Check that we haven't already send a check result.
  if (_cmd_id) {
    // Reset command ID.
    _cmd_id = 0;

    // Send check result to listener.
    if (_listnr)
      _listnr->on_result(r, this);
  }

  return ;
}
