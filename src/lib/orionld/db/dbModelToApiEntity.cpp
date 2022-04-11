/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjString
#include "kjson/kjClone.h"                                       // kjClone
#include "kjson/kjRender.h"                                      // kjFastRender (TEMP)
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "common/RenderFormat.h"                                 // RenderFormat

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/db/dbModelToApiAttribute.h"                    // dbModelToApiAttribute
#include "orionld/db/dbModelToApiEntity.h"                       // Own interface



// -----------------------------------------------------------------------------
//
// dbModelToApiEntity -
//
KjNode* dbModelToApiEntity(KjNode* dbEntityP, bool sysAttrs, const char* entityId)
{
  KjNode* apiEntityP  = kjObject(orionldState.kjsonP, NULL);
  KjNode* dbIdObjectP = kjLookup(dbEntityP, "_id");

  if (dbIdObjectP == NULL)
  {
    orionldError(OrionldInternalError, "Database Error (entity without _id)", NULL, 500);
    return NULL;
  }

  KjNode* dbIdNodeP = kjLookup(dbIdObjectP, "id");
  if (dbIdNodeP != NULL)
  {
    KjNode* idNodeP = kjClone(orionldState.kjsonP, dbIdNodeP);
    kjChildAdd(apiEntityP, idNodeP);
  }
  else
    LM_E(("Database Error (entity '%s' without id inside _id)", entityId));

  KjNode* dbTypeNodeP = kjLookup(dbIdObjectP, "type");
  if (dbTypeNodeP != NULL)
  {
    KjNode* typeNodeP = kjClone(orionldState.kjsonP, dbTypeNodeP);
    kjChildAdd(apiEntityP, typeNodeP);
  }
  else
    LM_E(("Database Error (entity '%s' without type inside _id)", entityId));

  if (sysAttrs)
  {
    const char* fieldName[]  = { "creDate",   "modDate"    };
    const char* ngsildName[] = { "createdAt", "modifiedAt" };

    for (unsigned int ix = 0; ix < K_VEC_SIZE(fieldName); ix++)
    {
      KjNode* nodeP = kjLookup(dbEntityP, fieldName[ix]);
      if (nodeP != NULL)
      {
        char* dateTimeBuf = kaAlloc(&orionldState.kalloc, 32);
        numberToDate(nodeP->value.f, dateTimeBuf, 32);
        nodeP->name       = (char*) ngsildName[ix];
        nodeP->value.s    = dateTimeBuf;
        nodeP->type       = KjString;
      }
    }
  }

  KjNode* dbAttrsP = kjLookup(dbEntityP, "attrs");

  if (dbAttrsP)
  {
    KjNode*  dbAttrP = dbAttrsP->value.firstChildP;
    KjNode*  next;

    while (dbAttrP != NULL)
    {
      next = dbAttrP->next;
      kjChildRemove(dbAttrP, dbAttrsP);
      dbModelToApiAttribute(dbAttrP, sysAttrs);
      kjChildAdd(apiEntityP, dbAttrP);  // No longer a DB attr - has been transformed to API representation
      dbAttrP = next;
    }
  }

  return apiEntityP;
}



// -----------------------------------------------------------------------------
//
// dbModelToApiEntity2 -
//
// USED BY
//   - orionldGetEntities  (GET /entities)
//   - orionldPostQuery    (POST /entityOperations/query)
//
// NOTE
//   GET /entities calls "kjTreeFromQueryContextResponse()" that takes care of
//   concise/keyValues and lang
//
//   POST /entityOperations/query (POST Query) on the other hand doewsn't use mongoBackend and thus
//   doesn't have any QueryContextResponse structure.
//   So, POST Query needs this to be done by "dbModelToApiEntity2()"
//
// FIXME:
//   Add "concise/keyValues" and "lang" treatment to "dbModelToApiEntity2()"
//   Remove "concise/keyValues" and "lang" treatment from "kjTreeFromQueryContextResponse()"
//
KjNode* dbModelToApiEntity2(KjNode* dbEntityP, bool sysAttrs, RenderFormat renderFormat, char* lang, OrionldProblemDetails* pdP)
{
  KjNode* _idP            = NULL;
  KjNode* attrsP          = NULL;

  for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "_id") == 0)
    {
      _idP = nodeP;
      if (attrsP != NULL)
        break;
    }
    else if (strcmp(nodeP->name, "attrs") == 0)
    {
      attrsP = nodeP;
      if (_idP != NULL)
        break;
    }
  }

  if (_idP == NULL)
  {
    LM_E(("Database Error (the field '_id' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id' is missing";
    return NULL;
  }

  if (attrsP == NULL)
  {
    LM_E(("Database Error (the field 'attrs' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field 'attrs' is missing";
    return NULL;
  }

  KjNode* idP   = NULL;
  KjNode* typeP = NULL;

  for (KjNode* nodeP = _idP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if ((strcmp(nodeP->name, "id") == 0) || (strcmp(nodeP->name, "@id") == 0))
      idP = nodeP;
    else if ((strcmp(nodeP->name, "type") == 0) || (strcmp(nodeP->name, "@type") == 0))
      typeP = nodeP;
  }

  if (idP == NULL)
  {
    LM_E(("Database Error (the field '_id.id' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id.id' is missing";
    return NULL;
  }

  if (typeP == NULL)
  {
    LM_E(("Database Error (the field '_id.type' is missing)"));
    pdP->title  = (char*) "Database Error";
    pdP->detail = (char*) "the field '_id.type' is missing";
    return NULL;
  }

  KjNode* entityP = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(entityP, idP);

  typeP->value.s = orionldContextItemAliasLookup(orionldState.contextP, typeP->value.s, NULL, NULL);
  kjChildAdd(entityP, typeP);

  //
  // Second loop over dbEntityP if sysAttrs == true (normally it's not - this saves time when sysAttrs is not used)
  //
  if (sysAttrs == true)
  {
    KjNode* creDateP = NULL;
    KjNode* modDateP = NULL;

    for (KjNode* nodeP = dbEntityP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "creDate") == 0)
      {
        creDateP = nodeP;
        creDateP->name = (char*) "createdAt";
        if (modDateP != NULL)
          break;
      }
      else if (strcmp(nodeP->name, "modDate") == 0)
      {
        modDateP = nodeP;
        modDateP->name = (char*) "modifiedAt";
        if (creDateP != NULL)
          break;
      }
    }

    kjChildAdd(entityP, creDateP);
    kjChildAdd(entityP, modDateP);
  }


  //
  // Now the attributes
  //
  if (attrsP != NULL)
  {
    for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
      KjNode* attributeP;

      if ((attributeP = dbModelToApiAttribute2(attrP, sysAttrs, renderFormat, lang, pdP)) == NULL)
      {
        LM_E(("Datamodel Error (%s: %s)", pdP->title, pdP->detail));
        return NULL;
      }

      kjChildAdd(entityP, attributeP);
    }
  }

  return entityP;
}
