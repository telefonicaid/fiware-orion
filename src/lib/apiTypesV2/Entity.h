#ifndef SRC_LIB_APITYPESV2_ENTITY_H_
#define SRC_LIB_APITYPESV2_ENTITY_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "ngsi/ContextAttributeVector.h"
#include "apiTypesV2/ErrorCode.h"

struct QueryContextResponse;

/* ****************************************************************************
*
* Entity - 
*/
class Entity
{
public:
  std::string             id;               // Mandatory
  std::string             type;             // Optional
  std::string             isPattern;        // Optional
  ContextAttributeVector  attributeVector;  // Optional
  ErrorCode               errorCode;        // Optional - mandatory if not 200-OK

  std::string             servicePath;      // Not part of payload, just an internal field
  bool                    typeGiven;        // Was 'type' part of the incoming payload?

  Entity();
  ~Entity();

  std::string  render(ConnectionInfo* ciP, RequestType requestType, bool comma = false);
  std::string  check(ConnectionInfo*  ciP, RequestType requestType);
  void         present(const std::string& indent);
  void         release(void);
  void         fill(const std::string& id, const std::string& type, const std::string& isPattern, ContextAttributeVector* aVec);
  void         fill(QueryContextResponse* qcrsP);
};

#endif  // SRC_LIB_APITYPESV2_ENTITY_H_
