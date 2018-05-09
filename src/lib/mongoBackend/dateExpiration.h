#ifndef SRC_LIB_MONGOBACKEND_DATEEXPIRATION_H_
#define SRC_LIB_MONGOBACKEND_DATEEXPIRATION_H_

/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*
*/
#include <string>

#include "mongo/client/dbclient.h"

#include "common/globals.h"
#include "rest/OrionError.h"
#include "ngsi/ContextAttributeVector.h"



/* ****************************************************************************
*
* processDateExpirationAtEntityCreation -
*/
extern bool processDateExpirationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  mongo::Date_t*                 dateExpiration,
  std::string*                   errDetail,
  OrionError*                    oe
);



/* ****************************************************************************
*
* processDateExpirationAtUpdateAttribute -
*/
extern bool processDateExpirationAtUpdateAttribute
(
  const ContextAttribute*  targetAttr,
  mongo::Date_t*           dateExpiration,
  bool*                    replaceDate,
  std::string*             errDetail,
  OrionError*              oe
);



/* ****************************************************************************
*
* processDateExpirationAtAppendAttribute -
*/
extern bool processDateExpirationAtAppendAttribute
(
  mongo::Date_t*           dateExpiration,
  const ContextAttribute*  targetAttr,
  bool                     actualAppend,
  std::string*             errDetail,
  OrionError*              oe
);

#endif  // SRC_LIB_MONGOBACKEND_DATEEXPIRATION_H_
