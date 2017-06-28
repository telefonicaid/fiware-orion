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

#include <string>
#include <vector>
#include <map>

#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"


class JsonHelper {
public:
  JsonHelper(int indent = -1);

  void Null();
  void Bool(bool b);
  void Date(const std::string& key, long long value);
  void Date(long long value);
  void Double(const std::string& key, double d);
  void Double(double d);
  void Int(const std::string& key, int64_t i);
  void Int(int64_t i);
  void Uint(const std::string& key, uint64_t i);
  void Uint(uint64_t i);
  void String(const std::string& key, const std::string& value);
  void String(const std::string& value);
  void StartObject(const std::string& key);
  void StartObject();
  void Key(const std::string& key);
  void EndObject();
  void StartArray(const std::string& key);
  void StartArray();
  void EndArray();

  std::string str();

private:
  int                                              indent;
  rapidjson::StringBuffer                          sb;
  rapidjson::Writer<rapidjson::StringBuffer>       writer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> pwriter;
};



/* ****************************************************************************
*
* vectorToJson -
*/
template <class T>
void vectorToJson
(
  JsonHelper& writer,
  std::vector<T> &list
)
{
  typedef typename std::vector<T>::size_type size_type;

  writer.StartArray();

  for (size_type i = 0; i != list.size(); ++i)
  {
    list[i].toJson(writer);
  }

  writer.EndArray();
}

template <>
void vectorToJson
(
  JsonHelper& writer,
  std::vector<std::string>& list
);



/* ****************************************************************************
*
* objectToJson -
*/
extern void objectToJson
(
  JsonHelper& writer,
  std::map<std::string, std::string>& list
);

#endif  // SRC_LIB_COMMON_JSONHELPER_H_
