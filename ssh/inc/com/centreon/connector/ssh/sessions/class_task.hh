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

#ifndef CCCS_SESSIONS_CLASS_TASK_HH
#  define CCCS_SESSIONS_CLASS_TASK_HH

#  include "com/centreon/connector/ssh/namespace.hh"
#  include "com/centreon/task.hh"

CCCS_BEGIN()

/**
 *  @class class_task class_task.hh "com/centreon/connector/ssh/class_task.hh"
 *  @brief Execute a class method.
 *
 *  Task used to execute a method of a class.
 */
template <typename T>
class         class_task : public com::centreon::task {
public:
  /**
   *  Constructor.
   *
   *  @param[in] obj  Target object.
   *  @param[in] meth Target method.
   */
              class_task(T* obj, void (T::* meth)())
    : _meth(meth), _obj(obj) {}

  /**
   *  Copy constructor.
   *
   *  @param[in] ct Object to copy.
   */
              class_task(class_task const& ct)
    : com::centreon::task(ct) {
    _internal_copy(ct);
  }

  /**
   *  Destructor.
   */
              ~class_task() throw () {}

  /**
   *  Assignment operator.
   *
   *  @param[in] ct Object to copy.
   *
   *  @return This object.
   */
  class_task& operator=(class_task const& ct) {
    if (this != &ct) {
      com::centreon::task::operator=(ct);
      _internal_copy(ct);
    }
    return (*this);
  }

  /**
   *  Run.
   */
  void        run() {
    (_obj->*_meth)();
    return ;
  }

private:
  /**
   *  Copy internal data members.
   *
   *  @param[in] ct Object to copy.
   */
  void        _internal_copy(class_task const& ct) {
    _meth = ct._meth;
    _obj = ct._obj;
    return ;
  }

  void (T::*  _meth)();
  T*          _obj;
};

CCCS_END()

#endif // !CCCS_SESSIONS_CLASS_TASK_HH
