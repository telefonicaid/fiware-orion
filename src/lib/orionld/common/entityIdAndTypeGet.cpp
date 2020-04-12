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
* Author: Gabriel Quaresma and Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, ...
}

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "orionld/common/orionldErrorResponse.h"                 // OrionldResponseErrorType
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/entityIdCheck.h"                        // entityIdCheck
#include "orionld/common/entityTypeCheck.h"                        // entityTypeCheck
#include "orionld/common/SCOMPARE.h"                             // SCOMPAREx


// -----------------------------------------------------------------------------
//
// entityIdAndTypeGet - lookup 'id' and 'type' in a KjTree
//
bool entityIdAndTypeGet(KjNode* entityNodeP, char** idP, char** typeP, KjNode* errorsArrayP)
{
  KjNode*  idNodeP        = NULL;
  KjNode*  typeNodeP      = NULL;
  bool     idDuplicated   = false;
  bool     typeDuplicated = false;

  *idP   = NULL;
  *typeP = NULL;

  for (KjNode* itemP = entityNodeP->value.firstChildP; itemP != NULL; itemP = itemP->next)
  {
    if (SCOMPARE3(itemP->name, 'i', 'd', 0))
    {
      if (idNodeP != NULL)
        idDuplicated = true;
      else
        idNodeP = itemP;
    }
    else if (SCOMPARE5(itemP->name, 't', 'y', 'p', 'e', 0))
    {
      if (typeNodeP != NULL)
        typeDuplicated = true;
      else
        typeNodeP = itemP;
    }
  }

  if (entityIdCheck(idNodeP, idDuplicated, errorsArrayP) == false)
  {
    LM_E(("entityIdCheck flagged error"));
    return false;
  }

  *idP = idNodeP->value.s;

  if (typeNodeP != NULL)
  {
    if (entityTypeCheck(typeNodeP, typeDuplicated, idNodeP->value.s, false, errorsArrayP) == false)
    {
      LM_E(("entityTypeCheck flagged error"));
      return false;
    }

    *typeP = typeNodeP->value.s;
  }

  return true;
}
