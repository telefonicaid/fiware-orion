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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "ngsi/MetadataVector.h"
#include "common/Format.h"
#include "ngsi/Request.h"
#include "parse/CompoundValueNode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ContextAttribute -
*/
typedef struct ContextAttribute
{
  std::string     name;            // Mandatory
  std::string     type;            // Optional
  std::string     value;           // Optional (FI-WARE changes - MANDATORY in OMA spec)
                                   //          Especially for the new convops, value is NOT mandapory
                                   //          E.g. /v1/contextTypes
  MetadataVector  metadataVector;  // Optional

  std::string                typeFromXmlAttribute;
  orion::CompoundValueNode*  compoundValueP;

  ContextAttribute();
  ContextAttribute(ContextAttribute* caP);
  ContextAttribute(const std::string& _name, const std::string& _type, const std::string& _value = "");
  ContextAttribute(const std::string& _name, const std::string& _type, orion::CompoundValueNode* _compoundValueP);

  /* Grabbers for metadata to which CB gives a special semantic */
  std::string  getId();
  std::string  getLocation();

  std::string  render(ConnectionInfo* ciP, const std::string& indent, bool comma = false, bool omitValue = false);
  std::string  renderAsJsonObject(ConnectionInfo* ciP, const std::string& indent, bool comma, bool omitValue = false);
  void         present(const std::string& indent, int ix);
  void         release(void);
  std::string  toString(void);

  std::string  check(RequestType         requestType,
                     Format              format,
                     const std::string&  indent,
                     const std::string&  predetectedError,
                     int                 counter);
} ContextAttribute;

#endif  // SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_
