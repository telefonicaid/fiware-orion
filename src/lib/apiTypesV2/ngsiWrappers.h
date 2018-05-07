#ifndef SRC_LIB_APITYPESV2_NGSIWRAPPERS_H_
#define SRC_LIB_APITYPESV2_NGSIWRAPPERS_H_

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
*/
#include <vector>
#include <string>

#include "apiTypesV2/Subscription.h"  // EntID
#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "ngsi/NotifyConditionVector.h"



/* *****************************************************************************
*
* The aim of this module is to hold a set of wrapper functions needed
* for transforming NGSIv1 into NGSIv2 types and viceversa. We need this
* while both versions of the API coexist. However, at the end, this module
* should be removed 
*/



/* ****************************************************************************
*
* attrsStdVector2NotifyConditionVector -
*/
extern void attrsStdVector2NotifyConditionVector(const std::vector<std::string>& attrs, NotifyConditionVector* ncVP);



/* ****************************************************************************
*
* entIdStdVector2EntityIdVector -
*/
extern void entIdStdVector2EntityIdVector(const std::vector<ngsiv2::EntID>& entitiesV, EntityIdVector* enVP);

#endif  // SRC_LIB_APITYPESV2_NGSIWRAPPERS_H_
