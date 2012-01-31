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

#include "com/centreon/connector/ssh/sessions/socket_handle.hh"
#include "com/centreon/exceptions/basic.hh"

using namespace com::centreon;
using namespace com::centreon::connector::ssh::sessions;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Default constructor.
 *
 *  @param[in] handl Native socket descriptor.
 */
socket_handle::socket_handle(native_handle handl) : _handl(handl) {}

/**
 *  Destructor.
 */
socket_handle::~socket_handle() throw () {}

/**
 *  Get the native socket handle.
 *
 *  @return Native socket handle.
 */
native_handle socket_handle::get_native_handle() {
  return (_handl);
}

/**
 *  Read from socket descriptor.
 *
 *  @param[out] data Where data will be stored.
 *  @param[in]  size How much data in bytes to read at most.
 *
 *  @return Number of bytes actually read.
 */
unsigned long socket_handle::read(void* data, unsigned long size) {
  (void)data;
  (void)size;
  throw (basic_error() << "direct read attempt on socket handle "
         << _handl);
  return (0);
}

/**
 *  Set socket descriptor.
 *
 *  @param[in] handl Native socket descriptor.
 */
void socket_handle::set_native_handle(native_handle handl) {
  _handl = handl;
  return ;
}

/**
 *  Write to socket descriptor.
 *
 *  @param[in] data Data to write.
 *  @param[in] size How much data to write at most.
 *
 *  @return Number of bytes actually written.
 */
unsigned long socket_handle::write(
                               void const* data,
                               unsigned long size) {
  (void)data;
  (void)size;
  throw (basic_error() << "direct write attempt on socket handle "
         << _handl);
  return (0);
}
