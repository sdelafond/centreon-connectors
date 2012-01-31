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
#include <stdlib.h>
#include "com/centreon/connector/ssh/multiplexer.hh"
#include "com/centreon/connector/ssh/sessions/class_task.hh"
#include "com/centreon/connector/ssh/sessions/listener.hh"
#include "com/centreon/connector/ssh/sessions/session.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon;
using namespace com::centreon::connector::ssh::sessions;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Constructor.
 *
 *  @param[in] creds Connection credentials.
 */
session::session(credentials const& creds)
  : _creds(creds),
    _session(NULL) {
  // Create session instance.
  logging::debug(logging::low) << "allocating SSH session object for "
    << _creds.get_user() << "@" << _creds.get_host();
  _session = ssh_new();
  if (!_session)
    throw (basic_error()
             << "SSH session creation failed (out of memory ?)");
}

/**
 *  Destructor.
 */
session::~session() throw () {
  // Delete session.
  logging::debug(logging::low) << "deleting SSH session object of "
    << _creds.get_user() << "@" << _creds.get_host();
  this->close();
  ssh_free(_session);
}

/**
 *  Close session.
 */
void session::close() {
  if (is_connected()) {
    // Close session.
    logging::info(logging::medium) << "gracefully disconnecting from "
      << _creds.get_user() << "@" << _creds.get_host();
    ssh_disconnect(_session);

    // Notify listeners.
    logging::debug(logging::medium) << "notifying listeners of session "
      << _creds.get_user() << "@" << _creds.get_host()
      << " that session is getting closed";
    {
      std::set<listener*> listnrs(_listnrs);
      for (std::set<listener*>::iterator
             it = listnrs.begin(),
             end = listnrs.end();
           it != end;
           ++it)
        (*it)->on_close(*this);
    }
  }

  return ;
}

/**
 *  Open session.
 */
void session::connect() {
  // Check that session wasn't already open.
  if (is_connected()) {
    logging::info(logging::high)
      << "attempt to open already opened session";
    return ;
  }

  // Set options.
  try {
    logging::info(logging::high) << "connecting to "
      << _creds.get_user() << "@" << _creds.get_host();
    logging::debug(logging::medium) << "setting options of session "
      << _creds.get_user() << "@" << _creds.get_host();
    if (!ssh_options_set(
           _session,
           SSH_OPTIONS_HOST,
           _creds.get_user().c_str())
        || !ssh_options_set(
              _session,
              SSH_OPTIONS_USER,
              _creds.get_user().c_str())) { // XXX: set timeout
      char const* msg(ssh_get_error(_session));
      throw (basic_error() << "error occurred while preparing session "
             << _creds.get_user() << "@" << _creds.get_host()
             << " for connection: " << msg);
    }

    // Connect.
    logging::debug(logging::medium)
      << "launching real connection of session "
      << _creds.get_user() << "@" << _creds.get_host();
    if (ssh_connect(_session) != SSH_OK) {
      char const* msg(ssh_get_error(_session));
      throw (basic_error() << "could not connect session "
             << _creds.get_user() << "@" << _creds.get_host()
             << ": " << msg);
    }

    // Public key authentication.
    logging::debug(logging::medium)
      << "attempting public key authentication on session "
      << _creds.get_user() << "@" << _creds.get_host();
    int ret;
    ret = ssh_userauth_autopubkey(_session, NULL);
    if (ret != SSH_AUTH_SUCCESS) {
      // Password-based authentication.
      logging::debug(logging::medium)
        << "attempting password authentication on session "
        << _creds.get_user() << "@" << _creds.get_host();
      ret = ssh_userauth_password(
              _session,
              NULL,
              _creds.get_password().c_str());
      if (ret != SSH_AUTH_SUCCESS) {
        char const* msg(ssh_get_error(_session));
        throw (basic_error() << "could not connect session "
               << _creds.get_user() << "@" << _creds.get_host()
               << ": " << msg);
      }
      else
        logging::info(logging::medium)
          << "password authentication succeeded on session "
          << _creds.get_user() << "@" << _creds.get_host();
    }
    else
      logging::info(logging::medium)
        << "public key authentication succeeded on session "
        << _creds.get_user() << "@" << _creds.get_host();

    // Asynchronously notify listeners of connection success.
    std::auto_ptr<class_task<session> >
      ct(new class_task<session>(this, &session::_on_connected));
    multiplexer::instance().task_manager::add(
      ct.get(),
      0,
      false,
      true);
    ct.release();
  }
  catch (...) {
    // Asynchronously notify listeners of connection issue.
    std::auto_ptr<class_task<session> >
      ct(new class_task<session>(this, &session::_on_error));
    multiplexer::instance().task_manager::add(
      ct.get(),
      0,
      false,
      true);
    ct.release();
    throw ;
  }

  return ;
}

/**
 *  Get the session credentials.
 *
 *  @return Credentials associated to this session.
 */
credentials const& session::get_credentials() const throw () {
  return (_creds);
}

/**
 *  Get the libssh session associated to this session.
 *
 *  @return libssh_session object.
 */
ssh_session session::get_libssh_session() const throw () {
  return (_session);
}

/**
 *  Check if session is connected.
 *
 *  @return true if session is connected.
 */
bool session::is_connected() const throw () {
  return (ssh_is_connected(_session));
}

/**
 *  Add listener to session.
 *
 *  @param[in] listnr New listener.
 */
void session::listen(listener* listnr) {
  _listnrs.insert(listnr);
  return ;
}

/**
 *  Remove a listener.
 *
 *  @param[in] listnr Listener to remove.
 */
void session::unlisten(listener* listnr) {
  unsigned int size(_listnrs.size());
  _listnrs.erase(listnr);
  logging::debug(logging::low) << "session " << this
    << " is removing listener " << listnr << " (there was "
    << size << ", there is " << _listnrs.size() << ")";
  return ;
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
 *  @param[in] s Object to copy.
 */
session::session(session const& s) : socket_handle() {
  (void)s;
  assert(!"session is not copyable");
  abort();
}

/**
 *  @brief Assignment operator.
 *
 *  Any call to this method will result in a call to abort().
 *
 *  @param[in] s Object to copy.
 *
 *  @return This object.
 */
session& session::operator=(session const& s) {
  (void)s;
  assert(!"session is not copyable");
  abort();
  return (*this);
}

/**
 *  Notify listeners that session connected.
 */
void session::_on_connected() throw () {
  try {
    set_native_handle(ssh_get_fd(_session));
    for (std::set<listener*>::iterator
           it = _listnrs.begin(),
           end = _listnrs.end();
         it != end;
         ++it)
      (*it)->on_connected(*this);
  }
  catch (...) {}
  return ;
}

/**
 *  Notify listeners that an error occurred on session.
 */
void session::_on_error() throw () {
  try {
    for (std::set<listener*>::iterator
           it = _listnrs.begin(),
           end = _listnrs.end();
         it != end;
         ++it)
      (*it)->on_error(*this);
  }
  catch (...) {}
  return ;
}
