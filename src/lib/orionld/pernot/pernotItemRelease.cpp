/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kjson/kjFree.h"                                      // kjFree
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/PernotSubscription.h"                  // PernotSubscription
#include "orionld/pernot/pernotItemRelease.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// pernotItemRelease -
//
bool pernotItemRelease(PernotSubscription* pSubP)
{
  LM_T(LmtLeak, ("Releasing pernot at %p", pSubP));
  if (pSubP->subscriptionId != NULL)
  {
    LM_T(LmtLeak, ("Releasing pernot id '%s'", pSubP->subscriptionId));
    free(pSubP->subscriptionId);
  }

  if (pSubP->kjSubP != NULL)
  {
    LM_T(LmtLeak, ("Releasing pernot kj-tree at %p", pSubP->kjSubP));
    kjFree(pSubP->kjSubP);
  }

  return true;
}
