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
#include <string>
#include <vector>

#include "apiTypesV2/ngsiWrappers.h"
#include "apiTypesV2/Subscription.h"
#include "ngsi/EntityIdVector.h"
#include "ngsi/StringList.h"
#include "ngsi/NotifyConditionVector.h"
#include "ngsi/NotifyCondition.h"



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
*
*/
void attrsStdVector2NotifyConditionVector(const std::vector<std::string>& attrs, NotifyConditionVector* ncVP)
{
  NotifyCondition* nc = new NotifyCondition;

  for (unsigned int ix = 0; ix < attrs.size(); ix++)
  {
    nc->condValueList.push_back(attrs[ix]);
  }

  nc->type = ON_CHANGE_CONDITION;
  ncVP->push_back(nc);
}



/* ****************************************************************************
*
* entIdStdVector2EntityIdVector -
*
*/
void entIdStdVector2EntityIdVector(const std::vector<ngsiv2::EntID>& entitiesV, EntityIdVector* enVP)
{
  for (unsigned int ix = 0; ix < entitiesV.size(); ix++)
  {
    EntityId* enP = new EntityId();

    if (!entitiesV[ix].id.empty())
    {
      enP->fill(entitiesV[ix].id, entitiesV[ix].type, "false");
    }
    else  // idPattern
    {
      enP->fill(entitiesV[ix].idPattern, entitiesV[ix].type, "true");
    }

    enVP->push_back(enP);
  }
}
