#ifndef SRC_LIB_NGSI_ENTITYID_H_
#define SRC_LIB_NGSI_ENTITYID_H_

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

#include "ngsi/Request.h"
#include "common/Format.h"



/* ****************************************************************************
*
* EntityId - 
*/
class EntityId
{
 public:
  std::string  id;           // Mandatory
  std::string  type;         // Optional
  std::string  isPattern;    // Optional

  std::string  servicePath;  // Not part of payload, just an internal field
  std::string  keyName;      // Help variable for the 'render' method

  EntityId();
  EntityId(EntityId* eP);
  EntityId(const std::string&  _id,
           const std::string&  _type,
           const std::string&  _isPattern = "",
           const std::string&  _keyName = "entityId");

  void         keyNameSet(const std::string& _keyName);
  void         fill(const std::string& _id, const std::string& _type, const std::string& _isPattern);
  void         fill(const struct EntityId* eidP, bool useDefaultType = false);
  void         present(const std::string& indent, int ix);
  void         release(void);
  std::string  toString(bool useIsPattern = false, const std::string& delimiter = ", ");
  bool         equal(EntityId* eP);
  bool         isPatternIsTrue(void);

  std::string  render(const std::string&  indent,
                      bool                comma      = false,
                      bool                isInVector = false);

  std::string  check(ConnectionInfo*      ciP,
                     RequestType          requestType,
                     const std::string&   indent,
                     const std::string&   predetectedError,
                     int                  counter);
};

#endif  // SRC_LIB_NGSI_ENTITYID_H_
