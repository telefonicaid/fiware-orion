#ifndef SRC_LIB_MONGOBACKEND_MONGOREGISTRATIONGET_H_
#define SRC_LIB_MONGOBACKEND_MONGOREGISTRATIONGET_H_

/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "rest/HttpStatusCode.h"
#include "rest/OrionError.h"
#include "apiTypesV2/Registration.h"



/* ****************************************************************************
*
* mongoRegistrationGet - 
*/
extern void mongoRegistrationGet
(
  ngsiv2::Registration*  regP,
  const std::string&     regId,
  const std::string&     tenant,
  const std::string&     servicePathV,
  OrionError*            oeP
);



/* ****************************************************************************
*
* mongoRegistrationsGet - 
*/
extern void mongoRegistrationsGet
(
  std::vector<ngsiv2::Registration>*  regV,
  const std::string&                  tenant,
  const std::vector<std::string>&     servicePathV,
  int                                 offset,
  int                                 limit,
  long long*                          countP,
  OrionError*                         oeP
);

#endif  // SRC_LIB_MONGOBACKEND_MONGOREGISTRATIONGET_H_
