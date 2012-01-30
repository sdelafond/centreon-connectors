/*
** Copyright 2012 Merethis
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

#ifndef CCCS_LIBSSH_THREADS_CLIB_HH
#  define CCCS_LIBSSH_THREADS_CLIB_HH

#  include <libssh/callbacks.h>
#  include "com/centreon/connector/ssh/namespace.hh"

CCCS_BEGIN()

void init_libssh_threads_clib(ssh_threads_callbacks_struct* c);

CCCS_END()

#endif // !CCCS_LIBSSH_THREADS_CLIB_HH
