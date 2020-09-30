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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/qAliasCompact.h"                        // Own interface



// -----------------------------------------------------------------------------
//
// qAliasCompact - replace long attr names to their corresponding aliases
//
// OR, if compact == false, replace the alias with their long names
//
// So, need to find all th variable names and replace them.
//
// Possibilities:
//   var
//   !var
//   var!=XXX
//   var~=XXX
//   var==XXX
//   var<=XXX
//   var>=XXX
//   var>XXX
//   var<XXX
//
bool qAliasCompact(KjNode* qP, bool compact)
{
  char*  cP            = qP->value.s;
  char*  varStart      = cP;
  bool   insideVarName = true;
  char*  out           = kaAlloc(&orionldState.kalloc, 2048);  // 2048 ... should be enough
  int    outIx         = 0;

  bzero(out, sizeof(out));

  while (*cP != 0)
  {
    char c0 = cP[0];
    char c1 = cP[1];

    if (c0 == '!')
    {
      varStart = &cP[1];
      out[outIx] = '!';
      ++outIx;
    }
    else if (((c1 == '=') &&
              ((c0 == '=') ||
               (c0 == '!') ||
               (c0 == '~') ||
               (c0 == '<') ||
               (c0 == '>')))     ||
             (c0 == '<') ||
             (c0 == '>'))
    {
      char  savedCp0 = cP[0];
      char* alias;
      char* eqP;

      cP[0] = 0;

      if (compact == true)
      {
        // Changing all equal-signs to dots
        eqP = varStart;
        while (*eqP != 0)
        {
          if (*eqP == '=')
            *eqP = '.';
          ++eqP;
        }

        alias = orionldContextItemAliasLookup(orionldState.contextP, varStart, NULL, NULL);
      }
      else
      {
        // Expand the variable
        alias = orionldContextItemExpand(orionldState.contextP, varStart, true, NULL);
      }

      if (alias != NULL)
      {
        strcpy(&out[outIx], alias);
        outIx += strlen(alias);
      }
      else
      {
        strcpy(&out[outIx], varStart);
        outIx += strlen(varStart);
      }

      out[outIx++] = savedCp0;
      if (c1 == '=')
      {
        out[outIx++] = c1;
        ++cP;
      }

      insideVarName = false;
    }
    else if (*cP == ';')
    {
      insideVarName = true;
      out[outIx++] = ';';
      varStart = &cP[1];
    }
    else if (insideVarName == false)
    {
      out[outIx++] = *cP;
    }

    ++cP;
  }

  out[outIx] = 0;

  //
  // Time to set the new value of 'qP'
  //
  qP->value.s = out;

  return true;
}
