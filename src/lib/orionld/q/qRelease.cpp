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
#include <stdlib.h>                                            // free

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/q/QNode.h"                                   // QNode
#include "orionld/q/qRelease.h"                                // Own interface



// -----------------------------------------------------------------------------
//
// qRelease -
//
void qRelease(QNode* qP)
{
  if ((qP->type == QNodeAnd)        ||
      (qP->type == QNodeOr)         ||
      (qP->type == QNodeExists)     ||
      (qP->type == QNodeNotExists)  ||
      (qP->type == QNodeEQ)         ||
      (qP->type == QNodeNE)         ||
      (qP->type == QNodeGE)         ||
      (qP->type == QNodeGT)         ||
      (qP->type == QNodeLE)         ||
      (qP->type == QNodeLT))
  {
    for (QNode* childP = qP->value.children; childP != NULL; childP = childP->next)
    {
      qRelease(childP);
    }
  }
  else if ((qP->type == QNodeMatch) || (qP->type == QNodeNoMatch))
  {
    // regfree(qP->value.regex);
  }

  if ((qP->type == QNodeVariable) && (qP->value.v != NULL))
    free(qP->value.v);

  LM_TMP(("Q: Releasing a %s at %p", qNodeType(qP->type), qP));
  free(qP);
}



// ----------------------------------------------------------------------------
//
// qListRelease -
//
void qListRelease(QNode* qP)
{
  if (qP->next != NULL)
    qListRelease(qP->next);
  free(qP);
}
