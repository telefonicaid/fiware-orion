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

#include "mongo/client/dbclient.h"

#include "orionTypes/OrionValueType.h"
#include "ngsi/Request.h"
#include "parse/CompoundValueNode.h"



/* ****************************************************************************
*
* Defines -
*
* Metadata interpreted by Orion Context Broker, i.e. not custom metadata
*/
#define NGSI_MD_ID                 "ID"
#define NGSI_MD_LOCATION           "location"
#define NGSI_MD_PREVIOUSVALUE      "previousValue"   // Special metadata
#define NGSI_MD_ACTIONTYPE         "actionType"      // Special metadata
#define NGSI_MD_DATECREATED        "dateCreated"     // Special metadata
#define NGSI_MD_DATEMODIFIED       "dateModified"    // Special metadata
#define NGSI_MD_ALL                "*"               // Special metadata (alias meaning "all metadata")
#define NGSI_MD_ACTIONTYPE_UPDATE  "update"
#define NGSI_MD_ACTIONTYPE_APPEND  "append"
#define NGSI_MD_ACTIONTYPE_DELETE  "delete"          // FIXME #1494: reserved for future use

#if 0
// FIXME #920: disabled by the moment, maybe removed at the end
#define NGSI_MD_NOTIF_ONSUBCHANGE  "ngsi:onSubscriptionChange"
#endif



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

  double                     createdAt;
  double                     modifiedAt;

  Metadata();
  Metadata(Metadata* mP, bool useDefaultType = false);
  Metadata(const std::string& _name, const std::string& _type, const char* _value);
  Metadata(const std::string& _name, const std::string& _type, const std::string& _value);
  Metadata(const std::string& _name, const std::string& _type, double _value);
  Metadata(const std::string& _name, const std::string& _type, bool _value);
  Metadata(const char* _name, mongo::BSONObj* mdB);
  ~Metadata();

  std::string  render(bool comma);
  std::string  toJson(bool isLastElement);
  void         release(void);
  void         fill(const struct Metadata& md);
  std::string  toStringValue(void) const;
  bool         compoundItemExists(const std::string& compoundPath, orion::CompoundValueNode** compoundItemPP = NULL);

  std::string  check(ApiVersion apiVersion);
} Metadata;

#endif  // SRC_LIB_NGSI_METADATA_H_
