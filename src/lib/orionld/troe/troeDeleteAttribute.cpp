/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include "kalloc/kaStrdup.h"                                   // kaStrdup
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgAttributeDelete.h"                    // pgAttributeDelete
#include "orionld/troe/troeDeleteAttribute.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// troeDeleteAttribute -
//
bool troeDeleteAttribute(ConnectionInfo* ciP)
{
  PGconn* connectionP = pgConnectionGet(orionldState.troeDbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres at %s", orionldState.tenant));

  if (pgTransactionBegin(connectionP) != true)
  {
    pgConnectionRelease(connectionP);
    LM_RE(false, ("pgTransactionBegin failed"));
  }

  char* entityId      = orionldState.wildcard[0];
  char* attributeName = orionldState.wildcard[1];
  char* attributeNameEq;
  char  instanceId[80];
  uuidGenerate(instanceId, sizeof(instanceId), true);

  attributeName   = orionldAttributeExpand(orionldState.contextP, attributeName, true, NULL);
  attributeNameEq = kaStrdup(&orionldState.kalloc, attributeName);
  dotForEq(attributeNameEq);

  if (orionldState.uriParams.datasetId != NULL)
  {
    if (pgAttributeDelete(connectionP, entityId, instanceId, attributeName, orionldState.uriParams.datasetId, orionldState.requestTimeString) == false)
    {
      LM_E(("Database Error (delete attribute with datasetId troe layer failed)"));

      if (pgTransactionRollback(connectionP) == false)
        LM_E(("pgTransactionRollback failed"));

      pgConnectionRelease(connectionP);
      return false;
    }
  }
  else if (orionldState.uriParams.deleteAll == true)
  {
    LM_TMP(("DA: orionldState.uriParams.deleteAll == true"));
    if (orionldState.dbAttrWithDatasetsP == NULL)
      LM_W(("DA: orionldState.dbAttrWithDatasetsP == NULL ... how?"));
    else
    {
      KjNode* attrsP = kjLookup(orionldState.dbAttrWithDatasetsP, "attrs");

      if (attrsP != NULL)
      {
        KjNode* defaultInstanceP = kjLookup(attrsP, attributeNameEq);
        if (defaultInstanceP != NULL)
        {
          LM_TMP(("DA: Calling pgAttributeDelete without datasetId"));
          if (pgAttributeDelete(connectionP, entityId, instanceId, attributeName, NULL, orionldState.requestTimeString) == false)
          {
            LM_E(("Database Error (delete attribute default instance troe layer failed)"));

            if (pgTransactionRollback(connectionP) == false)
              LM_E(("pgTransactionRollback failed"));

            pgConnectionRelease(connectionP);
            return false;
          }
        }
      }

      KjNode* datasetsP = kjLookup(orionldState.dbAttrWithDatasetsP, "@datasets");

      if (datasetsP != NULL)
      {
        KjNode* attrV = kjLookup(datasetsP, attributeNameEq);

        if (attrV != NULL)
        {
          for (KjNode* aP = attrV->value.firstChildP; aP != NULL; aP = aP->next)
          {
            KjNode* datasetIdNodeP = kjLookup(aP, "datasetId");

            if (datasetIdNodeP != NULL)
            {
              LM_TMP(("DA: Calling pgAttributeDelete for datasetId '%s'", datasetIdNodeP->value.s));
              if (pgAttributeDelete(connectionP, entityId, instanceId, attributeName, datasetIdNodeP->value.s, orionldState.requestTimeString) == false)
              {
                LM_E(("Database Error (delete attribute datasetId troe layer failed)"));

                if (pgTransactionRollback(connectionP) == false)
                  LM_E(("pgTransactionRollback failed"));

                pgConnectionRelease(connectionP);
                return false;
              }
            }
          }
        }
      }
    }
  }
  else
  {
    if (pgAttributeDelete(connectionP, entityId, instanceId, attributeName, NULL, orionldState.requestTimeString) == false)
    {
      LM_E(("Database Error (delete attribute troe layer failed)"));

      if (pgTransactionRollback(connectionP) == false)
        LM_E(("pgTransactionRollback failed"));

      pgConnectionRelease(connectionP);
      return false;
    }
  }

  if (pgTransactionCommit(connectionP) != true)
  {
    pgConnectionRelease(connectionP);
    LM_RE(false, ("pgTransactionCommit failed"));
  }

  pgConnectionRelease(connectionP);

  return true;
}
