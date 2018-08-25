/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"

extern "C"
{
#include "kjson/kjson.h"                                // Kjson
#include "kjson/KjNode.h"                               // KjNode
#include "kjson/kjParse.h"                              // kjParse
#include "kjson/kjFree.h"                               // kjFree
}

#include "orionld/common/OrionldResponseBuffer.h"            // OrionldResponseBuffer
#include "orionld/common/orionldRequestSend.h"               // orionldRequestSend


// -----------------------------------------------------------------------------
//
// orionldContextDownloadAndParse -
//
// The returned buffer must be freed by the caller
//
KjNode* orionldContextDownloadAndParse(Kjson* kjsonP, const char* url, char** detailsPP)
{
  //
  // Prepare the httpResponse buffer
  //
  // A smarter way here would be to use a thread local buffer and allocate only if that buffer
  // is not big enough
  //
  OrionldResponseBuffer  httpResponse;
  
  httpResponse.buf       = (char*) malloc(1024);
  httpResponse.size      = 1024;
  httpResponse.used      = 0;
  httpResponse.allocated = true;

  if (httpResponse.buf == NULL)
  {
    *detailsPP = (char*) "out of memory";
    return NULL;
  }

  if (orionldRequestSend(&httpResponse, url, 5000, detailsPP) == false)
  {
    // detailsPP filled in by orionldRequestSend()
    // httpResponse.buf freed by orionldRequestSend()
    LM_E(("orionldRequestSend failed: %s", *detailsPP));
    return NULL;
  }

  // <TMP>
  for (unsigned int ix = 0; ix < strlen(httpResponse.buf); ix++)
  {
    if ((httpResponse.buf[ix] == '\n') || (httpResponse.buf[ix] == '\r'))
      httpResponse.buf[ix] = ' ';
  }
  LM_TMP(("Downloaded payload: %s", httpResponse.buf));
  // </TMP>

  // Now parse the payload
  KjNode* tree = kjParse(kjsonP, httpResponse.buf);

  // And then we can free the httpResponse buffer
  free(httpResponse.buf);

  if (tree == NULL)
  {
    *detailsPP = (char*) "Error parsing context";
    LM_E(("kjParse returned NULL"));
    return NULL;
  }

  if ((tree->type != KjArray) && (tree->type != KjString) && (tree->type != KjObject))
  {
    kjFree(tree);
    *detailsPP = (char*) "Invalid JSON type of response";
    LM_E((*detailsPP));
    return NULL;
  }

  return tree;
}
