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
#include <string>                                              // std::string
#include <vector>                                              // std::vector

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRender.h"                                    // kjFastRender
#include "kjson/kjRenderSize.h"                                // kjFastRenderSize
}

#include "logMsg/logMsg.h"                                     // lmTraceIsSet, lmOut

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/fileName.h"                           // fileName
#include "orionld/kjTree/kjTreeLog.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeLogFunction -
//
void kjTreeLogFunction(KjNode* tree, const char* msg, const char* path, int lineNo, const char* functionName, int traceLevel)
{
  if (lmTraceIsSet(traceLevel) == false)
    return;

  char* fileNameOnly = fileName(path);

  if (tree == NULL)
  {
    char error[256];
    snprintf(error, sizeof(error) - 1, "%s: NULL", msg);
    lmOut(error, 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    return;
  }

  int bufSize = kjFastRenderSize(tree);

  // Too big trees will not be rendered - this is just logging
  if (bufSize < 10 * 1024)
  {
    char* treeBuf = kaAlloc(&orionldState.kalloc, bufSize + 512);

    if (treeBuf != NULL)
    {
      bzero(treeBuf, bufSize);
      kjFastRender(tree, treeBuf);

      char* buf = kaAlloc(&orionldState.kalloc, bufSize + strlen(msg) + 512);
      if (buf != NULL)
      {
        snprintf(buf, bufSize + strlen(msg) + 512, "%s: %s", msg, treeBuf);
        lmOut(buf, 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
      }
      else
        lmOut((char*) "KjNode Tree Render not possible - kaAlloc error 2", 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
    }
    else
      lmOut((char*) "KjNode Tree Render not possible - kaAlloc error 1", 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
  }
  else
    lmOut((char*) "KjNode Tree too large to be rendered", 'T', fileNameOnly, lineNo, functionName, traceLevel, NULL);
}
