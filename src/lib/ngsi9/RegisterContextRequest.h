#ifndef SRC_LIB_NGSI9_REGISTERCONTEXTREQUEST_H_
#define SRC_LIB_NGSI9_REGISTERCONTEXTREQUEST_H_

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

#include "convenience/RegisterProviderRequest.h"
#include "ngsi/ContextRegistrationVector.h"
#include "ngsi/Duration.h"
#include "ngsi/RegistrationId.h"



/* ****************************************************************************
*
* RegisterContextRequest - 
*/
typedef struct RegisterContextRequest
{
  ContextRegistrationVector  contextRegistrationVector;  // Mandatory
  Duration                   duration;                   // Optional
  RegistrationId             registrationId;             // Optional

  std::string                servicePath;                // Not part of payload, just an internal field

  std::string   toJsonV1(void);
  std::string   check(ApiVersion apiVersion, const std::string& predetectedError, int counter);
  void          release(void);
  void          fill(RegisterProviderRequest& rpr, const std::string& entityId, const std::string& entityType, const std::string& attributeName);
} RegisterContextRequest;

#endif  // SRC_LIB_NGSI9_REGISTERCONTEXTREQUEST_H_
