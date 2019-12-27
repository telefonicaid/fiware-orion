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

#include "ngsi10/UpdateContextRequest.h"                         // UpdateContextRequest
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjTreeToContextAttribute.h"             // kjTreeToContextAttribute
#include "orionld/kjTree/kjTreeToEntity.h"                       // kjTreeToEntity



// -----------------------------------------------------------------------------
//
// kjTreeToEntity -
//
bool kjTreeToEntity(UpdateContextRequest* ucrP, KjNode* treeP)
{
  // Get 'id' and 'type' from '_id'

  for (KjNode* attrP = treeP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    if (strcmp(attrP->name, "_id") == 0)
    {
      for (KjNode* idPartP = attrP->value.firstChildP; idPartP != NULL; idPartP = idPartP->next)
      {
        if (strcmp(idPartP->name, "id") == 0)
        {
          ucrP->contextElementVector[0]->entityId.id = idPartP->value.s;
        }
        else if (strcmp(idPartP->name, "type") == 0)
          ucrP->contextElementVector[0]->entityId.type = idPartP->value.s;
        else if (strcmp(idPartP->name, "isPattern") == 0)
          ucrP->contextElementVector[0]->entityId.isPattern = idPartP->value.s;
        else if (strcmp(idPartP->name, "servicePath") == 0)
          ucrP->contextElementVector[0]->entityId.servicePath = idPartP->value.s;
      }
    }
    else if (strcmp(attrP->name, "creDate") == 0)
      ucrP->contextElementVector[0]->entityId.creDate = attrP->value.f;
    else if (strcmp(attrP->name, "modDate") == 0)
      ucrP->contextElementVector[0]->entityId.modDate = attrP->value.f;
    else  if (strcmp(attrP->name, "attrs") == 0)  // Attribute Vector
    {
      for (KjNode* aP = attrP->value.firstChildP; aP != NULL; aP = aP->next)
      {
        ContextAttribute* caP = new ContextAttribute();
        KjNode*           typeNodeP;
        char*             detail;

        caP->name = aP->name;
        //
        // Here, as the data comes from the DB, no expansion is necessary - kjTreeToContextAttribute should have a parameter for that
        //
        if (kjTreeToContextAttribute(orionldState.ciP, aP, caP, &typeNodeP, &detail) == false)
        {
          delete caP;
          LM_E(("Internal Error (kjTreeToContextAttribute: %s)", detail));
          return false;
        }

        ucrP->contextElementVector[0]->contextAttributeVector.push_back(caP);
      }
    }
  }

  return true;
}
