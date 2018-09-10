#ifndef SRC_LIB_SERVICEROUTINESV2_SERVICEROUTINESCOMMON_H_
#define SRC_LIB_SERVICEROUTINESV2_SERVICEROUTINESCOMMON_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/

/* Common function used by serveral modules within the serviceRoutinesV2 library */

#include <string>
#include <map>

#include "common/string.h"
#include "common/RenderFormat.h"
#include "ngsi/StringList.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* semRender -
*
*/
extern void setFilters
(
  std::map<std::string, std::string>&  uriParam,
  std::map<std::string, bool>&         uriParamOptions,
  StringList*                          attrsFilter,
  StringList*                          metadataFilter
);



/* ****************************************************************************
*
* renderFormat -
*
*/
extern RenderFormat getRenderFormat(std::map<std::string, bool>&  uriParamOptions);

#endif  // SRC_LIB_SERVICEROUTINESV2_SERVICEROUTINESCOMMON_H_

