#ifndef SRC_LIB_MONGOBACKEND_MONGOGETREGISTRATIONS_H_
#define SRC_LIB_MONGOBACKEND_MONGOGETREGISTRATIONS_H_

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
* Author: Fermín Galán Márquez
*/
#include <string>
#include <map>

#include "rest/OrionError.h"
#include "apiTypesV2/Registration.h"



#if 0
// Stub ready to be enabled when the time of GET /v2/registrations come

/* ****************************************************************************
*
* mongoListRegistrations -
*/
extern void mongoListRegistrations
(
  std::vector<ngsiv2::Registration>*   vec,
  OrionError*                          oe,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant,
  const std::string&                   servicePath,
  int                                  limit,
  int                                  offset,
  long long*                           count
);
#endif



/* ****************************************************************************
*
* mongoGetRegistrations -
*/
extern void mongoGetRegistrations
(
  ngsiv2::Registration*                sub,
  OrionError*                          oe,
  const std::string&                   idReg,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant
);

#endif  // SRC_LIB_MONGOBACKEND_MONGOGETREGISTRATIONS_H_
