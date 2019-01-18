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
#include <microhttpd.h>

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kalloc/kaBufferInit.h"                               // kaBufferInit
#include "kjson/kjInit.h"                                      // kjInit
#include "kjson/kjBufferCreate.h"                              // kjBufferCreate
#include "kjson/kjParse.h"                                     // kjParse
#include "kjson/kjClone.h"                                     // kjClone
}

#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/common/OrionldConnection.h"                  // Global vars: kjson, kalloc, kallocBuffer, ...
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/context/orionldContextDownloadAndParse.h"    // orionldContextDownloadAndParse
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext, ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInit, orionldContextListInsert
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService, ORION_LD_SERVICE_PREFIX_LEN
#include "orionld/rest/temporaryErrorPayloads.h"               // Temporary Error Payloads
#include "orionld/rest/orionldMhdConnection.h"                 // Own Interface



// -----------------------------------------------------------------------------
//
// orionldHostName
//
char orionldHostName[1024];
int  orionldHostNameLen = -1;



// -----------------------------------------------------------------------------
//
// libLogBuffer -
//
thread_local char libLogBuffer[1024 * 8];



// -----------------------------------------------------------------------------
//
// libLogFunction -
//
static void libLogFunction
(
  int          severity,              // 1: Error, 2: Warning, 3: Info, 4: Verbose, 5: Trace
  int          level,                 // Trace level || Error code || Info Code
  const char*  fileName,
  int          lineNo,
  const char*  functionName,
  const char*  format,
  ...
)
{
  va_list  args;

  /* "Parse" the variable arguments */
  va_start(args, format);

  /* Print message to variable */
  vsnprintf(libLogBuffer, sizeof(libLogBuffer), format, args);
  va_end(args);

  // LM_TMP(("Got a message, severity: %d", severity));

  if (severity == 1)
    lmOut(libLogBuffer, 'E', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 2)
    lmOut(libLogBuffer, 'W', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 3)
    lmOut(libLogBuffer, 'I', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 4)
    lmOut(libLogBuffer, 'V', fileName, lineNo, functionName, 0, NULL);
  else if (severity == 5)
    lmOut(libLogBuffer, 'T', fileName, lineNo, functionName, level + LmtKjlParse, NULL);
}



// -----------------------------------------------------------------------------
//
// orionldRestServiceV -
//
OrionLdRestServiceVector orionldRestServiceV[9];



// -----------------------------------------------------------------------------
//
// restServicePrepare -
//
static void restServicePrepare(OrionLdRestService* serviceP, OrionLdRestServiceSimplified* simpleServiceP)
{
  // 1. Simply copy/reference the two fields of OrionLdRestServiceSimplified
  serviceP->url             = (char*) simpleServiceP->url;
  serviceP->serviceRoutine  = simpleServiceP->serviceRoutine;

  // 2. Loop over the URL Path, count wildcards, charsBeforeFirstWildcard, etc
  int    ix            = ORION_LD_SERVICE_PREFIX_LEN - 1;
  char*  wildCardStart = NULL;
  char*  wildCardEnd   = NULL;

  while (serviceP->url[++ix] != 0)
  {
    char c = serviceP->url[ix];

    if (c == '*')
    {
      if (serviceP->wildcards == 0)
        wildCardStart = &serviceP->url[ix + 1];
      else if (serviceP->wildcards == 1)
        wildCardEnd = &serviceP->url[ix];

      LM_T(LmtUrlParse, ("Found a wildcard in index %d of '%s'", ix, serviceP->url));
      serviceP->wildcards += 1;
      continue;
    }

    if (serviceP->wildcards == 0)
    {
      ++serviceP->charsBeforeFirstWildcard;
      serviceP->charsBeforeFirstWildcardSum += c;
    }
    else if (serviceP->wildcards == 1)
    {
      ++serviceP->charsBeforeSecondWildcard;
      serviceP->charsBeforeSecondWildcardSum += c;
    }
  }

  if (serviceP->wildcards != 0)
  {
    if (wildCardEnd == NULL)  // If only one '*', make wildCardEnd point to end of URL
      wildCardEnd = &serviceP->url[ix];

    serviceP->matchForSecondWildcardLen = wildCardEnd - wildCardStart;

    if (serviceP->matchForSecondWildcardLen < 0)
      LM_X(1, ("Negative length of matchForSecondWildcard - not possible. SW bug"));
    if (serviceP->matchForSecondWildcardLen > (int) sizeof(serviceP->matchForSecondWildcard))
      LM_X(1, ("Too big matchForSecondWildcard - not possible. SW bug"));

    if (serviceP->matchForSecondWildcardLen != 0)
    {
      strncpy(serviceP->matchForSecondWildcard, wildCardStart, wildCardEnd - wildCardStart);
      LM_T(LmtUrlParse, ("matchForSecondWildcard: %s", serviceP->matchForSecondWildcard));
    }
  }
}



// -----------------------------------------------------------------------------
//
// orionldServiceInit -
//
// This function converts the OrionLdRestServiceSimplified vectors to OrionLdRestService vectors
//
void orionldServiceInit(OrionLdRestServiceSimplifiedVector* restServiceVV, int vecItems, bool defContextFromFile)
{
  int    svIx;    // Service Vector Index
  char*  details;
  int    retries;

  bzero(orionldRestServiceV, sizeof(orionldRestServiceV));

  for (svIx = 0; svIx < vecItems; svIx++)
  {
    // svIx is really the Verb (GET=0, POST=2, up to NOVERB=9. See src/lib/rest/Verb.h)

    LM_T(LmtUrlParse, ("svIx: %d", svIx));
    if (restServiceVV[svIx].serviceV == NULL)
      continue;

    int services = restServiceVV[svIx].services;

    orionldRestServiceV[svIx].serviceV  = (OrionLdRestService*) calloc(sizeof(OrionLdRestService), services);
    orionldRestServiceV[svIx].services  = services;

    int sIx;  // Service Index inside Rest Service vector

    for (sIx = 0; sIx < services; sIx++)
    {
      LM_T(LmtUrlParse, ("sIx: %d", sIx));
      restServicePrepare(&orionldRestServiceV[svIx].serviceV[sIx], &restServiceVV[svIx].serviceV[sIx]);
    }
  }

  //
  // Initialize the KSON library
  //
  kjInit(libLogFunction);

  // Set up the global kjson instance with preallocated kalloc buffer
  kaBufferInit(&kalloc, kallocBuffer, sizeof(kallocBuffer), 2 * 1024, NULL, "Global KAlloc buffer");
  kjsonP = kjBufferCreate(&kjson, &kalloc);


  //
  // Initialize the global Context List
  //
  if (orionldContextListInit(&details) != 0)
    LM_X(1, ("Internal Error (orionldContextListInit failed: %s)", details));


  //
  // Now download the default context
  // FIXME: Save the default context in mongo?
  //
  details  = (char*) "OK";
  retries  = 0;

  orionldCoreContext.url           = ORIONLD_CORE_CONTEXT_URL;
  orionldCoreContext.next          = NULL;
  orionldCoreContext.tree          = NULL;
  orionldCoreContext.type          = OrionldCoreContext;
  orionldCoreContext.ignore        = true;

  orionldDefaultUrlContext.url     = ORIONLD_DEFAULT_URL_CONTEXT_URL;
  orionldDefaultUrlContext.next    = NULL;
  orionldDefaultUrlContext.tree    = NULL;
  orionldDefaultUrlContext.type    = OrionldDefaultUrlContext;
  orionldDefaultUrlContext.ignore  = true;

  orionldDefaultContext.url        = ORIONLD_DEFAULT_CONTEXT_URL;
  orionldDefaultContext.next       = NULL;
  orionldDefaultContext.tree       = NULL;
  orionldDefaultContext.type       = OrionldDefaultContext;
  orionldDefaultContext.ignore     = true;

  if (defContextFromFile == true)
  {
    char* buf;

    buf = strdup(orionldCoreContextString);
    orionldCoreContext.tree = kjParse(kjsonP, buf);

    buf = strdup(orionldDefaultUrlContextString);
    orionldDefaultUrlContext.tree = kjParse(kjsonP, buf);

    buf = strdup(orionldDefaultContextString);
    orionldDefaultContext.tree = kjParse(kjsonP, buf);
  }
  else
  {
    // Download the Core Context
    while ((orionldCoreContext.tree == NULL) && (retries < 5))
    {
      orionldCoreContext.tree = orionldContextDownloadAndParse(kjsonP, ORIONLD_CORE_CONTEXT_URL, &details);
      if (orionldCoreContext.tree != NULL)
        break;

      ++retries;
      LM_E(("Error %d downloading Core Context %s: %s", retries, ORIONLD_CORE_CONTEXT_URL, details));
    }

    // Download the "Default URL" context
    retries = 0;
    while ((orionldDefaultUrlContext.tree == NULL) && (retries < 5))
    {
      orionldDefaultUrlContext.tree = orionldContextDownloadAndParse(kjsonP, ORIONLD_DEFAULT_URL_CONTEXT_URL,  &details);
      if (orionldDefaultUrlContext.tree != NULL)
        break;

      ++retries;
      LM_E(("Error %d downloading Default URI Context %s: %s", retries, ORIONLD_DEFAULT_URL_CONTEXT_URL, details));
    }

    // Download the "Default" context
    retries = 0;
    while ((orionldDefaultContext.tree == NULL) && (retries < 5))
    {
      orionldDefaultContext.tree = orionldContextDownloadAndParse(kjsonP, ORIONLD_DEFAULT_CONTEXT_URL,  &details);
      if (orionldDefaultContext.tree != NULL)
        break;

      ++retries;
      LM_E(("Error %d downloading Default Context %s: %s", retries, ORIONLD_DEFAULT_CONTEXT_URL, details));
    }
  }

  if ((orionldCoreContext.tree == NULL) || (orionldDefaultUrlContext.tree == NULL) || (orionldDefaultContext.tree == NULL))
    LM_X(1, ("EXITING - Without default context, orionld cannot function - error downloading default context '%s': %s", ORIONLD_CORE_CONTEXT_URL, details));

  // Adding the core context to the list of contexts
  orionldContextListInsert(&orionldCoreContext, false);

  // Adding the Default URL context to the list of contexts
  orionldContextListInsert(&orionldDefaultUrlContext, false);

  //
  // Checking the "Default URL Context" and extracting the Default URL path.
  //
  KjNode* contextNodeP = orionldDefaultUrlContext.tree->value.firstChildP;

  if (contextNodeP == NULL)
    LM_X(1, ("Invalid Default URL Context - Empty JSON object"));

  if (strcmp(contextNodeP->name, "@context") != 0)
    LM_X(1, ("Invalid Default URL Context - first (and only) member must be called '@context'"));

  if (contextNodeP->type != KjObject)
    LM_X(1, ("Invalid Default URL Context - not a JSON object"));

  if (contextNodeP->next != NULL)
    LM_X(1, ("Invalid Default URL Context - '@context' must be the only member of the toplevel object"));

  KjNode* vocabNodeP = contextNodeP->value.firstChildP;

  if (strcmp(vocabNodeP->name, "@vocab") != 0)
    LM_X(1, ("Invalid Default URL Context - first (and only) member of '@context' must be called '@vocab'"));

  if (vocabNodeP->type != KjString)
    LM_X(1, ("Invalid Default URL Context - the member '@vocab' must be of 'String' type"));

  orionldDefaultUrl    = strdup(vocabNodeP->value.s);
  orionldDefaultUrlLen = strlen(orionldDefaultUrl);

  if (urlCheck(orionldDefaultUrl, &details) == false)
    LM_X(1, ("Invalid Default URL Context - the value (%s) of the only member '@vocab' must be a valid URL: %s", orionldDefaultUrl, details));

  // Finally, get the hostname
  gethostname(orionldHostName, sizeof(orionldHostName));
  orionldHostNameLen = strlen(orionldHostName);
}
