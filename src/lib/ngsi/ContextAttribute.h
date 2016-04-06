#ifndef SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_
#define SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_

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

#include "ngsi/MetadataVector.h"
#include "common/Format.h"
#include "orionTypes/OrionValueType.h"
#include "ngsi/Request.h"
#include "ngsi/ProvidingApplication.h"
#include "parse/CompoundValueNode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextAttribute -
*/
typedef struct ContextAttribute
{
public:
  std::string     name;                    // Mandatory
  std::string     type;                    // Optional
  MetadataVector  metadataVector;          // Optional

  //
  // Value - Optional (FI-WARE changes - MANDATORY in OMA spec)
  //            Especially for the new convops, value is NOT mandatory
  //            E.g. /v1/contextTypes
  //
  orion::ValueType           valueType;    // Type of value: taken from JSON parse
  std::string                stringValue;  // "value" as a String
  double                     numberValue;  // "value" as a Number
  bool                       boolValue;    // "value" as a Boolean  

  ProvidingApplication       providingApplication;    // Not part of NGSI, used internally for CPr forwarding functionality
  bool                       found;                   // Not part of NGSI, used internally for CPr forwarding functionality (update case)
                                                      // It means attribute found either locally or remotely in providing application

  bool                       skip;                    // For internal use in mongoBackend - in case of 'op=append' and the attribute already exists
  orion::CompoundValueNode*  compoundValueP;
  bool                       typeGiven;               // Was 'type' part of the incoming payload?

  ~ContextAttribute();
  ContextAttribute();
  ContextAttribute(ContextAttribute* caP, bool useDefaultType = false);
  ContextAttribute(const std::string& _name, const std::string& _type, const char* _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, const std::string& _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, double _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, bool _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, orion::CompoundValueNode* _compoundValueP);

  /* Grabbers for metadata to which CB gives a special semantic */
  std::string  getId() const;
  std::string  getLocation(const std::string& apiValue ="v1") const;

  std::string  render(ConnectionInfo* ciP, RequestType request, const std::string& indent, bool comma = false, bool omitValue = false);
  std::string  renderAsJsonObject(ConnectionInfo* ciP, RequestType request, const std::string& indent, bool comma, bool omitValue = false);
  std::string  renderAsNameString(ConnectionInfo* ciP, RequestType request, const std::string& indent, bool comma = false);
  std::string  toJson(bool isLastElement, bool types, const std::string& renderMode, RequestType requestType = NoRequest);
  std::string  toJsonAsValue(ConnectionInfo* ciP);
  void         present(const std::string& indent, int ix);
  void         release(void);
  std::string  getName(void);

  /* Used to render attribute value to BSON */
  void valueBson(mongo::BSONObjBuilder& bsonAttr) const;

  /* Helper method to be use in some places wher '%s' is needed. Maybe could be merged with toString? FIXME P2 */
  std::string  getValue(void) const;

  std::string  check(ConnectionInfo*     ciP,
                     RequestType         requestType,
                     const std::string&  indent,
                     const std::string&  predetectedError,
                     int                 counter);
  ContextAttribute* clone();

private:
  void bsonAppendAttrValue(mongo::BSONObjBuilder& bsonAttr) const;

} ContextAttribute;

#endif  // SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_
