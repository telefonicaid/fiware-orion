#ifndef UPDATE_CONTEXT_ELEMENT_RESPONSE_H
#define UPDATE_CONTEXT_ELEMENT_RESPONSE_H

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

#include "common/Format.h"
#include "convenience/ContextAttributeResponseVector.h"
#include "ngsi/ErrorCode.h"



/* ****************************************************************************
*
* UpdateContextElementResponse - 
*
* FIXME P5: AppendContextElementResponse and UpdateContextElementResponse are
*           identical. They should 'merge' into ONE struct.
*           This problem origins from an ?error? in the ngsi10 binding doc by NEC
*           See: https://github.com/telefonicaid/fiware-orion/issues/106
*/
typedef struct UpdateContextElementResponse
{
  ContextAttributeResponseVector   contextResponseVector;      // Optional, but mandatory if success
  ErrorCode                        errorCode;                  // Optional, but mandatory if failure

  std::string render(Format format, std::string indent);
  std::string check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void        present();
  void        release();
} UpdateContextElementResponse;

#endif
