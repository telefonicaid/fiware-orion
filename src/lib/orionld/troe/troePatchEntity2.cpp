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
extern "C"
{
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjChildRemove
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldPatchApply.h"                    // orionldPatchApply
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/troe/troePatchEntity.h"                        // troePatchEntity - to reuse the "push to TRoE" of troePatchEntity
#include "orionld/troe/troePatchEntity2.h"                       // Own interface



// ----------------------------------------------------------------------------
//
// troePatchEntity2 -
//
bool troePatchEntity2(void)
{
  KjNode* patchTree = orionldState.requestTree;
  KjNode* patchBase = orionldState.patchBase;

  // kjTreeLog(orionldState.patchBase, "KZ: patchBase");
  // kjTreeLog(patchTree, "KZ: patchTree");

  for (KjNode* patchP = patchTree->value.firstChildP; patchP != NULL; patchP = patchP->next)
  {
    orionldPatchApply(patchBase, patchP);
  }

  // kjTreeLog(patchBase, "KZ: patched result");

  //
  // No need to reimplement what troePatchEntity already implement - push to the TRoE DB
  // We just need to "pretend" that the patched result (orionldState.patchBase) was what came in
  //
  orionldState.requestTree = patchBase;
  orionldState.patchTree   = patchTree;

  troePatchEntity();

  return true;
}
