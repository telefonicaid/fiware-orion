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

/* The aim of this module is to hold a set of wrapper functions needed
 * for transforming NGSIv1 into NGSIv2 types and viceversa. We need this
 * while both versions of the API coexist. However, at the end, this module
 * should be removed */

#include "apiTypesV2/ngsiWrappers.h"

#include "apiTypesV2/Subscription.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/AttributeList.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/NotifyCondition.h"

using namespace ngsiv2;

// FIXME: fill with a cut-paste from #if 1 in the MongoGlobal.cpp file
