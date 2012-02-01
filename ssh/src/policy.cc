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
#include <errno.h>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "com/centreon/concurrency/locker.hh"
#include "com/centreon/connector/ssh/checks/check.hh"
#include "com/centreon/connector/ssh/checks/result.hh"
#include "com/centreon/connector/ssh/multiplexer.hh"
#include "com/centreon/connector/ssh/policy.hh"
#include "com/centreon/connector/ssh/sessions/credentials.hh"
#include "com/centreon/connector/ssh/sessions/session.hh"
#include "com/centreon/delayed_delete.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon;
using namespace com::centreon::connector::ssh;

// Exit flag.
static volatile bool should_exit;

// Termination handle.
static void term_handler(int signum) {
  (void)signum;
  logging::info(logging::low)
    << "termination request received (SIGTERM)";
  int old_errno(errno);
  should_exit = true;
  signal(SIGTERM, SIG_DFL);
  errno = old_errno;
  return ;
}

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 */
policy::policy() : _sin(stdin), _sout(stdout) {
  // Install termination handler.
  logging::debug(logging::medium) << "installing termination handler";
  should_exit = false;
  signal(SIGTERM, &term_handler);

  // Send information back.
  multiplexer::instance().handle_manager::add(&_sout, &_reporter);

  // Listen orders.
  _parser.listen(this);

  // Parser listens stdin.
  multiplexer::instance().handle_manager::add(&_sin, &_parser);
}

/**
 *  Destructor.
 */
policy::~policy() throw () {
  try {
    // Remove from multiplexer.
    multiplexer::instance().handle_manager::remove(&_sin);
    multiplexer::instance().handle_manager::remove(&_sout);
  }
  catch (...) {}

//   // Close checks.
//   for (std::map<
//          unsigned long long,
//          std::pair<checks::check*, sessions::session*> >::iterator
//          it = _checks.begin(),
//          end = _checks.end();
//        it != end;
//        ++it) {
//     try {
//       it->second.first->unlisten(this);
//     }
//     catch (...) {}
//     delete it->second.first;
//   }
//   _checks.clear();

//   // Close sessions.
//   for (std::map<sessions::credentials, sessions::session*>::iterator
//          it = _sessions.begin(),
//          end = _sessions.end();
//        it != end;
//        ++it) {
//     try {
//       it->second->close();
//     }
//     catch (...) {}
//     delete it->second;
//   }
}

/**
 *  Run the program.
 *
 *  @return false if program terminated prematurely.
 */
bool policy::run() {
  // No error occurred yet.
  _error = false;

  // Run multiplexer.
  while (!should_exit)
    multiplexer::instance().multiplex();

  // Run as long as a check remains.
  logging::info(logging::low) << "waiting for checks to terminate";
  while (!_checks.empty())
    multiplexer::instance().multiplex();

  // Run as long as some data remains.
  logging::info(logging::low)
    << "reporting last data to monitoring engine";
  while (_reporter.can_report() && _reporter.want_write(_sout))
    multiplexer::instance().multiplex();

  return (!_error);
}

//
// Orders listener methods.
//

/**
 *  Called if stdin is closed.
 */
void policy::on_eof() {
  logging::info(logging::low) << "stdin is closed";
  on_quit();
  return ;
}

/**
 *  Called if an error occured on stdin.
 */
void policy::on_error() {
  logging::info(logging::low)
    << "error occurred while parsing stdin";
  _error = true;
  on_quit();
  return ;
}

/**
 *  Execution command received.
 *
 *  @param[in] cmd_id   Command ID.
 *  @param[in] timeout  Time the command has to execute.
 *  @param[in] host     Target host.
 *  @param[in] user     User.
 *  @param[in] password Password.
 *  @param[in] cmd      Command to execute.
 */
void policy::on_execute(
               unsigned long long cmd_id,
               time_t timeout,
               std::string const& host,
               std::string const& user,
               std::string const& password,
               std::string const& cmd) {
  concurrency::locker lock(&_mutex);
  try {
    // Log message.
    logging::info(logging::medium) << "request to execute command #"
      << cmd_id << " on " << user << "@" << host << " (" << cmd << ")";

    // Credentials.
    sessions::credentials creds;
    creds.set_host(host);
    creds.set_user(user);
    creds.set_password(password);

    // Find session.
    std::map<sessions::credentials, sessions::session*>::iterator it;
    it = _creds.find(creds);
    if (it == _creds.end()) {
      logging::info(logging::low) << "creating session for "
        << user << "@" << host;
      std::auto_ptr<sessions::session>
        sess(new sessions::session(creds));
      _add(sess.get());
      logging::debug(logging::high) << "policy " << this
        << " will listen to session " << user << "@" << host;
      sess->listen(this);
      std::auto_ptr<threaded_method<sessions::session, void> >
        conn_thread(new threaded_method<sessions::session, void>(
                      sess.release(),
                      &sessions::session::connect));
      _threads.push_back(conn_thread.get());
      logging::debug(logging::low)
        << "launching threaded connection on session "
        << user << "@" << host;
      conn_thread.release()->exec();
      it = _creds.find(creds);
    }

    // Launch check.
    std::auto_ptr<checks::check>
      chk(new checks::check(cmd_id, cmd, timeout));
    _add(chk.get(), it->second);
    chk->listen(this);
    concurrency::locker lock(it->second);
    if (it->second->is_connected())
      chk->execute(*it->second);
    chk.release();
  }
  catch (std::exception const& e) {
    logging::error(logging::low) << "could not launch check ID "
      << cmd_id << " on host " << host << " because an error occurred: "
      << e.what();
    checks::result r;
    r.set_command_id(cmd_id);
    on_result(r);
  }
  catch (...) {
    logging::error(logging::low) << "could not launch check ID "
      << cmd_id << " on host " << host << " because an error occurred";
    checks::result r;
    r.set_command_id(cmd_id);
    on_result(r);
  }

  return ;
}

/**
 *  Quit order was received.
 */
void policy::on_quit() {
  // Exiting.
  logging::info(logging::low)
    << "quit request received";
  should_exit = true;
  multiplexer::instance().handle_manager::remove(&_sin);
  return ;
}

/**
 *  Version request was received.
 */
void policy::on_version() {
  // Report version 1.0.
  logging::info(logging::medium)
    << "monitoring engine requested protocol version, sending 1.0";
  concurrency::locker lock(&_mutex);
  _reporter.send_version(1, 0);
  return ;
}

//
// Handle listener methods.
//

/**
 *  Error occurred on handle.
 *
 *  @param[in] h Handle.
 */
void policy::error(handle& h) {
  concurrency::locker lock(&_mutex);
  logging::debug(logging::medium) << "handle " << &h << " has error";
  std::map<sessions::session*, std::set<checks::check*> >::iterator
    it(_sessions.find(static_cast<sessions::session*>(&h)));
  if (it != _sessions.end()) {
    logging::info(logging::low) << "error occurred on session "
      << it->first->get_credentials().get_user() << "@"
      << it->first->get_credentials().get_host();
    _remove(it->first);
  }
  return ;
}

/**
 *  Read event on session.
 *
 *  @param[in] h Session handle.
 */
void policy::read(handle& h) {
  _process_io(static_cast<sessions::session*>(&h), false);
  return ;
}

/**
 *  Check if we want to read on the handle.
 *
 *  @param[in] h Handle to check.
 *
 *  @return true if we want to read on the handle.
 */
bool policy::want_read(handle& h) {
  // Find check set.
  std::map<sessions::session*, std::set<checks::check*> >::iterator
    sess(_sessions.find(static_cast<sessions::session*>(&h)));
  bool wr(false);
  if (sess != _sessions.end()) {
    for (std::set<checks::check*>::iterator
           it = sess->second.begin(),
           end = sess->second.end();
         !wr && (it != end);
         ++it)
      wr = (*it)->want_read();
  }
  return (wr);
}

/**
 *  Check if we want to write on the handle.
 *
 *  @param[in] h Handle to check.
 *
 *  @return true if we want to write on the handle.
 */
bool policy::want_write(handle& h) {
  // Find check set.
  concurrency::locker lock(&_mutex);
  std::map<sessions::session*, std::set<checks::check*> >::iterator
    sess(_sessions.find(static_cast<sessions::session*>(&h)));
  bool ww(false);
  if (sess != _sessions.end()) {
    for (std::set<checks::check*>::iterator
           it = sess->second.begin(),
           end = sess->second.end();
         !ww && (it != end);
         ++it)
      ww = (*it)->want_write();
  }
  return (ww);
}

/**
 *  Write event on session.
 *
 *  @param[in] h Session handle.
 */
void policy::write(handle& h) {
  _process_io(static_cast<sessions::session*>(&h), true);
  return ;
}

//
// Session listener methods.
//

/**
 *  SSH session got closed.
 *
 *  @param[in] s SSH session.
 */
void policy::on_close(sessions::session& s) {
  concurrency::locker lock(&_mutex);
  logging::info(logging::low) << "session "
    << s.get_credentials().get_user() << "@"
    << s.get_credentials().get_host() << " got closed";
  _remove(&s);
  return ;
}

/**
 *  SSH session just connected, launch associated checks.
 *
 *  @param[in] s SSH session.
 */
void policy::on_connected(sessions::session& s) {
  concurrency::locker lock(&_mutex);
  logging::info(logging::medium) << "session "
    << s.get_credentials().get_user() << "@"
    << s.get_credentials().get_host() << " successfully connected";

  // Listen session events.
  multiplexer::instance().handle_manager::add(&s, this);

  // Search session.
  std::map<sessions::session*, std::set<checks::check*> >::iterator
    sess(_sessions.find(&s));
  if (sess == _sessions.end())
    throw (basic_error() << "cannot find session "
           << s.get_credentials().get_user() << "@"
           << s.get_credentials().get_host());

  // Launch associated checks.
  for (std::set<checks::check*>::iterator
         it = sess->second.begin(),
         end = sess->second.end();
       it != end;
       ++it) {
    logging::debug(logging::medium) << "launching check " << *it;
    (*it)->execute(s);
  }

  return ;
}

/**
 *  SSH session has an error.
 *
 *  @param[in] s SSH session.
 */
void policy::on_error(sessions::session& s) {
  concurrency::locker lock(&_mutex);
  logging::info(logging::low) << "error occurred on session "
    << s.get_credentials().get_user() << "@"
    << s.get_credentials().get_host();
  _remove(&s);
  return ;
}

//
// Check listener methods.
//

/**
 *  Check result has arrived.
 *
 *  @param[in] r Check result.
 *  @param[in] c Check object.
 */
void policy::on_result(checks::result const& r, checks::check* c) {
  // Lock mutex.
  concurrency::locker lock(&_mutex);

  // Remove check from list.
  _remove(c);

  // Send check result back to monitoring engine.
  _reporter.send_result(r);

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
 *  @param[in] p Unused.
 */
policy::policy(policy const& p)
  : orders::listener(p),
    handle_listener(p),
    sessions::listener(p),
    checks::listener(p) {
  _internal_copy(p);
}

/**
 *  @brief Assignment operator.
 *
 *  Any call to this method will result in a call to abort().
 *
 *  @param[in] p Unused.
 *
 *  @return This object.
 */
policy& policy::operator=(policy const& p) {
  _internal_copy(p);
  return (*this);
}

/**
 *  Add a SSH session.
 *
 *  @param[in] sess SSH session object.
 */
void policy::_add(sessions::session* sess) {
  if (_creds.find(sess->get_credentials()) != _creds.end())
    throw (basic_error() << "attempt to register session "
           << sess->get_credentials().get_user() << "@"
           << sess->get_credentials().get_host()
           << " which is already registered");
  _creds[sess->get_credentials()] = sess;
  _sessions[sess];
  return ;
}

/**
 *  Add a check.
 *
 *  @param[in] chk  Check object.
 *  @param[in] sess Associated SSH session object.
 */
void policy::_add(checks::check* chk, sessions::session* sess) {
  _checks[chk] = sess;
  _sessions[sess].insert(chk);
  return ;
}

/**
 *  Calls abort().
 *
 *  @param[in] p Unused.
 */
void policy::_internal_copy(policy const& p) {
  (void)p;
  assert(!"policy is not copyable");
  abort();
  return ;
}

/**
 *  Process SSH session IO.
 *
 *  @param[in] sess SSH session.
 *  @param[in] out  false is this is a read event, true if it is a write
 *                  event.
 */
void policy::_process_io(sessions::session* sess, bool out) {
  concurrency::locker lock(&_mutex);
  std::map<sessions::session*, std::set<checks::check*> >::iterator
    s(_sessions.find(sess));
  if (s != _sessions.end()) {
    // Set flags.
    if (!out)
      ssh_set_fd_toread(sess->get_libssh_session());
    else
      ssh_set_fd_towrite(sess->get_libssh_session());

    // Process channels.
    for (std::set<checks::check*>::iterator
           it(s->second.begin()),
           end(s->second.end());
         it != end;
         ++it)
      (*it)->run();
  }
  return ;
}

/**
 *  Remove a check.
 *
 *  @param[in] chk Check to remove.
 */
void policy::_remove(checks::check* chk) {
  concurrency::locker lock(&_mutex);
  std::map<checks::check*, sessions::session*>::iterator
    it(_checks.find(chk));
  if (it != _checks.end()) {
    std::map<sessions::session*, std::set<checks::check*> >::iterator
      it2(_sessions.find(it->second));
    if (it2 != _sessions.end())
      it2->second.erase(chk);
    _checks.erase(it);
  }
  std::auto_ptr<delayed_delete<checks::check> >
    dd(new delayed_delete<checks::check>(chk));
  multiplexer::instance().task_manager::add(
    dd.get(),
    0,
    true,
    true);
  dd.release();
  return ;
}

/**
 *  Remove a session.
 *
 *  @param[in] sess Session to remove.
 */
void policy::_remove(sessions::session* sess) {
  concurrency::locker lock(&_mutex);

  // Stop listening.
  sess->unlisten(this);
  multiplexer::instance().handle_manager::remove(sess);

  // Remove associated checks.
  std::set<checks::check*> my_checks(_sessions[sess]);
  for (std::set<checks::check*>::iterator
         it = my_checks.begin(),
         end = my_checks.end();
       it != end;
       ++it)
    _remove(*it);

  // Remove credentials.
  _creds.erase(sess->get_credentials());

  // Remove session.
  _sessions.erase(sess);

  // Wait for connection thread (if exist).
  for (std::list<threaded_method<sessions::session, void>*>::iterator
         it = _threads.begin(),
         end = _threads.end();
       it != end;
       ++it)
    if ((*it)->get_object() == sess) {
      (*it)->wait();
      _threads.erase(it);
      break ;
    }

  // Delete session.
  std::auto_ptr<delayed_delete<sessions::session> >
    dd(new delayed_delete<sessions::session>(sess));
  multiplexer::instance().task_manager::add(
    dd.get(),
    0,
    true,
    true);
  dd.release();

  return ;
}
