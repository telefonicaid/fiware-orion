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
#include <unistd.h>                                              // usleep
#include <semaphore.h>                                           // sem_init, sem_wait, sem_post

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldContext.h"                        // OrionldContext
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/context/orionldContextFromBuffer.h"            // orionldContextFromBuffer
#include "orionld/contextCache/orionldContextCacheLookup.h"      // orionldContextCacheLookup
#include "orionld/context/orionldContextDownload.h"              // orionldContextDownload
#include "orionld/contextCache/orionldContextCachePersist.h"     // orionldContextCachePersist
#include "orionld/context/orionldContextFromUrl.h"               // Own interface



// -----------------------------------------------------------------------------
//
// StringListItem -
//
typedef struct StringListItem
{
  char                    name[256];
  struct StringListItem*  next;
} StringListItem;

static sem_t            contextDownloadListSem;
static StringListItem*  contextDownloadList = NULL;



// -----------------------------------------------------------------------------
//
// contextDownloadListInit - initialize the 'context download list'
//
void contextDownloadListInit(void)
{
  sem_init(&contextDownloadListSem, 0, 1);  // 0: shared between threads of the same process. 1: free to be taken
  contextDownloadList = NULL;
}



// -----------------------------------------------------------------------------
//
// contextDownloadListLookup - lookup a URL in the list and report if found or not
//
bool contextDownloadListLookup(const char* url)
{
  StringListItem* itemP = contextDownloadList;

  LM_T(LmtContextDownload, ("Looking for context URL '%s'", url));
  while (itemP != NULL)
  {
    LM_T(LmtContextDownload, ("Comparing existing '%s' to wanted '%s'", itemP->name, url));
    if (strcmp(itemP->name, url) == 0)
    {
      LM_T(LmtContextDownload, ("Found a match: '%s'", url));
      return true;
    }

    itemP = itemP->next;
  }

  LM_T(LmtContextDownload, ("Found no match for '%s'", url));
  return false;
}



// -----------------------------------------------------------------------------
//
// contextDownloadListDebug -
//
static void contextDownloadListDebug(const char* what)
{
  LM_T(LmtContextDownload, ("contextDownloadList (%s)", what));
  LM_T(LmtContextDownload, ("----------------------------------------------------"));

  for (StringListItem* iterP = contextDownloadList; iterP != NULL; iterP = iterP->next)
  {
    LM_T(LmtContextDownload, ("  o %s", iterP->name));
  }

  LM_T(LmtContextDownload, ("----------------------------------------------------"));
}



// -----------------------------------------------------------------------------
//
// contextDownloadListAdd - add a URL to the list
//
void contextDownloadListAdd(const char* url)
{
  StringListItem* itemP = (StringListItem*) malloc(sizeof(StringListItem));

  LM_T(LmtContextDownload, ("Adding '%s' to contextDownloadList", url));
  strncpy(itemP->name, url, sizeof(itemP->name) - 1);
  itemP->next = contextDownloadList;
  contextDownloadList = itemP;
  contextDownloadListDebug("after item added");
}



// -----------------------------------------------------------------------------
//
// contextDownloadListRemove - remove a URL from the list
//
void contextDownloadListRemove(const char* url)
{
  StringListItem* iterP = contextDownloadList;
  StringListItem* prevP = NULL;
  StringListItem* itemP = NULL;

  LM_T(LmtContextDownload, ("Removing '%s' from contextDownloadList", url));

  while (iterP != NULL)
  {
    if (strcmp(iterP->name, url) == 0)
    {
      itemP = iterP;
      break;
    }

    prevP = iterP;
    iterP = iterP->next;
  }

  if (itemP == NULL)  // Not found!
  {
    LM_T(LmtContextDownload, ("Cannot find '%s' in contextDownloadList", url));
    return;
  }

  if (prevP == NULL)  // Found as the first item of the list
  {
    LM_T(LmtContextDownload, ("Removing '%s' as first item in contextDownloadList", url));
    contextDownloadList = itemP->next;
    free(itemP);
  }
  else if (itemP->next == NULL)  // Found as the last item of the list
  {
    LM_T(LmtContextDownload, ("Removing '%s' as last item in contextDownloadList", url));
    prevP->next = NULL;
    free(itemP);
  }
  else  // Found in the middle of the list
  {
    LM_T(LmtContextDownload, ("Removing '%s' as middle item in contextDownloadList", url));
    prevP->next = itemP->next;
    free(itemP);
  }

  contextDownloadListDebug("after item removal");
}



// -----------------------------------------------------------------------------
//
// contextDownloadListRelease - release all items in the 'context download cache'
//
// The cache is self-cleaning and this function isn't really necessary - except perhaps
// if the broker is killed while serving requests, e.g. while running tests.
//
// This function is ONLY called from the main exit-function, to avoid leaks for valgrind tests.
//
void contextDownloadListRelease(void)
{
  StringListItem* iterP = contextDownloadList;

  while (iterP != NULL)
  {
    StringListItem* current = iterP;
    iterP = iterP->next;
    free(current);
  }
}



// -----------------------------------------------------------------------------
//
// contextCacheWait -
//
static OrionldContext* contextCacheWait(char* url)
{
  int             sleepTime = 0;
  OrionldContext* contextP;

  LM_T(LmtContextDownload, ("Awaiting a context download by other (URL: %s)", url));

  while (sleepTime < 3000000)  // 3 secs - 3 million microsecs ... CLI param?
  {
    usleep(20000);  // sleep 20 millisecs ... CLI param?
    LM_T(LmtContextDownload, ("Awaiting context download: looking up context '%s'", url));
    contextP = orionldContextCacheLookup(url);
    if (contextP != NULL)
    {
      LM_T(LmtContextDownload, ("Got it! (%s)", url));
      return contextP;
    }
    LM_T(LmtContextDownload, ("Still not there (%s)", url));
    sleepTime += 20000;
  }
  LM_T(LmtContextDownload, ("Timeout during download of an @context (%s)", url));

  // The wait timed out
  orionldError(OrionldInternalError, "Timeout during download of an @context", url, 504);
  return NULL;
}



// -----------------------------------------------------------------------------
//
// orionldContextFromUrl -
//
OrionldContext* orionldContextFromUrl(char* url, char* id)
{
  LM_T(LmtContextDownload, ("Possibly downloading a context URL: '%s'", url));

  OrionldContext* contextP = orionldContextCacheLookup(url);

  if (contextP != NULL)
  {
    LM_T(LmtContextDownload, ("Found already downloaded URL '%s'", url));
    return contextP;
  }

  //
  // Make sure the context isn't already being downloaded
  //
  // Three possibilities:
  // CASE 1. No one is trying to download the context, so:
  //         - mark the URL to be downloading - for the next to know
  //         - download it and add it to the context cache
  //         - remove the mark from step 1
  // CASE 2. No one was trying to download the context, so I tried to take the semaphore.
  //         However, another thread got the semaphore before me and started the download
  //         In this case, I will NOT try to download (somebody else is already doing that).
  //         Instead, I will wait for that download to finish and then lookup the context from the cache
  //
  // CASE 3. Someone was actually downloading the context when I wanted to do the same.
  //         Just like step 2 - I wait for the download to complete and then lookup the context from the cache.
  //
  bool urlDownloading = contextDownloadListLookup(url);
  if (urlDownloading == false)
  {
    //
    // Not there, so, we'll download it
    // First take the 'download semaphore'
    //
    LM_T(LmtContextDownload, ("The context '%s' is not downloading, getting the downloadList semaphore", url));
    sem_wait(&contextDownloadListSem);
    LM_T(LmtContextDownload, ("Got the downloadList semaphore for '%s'", url));

    //
    // OK - got the semaphore - but, did I have to wait?
    // I must look the context up again, to be sure (in the 'contextDownloadList')
    //
    // If the URL is in 'contextDownloadList' then somebody else took the semaphore before me
    // and started downloading.
    //
    LM_T(LmtContextDownload, ("Looking up '%s' again, in case I got the semaphore late", url));
    urlDownloading = contextDownloadListLookup(url);
    if (urlDownloading == false)
    {
      LM_T(LmtContextDownload, ("The context '%s' is not downloading by other - will be downloaded here", url));
      contextDownloadListAdd(url);  // CASE 1: Mark the URL as being downloading
    }

    LM_T(LmtContextDownload, ("Giving back the downloadList semaphore for '%s'", url));
    sem_post(&contextDownloadListSem);

    if (urlDownloading == true)  // If somebody has taken the semaphore before me and is downloading the context - I'll have to wait
    {
      LM_T(LmtContextDownload, ("The context '%s' is downloading by other - I wait until it's done", url));
      return contextCacheWait(url);  // CASE 2 - another thread has downloaded the context
    }

    // CASE 1 - the context will be downloaded
  }
  else
  {
    LM_T(LmtContextDownload, ("The context '%s' is downloading by other - I wait until it's done", url));
    return contextCacheWait(url);  // CASE 3 - another thread has downloaded the context
  }

  LM_T(LmtContextDownload, ("Downloading the context '%s' and adding it to the context cache", url));
  char* buffer = orionldContextDownload(url);  // orionldContextDownload fills in ProblemDetails

  if (buffer != NULL)  // All OK
  {
    LM_T(LmtCoreContext, ("Downloaded the context '%s'", url));
    contextP = orionldContextFromBuffer(url, OrionldContextDownloaded, id, buffer);
    if (contextP == NULL)
      LM_E(("Context Error (%s: %s)", orionldState.pd.title, orionldState.pd.detail));
  }
  else
    LM_E(("Context Error (%s: %s)", orionldState.pd.title, orionldState.pd.detail));

  if (contextP != NULL)
  {
    contextP->origin    = OrionldContextDownloaded;
    contextP->createdAt = orionldState.requestTime;
    contextP->usedAt    = orionldState.requestTime;

    orionldContextCachePersist(contextP);
  }

  // Remove the 'url' from the contextDownloadList and persist it to DB
  sem_wait(&contextDownloadListSem);
  contextDownloadListRemove(url);
  sem_post(&contextDownloadListSem);

  return contextP;
}
