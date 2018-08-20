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
#include <curl/curl.h>

#include "logMsg/logMsg.h"

extern "C"
{
#include "kjson/kjParse.h"                              // kjParse
#include "kjson/kjFree.h"                               // kjFree
}

#include "rest/ConnectionInfo.h"
#include "orionld/context/OrionldContext.h"


  
// ----------------------------------------------------------------------------
//
// orionLdContextList
//
OrionldContext* orionLdContextHead = NULL;
OrionldContext* orionLdContextTail = NULL;



// -----------------------------------------------------------------------------
//
// orionldContextLookup -
//
OrionldContext* orionldContextLookup(char* url)
{
  OrionldContext* contextP = orionLdContextHead;

  while (contextP != NULL)
  {
    LM_TMP(("Comparing '%s' with '%s'", contextP->url, url));
    if (strcmp(contextP->url, url) == 0)
    {
      LM_TMP(("Found it!"));
      return contextP;
    }

    contextP = contextP->next;
    LM_TMP(("No match. Next context at %p", contextP));
  }

  LM_TMP(("NOT Found"));
  return NULL;
}


// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(OrionldContext* contextP, char* itemName)
{
  //
  // A context must be a tree object and inside the tree there must be only
  // String values or Object values
  //
  KjNode* nodeP = contextP->tree->children;

  while (nodeP != NULL)
  {
    if (strcmp(nodeP->name, itemName) == 0)
      return nodeP;

    nodeP = nodeP->next;
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(char* contextUrl, char* itemName)
{
  OrionldContext* contextP = orionldContextLookup(contextUrl);

  if (contextP == NULL)
    return NULL;

  return orionldContextItemLookup(contextP, itemName);
}



// -----------------------------------------------------------------------------
//
// orionldContextItemLookup -
//
KjNode* orionldContextItemLookup(KjNode* contextVector, char* itemName)
{
  if (contextVector->type != KjArray)
  {
    LM_E(("Not an Array"));
    return NULL;
  }

  KjNode* contextNodeP = contextVector->children;
  KjNode* itemP;

  while (contextNodeP != NULL)
  {
    OrionldContext* contextP = orionldContextLookup(contextNodeP->value.s);

    if (contextP == NULL)
      return NULL;

    itemP = orionldContextItemLookup(contextP, itemName);
    if (itemP != NULL)
      return itemP;

    contextNodeP = contextNodeP->next;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// ResponseBuffer -
//
typedef struct ResponseBuffer
{
  char*   buf;
  size_t  used;
  size_t  size;
} ResponseBuffer;



// -----------------------------------------------------------------------------
//
// writeCallback -
//
static size_t writeCallback(void* contents, size_t size, size_t members, void* userP)
{
  size_t           realSize = size * members;
  ResponseBuffer*  rBuf     = (ResponseBuffer*) userP;

  LM_TMP(("Got a chunk: %s", (char*) contents));

  if (realSize + rBuf->used > rBuf->size)
  {
    rBuf->buf = (char*) realloc(rBuf->buf, rBuf->size + realSize + 1);
    if (rBuf->buf == NULL)
    {
      LM_E(("Runtime Error (out of memory)"));
      return 0;
    }
  }

  memcpy(&rBuf->buf[rBuf->used], contents, realSize);
  rBuf->used += realSize;
  rBuf->size += realSize;
  rBuf->buf[rBuf->size] = 0;

  return realSize;
}



// ----------------------------------------------------------------------------
//
// orionldContextAdd -
//
bool orionldContextAdd(ConnectionInfo* ciP, char* url)
{
  LM_TMP(("Getting URL '%s' and adding it as a context", url));

  LM_TMP(("But first, looking up '%s'", url));
  if (orionldContextLookup(url) != NULL)
  {
    LM_TMP(("Context '%s' already cached", url));
  }
  LM_TMP(("looked up '%s'", url));

  ResponseBuffer       httpResponse = { NULL, 0 };
  CURLcode             cCode;
  char*                ip;
  char*                protocol;
  char*                urlPath;
  struct curl_context  cc;
  char*                initialUrl = strdup(url);

  // Extracting protocol (http|https) from URL
  protocol = url;
  while ((*url != ':') && (*url != 0))
    ++url;

  if (*url != ':')
  {
    LM_E(("Invalid URL, no protocol found"));
    free(initialUrl);
    return false;
  }
  *url = 0;  // Nulling out the end of the protocol
  
  // Checking that "//" comes after ':'
  if ((url[1] != '/') || (url[2] != '/'))
  {
    LM_E(("Invalid URL, no '//' after ':'"));
    free(initialUrl);
    return false;
  }
  url += 3;  // Skip over the "://"

  // Extracting IP from URL
  ip = url;
  while ((*url != '/') && (*url != 0))
    ++url;

  if (*url != '/')
  {
    LM_E(("Invalid URL, no IP found"));
    free(initialUrl);
    return false;
  }
  *url = 0;  // Nulling out the end of the IP (which is also the start of the URL PATH ...)

  url += 1;  // Skip over the out-nulled '/'

  urlPath = url;  // Initial '/' is missing ...

  //
  // Putting the missing initial '/' back where it belongs
  //
  int   size     = strlen(urlPath) + 1;
  char* urlPath2 = (char*) malloc(size + 1);

  snprintf(urlPath2, size, "/%s", urlPath);

  LM_TMP(("protocol: %s", protocol));
  LM_TMP(("IP:       %s", ip));
  LM_TMP(("URL Path: %s", urlPath2));

  get_curl_context(ip, &cc);
  if (cc.curl == NULL)
  {
    LM_E(("Unable to obtain CURL context"));
    free(initialUrl);
    return false;
  }

  
  //
  // Prepare the httpResponse
  // Note that the buffer will be reallocated on demand by 'writeCallback'
  //
  // A smarter way here would be to use a thread local buffer and allocate only if that buffer
  // is not big enough
  //
  httpResponse.buf  = (char*) malloc(1024);
  httpResponse.size = 1024;
  httpResponse.used = 0;
  

  //
  // Prepare the CURL handle
  //
  curl_easy_setopt(cc.curl, CURLOPT_URL, initialUrl);                      // Set the URL Path
  curl_easy_setopt(cc.curl, CURLOPT_CUSTOMREQUEST, "GET");                 // Set the HTTP verb
  curl_easy_setopt(cc.curl, CURLOPT_FOLLOWLOCATION, 1L);                   // Allow redirection
  // curl_easy_setopt(cc.curl, CURLOPT_HEADER, 1);                         // Activate include the header in the body output
  // curl_easy_setopt(cc.curl, CURLOPT_HTTPHEADER, headers);               // Put headers in place
  curl_easy_setopt(cc.curl, CURLOPT_WRITEFUNCTION, writeCallback);         // Callback function for writes
  curl_easy_setopt(cc.curl, CURLOPT_WRITEDATA, &httpResponse);             // Custom data for response handling
  curl_easy_setopt(cc.curl, CURLOPT_TIMEOUT_MS, 5000);                     // 5 second timeout

  LM_TMP(("Calling curl_easy_perform for GET %s", urlPath2));
  cCode = curl_easy_perform(cc.curl);
  LM_TMP(("curl_easy_perform returned %d", cCode));
  if (cCode != CURLE_OK)
  {
    release_curl_context(&cc);
    LM_E(("curl_easy_perform error %d", cCode));
    free(urlPath2);
    free(httpResponse.buf);
    free(initialUrl);
    return false;
  }

  // Got data in httpResponse.buf
  LM_TMP(("Got response: %s", httpResponse.buf));

  // Remove garbage at end of payload?
  char* endP = &httpResponse.buf[httpResponse.used];
  while ((*endP != '}') && (endP > httpResponse.buf))
  {
    --endP;
  }
  if (*endP == '}')
  {
    endP[1] = 0;
  }

  // Now parse the payload
  KjNode* tree = kjParse(ciP->kjsonP, httpResponse.buf);

  release_curl_context(&cc);
  free(urlPath2);
  free(httpResponse.buf);

  if (tree == NULL)
  {
    LM_E(("Error parsing context"));
    free(initialUrl);
    return false;
  }

  OrionldContext* contextP = (OrionldContext*) malloc(sizeof(OrionldContext));
  if (contextP == NULL)
  {
    LM_E(("Error allocating context"));
    free(initialUrl);
    return false;
  }
  
  contextP->url  = initialUrl;
  contextP->tree = tree;
  contextP->next = NULL;

  //
  // Appending new context to the list
  //
  if (orionLdContextHead == NULL)
  {
    orionLdContextHead = contextP;
    orionLdContextTail = contextP;
  }
  else
    orionLdContextTail->next = contextP;
  
  return true;
}



// -----------------------------------------------------------------------------
//
// orionldContextFreeAll -
//
void orionldContextFreeAll(void)
{
  OrionldContext* contextP = orionLdContextHead;

  LM_TMP(("Freeing all context"));
  while (contextP != NULL)
  {
    OrionldContext* next = contextP->next;

    kjFree(contextP->tree);
    free(contextP->url);
    free(contextP);
    contextP = next;
  }

  LM_TMP(("Freed all context"));
}
