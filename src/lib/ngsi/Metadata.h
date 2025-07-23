#ifndef SRC_LIB_NGSI_METADATA_H_
#define SRC_LIB_NGSI_METADATA_H_

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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "common/globals.h"

#include "orionTypes/OrionValueType.h"
#include "ngsi/Request.h"
#include "parse/CompoundValueNode.h"

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"



/* ****************************************************************************
*
* Defines -
*
* Metadata interpreted by Orion Context Broker, i.e. not custom metadata
*/
#define NGSI_MD_EVAL_PRIORITY      "evalPriority"
#define NGSI_MD_IGNORE_TYPE        "ignoreType"
#define NGSI_MD_PREVIOUSVALUE      "previousValue"   // Special metadata
#define NGSI_MD_ACTIONTYPE         "actionType"      // Special metadata
#define NGSI_MD_DATECREATED        "dateCreated"     // Special metadata
#define NGSI_MD_DATEMODIFIED       "dateModified"    // Special metadata
#define NGSI_MD_ALL                "*"               // Special metadata (alias meaning "all metadata")
#define NGSI_MD_ACTIONTYPE_UPDATE  "update"
#define NGSI_MD_ACTIONTYPE_APPEND  "append"
#define NGSI_MD_ACTIONTYPE_DELETE  "delete"          // FIXME #1494: reserved for future use



/* ****************************************************************************
*
* Metadata -
*
*/
typedef struct Metadata
{
  std::string  name;         // Mandatory
  std::string  type;         // Optional

  bool         typeGiven;    // Was 'type' part of the incoming payload?

  orion::ValueType           valueType;    // Type of value: taken from JSON parse
  std::string                stringValue;  // "value" as a String
  double                     numberValue;  // "value" as a Number
  bool                       boolValue;    // "value" as a Boolean
  orion::CompoundValueNode*  compoundValueP;

  bool                       shadowed;     // shadowed true means that the metadata is rendered only if explicitly required
                                           // in metadata filter (typically for builtin metadata)

  Metadata();
  Metadata(Metadata* mP, bool useDefaultType = false);
  Metadata(const std::string& _name, const std::string& _type, const char* _value);
  Metadata(const std::string& _name, const std::string& _type, const std::string& _value);
  Metadata(const std::string& _name, const std::string& _type, double _value);
  Metadata(const std::string& _name, const std::string& _type, bool _value);
  Metadata(const std::string& _name, const orion::BSONObj& mdB);
  ~Metadata();

  std::string  toJson(void);
  void         release(void);
  bool         compoundItemExists(const std::string& compoundPath, orion::CompoundValueNode** compoundItemPP = NULL);

  void         appendToBson(orion::BSONObjBuilder* md, orion::BSONArrayBuilder* mdNames);

  void         addToContext(ExprContextObject* exprContextObjectP, bool legacy);

  std::string  check(void);
} Metadata;

#endif  // SRC_LIB_NGSI_METADATA_H_
