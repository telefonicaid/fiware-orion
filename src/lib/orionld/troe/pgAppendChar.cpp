/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include <string.h>                                            // strlen, strncpy
#include <stdlib.h>                                            // malloc

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/PgAppendBuffer.h"                       // PgAppendBuffer
#include "orionld/troe/pgAppendChar.h"                         // Own interface



// ----------------------------------------------------------------------------
//
// pgAppendChar -
//
void pgAppendChar(PgAppendBuffer* pgBufP, char c)
{
  // realloc necessary?
  if (pgBufP->currentIx + 1 >= pgBufP->bufSize)
  {
    char* old = pgBufP->buf;

    pgBufP->bufSize += 4 * 1024;  // Add 4k every time

    if (pgBufP->bufSize < 16 * 1024)  // Use kaAlloc for smaller buffers
      pgBufP->buf = kaAlloc(&orionldState.kalloc, pgBufP->bufSize);
    else
    {
      if (pgBufP->bufSize > 32 * 1024)
        pgBufP->bufSize += 12 * 1024;  // Add an additional 12k if buffer > 32k (16k in total: 4+12)

      pgBufP->buf = (char*) malloc(pgBufP->bufSize);
    }

    strncpy(pgBufP->buf, old, pgBufP->currentIx);

    if (pgBufP->toFree != NULL)
    {
      free(pgBufP->toFree);
      pgBufP->toFree = pgBufP->buf;
    }
  }

  pgBufP->buf[pgBufP->currentIx]     = c;
  pgBufP->buf[pgBufP->currentIx + 1] = 0;

  pgBufP->currentIx += 1;
}
