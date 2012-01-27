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

#include <iostream>
#include <stdlib.h>
#include "com/centreon/connector/ssh/options.hh"
#include "com/centreon/exceptions/basic.hh"

using namespace com::centreon;
using namespace com::centreon::connector::ssh;

/**
 *  Check options parsing.
 *
 *  @return 0 on success.
 */
int main() {
  // Object.
  options opts;

  int retval(EXIT_FAILURE);
  try {
    // #1
    {
      // Prepare.
      char argv1[] = "-v";
      char argv2[] = "--debug";
      char argv3[] = "--help";
      char* argv[] = { argv1, argv2, argv3, NULL };

      // Parse.
      options opts;
      opts.parse(sizeof(argv) / sizeof(*argv) - 1, argv);

      // Check.
      if (!opts.get_argument("version").get_is_set()
          || !opts.get_argument("debug").get_is_set()
          || !opts.get_argument("help").get_is_set())
        throw (exceptions::basic() << "check #1 failed");
    }

    // #2
    {
      // Prepare.
      char argv1[] = "--help";
      char argv2[] = "--merethis";
      char argv3[] = "--debug";
      char* argv[] = { argv1, argv2, argv3, NULL };

      // Parse.
      bool error(false);
      try {
        opts.parse(sizeof(argv) / sizeof(*argv) - 1, argv);
        error = true;
      }
      catch (...) {}

      // Check.
      if (error)
        throw (exceptions::basic() << "check #2 failed");
    }

    // Success.
    retval = EXIT_SUCCESS;
  }
  catch (std::exception const& e) {
    std::cout << e.what() << std::endl;
  }

  // Return check result.
  return (retval);
}
