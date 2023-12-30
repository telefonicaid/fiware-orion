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
#include <string.h>                                            // strlen, strncpy, strdup
#include <stdlib.h>                                            // free

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/urlDecode.h"                          // urlDecode
#include "orionld/q/qLex.h"                                    // qLex
#include "orionld/q/qParse.h"                                  // qParse
#include "orionld/q/qListRelease.h"                            // qListRelease
#include "orionld/q/qPresent.h"                                // qPresent
#include "orionld/q/qLexListRender.h"                          // qLexListRender
#include "orionld/q/qBuild.h"                                  // Own interface



// -----------------------------------------------------------------------------
//
// qBuild - build QNode tree
//
// DESCRIPTION
//   'q' filters are used for querying entities (GET /entities, POST /entityOperations/query) and
//   for subscriptions/notifications.
//
//   For entity queries, the filter is parsed, used and thrown away - kalloc is used.
//
//   However, for subscriptions:
//   - the filter needs to be expanded and saved in the database
//   - the filter needs to ba allocated and stored in the subscription cache (for GET /subscriptions ops)
//   - the resulting QNode tree needs to be allocated and stored in the subscription cache
//
//   At startup, when the subscription cache is populated from the database content, the same applies (for subscriptions).
//   As well as for regular sub-cache refresh operations.
//
//   We assume for now that "GET /subscriptions" operations always use the sub-cache.
//
// PARAMETERS
//   q            the string as it figures in uri param or payload body
//   qRenderP     output parameter for the prepared filter (expansion, dotForEq, '.value')
//   v2ValidP     output parameter indicating whether the q string id valid for NGSIv2
//   isMqP        output parameter indicating whether the q para meter corresponds to 'mq' in NGSIv2
//   qToDbModel   transform the variables in 'q' to look like they look in the q in the database
//
QNode* qBuild(const char* q, char** qRenderP, bool* v2ValidP, bool* isMqP, bool qToDbModel, bool pernot)
{
  QNode*      qP = NULL;
  char*       title;
  char*       detail;
  QNode*      qList;

  // qLex destroys the input string, but we need it intact
  char        buf[512];
  char*       qString   = buf;

  if (strlen(q) < sizeof(buf))
    strncpy(buf, q, sizeof(buf) - 1);
  else
    qString = strdup(q);  // Freed after use - "if (qString != buf)"

  urlDecode(qString);
  orionldState.useMalloc = true;  // the Q-Tree needs real alloction for the sub-cache
  qList = qLex(qString, false, &title, &detail);
  if (qList == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
    LM_RE(NULL, ("Error (qLex: %s: %s)", title, detail));
  }
  else
  {
    if (qRenderP != NULL)
    {
      // NOTE: qLexListRender MUST NOT destroy the qList!!!
      *qRenderP = qLexListRender(qList, v2ValidP, isMqP);
      if (*qRenderP == NULL)
      {
        // qLexListRender prepares orionldError
        return NULL;
      }
    }

    qP = qParse(qList, NULL, pernot, qToDbModel, &title, &detail);  // 3rd parameter: 'forDb' false unless pernot subscription
    if (qP == NULL)
    {
      orionldError(OrionldBadRequestData, "Invalid Q-Filter", detail, 400);
      LM_RE(NULL, ("Error (qParse: %s: %s) - but, the subscription will be inserted in the sub-cache without 'q'", title, detail));
    }
  }

  if (qString != buf)
    free(qString);

  orionldState.useMalloc = false;

  qListRelease(qList);

  return qP;
}
