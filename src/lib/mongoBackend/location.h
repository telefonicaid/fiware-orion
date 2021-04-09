#ifndef SRC_LIB_MONGOBACKEND_LOCATION_H_
#define SRC_LIB_MONGOBACKEND_LOCATION_H_

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

#include "common/globals.h"
#include "rest/OrionError.h"
#include "ngsi/ContextAttributeVector.h"



/* ****************************************************************************
*
* processLocationAtEntityCreation -
*/
extern bool processLocationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  std::string*                   locAttr,
  orion::BSONObjBuilder*         geoJson,
  std::string*                   errDetail,
  ApiVersion                     apiVersion,
  OrionError*                    oe
);



/* ****************************************************************************
*
* processLocationAtUpdateAttribute -
*/
extern bool processLocationAtUpdateAttribute
(
  std::string*             currentLocAttrName,
  const ContextAttribute*  targetAttr,
  orion::BSONObjBuilder*   geoJson,
  std::string*             errDetail,
  ApiVersion               apiVersion,
  OrionError*              oe
);



/* ****************************************************************************
*
* processLocationAtAppendAttribute -
*/
extern bool processLocationAtAppendAttribute
(
  std::string*             currentLocAttrName,
  const ContextAttribute*  targetAttr,
  bool                     actualAppend,
  orion::BSONObjBuilder*   geoJson,
  std::string*             errDetail,
  ApiVersion               apiVersion,
  OrionError*              oe
);

#endif  // SRC_LIB_MONGOBACKEND_LOCATION_H_
