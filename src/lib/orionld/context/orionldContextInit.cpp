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
* Author: Ken Zangelin
*/
#ifdef DEBUG
#include <sys/types.h>                                           // DIR, dirent
#include <fcntl.h>                                               // O_RDONLY
#include <dirent.h>                                              // opendir(), readdir(), closedir()
#include <sys/stat.h>                                            // statbuf
#include <unistd.h>                                              // stat()
#endif  // DEBUG

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem
#include "orionld/context/orionldCoreContext.h"                  // ORIONLD_CORE_CONTEXT_URL
#include "orionld/context/orionldContextCacheInit.h"             // orionldContextCacheInit
#include "orionld/context/orionldContextCacheInsert.h"           // orionldContextCacheInsert
#include "orionld/context/orionldContextFromBuffer.h"            // orionldContextFromBuffer
#include "orionld/context/orionldContextFromUrl.h"               // orionldContextFromUrl
#include "orionld/context/orionldContextItemLookup.h"            // orionldContextItemLookup
#include "orionld/context/orionldContextInit.h"                  // Own interface



#ifdef DEBUG
// -----------------------------------------------------------------------------
//
// contextFileParse -
//
int contextFileParse(char* fileBuffer, int bufLen, char** urlP, char** jsonP, OrionldProblemDetails* pdP)
{
  //
  // 1. Skip initial whitespace
  // Note: 0xD (13) is the Windows 'carriage ret' character
  //
  while ((*fileBuffer != 0) && ((*fileBuffer == ' ') || (*fileBuffer == '\t') || (*fileBuffer == '\n') || (*fileBuffer == 0xD)))
    ++fileBuffer;

  if (*fileBuffer == 0)
  {
    pdP->type   = OrionldBadRequestData;
    pdP->title  = (char*) "Invalid @context";
    pdP->detail = (char*) "empty context file (or, only whitespace)";
    pdP->status = 400;

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
    pdP->type   = OrionldBadRequestData;
    pdP->title  = (char*) "Invalid @context";
    pdP->detail = (char*) "can't find the end of the URL line";
    pdP->status = 400;

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
    pdP->type   = OrionldBadRequestData;
    pdP->title  = (char*) "Invalid @context";
    pdP->detail = (char*) "no JSON Context found";
    pdP->status = 400;

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
  char*                  fileBuffer;
  struct stat            statBuf;
  char                   path[512];
  OrionldProblemDetails  pd;

  snprintf(path, sizeof(path), "%s/%s", dir, dirItemP->d_name);
  LM_T(LmtPreloadedContexts, ("Treating 'preloaded' context file '%s'", path));

  if (stat(path, &statBuf) != 0)
    LM_X(1, ("stat(%s): %s", path, strerror(errno)));

  fileBuffer = (char*) malloc(statBuf.st_size + 1);
  if (fileBuffer == NULL)
    LM_X(1, ("Out of memory"));

  int fd = open(path, O_RDONLY);
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

  if (contextFileParse(fileBuffer, statBuf.st_size, &url, &json, &pd) != 0)
    LM_X(1, ("error parsing the context file '%s': %s", path, pd.detail));

  //
  // We have both the URL and the 'JSON Context'.
  // Time to parse the 'JSON Context', create the OrionldContext, and insert it into the list of contexts
  //

  OrionldContext* contextP = orionldContextFromBuffer(url, json, &pd);

  if (strcmp(url, ORIONLD_CORE_CONTEXT_URL) == 0)
  {
    if (contextP == NULL)
      LM_X(1, ("error creating the core context from file system file '%s'", path));
    orionldCoreContextP = contextP;
  }
  else
  {
    if (contextP == NULL)
      LM_E(("error creating context from file system file '%s'", path));
    else
      orionldContextCacheInsert(contextP);
  }
}



// -----------------------------------------------------------------------------
//
// fileSystemContexts -
//
static bool fileSystemContexts(char* cacheContextDir)
{
  DIR*            dirP;
  struct  dirent  dirItem;
  struct  dirent* result;

  dirP = opendir(cacheContextDir);
  if (dirP == NULL)
  {
    //
    // FIXME PR: Should the broker die here (Cache Context Directory given but it doesn't exist)
    //           or should the broker continue (downloading the core context) ???
    //           Continue, by returning false.
    LM_X(1, ("opendir(%s): %s", cacheContextDir, strerror(errno)));
  }

  while (readdir_r(dirP, &dirItem, &result) == 0)
  {
    if (result == NULL)
      break;

    if (dirItem.d_name[0] == '.')  // skip hidden files and '.'/'..'
      continue;

    contextFileTreat(cacheContextDir, &dirItem);
  }
  closedir(dirP);
  return true;
}
#endif  // DEBUG



// -----------------------------------------------------------------------------
//
// orionldContextInit -
//
bool orionldContextInit(OrionldProblemDetails* pdP)
{
  orionldContextCacheInit();

  bool  gotCoreContext  = false;

#if DEBUG
  char* cacheContextDir = getenv("ORIONLD_CACHED_CONTEXT_DIRECTORY");
  if (cacheContextDir != NULL)
  {
    gotCoreContext = fileSystemContexts(cacheContextDir);
    if (gotCoreContext == false)
      LM_E(("Unable to cache pre-loaded contexts from '%s'", cacheContextDir));
  }
#endif

  if (gotCoreContext == false)
  {
    orionldCoreContextP = orionldContextFromUrl(ORIONLD_CORE_CONTEXT_URL, pdP);

    if (orionldCoreContextP == NULL)
      return false;
  }

  OrionldContextItem* vocabP = orionldContextItemLookup(orionldCoreContextP, "@vocab", NULL);

  if (vocabP == NULL)
  {
    LM_E(("Context Error (no @vocab item found in Core Context)"));
    orionldDefaultUrl = (char*) "https://example.org/ngsi-ld/default/";
  }
  else
    orionldDefaultUrl = vocabP->id;

  orionldDefaultUrlLen = strlen(orionldDefaultUrl);

  return true;
}
