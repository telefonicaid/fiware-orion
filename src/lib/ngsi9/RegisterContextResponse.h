#ifndef SRC_LIB_NGSI9_REGISTERCONTEXTRESPONSE_H_
#define SRC_LIB_NGSI9_REGISTERCONTEXTRESPONSE_H_

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

#include "ngsi/StatusCode.h"
#include "ngsi/RegistrationId.h"
#include "ngsi/Duration.h"
#include "ngsi9/RegisterContextRequest.h"



/* ****************************************************************************
*
* RegisterContextResponse - 
*/
typedef struct RegisterContextResponse
{
  Duration        duration;         // Optional
  RegistrationId  registrationId;   // Mandatory
  StatusCode      errorCode;        // Optional

  RegisterContextResponse();
  ~RegisterContextResponse();
  RegisterContextResponse(RegisterContextRequest* rcrP);
  RegisterContextResponse(const std::string& _registrationId, const std::string& _duration);
  RegisterContextResponse(const std::string& _registrationId, StatusCode& _errorCode);

  std::string render(void);
  std::string check(const std::string& predetectedError, int counter);
  void        release(void);
} RegisterContextResponse;

#endif  // SRC_LIB_NGSI9_REGISTERCONTEXTRESPONSE_H_
