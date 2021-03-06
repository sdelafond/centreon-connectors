/*
** Copyright 2012-2013 Centreon
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

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "com/centreon/clib.hh"
#include "com/centreon/exceptions/basic.hh"
#include "com/centreon/io/file_stream.hh"
#include "com/centreon/process.hh"
#include "test/connector/misc.hh"
#include "test/connector/paths.hh"

#define LOG_FILE "/tmp/toto"

using namespace com::centreon;

#define CMD1 "2\0" \
             "4242\0" \
             "5\0" \
             "123456789\0"
#define CMD2 "\0\0\0\0"
#define RESULT "3\0" \
               "4242\0" \
               "1\0" \
               "0\0" \
               " \0" \
               "Centreon is wonderful\n\0\0\0\0"

/**
 *  Check that connector can execute a script with a log file.
 *  The goal is first to verify the log file is well written.
 *
 *  @return 0 on success.
 */
int main() {
  // If the log file exists, we remove it.
  std::ifstream f(LOG_FILE);
  if (f.good()) {
    f.close();
    remove(LOG_FILE);
  }

  clib::load();
  // Write Perl script.
  std::string script_path(io::file_stream::temp_path());
  write_file(
    script_path.c_str(),
    "#!/usr/bin/perl\n" \
    "\n" \
    "print \"Centreon is wonderful\\n\";\n" \
    "exit 0;\n");

  // Process.
  process p;
  p.enable_stream(process::in, true);
  p.enable_stream(process::out, true);
  p.exec(CONNECTOR_PERL_BINARY " --log-file " LOG_FILE);

  // Write command.
  std::ostringstream oss;
  oss.write(CMD1, sizeof(CMD1) - 1);
  oss << script_path;
  oss.write(CMD2, sizeof(CMD2) - 1);
  std::string cmd(oss.str());
  char const* ptr(cmd.c_str());
  unsigned int size(cmd.size());
  while (size > 0) {
    unsigned int rb(p.write(ptr, size));
    size -= rb;
    ptr += rb;
  }
  p.enable_stream(process::in, false);

  // Read reply.
  std::string output;
  while (true) {
    std::string buffer;
    p.read(buffer);
    if (buffer.empty())
      break;
    output.append(buffer);
  }

  // Wait for process termination.
  int retval(1);
  if (!p.wait(5000)) {
    p.terminate();
    p.wait();
  }
  else
    retval = (p.exit_code() != 0);

  // Remove temporary files.
  remove(script_path.c_str());

  clib::unload();

  try {
    if (retval)
      throw (basic_error() << "invalid return code: " << retval);
    if (output.size() != (sizeof(RESULT) - 1)
        || memcmp(output.c_str(), RESULT, sizeof(RESULT) - 1))
      throw (basic_error()
             << "invalid output: size=" << output.size()
             << ", output=" << replace_null(output));

    std::string line;
    std::ifstream file(LOG_FILE);
    if (file.is_open()) {
      getline(file, line);
      if (line.find("[info] Centreon Perl Connector " CENTREON_CONNECTOR_PERL_VERSION " starting") == std::string::npos)
        throw (basic_error()
               << "bad content: the first line does not start with 'Centreon Perl Connector " CENTREON_CONNECTOR_PERL_VERSION " starting'");
      file.close();
    }
    else {
      throw (basic_error()
             << "the file " LOG_FILE " has not been created.");
    }
  }
  catch (std::exception const& e) {
    retval = 1;
    std::cerr << "error: " << e.what() << std::endl;
  }

  return retval;
}
