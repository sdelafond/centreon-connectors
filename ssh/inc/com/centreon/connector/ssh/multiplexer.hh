/*
** Copyright 2011-2013 Centreon
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** For more information : contact@centreon.com
*/

#ifndef CCCS_MULTIPLEXER_HH
#  define CCCS_MULTIPLEXER_HH

#  include "com/centreon/handle_manager.hh"
#  include "com/centreon/task_manager.hh"
#  include "com/centreon/connector/ssh/namespace.hh"

CCCS_BEGIN()

/**
 *  @class multiplexer multiplexer.hh "com/centreon/connector/ssh/multiplexer.hh"
 *  @brief Multiplexing class.
 *
 *  Singleton that aggregates multiplexing features such as file
 *  descriptor monitoring and task execution.
 */
class                 multiplexer
  : public com::centreon::task_manager,
    public com::centreon::handle_manager {
public:
                      ~multiplexer() throw ();
  static multiplexer& instance() throw ();
  static void         load();
  static void         unload();

private:
                      multiplexer();
                      multiplexer(multiplexer const& m);
  multiplexer&        operator=(multiplexer const& m);
};

CCCS_END()

#endif // !CCCS_MULTIPLEXER_HH
