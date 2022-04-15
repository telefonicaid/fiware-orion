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
#include <string.h>                                              // strcpy

extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove, kjChildAdd
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/kjTree/kjNavigate.h"                           // kjNavigate
#include "orionld/common/orionldPatchApply.h"                    // Own interface



// -----------------------------------------------------------------------------
//
// dbModelPathComponentsSplit - move to new library orionld/dbModel/dbModelPathComponentsSplit.h/cpp
//
static int dbModelPathComponentsSplit(char* path, char** compV)
{
  int compIx = 1;

  compV[0] = path;

  while (*path != 0)
  {
    if (*path == '.')
    {
      *path = 0;
      ++path;
      compV[compIx] = path;

      //
      // We only split the first 6 components
      // Max PATH is "attrs.P1.md.Sub-P1.value[.X]*
      //
      if (compIx == 5)
        return 6;

      //
      // We break if we find "attrs.X.value", but not until we have 4 components; "attrs.P1.value.[.X]*"
      // It is perfectly possible 'path' is only "attrs.P1.value", and if so, we'd have left the function already
      //
      if ((compIx == 3) && (strcmp(compV[2], "value") == 0))
        return 4;

      ++compIx;
    }

    ++path;
  }

  return compIx;
}



// -----------------------------------------------------------------------------
//
// pathComponentsFix -
//
// Some pieces of information for the DB are either redundant info for the database model of Orion or
// builtin timestamps, that are not of interest for the TRoE database
//
// Also, all fields that are entity-characteristics, not attribute, are skipped (for now)
// Once multi-attribute and/or Scope is supported, this will have to change
//
static int pathComponentsFix(char** compV, int components, bool* skipP)
{
  int oldIx = 0;
  int newIx = 0;

  for (int ix = 0; ix < components; ix++)
  {
    if (oldIx == 0)
    {
      if (strcmp(compV[oldIx], "attrs") == 0)
      {
        ++oldIx;  // Jump over "attrs"
        continue;
      }
      else
      {
        *skipP = true;
        return -1;
      }
    }

    if ((oldIx == 2) && strcmp(compV[oldIx], "md") == 0)
    {
      ++oldIx;  // Jump over "md"
      continue;
    }
#if 1
    if ((oldIx == 4) && strcmp(compV[oldIx], "value") == 0)  // This will change once I eat the "value" object in dbModel function
    {
      if (strcmp(compV[3], "observedAt") == 0)
      {
        compV[newIx] = NULL;  // Never mind "value"
        return newIx;
      }

      if (strcmp(compV[3], "unitCode") == 0)
      {
        compV[newIx] = NULL;  // Never mind "value"
        return newIx;
      }
    }
#endif
    compV[newIx] = compV[oldIx];
    ++newIx;
    ++oldIx;
  }

  if (strcmp(compV[newIx - 1], "modDate") == 0) *skipP = true;
  if (strcmp(compV[newIx - 1], "creDate") == 0) *skipP = true;
  if (strcmp(compV[newIx - 1], "mdNames") == 0) *skipP = true;

  compV[newIx] = NULL;

  return newIx;
}



// -----------------------------------------------------------------------------
//
// pathArrayJoin -
//
static char* pathArrayJoin(char* out, char** compV, int components)
{
  strcpy(out, compV[0]);

  for (int ix = 1; ix < components; ix++)
  {
    strcat(out, ".");
    strcat(out, compV[ix]);
  }

  return out;
}



// -----------------------------------------------------------------------------
//
// orionldPatchApply
//
void orionldPatchApply(KjNode* patchBase, KjNode* patchP)
{
  KjNode* pathNode = kjLookup(patchP, "PATH");
  KjNode* treeNode = kjLookup(patchP, "TREE");

  if (pathNode == NULL)           return;
  if (treeNode == NULL)           return;
  if (pathNode->type != KjString) return;

  char* compV[7];
  bool  skip       = false;
  char* path       = pathNode->value.s;
  int   components = dbModelPathComponentsSplit(path, compV);
  char  buf[512];

  components = pathComponentsFix(compV, components, &skip);
  if (skip)
    return;

  pathNode->value.s = kaStrdup(&orionldState.kalloc, pathArrayJoin(buf, compV, components));  // FIXME: buf is 512 bytes long ...

  KjNode*  parentP         = NULL;
  bool     onlyLastMissing = false;
  KjNode*  nodeP           = kjNavigate(patchBase, compV, &parentP, &onlyLastMissing);

  // Remove non-wanted parts of objects
  if (treeNode->type == KjObject)
  {
    const char* unwanted[] = { "modDate", "creDate", "mdNames" };

    for (unsigned int ix = 0; ix < K_VEC_SIZE(unwanted); ix++)
    {
      KjNode* nodeP = kjLookup(treeNode, unwanted[ix]);
      if (nodeP != NULL)
        kjChildRemove(treeNode, nodeP);
    }
  }

  if (parentP == NULL)  // New attribute?  Add to patchBase (entity)
  {
    treeNode->name = pathNode->value.s;
    kjChildAdd(patchBase, treeNode);
  }
  if (nodeP == NULL)  // Did not exist in the "patch base" - add to parentP
  {
    if (onlyLastMissing == true)
    {
      kjChildRemove(patchP, treeNode);
      treeNode->name = compV[components - 1];
      kjChildAdd(parentP, treeNode);
    }
    else
    {
      // Must be a new attribute ...
      treeNode->name = pathNode->value.s;
      kjChildAdd(patchBase, treeNode);
    }

    return;
  }

  if (treeNode->type == KjNull)
  {
    kjChildRemove(parentP, nodeP);
    return;
  }

  treeNode->name = nodeP->name;
  kjChildRemove(parentP, nodeP);
  kjChildRemove(patchP, treeNode);
  kjChildAdd(parentP, treeNode);
}
