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
#include "kalloc/kaAlloc.h"                                      // kaAlloc
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjRender.h"                                      // kjFastRender
#include "kjson/kjRenderSize.h"                                  // kjFastRenderSize
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/types/OrionldAlteration.h"                     // OrionldAlteration
#include "orionld/notifications/orionldAlterations.h"            // Own interface



// -----------------------------------------------------------------------------
//
// ALTERATION -
//
#define ALTERATION(altType)                                 \
do                                                          \
{                                                           \
  aeP->alteredAttributeV[ix].alterationType = altType;      \
  aeP->alteredAttributeV[ix].attrName       = attrP->name;  \
  aeP->alteredAttributeV[ix].attrNameEq     = attrNameEq;   \
  ++ix;                                                     \
} while (0)



// -----------------------------------------------------------------------------
//
// kjValuesDiffer -
//
static bool kjValuesDiffer(KjNode* leftAttr, KjNode* rightAttr)
{
  KjNode* left  = kjLookup(leftAttr,  "value");  // "object", "languageMap" ... First lookup "type" ...
  KjNode* right = kjLookup(rightAttr, "value");

  if (left == NULL)
    LM_RE(true, ("Internal Error (left KjNode has no value member)"));
  if (right == NULL)
    LM_RE(true, ("Database Error (DB KjNode has no value member)"));

  if (left->type != right->type)
    return true;

  KjValueType type = left->type;

  if (type == KjString)   return (strcmp(left->value.s, right->value.s) == 0)? false : true;
  if (type == KjInt)      return (left->value.i == right->value.i)?            false : true;
  if (type == KjFloat)    return (left->value.f == right->value.f)?            false : true;
  if (type == KjBoolean)  return (left->value.b == right->value.b)?            false : true;

  //
  // Compound values ... let's just render the values and do a strcmp on the rendered buffers
  // However, might be an empty array/object  (kjFastRenderSize crashes if the parameter is NULL)
  //
  if ((left->value.firstChildP == NULL) && (right->value.firstChildP == NULL))
    return false;
  else if ((left->value.firstChildP == NULL) || (right->value.firstChildP == NULL))
    return true;

  int   leftBufSize     = kjFastRenderSize(left->value.firstChildP);
  int   rightBufSize    = kjFastRenderSize(right->value.firstChildP);
  char* leftBuf         = kaAlloc(&orionldState.kalloc, leftBufSize);
  char* rightBuf        = kaAlloc(&orionldState.kalloc, rightBufSize);

  kjFastRender(left->value.firstChildP,  leftBuf);
  kjFastRender(right->value.firstChildP, rightBuf);

  if (strcmp(leftBuf, rightBuf) == 0)
    return false;

  return true;
}



// -----------------------------------------------------------------------------
//
// orionldAlterations -
//
// If replace == true, all those attrs in dbAttrsP that are not in attrsP have been DELETED
//
OrionldAlteration* orionldAlterations(char* entityId, char* entityType, KjNode* attrsP, KjNode* dbAttrsP, bool replace)
{
  OrionldAlteration* aeP   = (OrionldAlteration*) kaAlloc(&orionldState.kalloc, sizeof(OrionldAlteration));
  int                attrs = 0;

  // Count deleted attributes - if replace
  if ((replace == true) && (dbAttrsP != NULL))  // dbAttrsP might be NULL - if the entity has no attributes in DB
  {
    for (KjNode* dbAttrP = dbAttrsP->value.firstChildP; dbAttrP != NULL; dbAttrP = dbAttrP->next)
    {
      KjNode* attrP = kjLookup(attrsP, dbAttrP->name);

      if (attrP == NULL)  // Present in DB but not in replacing update - the attr is being deleted
        ++attrs;
    }

    LM(("%d attributes are deleted in this REPLACE operation", attrs));
  }

  // Count attributes that have been modified (and that survive the modification)
  for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    ++attrs;
  }

  aeP->entityId          = entityId;
  aeP->entityType        = entityType;
  aeP->alteredAttributes = attrs;
  aeP->alteredAttributeV = (OrionldAttributeAlteration*) kaAlloc(&orionldState.kalloc, attrs * sizeof(OrionldAttributeAlteration));

  int ix = 0;
  for (KjNode* attrP = attrsP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    LM(("Alteration for attribute '%s'", attrP->name));
    char* attrNameEq = kaStrdup(&orionldState.kalloc, attrP->name);  // Must copy to change dot for eq for ...
    dotForEq(attrNameEq);

    if (attrP->type == KjNull)
    {
      ALTERATION(AttributeDeleted);
      continue;
    }

    KjNode* dbAttrP = kjLookup(dbAttrsP, attrNameEq);

    if (dbAttrP == NULL)
    {
      ALTERATION(AttributeAdded);
      continue;
    }

    bool valuesDiffer = kjValuesDiffer(attrP, dbAttrP);

    if (valuesDiffer)
      ALTERATION(AttributeValueChanged);
    else
      ALTERATION(AttributeModifiedAtChanged);  // Need to check all metadata - could also be AttributeMetadataChanged
  }

  aeP->next = NULL;

  return aeP;
}
