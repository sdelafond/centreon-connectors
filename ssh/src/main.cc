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

#include <exception>
#include <iostream>
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <stdlib.h>
#include "com/centreon/connector/ssh/multiplexer.hh"
#include "com/centreon/connector/ssh/options.hh"
#include "com/centreon/connector/ssh/libssh_threads_clib.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/logging/file.hh"
#include "com/centreon/logging/logger.hh"

using namespace com::centreon;
using namespace com::centreon::connector::ssh;

// Should be defined by build tools.
#ifndef CENTREON_CONNECTOR_SSH_VERSION
#  define CENTREON_CONNECTOR_SSH_VERSION "(development version)"
#endif // !CENTREON_CONNECTOR_SSH_VERSION

/**
 *  Connector entry point.
 *
 *  @param[in] argc Arguments count.
 *  @param[in] argv Arguments values.
 *
 *  @return 0 on successful execution.
 */
int main(int argc, char* argv[]) {
  // Return value.
  int retval(EXIT_FAILURE);

  // Log object.
  logging::file log_file(stderr);

  // libssh struct.
  ssh_threads_callbacks_struct clib_callbacks;
  init_libssh_threads_clib(&clib_callbacks);

  try {
    // Initializations.
    logging::engine::load();
    multiplexer::load();

    // Command line parsing.
    options opts;
    try {
      opts.parse(argc - 1, argv + 1);
    }
    catch (exceptions::basic const& e) {
      std::cout << e.what() << std::endl << opts.usage() << std::endl;
      return (EXIT_FAILURE);
    }
    if (opts.get_argument("help").get_is_set()) {
      std::cout << opts.help() << std::endl;
      retval = EXIT_SUCCESS;
    }
    else if (opts.get_argument("version").get_is_set()) {
      std::cout << "Centreon Connector SSH "
        << CENTREON_CONNECTOR_SSH_VERSION << std::endl;
      retval = EXIT_SUCCESS;
    }
    else {
      // Set logging object.
      if (opts.get_argument("debug").get_is_set()) {
        logging::engine::instance().set_show_pid(true);
        logging::engine::instance().set_show_thread_id(true);
        logging::engine::instance().add(
          &log_file,
          (1ull << logging::type_debug)
          | (1ull << logging::type_info)
          | (1ull << logging::type_error),
          logging::high);
      }
      else {
        logging::engine::instance().set_show_pid(false);
        logging::engine::instance().set_show_thread_id(false);
        logging::engine::instance().add(
          &log_file,
          (1ull << logging::type_info) | (1ull << logging::type_error),
          logging::low);
      }
      logging::info(logging::low) << "Centreon Connector SSH "
        << CENTREON_CONNECTOR_SSH_VERSION << " starting";

      // Initialize libssh2
      logging::debug(logging::medium) << "initializing libssh";
      if (ssh_threads_set_callbacks(&clib_callbacks)
          || ssh_init())
        throw (basic_error() << "libssh initialization failed");
      {
        char const* version(ssh_version(LIBSSH_VERSION_INT));
        if (!version)
          throw (basic_error() << "libssh version is too old (>= "
                 << SSH_STRINGIFY(LIBSSH_VERSION) << " required)");
        logging::info(logging::low) << "libssh version "
          << version << " successfully loaded";
      }

//       // Program policy.
//       policy p;
//       retval = (p.run() ? EXIT_SUCCESS : EXIT_FAILURE);
    }
  }
  catch (std::exception const& e) {
    logging::error(logging::low) << e.what();
  }

  // Deinitializations.
  ssh_finalize();
  multiplexer::unload();
  logging::engine::unload();

  return (retval);
}
