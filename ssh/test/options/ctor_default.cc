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

#include "com/centreon/connector/ssh/options.hh"

using namespace com::centreon::connector::ssh;

/**
 *  Check if SSH connector options are properly default-constructed.
 *
 *  @return 0 on success.
 */
int main() {
  // Object.
  options opts;

  // Check of non-existing arguments.
  int retval(0);
  try {
    opts.get_argument('x');
    retval |= 1;
  }
  catch (...) {}
  try {
    opts.get_argument("y");
    retval |= 1;
  }
  catch (...) {}
  try {
    opts.get_argument('z');
    retval |= 1;
  }
  catch (...) {}

  // Check of existing arguments.
  return (retval
          || opts.get_argument('d').get_is_set()
          || opts.get_argument('h').get_is_set()
          || opts.get_argument('v').get_is_set());
}
