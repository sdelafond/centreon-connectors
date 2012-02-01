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

#ifndef CCCS_THREADED_METHOD_HH
#  define CCCS_THREADED_METHOD_HH

#  include <assert.h>
#  include <stdlib.h>
#  include "com/centreon/concurrency/thread.hh"
#  include "com/centreon/connector/ssh/namespace.hh"

CCCS_BEGIN()

/**
 *  @class threaded_method threaded_method.hh "com/centreon/connector/ssh/threaded_method.hh"
 *  @brief Threaded method call.
 *
 *  Call an object's method within a thread.
 */
template <typename T, typename U>
class              threaded_method : public concurrency::thread {
public:
  /**
   *  Constructor.
   *
   *  @param[in] obj  Target object.
   *  @param[in] meth Target method.
   */
                   threaded_method(T* obj, U (T::* meth)())
    : _meth(meth), _obj(obj) {}

  /**
   *  Destructor.
   */
                   ~threaded_method() throw () {}

  /**
   *  Get the object.
   *
   *  @return Object pointer.
   */
  T*               get_object() const throw () {
    return (_obj);
  }

private:
  /**
   *  Copy constructor.
   *
   *  @param[in] tm Object to copy.
   */
                   threaded_method(threaded_method const& tm) {
    _internal_copy(tm);
  }

  /**
   *  Assignment operator.
   *
   *  @param[in] tm Object to copy.
   *
   *  @return This object.
   */
  threaded_method& operator=(threaded_method const& tm) {
    _internal_copy(tm);
    return (*this);
  }

  /**
   *  Calls abort().
   *
   *  @param[in] tm Unused.
   */
  void             _internal_copy(threaded_method const& tm) {
    (void)tm;
    assert(!"threaded_method is not copyable");
    abort();
    return ;
  }

  /**
   *  Thread entry point.
   */
  void             _run() {
    try {
      (_obj->*_meth)();
    }
    catch (...) {}
    return;
  }

  U                (T::* _meth)();
  T*               _obj;
};

CCCS_END()

#endif // !CCCS_THREADED_METHOD_HH
