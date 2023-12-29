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
#include <string.h>                                            // strncpy

extern "C"
{
#include "kalloc/kaStrdup.h"                                   // kaStrdup
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/context/orionldSubAttributeExpand.h"         // orionldSubAttributeExpand
#include "orionld/q/qVariableFix.h"                            // Own interface



// ----------------------------------------------------------------------------
//
// qVariableFix -
//
// - If simple attribute name - all OK
// - If attr.b.c, then 'attr' must be extracted, expanded and then '.md.b.c' appended
// - If attr[b.c], then 'attr' must be extracted, expanded and then '.b.c' appended
//
// After implementing expansion in metadata names, for attr.b.c, 'b' needs expansion also
//
char* qVariableFix(char* varPathIn, bool forDb, bool* isMdP, char** detailsP)
{
  char  varPath[1024];  // Can't destroy varPathIn - need to copy it

  strncpy(varPath, varPathIn, sizeof(varPath) - 1);

  char* cP            = varPath;
  char* attrNameP     = varPath;
  char* firstDotP     = NULL;
  char* secondDotP    = NULL;
  char* startBracketP = NULL;
  char* endBracketP   = NULL;
  char* mdNameP       = NULL;
  char* rest          = NULL;
  char  fullPath[1500];

  //
  // Cases (for forDb==true):
  //
  // 1. A                 => single attribute     => attrs.A.value
  // 2. A[B]    (qPath)   => B is inside A.value  => attrs.A.value.B
  // 3. A.B     (mqPath)  => B is a metadata      => attrs.A.md.B.value   OR  "attrs.A.md.B" if B is TIMESTAMP and forDb==false
  // 4. A.B.C   (mqPath)  => B is a metadata      => attrs.A.md.B.value.C
  //
  // - There can be only one '[' in the path
  // - If '[' is found, then there must be a matching ']'
  //
  // So, we need to know:
  // - attrName
  // - mdName (if NO '[' in path)
  // - rest
  //   - For "A.B.C",  attrName == "A",                rest == "B.C"
  //   - For "A[B.C]", attrName == "A", mdName == "B", rest == "C"
  //   => rest == After first '.'
  //
  while (*cP != 0)
  {
    if (*cP == '.')
    {
      if (firstDotP == NULL)
        firstDotP = cP;
      else if (secondDotP == NULL)
        secondDotP = cP;
    }
    else if (*cP == '[')
    {
      if (startBracketP != NULL)
      {
        *detailsP = (char*) "More than one start brackets found";
        return NULL;
      }
      startBracketP  = cP;
    }
    else if (*cP == ']')
    {
      if (endBracketP != NULL)
      {
        *detailsP = (char*) "More than one end brackets found";
        return NULL;
      }
      endBracketP = cP;
    }

    ++cP;
  }


  //
  // Error handling
  //
  if ((startBracketP != NULL) && (endBracketP == NULL))
  {
    *detailsP = (char*) "missing end bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }
  else if ((startBracketP == NULL) && (endBracketP != NULL))
  {
    *detailsP = (char*) "end bracket but no start bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }
  else if ((firstDotP != NULL) && (startBracketP != NULL) && (firstDotP < startBracketP))
  {
    *detailsP = (char*) "found a dot before a start bracket";
    LM_W(("Bad Input (%s)", *detailsP));
    return NULL;
  }

  //
  // Now we need to NULL out certain characters:
  //
  // Again, four cases:
  // 1. A
  // 2. A[B]
  // 3. A.B   (special case if B is timestamp and if forDb==false)
  // 4. A.B.C
  //
  // - If A:  ((startBracketP == NULL) && (firstDotP == NULL))
  //   - attribute: A
  //   => nothing to NULL our
  //   - fullPath:  A-EXPANDED.value
  //
  // - if A[B]: (startBracketP != NULL)
  //   - attribute: A
  //   - rest:      B
  //   => Must NULL out '[' and ']'
  //   - fullPath:  A-EXPANDED.value.B
  //
  // - if A.B ((startBracketP == NULL) && (firstDotP != NULL) && (secondDotP == NULL))
  //   - attribute:  A
  //   - metadata:   B
  //   => Must NULL out the first dot
  //   - fullPath:  A-EXPANDED.md.B.value
  //
  // - if A.B.C ((startBracketP == NULL) && (firstDotP != NULL) && (secondDotP != NULL))
  //   - attribute:  A
  //   - metadata:   B
  //   - rest:       C
  //   => Must NULL out the first two dots
  //   - fullPath:  A-EXPANDED.md.B.value.C
  //
  int caseNo = 0;

  if ((startBracketP == NULL) && (firstDotP == NULL))
    caseNo = 1;
  else if (startBracketP != NULL)
  {
    *startBracketP = 0;
    *endBracketP   = 0;
    rest           = &startBracketP[1];
    caseNo         = 2;
  }
  else if (firstDotP != NULL)
  {
    if (secondDotP == NULL)
    {
      *firstDotP = 0;
      mdNameP    = &firstDotP[1];
      caseNo     = 3;
    }
    else
    {
      *firstDotP  = 0;
      mdNameP     = &firstDotP[1];
      *secondDotP = 0;
      rest        = &secondDotP[1];
      caseNo = 4;
    }
  }

  if (caseNo == 0)
  {
    *detailsP = (char*) "invalid RHS in Q-filter";
    return NULL;
  }

  if ((caseNo == 3) || (caseNo == 4))
    *isMdP = true;

  //
  // All OK - let's compose ...
  //
  char* longNameP = orionldAttributeExpand(orionldState.contextP, attrNameP, true, NULL);

  //
  // Now 'longNameP' needs to be adjusted for the DB model - that changes '.' for '=' in the database.
  // If we use 'longNameP', that points to the context-cache, we will destroy the cache. We have to work on a copy
  //
  char longName[512];    // 512 seems like an OK limit for max length of an expanded attribute name
  char mdLongName[512];  // 512 seems like an OK limit for max length of an expanded metadata name

  strncpy(longName, longNameP, sizeof(longName) - 1);

  // Turn '.' into '=' for longName
  char* sP = longName;
  while (*sP != 0)
  {
    if (*sP == '.')
      *sP = '=';
    ++sP;
  }

  //
  // Expand mdName if present
  //
  if (mdNameP != NULL)
  {
    if (strcmp(mdNameP, "observedAt") != 0)  // Don't expand "observedAt", nor ...
    {
      char* mdLongNameP  = orionldSubAttributeExpand(orionldState.contextP, mdNameP, true, NULL);

      strncpy(mdLongName, mdLongNameP, sizeof(mdLongName) - 1);  // NOT overwriting longnames from the context !!!

      // Turn '.' into '=' for md-longname
      char* sP = mdLongName;
      while (*sP != 0)
      {
        if (*sP == '.')
          *sP = '=';
        ++sP;
      }

      mdNameP = mdLongName;
    }
  }

  if (forDb)
  {
    if (caseNo == 1)
    {
      LM_T(LmtQ, ("Case 1: attr: '%s'", longName));

      if (strcmp(longName, "createdAt") == 0)
        snprintf(fullPath, sizeof(fullPath) - 1, "%s", "creDate");
      else if (strcmp(longName, "modifiedAt") == 0)
        snprintf(fullPath, sizeof(fullPath) - 1, "%s", "modDate");
      else
        snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.value", longName);
    }
    else if (caseNo == 2)
    {
      LM_T(LmtQ, ("Case 2: attr: '%s', sub-attr: '%s', rest: '%s'", longName, mdNameP, rest));
      snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.value.%s", longName, rest);
    }
    else if (caseNo == 3)
    {
      LM_T(LmtQ, ("Case 3: attr: '%s', sub-attr: '%s'", longName, mdNameP));

      if (strcmp(mdNameP, "createdAt") == 0)
        snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.%s", longName, "creDate");
      else if (strcmp(mdNameP, "modifiedAt") == 0)
        snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.%s", longName, "modDate");
      else
        snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.md.%s.value", longName, mdNameP);
    }
    else
    {
      LM_T(LmtQ, ("Case 3+: attr: '%s', sub-attr: '%s', rest: '%s'", longName, mdNameP, rest));
      snprintf(fullPath, sizeof(fullPath) - 1, "attrs.%s.md.%s.value.%s", longName, mdNameP, rest);
    }
  }
  else
  {
    LM_T(LmtQ, ("Not for DB, case %d: attr: '%s', sub-attr: '%s', rest: '%s'", caseNo, longName, mdNameP, rest));

    // If observedAt, createdAt, modifiedAt, unitCode, ...   No ".value" must be appended
    if (caseNo == 3)
    {
      if ((strcmp(mdNameP, "observedAt") == 0) || (strcmp(mdNameP, "modifiedAt") == 0) || (strcmp(mdNameP, "createdAt") == 0))
      {
        LM_T(LmtQ, ("Case 5 - it's a timestamp (%s)", mdNameP));
        caseNo = 5;
      }
    }

    if (caseNo == 1)
      snprintf(fullPath, sizeof(fullPath) - 1, "%s.value", longName);
    else if (caseNo == 2)
      snprintf(fullPath, sizeof(fullPath) - 1, "%s.value.%s", longName, rest);
    else if (caseNo == 3)
      snprintf(fullPath, sizeof(fullPath) - 1, "%s.%s.value", longName, mdNameP);
    else if (caseNo == 4)
      snprintf(fullPath, sizeof(fullPath) - 1, "%s.%s.value.%s", longName, mdNameP, rest);
    else if (caseNo == 5)
      snprintf(fullPath, sizeof(fullPath) - 1, "%s.%s", longName, mdNameP);
  }

  return kaStrdup(&orionldState.kalloc, fullPath);
}
