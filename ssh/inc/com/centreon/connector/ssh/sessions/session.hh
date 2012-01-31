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

#ifndef CCCS_SESSIONS_SESSION_HH
#  define CCCS_SESSIONS_SESSION_HH

#  include <libssh/libssh.h>
#  include <set>
#  include "com/centreon/connector/ssh/namespace.hh"
#  include "com/centreon/connector/ssh/sessions/credentials.hh"

CCCS_BEGIN()

namespace                 sessions {
  // Forward declaration.
  class                   listener;

  /**
   *  @class session session.hh "com/centreon/connector/ssh/session.hh"
   *  @brief SSH session.
   *
   *  SSH session between Centreon Connector SSH and a remote
   *  host. The session is kept open as long as needed.
   */
  class                   session {
  public:
                          session(credentials const& creds);
                          ~session() throw ();
    void                  close();
    void                  connect();
    credentials const&    get_credentials() const throw ();
    ssh_session           get_libssh_session() const throw ();
    bool                  is_connected() const throw ();
    void                  listen(listener* listnr);
    void                  unlisten(listener* listnr);

  private:
                          session(session const& s);
    session&              operator=(session const& s);
    void                  _on_connected() throw ();
    void                  _on_error() throw ();

    credentials           _creds;
    std::set<listener*>   _listnrs;
    ssh_session           _session;
  };
}

CCCS_END()

#endif // !CCCS_SESSIONS_SESSION_HH
