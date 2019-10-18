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
#include "kjson/KjNode.h"                                      // KjNode, kjValueType
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldContextList.h"                // orionldContextHead
#include "orionld/context/orionldContextPresent.h"             // orionldContextPresent
#include "orionld/context/orionldContextListPresent.h"         // Own interface



// -----------------------------------------------------------------------------
//
// orionldContextListPresent -
//
void orionldContextListPresent(void)
{
  int contexts = 0;

  for (OrionldContext* contextP = orionldContextHead; contextP != NULL; contextP = contextP->next)
  {
    ++contexts;
  }

  LM_T(LmtContextList, ("---------------- %d Contexts in Cache ------------------------", contexts));

  for (OrionldContext* ctxP = orionldContextHead; ctxP != NULL; ctxP = ctxP->next)
  {
    if ((ctxP->tree != NULL) && (ctxP->tree->value.firstChildP != NULL))
    {
      LM_T(LmtContextList, ("o Context at:     %p", ctxP));
      LM_T(LmtContextList, ("o tree at:        %p", ctxP->tree));
      LM_T(LmtContextList, ("o next at:        %p", ctxP->next));
      LM_T(LmtContextList, ("o url:            %s", ctxP->url));
      LM_T(LmtContextList, ("o firstChildP at: %p", ctxP->tree->value.firstChildP));
      LM_T(LmtContextList, ("o type:           %s", kjValueType(ctxP->tree->value.firstChildP->type)));
    }
    else
    {
      LM_T(LmtContextList, ("o Context at:     %p", ctxP));
      LM_T(LmtContextList, ("o tree at:        %p", ctxP->tree));
      LM_T(LmtContextList, ("o next at:        %p", ctxP->next));
      LM_T(LmtContextList, ("o url:            %s", ctxP->url));
      LM_T(LmtContextList, ("o firstChildP     NULL"));
    }
    LM_T(LmtContextList, ("--------------------------"));
  }

#if 0
  for (OrionldContext* contextP = orionldContextHead; contextP != NULL; contextP = contextP->next)
  {
    LM_T(LmtContextList, ("=================== %s ======================", contextP->url));
    orionldContextPresent(contextP);
    LM_T(LmtContextList, ("==============================================================="));
    LM_T(LmtContextList, (""));
  }
#endif
}
