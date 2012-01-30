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

#include <string.h>
#include "com/centreon/concurrency/mutex.hh"
#include "com/centreon/concurrency/thread.hh"
#include "com/centreon/connector/ssh/libssh_threads_clib.hh"

using namespace com::centreon;

/**************************************
*                                     *
*           Local Functions           *
*                                     *
**************************************/

/**
 *  Destroy a mutex.
 *
 *  @param[in] lock Mutex to destroy.
 *
 *  @return 0.
 */
static int clib_mutex_destroy(void** lock) {
  delete static_cast<concurrency::mutex*>(*lock);
  *lock = NULL;
  return (0);
}

/**
 *  Create a new mutex.
 *
 *  @param[out] lock New mutex.
 *
 *  @return 0 on success.
 */
static int clib_mutex_init(void** lock) {
  int retval;
  try {
    *lock = new concurrency::mutex;
    retval = 0;
  }
  catch (...) {
    *lock = NULL;
    retval = 1;
  }
  return (retval);
}

/**
 *  Lock a mutex.
 *
 *  @param[in,out] lock Mutex to lock.
 *
 *  @return 0 on success.
 */
static int clib_mutex_lock(void** lock) {
  int retval;
  try {
    static_cast<concurrency::mutex*>(*lock)->lock();
    retval = 0;
  }
  catch (...) {
    retval = 1;
  }
  return (retval);
}

/**
 *  Unlock a mutex.
 *
 *  @param[in,out] lock Mutex to unlock.
 *
 *  @return 0 on success.
 */
static int clib_mutex_unlock(void** lock) {
  int retval;
  try {
    static_cast<concurrency::mutex*>(*lock)->unlock();
    retval = 0;
  }
  catch (...) {
    retval = 1;
  }
  return (retval);
}

/**
 *  Get current thread ID.
 *
 *  @return Current thread ID.
 */
static unsigned long clib_thread_id() {
  return (static_cast<unsigned long>(
            concurrency::thread::get_current_id()));
}

/**************************************
*                                     *
*           Public Functions          *
*                                     *
**************************************/

/**
 *  Initialize libssh threads structure for compatibility with Centreon
 *  Clib.
 *
 *  @param[out] c Callback structure.
 */
void com::centreon::connector::ssh::init_libssh_threads_clib(
                                      ssh_threads_callbacks_struct* c) {
  memset(c, 0, sizeof(*c));
  c->mutex_init = &clib_mutex_init;
  c->mutex_destroy = &clib_mutex_destroy;
  c->mutex_lock = &clib_mutex_lock;
  c->mutex_unlock = &clib_mutex_unlock;
  c->thread_id = &clib_thread_id;
  return ;
}
