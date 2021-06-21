#ifndef SRC_LIB_COMMON_SYNCQOVERFLOW_H_
#define SRC_LIB_COMMON_SYNCQOVERFLOW_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Orion dev team
*/

#include <queue>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

/* ****************************************************************************
*
* template class SyncQOverflow<>-
*/
template <typename Data>
class SyncQOverflow
{
private:
    std::queue<Data> queue;
    mutable boost::mutex mtx;
    boost::condition_variable addedElement;
    size_t max_size;

public:
    SyncQOverflow(size_t sz): max_size(sz) {}
    bool try_push(Data element, bool unstoppable = false);
    Data pop();
    size_t size() const;
};

/* ****************************************************************************
*
* SyncQOverflow<Data>::try_push -
*/
template <typename Data>
bool SyncQOverflow<Data>::try_push(Data element, bool unstoppable)
{
  boost::mutex::scoped_lock lock(mtx);

  if (unstoppable || (queue.size() < max_size))
    {
      queue.push(element);
      lock.unlock();
      addedElement.notify_one();
      return true;
    }
  return false;
}

/* ****************************************************************************
*
* SyncQOverflow<Data>::pop -
*/
template <typename Data>
Data SyncQOverflow<Data>::pop()
{
  boost::mutex::scoped_lock lock(mtx);
  while(queue.empty())
    {
      addedElement.wait(lock);
    }

  Data element=queue.front();
  queue.pop();
  return element;
}

/* ****************************************************************************
*
* SyncQOverflow<Data>::size -
*/
template <typename Data>
size_t SyncQOverflow<Data>::size() const
{
  boost::mutex::scoped_lock lock(mtx);

  return queue.size();
}

#endif  // SRC_LIB_COMMON_SYNCQOVERFLOW_H_
