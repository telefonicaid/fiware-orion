#ifndef SRC_LIB_MONGOBACKEND_MONGOCOMMONUPDATE_H_
#define SRC_LIB_MONGOBACKEND_MONGOCOMMONUPDATE_H_

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
* Author: Fermín Galán
*/
#include <map>
#include <string>
#include <vector>

#include "orionTypes/UpdateActionType.h"
#include "ngsi/UpdateContextResponse.h"



/* ****************************************************************************
*
*  UpdateCoverage -
*/
typedef enum UpdateCoverage
{
  UC_NONE             = 0,
  UC_SUCCESS          = 1,
  UC_FULL_ATTRS_FAIL  = 2,
  UC_ENTITY_NOT_FOUND = 3,
  UC_PARTIAL          = 4
} UpdateCoverage;



/* ****************************************************************************
*
* processContextElement -
*/
extern unsigned int processContextElement
(
  Entity*                              ceP,
  UpdateContextResponse*               responseP,
  ActionType                           action,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePath,
  std::map<std::string, std::string>&  uriParams,   // FIXME P7: we need this to implement "restriction-based" filters
  const std::string&                   xauthToken,
  const std::string&                   fiwareCorrelator,
  const std::string&                   ngsiV2AttrsFormat,
  const bool&                          forcedUpdate,
  const bool&                          overrideMetadata,
  unsigned int                         notifStartCounter,
  Ngsiv2Flavour                        ngsiV2Flavour    = NGSIV2_NO_FLAVOUR,
  UpdateCoverage*                      updateCoverageP  = NULL
);

#endif  // SRC_LIB_MONGOBACKEND_MONGOCOMMONUPDATE_H_
