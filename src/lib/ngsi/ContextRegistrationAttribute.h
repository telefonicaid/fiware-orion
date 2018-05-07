#ifndef SRC_LIB_NGSI_CONTEXTREGISTRATIONATTRIBUTE_H_
#define SRC_LIB_NGSI_CONTEXTREGISTRATIONATTRIBUTE_H_

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

#include "ngsi/MetadataVector.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* ContextRegistrationAttribute -
*/
typedef struct ContextRegistrationAttribute
{
  std::string     name;            // Mandatory
  std::string     type;            // Optional
  std::string     isDomain;        // Mandatory
  MetadataVector  metadataVector;  // Optional

  ContextRegistrationAttribute();
  ContextRegistrationAttribute(const std::string& _name, const std::string& _type, const std::string& _isDomain = "");
  std::string     render(bool comma);
  void            present(int ix, const std::string& indent);
  void            release(void);

  std::string     check(ApiVersion apiVersion);
} ContextRegistrationAttribute;

#endif  // SRC_LIB_NGSI_CONTEXTREGISTRATIONATTRIBUTE_H_
