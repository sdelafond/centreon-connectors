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

#ifndef CCCS_POLICY_HH
#  define CCCS_POLICY_HH

#  include <map>
#  include <set>
#  include "com/centreon/concurrency/mutex.hh"
#  include "com/centreon/connector/ssh/checks/listener.hh"
#  include "com/centreon/connector/ssh/namespace.hh"
#  include "com/centreon/connector/ssh/orders/listener.hh"
#  include "com/centreon/connector/ssh/orders/parser.hh"
#  include "com/centreon/connector/ssh/reporter.hh"
#  include "com/centreon/connector/ssh/sessions/credentials.hh"
#  include "com/centreon/connector/ssh/sessions/listener.hh"
#  include "com/centreon/connector/ssh/threaded_method.hh"
#  include "com/centreon/handle_listener.hh"
#  include "com/centreon/io/file_stream.hh"

CCCS_BEGIN()

// Forward declarations.
namespace            checks {
  class              check;
}
namespace            sessions {
  class              session;
}

/**
 *  @class policy policy.hh "com/centreon/connector/ssh/policy.hh"
 *  @brief Software policy.
 *
 *  Manage program execution.
 */
class                policy : public orders::listener,
                              public handle_listener,
                              public sessions::listener,
                              public checks::listener {
public:
                     policy();
                     ~policy() throw ();
  bool               run();

  // Orders listener.
  void               on_eof();
  void               on_error();
  void               on_execute(
                       unsigned long long cmd_id,
                       time_t timeout,
                       std::string const& host,
                       std::string const& user,
                       std::string const& password,
                       std::string const& cmd);
  void               on_quit();
  void               on_version();

  // Handle listener.
  void               error(handle& h);
  void               read(handle& h);
  bool               want_read(handle& h);
  bool               want_write(handle& h);
  void               write(handle& h);

  // Session listener.
  void               on_close(sessions::session& s);
  void               on_connected(sessions::session& s);
  void               on_error(sessions::session& s);

  // Check listener.
  void               on_result(
                       checks::result const& r,
                       checks::check* c = NULL);

private:
                     policy(policy const& p);
  policy&            operator=(policy const& p);
  void               _add(sessions::session* sess);
  void               _add(checks::check* chk, sessions::session* sess);
  void               _internal_copy(policy const& p);
  void               _process_io(sessions::session* sess, bool out);
  void               _remove(checks::check* chk);
  void               _remove(sessions::session* sess);

  std::map<checks::check*, sessions::session*>
                     _checks;
  std::map<sessions::credentials, sessions::session*>
                     _creds;
  bool               _error;
  concurrency::mutex _mutex;
  orders::parser     _parser;
  reporter           _reporter;
  std::map<sessions::session*, std::set<checks::check*> >
                     _sessions;
  io::file_stream    _sin;
  io::file_stream    _sout;
  std::list<threaded_method<sessions::session, void>*>
                     _threads;
};

CCCS_END()

#endif // !CCCS_POLICY_HH
