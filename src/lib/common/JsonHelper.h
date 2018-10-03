#ifndef SRC_LIB_COMMON_JSONHELPER_H_
#define SRC_LIB_COMMON_JSONHELPER_H_

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

#include <sstream>
#include <string>
#include <vector>
#include <map>


// FIXME P10: should be renamed to JsonObjectHelper
class JsonHelper
{
public:
  JsonHelper();

  void        addString(const std::string& key, const std::string& value);
  void        addRaw(const std::string& key, const std::string& value);
  void        addNumber(const std::string& key, long long value);
  void        addNumber(const std::string& key, double value);
  void        addDate(const std::string& key, long long timestamp);
  void        addBool(const std::string& key, bool b);

  std::string str();

private:
 std::string  ss;
 bool         empty;
};


class JsonVectorHelper
{
public:
  JsonVectorHelper();

  void        addString(const std::string& value);
  void        addRaw(const std::string& value);
  void        addNumber(long long value);
  void        addNumber(double value);
  void        addDate(long long timestamp);
  void        addBool(bool b);

  std::string str();

private:
 std::string  ss;
 bool         empty;
};



/* ****************************************************************************
*
* toJsonString -
*/
std::string toJsonString(const std::string& input);



/* ****************************************************************************
*
* vectorToJson -
*/
template <class T>
std::string vectorToJson(std::vector<T> &list)
{
  typedef typename std::vector<T>::size_type size_type;

  if (list.empty())
  {
    return "[]";
  }

  std::string ss;

  ss += '[';
  ss += list[0].toJson();
  for (size_type i = 1; i != list.size(); ++i)
  {
    ss += ',';
    ss += list[i].toJson();
  }
  ss += ']';
  return ss;
}

template <>
std::string vectorToJson(std::vector<std::string> &list);



/* ****************************************************************************
*
* objectToJson -
*/
extern std::string objectToJson(std::map<std::string, std::string>& list);

#endif  // SRC_LIB_COMMON_JSONHELPER_H_
