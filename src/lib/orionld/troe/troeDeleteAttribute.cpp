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
#include "orionld/troe/PgTableDefinitions.h"                   // PG_ENTITY_INSERT_START
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendInit.h"                         // pgAppendInit
#include "orionld/troe/pgAppend.h"                             // pgAppend
#include "orionld/troe/pgAttributeAppend.h"                    // pgAttributeAppend
#include "orionld/troe/pgCommands.h"                           // pgCommands
#include "orionld/troe/troeDeleteAttribute.h"                  // Own interface


// FIXME: Move to pgAttributeAppend.h/cpp


// ----------------------------------------------------------------------------
//
// troeDeleteAttribute -
//
bool troeDeleteAttribute(ConnectionInfo* ciP)
{
  LM_TMP(("In troeDeleteAttribute"));
  char* entityId      = orionldState.wildcard[0];
  char* attributeName = orionldState.wildcard[1];
  char* attributeNameEq;
  char  instanceId[80];

  uuidGenerate(instanceId, sizeof(instanceId), true);

  attributeName   = orionldAttributeExpand(orionldState.contextP, attributeName, true, NULL);
  attributeNameEq = kaStrdup(&orionldState.kalloc, attributeName);
  dotForEq(attributeNameEq);

  //
  // Prepare attributesBuffer
  //
  PgAppendBuffer  attributesBuffer;
  pgAppendInit(&attributesBuffer, 1024);       // Will grow if needed
  pgAppend(&attributesBuffer, PG_ATTRIBUTE_INSERT_START, 0);

  if (orionldState.uriParams.datasetId != NULL)
  {
    LM_TMP(("orionldState.uriParams.datasetId != NULL"));
    pgAttributeAppend(&attributesBuffer, instanceId, attributeName, "Delete", entityId, (char*) "NULL", NULL, false, NULL, orionldState.uriParams.datasetId, NULL, NULL);
  }
  else if (orionldState.uriParams.deleteAll == true)
  {
    LM_TMP(("orionldState.uriParams.deleteAll == true"));
    if (orionldState.dbAttrWithDatasetsP == NULL)
      LM_W(("DA: orionldState.dbAttrWithDatasetsP == NULL ... how?"));
    else
    {
      KjNode* attrsP = kjLookup(orionldState.dbAttrWithDatasetsP, "attrs");

      if (attrsP != NULL)
      {
        KjNode* defaultInstanceP = kjLookup(attrsP, attributeNameEq);
        if (defaultInstanceP != NULL)
          pgAttributeAppend(&attributesBuffer, instanceId, attributeName, "Delete", entityId, (char*) "NULL", NULL, false, NULL, orionldState.uriParams.datasetId, NULL, NULL);
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
              pgAttributeAppend(&attributesBuffer, instanceId, attributeName, "Delete", entityId, (char*) "NULL", NULL, false, NULL, datasetIdNodeP->value.s, NULL, NULL);
          }
        }
      }
    }
  }
  else
  {
    LM_TMP(("Calling pgAttributeAppend"));
    pgAttributeAppend(&attributesBuffer, instanceId, attributeName, "Delete", entityId, (char*) "NULL", NULL, false, NULL, NULL, NULL, NULL);
    LM_TMP(("After pgAttributeAppend. attributesBuffer.buf: '%s'", attributesBuffer.buf));
  }

  if (attributesBuffer.values > 0)
  {
    char* sqlV[1] = { attributesBuffer.buf };

    pgCommands(sqlV, 1);
  }

  return true;
}
