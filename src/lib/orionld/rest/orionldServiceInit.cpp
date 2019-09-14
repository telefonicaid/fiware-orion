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
#ifdef DEBUG
#include <sys/types.h>                                         // DIR, dirent
#include <fcntl.h>                                             // O_RDONLY
#include <dirent.h>                                            // opendir(), readdir(), closedir()
#include <sys/stat.h>                                          // statbuf
#include <unistd.h>                                            // stat()
#endif

#include <microhttpd.h>

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

extern "C"
{
#include "kalloc/kaBufferInit.h"                               // kaBufferInit
#include "kjson/kjInit.h"                                      // kjInit
#include "kjson/kjBufferCreate.h"                              // kjBufferCreate
#include "kjson/kjParse.h"                                     // kjParse
}

#include "orionld/common/OrionldConnection.h"                  // Global vars: orionldState, kjson, kalloc, kallocBuffer, ...
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/context/orionldContextDownloadAndParse.h"    // orionldContextDownloadAndParse
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContext, ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextListInsert.h"          // orionldContextListInit, orionldContextListInsert
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService, ORION_LD_SERVICE_PREFIX_LEN
#include "orionld/rest/temporaryErrorPayloads.h"               // Temporary Error Payloads
#include "orionld/serviceRoutines/orionldPostEntities.h"       // orionldPostEntities
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"  // orionldPostSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"   // orionldGetSubscriptions
#include "orionld/serviceRoutines/orionldGetSubscription.h"    // orionldGetSubscription
#include "orionld/serviceRoutines/orionldPostRegistrations.h"  // orionldPostRegistrations
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
thread_local char libLogBuffer[1024 * 32];



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

  // LM_TMP(("Got a message, severity: %d: %s", severity, libLogBuffer));

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

  //
  // Set options for the OrionLdRestService
  //
  // 1. For POST /ngsi-ld/v1/entities:
  //    - The context may have to be saved in the context cache (if @context is non-string in payload)
  //      - Why?  The broker needs to be able to serve it later
  //    - The Entity ID and type must be looked up and removed from the payload (pointers to the trees in orionldState
  //      - Why? The Entity ID is used when naming the context
  //      - The Entity Type is removed and saved to save time in the service routine (all on level 1 are attributes)
  //
  //
  if (serviceP->serviceRoutine == orionldPostEntities)
  {
    serviceP->options  = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldPostSubscriptions)
  {
    serviceP->options  = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscriptions)
  {
    serviceP->options  = ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldGetSubscription)
  {
    serviceP->options  = ORIONLD_SERVICE_OPTION_DONT_ADD_CONTEXT_TO_RESPONSE_PAYLOAD;
  }
  else if (serviceP->serviceRoutine == orionldPostRegistrations)
  {
    serviceP->options  = ORIONLD_SERVICE_OPTION_PREFETCH_ID_AND_TYPE;
    serviceP->options |= ORIONLD_SERVICE_OPTION_CREATE_CONTEXT;
  }
}



#ifdef DEBUG
// -----------------------------------------------------------------------------
//
// contextFileParse -
//
int contextFileParse(char* fileBuffer, int bufLen, char** urlP, char** jsonP, char** errorStringP)
{
  //
  // 1. Skip initial whitespace
  // Note: 0xD (13) is the Windows 'carriage ret' character
  //
  while ((*fileBuffer != 0) && ((*fileBuffer == ' ') || (*fileBuffer == '\t') || (*fileBuffer == '\n') || (*fileBuffer == 0xD)))
    ++fileBuffer;

  if (*fileBuffer == 0)
  {
    *errorStringP = (char*) "empty context file (or, only whitespace)";
    return -1;
  }


  //
  // 2. The URL is on the first line of the buffer
  //
  *urlP = fileBuffer;
  LM_T(LmtPreloadedContexts, ("Parsing fileBuffer. URL is %s", *urlP));


  //
  // 3. Find the '\n' that ends the URL
  //
  while ((*fileBuffer != 0) && (*fileBuffer != '\n'))
    ++fileBuffer;

  if (*fileBuffer == 0)
  {
    *errorStringP = (char*) "can't find the end of the URL line";
    return -1;
  }


  //
  // 4. Zero-terminate URL
  //
  *fileBuffer = 0;


  //
  // 5. Jump over the \n and onto the first char of the next line
  //
  ++fileBuffer;


  //
  // 1. Skip initial whitespace
  // Note: 0xD (13) is the Windows 'carriage ret' character
  //
  while ((*fileBuffer != 0) && ((*fileBuffer == ' ') || (*fileBuffer == '\t') || (*fileBuffer == '\n') || (*fileBuffer == 0xD)))
    ++fileBuffer;

  if (*fileBuffer == 0)
  {
    *errorStringP = (char*) "no JSON Context found";
    return -1;
  }

  *jsonP = fileBuffer;
  LM_T(LmtPreloadedContexts, ("Parsing fileBuffer. JSON is %s", *jsonP));

  return 0;
}



// -----------------------------------------------------------------------------
//
// contextFileTreat -
//
static void contextFileTreat(char* dir, struct dirent* dirItemP)
{
  char*        fileBuffer;
  struct stat  statBuf;
  char         path[512];

  snprintf(path, sizeof(path), "%s/%s", dir, dirItemP->d_name);
  LM_T(LmtPreloadedContexts, ("Treating 'preloaded' context file '%s'", path));

  if (stat(path, &statBuf) != 0)
    LM_X(1, ("stat(%s): %s", path, strerror(errno)));

  fileBuffer = (char*) malloc(statBuf.st_size + 1);
  if (fileBuffer == NULL)
    LM_X(1, ("Out of memory"));

  int fd;
  fd = open(path, O_RDONLY);
  if (fd == -1)
    LM_X(1, ("open(%s): %s", path, strerror(errno)));

  int nb;
  nb = read(fd, fileBuffer, statBuf.st_size);
  if (nb != statBuf.st_size)
    LM_X(1, ("read(%s): %s", path, strerror(errno)));
  fileBuffer[statBuf.st_size] = 0;
  close(fd);

  //
  // OK, the entire buffer is in 'fileBuffer'
  // Now let's parse the buffer to extract URL (first line)
  // and the "payload" that is the JSON of the context
  //
  char* url;
  char* json;
  char* errorString;

  if (contextFileParse(fileBuffer, statBuf.st_size, &url, &json, &errorString) != 0)
    LM_X(1, ("error parsing the context file '%s': %s", path, errorString));

  //
  // We have both the URL and the 'JSON Context'.
  // Time to parse the 'JSON Context', create the OrionldContext, and insert it into the list of contexts
  //
  KjNode* tree = kjParse(kjsonP, json);

  if (tree == NULL)
    LM_X(1, ("error parsing the JSON context of context file '%s'", path));

  LM_T(LmtPreloadedContexts, ("Successfully parsed the preloaded JSON of context '%s'", url));

  //
  // Is it the Core Context?
  //
  if (strcmp(url, ORIONLD_CORE_CONTEXT_URL) == 0)
  {
    orionldCoreContext.type   = OrionldCoreContext;
    orionldCoreContext.url    = url;
    orionldCoreContext.tree   = tree;
    orionldCoreContext.ignore = true;
    orionldCoreContext.next   = NULL;
  }
  else
  {
    OrionldContext* contextP = (OrionldContext*) malloc(sizeof(OrionldContext));

    if (contextP == NULL)
      LM_X(1, ("Out of memory"));

    contextP->url    = url;
    contextP->tree   = tree;
    contextP->ignore = false;
    contextP->type   = OrionldUserContext;

    orionldContextListInsert(contextP, true);  // true: sem not taken, just not needed here
  }
}
#endif



// -----------------------------------------------------------------------------
//
// orionldServiceInit -
//
// This function converts the OrionLdRestServiceSimplified vectors to OrionLdRestService vectors
//
void orionldServiceInit(OrionLdRestServiceSimplifiedVector* restServiceVV, int vecItems, char* cachedContextDir)
{
  int    svIx;    // Service Vector Index
  char*  details;

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

#ifdef DEBUG
  if (cachedContextDir != NULL)
  {
    DIR*            dirP;
    struct  dirent  dirItem;
    struct  dirent* result;

    dirP = opendir(cachedContextDir);
    if (dirP == NULL)
      LM_X(1, ("opendir(%s): %s", cachedContextDir, strerror(errno)));

    while (readdir_r(dirP, &dirItem, &result) == 0)
    {
      if (result == NULL)
        break;

      if (dirItem.d_name[0] == '.')  // skip hidden files and '.'/'..'
        continue;

      contextFileTreat(cachedContextDir, &dirItem);
    }
    closedir(dirP);
  }
  else
#endif
  {
    int    retries = 0;

    orionldCoreContext.url           = ORIONLD_CORE_CONTEXT_URL;
    orionldCoreContext.next          = NULL;
    orionldCoreContext.tree          = NULL;
    orionldCoreContext.type          = OrionldCoreContext;
    orionldCoreContext.ignore        = true;

    // Download the Core Context
    while ((orionldCoreContext.tree == NULL) && (retries < 5))
    {
      orionldCoreContext.tree = orionldContextDownloadAndParse(kjsonP, ORIONLD_CORE_CONTEXT_URL, false, &details);
      if (orionldCoreContext.tree != NULL)
        break;

      ++retries;
      LM_E(("Error %d downloading Core Context %s: %s", retries, ORIONLD_CORE_CONTEXT_URL, details));
    }
  }

  if (orionldCoreContext.tree == NULL)
    LM_X(1, ("EXITING - Without default context, orionld cannot function - error downloading default context '%s': %s", ORIONLD_CORE_CONTEXT_URL, details));

  //
  // Setting the Core Context to be ignored in lookups
  // This is to ignore the Core context as part of User Contexts
  //
  // An alternative method would to be to not add the Core/Default context to the context cache
  // But, if we do that, then orionldContextLookup would need to do strcmp with the Core Context also
  //
  orionldCoreContext.ignore        = true;

  //
  // FIXME: Should the Core Context be in the Cache or not ?
  //
  orionldContextListInsert(&orionldCoreContext, false);


  //
  // Get the hostname - needed for contexts created by the broker
  //
  gethostname(orionldHostName, sizeof(orionldHostName));
  orionldHostNameLen = strlen(orionldHostName);


  //
  // Checking the Core Context and extracting the Default URL path.
  //
  KjNode* contextNodeP = orionldCoreContext.tree->value.firstChildP;

  if (contextNodeP == NULL)
    LM_X(1, ("Invalid Core Context - Empty JSON object"));

  if (strcmp(contextNodeP->name, "@context") != 0)
    LM_X(1, ("Invalid Core Context - first (and only) member must be called '@context'"));

  if (contextNodeP->type != KjObject)
    LM_X(1, ("Invalid Core Context - not a JSON object"));

  if (contextNodeP->next != NULL)
    LM_X(1, ("Invalid Core Context - '@context' must be the only member of the toplevel object"));

  KjNode* vocabNodeP = NULL;
  for (KjNode* nodeP = contextNodeP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "@vocab") == 0)
    {
      vocabNodeP = nodeP;
      break;
    }
  }

  if (vocabNodeP == NULL)
  {
    //
    // FIXME: Temporarily acceptingh no @vocab in Core Context
    //
    LM_W(("WARNING - The Core Context didn't contain any '@vocab' item - OK for now ..."));
    orionldDefaultUrl    = (char*) "http://example.org/ngsi-ld/default/";
    orionldDefaultUrlLen = strlen(orionldDefaultUrl);
    return;
  }

  if (vocabNodeP->type != KjString)
    LM_X(1, ("Invalid Core Context - the member '@vocab' must be of 'String' type"));

  orionldDefaultUrl    = strdup(vocabNodeP->value.s);
  orionldDefaultUrlLen = strlen(orionldDefaultUrl);

  if (urlCheck(orionldDefaultUrl, &details) == false)
    LM_X(1, ("Invalid Core Context - the value (%s) of the member '@vocab' must be a valid URL: %s", orionldDefaultUrl, details));
}
