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

#ifndef CCCP_CHECKS_RESULT_HH
#  define CCCP_CHECKS_RESULT_HH

#  include <string>
#  include "com/centreon/connector/perl/namespace.hh"

CCCP_BEGIN()

namespace              checks {
  /**
   *  @class result result.hh "com/centreon/connector/perl/checks/result.hh"
   *  @brief Check result.
   *
   *  Store check result.
   */
  class                result {
  public:
                       result();
                       result(result const& r);
                       ~result();
    result&            operator=(result const& r);
    unsigned long long get_command_id() const throw ();
    std::string const& get_error() const throw ();
    bool               get_executed() const throw ();
    int                get_exit_code() const throw ();
    std::string const& get_output() const throw ();
    void               set_command_id(unsigned long long cmd_id) throw ();
    void               set_error(std::string const& error);
    void               set_executed(bool executed) throw ();
    void               set_exit_code(int code) throw ();
    void               set_output(std::string const& output);

  private:
    void               _internal_copy(result const& r);

    unsigned long long _cmd_id;
    std::string        _error;
    bool               _executed;
    int                _exit_code;
    std::string        _output;
  };
}

CCCP_END()

#endif // !CCCP_CHECKS_RESULT_HH
