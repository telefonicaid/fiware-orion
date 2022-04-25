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

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/urlDecode.h"                          // urlDecode
#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/q/qLex.h"                                    // qLex
#include "orionld/q/qParse.h"                                  // qParse
#include "orionld/q/qRelease.h"                                // qListRelease
#include "orionld/q/qPresent.h"                                // qPresent
#include "orionld/q/qBuild.h"                                  // Own interface



// -----------------------------------------------------------------------------
//
// qBuild - build QNode tree
//
QNode* qBuild(const char* q)
{
  QNode*      qP        = NULL;
  char*       title;
  char*       detail;
  QNode*      qList;

  LM_TMP(("Got a 'q': %s", q));

  // qLex destroys the input string, but we need it intact
  char        buf[512];
  char*       qString   = buf;

  if (strlen(q) < sizeof(buf))
    strncpy(buf, q, sizeof(buf) - 1);
  else
    qString = strdup(q);  // Freed after use - "if (qString != buf)"

  urlDecode(qString);
  orionldState.useMalloc = true;  // the Q-Tree needs real alloction for the sub-cache
  LM_TMP(("LEAK: ***** Calling qLex *****"));
  if ((qList = qLex(qString, &title, &detail)) == NULL)
    LM_RE(NULL, ("Error (qLex: %s: %s)", title, detail));
  else
  {
    LM_TMP(("LEAK: ***** Calling qParse *****"));
    qP = qParse(qList, NULL, false, &title, &detail);
    if (qP == NULL)
      LM_RE(NULL, ("Error (qParse: %s: %s) - but, the subscription will be inserted in the sub-cache without 'q'", title, detail));

    qPresent(qP, "LEAK", "Q-TREE");
  }

  if (qString != buf)
    free(qString);

  orionldState.useMalloc = false;
  qListRelease(qList);

  return qP;
}
