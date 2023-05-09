/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/payloadCheck/PCHECK.h"                         // PCHECK_*
#include "orionld/payloadCheck/pcheckEntityInfo.h"               // pcheckEntityInfo



// -----------------------------------------------------------------------------
//
// pcheckEntityInfoArray -
//
// This function is used by the "POST Query" request, where the entity type is NOT mandatory.
// It is also used for Subscriptions and Registrations, where entity type IS mandatory.
//
bool pcheckEntityInfoArray(KjNode* entityInfoArrayP, bool typeMandatory, bool idMandatory, const char** fieldPathV)
{
  for (KjNode* entityInfoP = entityInfoArrayP->value.firstChildP; entityInfoP != NULL; entityInfoP = entityInfoP->next)
  {
    PCHECK_OBJECT(entityInfoP, 0, NULL, fieldPathV[1], 400);

    if (pcheckEntityInfo(entityInfoP, typeMandatory, idMandatory, fieldPathV) == false)
      return false;
  }

  return true;
}
