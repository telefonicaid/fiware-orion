/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/tenantList.h"                           // tenantList
#include "orionld/serviceRoutines/orionldGetTenants.h"           // Own interface



// ----------------------------------------------------------------------------
//
// orionldGetTenants -
//
bool orionldGetTenants(ConnectionInfo* ciP)
{
  KjNode* tenantP;

  orionldState.responseTree = kjArray(orionldState.kjsonP, NULL);

  //
  // The default "tenant" is not in this list, and it will not be included.
  // Which is perfectly OK
  //
  OrionldTenant* tP = tenantList;
  while (tP != NULL)
  {
    tenantP = kjString(orionldState.kjsonP, NULL, tP->tenant);
    kjChildAdd(orionldState.responseTree, tenantP);
    tP = tP->next;
  }

  orionldState.noLinkHeader = true;
  return true;
}
