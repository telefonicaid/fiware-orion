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
#include <string.h>                                              // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/urlParse.h"                             // urlParse
#include "orionld/kjTree/kjTreeRegistrationInfoExtract.h"        // Owm interface



// -----------------------------------------------------------------------------
//
// kjTreeRegistrationInfoExtract -
//
bool kjTreeRegistrationInfoExtract
(
  KjNode*    registrationP,
  char*      protocol,
  int        protocolSize,
  char*      host,
  int        hostSize,
  uint16_t*  portP,
  char**     uriDirP,
  char*      registrationAttrV[],
  int        registrationAttrSize,
  int*       registrationAttrsP,
  char**     detailP
)
{
  *registrationAttrsP = 0;
  for (KjNode* nodeP = registrationP->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
  {
    if (strcmp(nodeP->name, "format") == 0)
    {
      // format = nodeP->value.s;
    }
    else if (strcmp(nodeP->name, "@context") == 0)
    {
      // contextP = nodeP;
    }
    else if (strcmp(nodeP->name, "contextRegistration") == 0)
    {
      for (KjNode* crNodeP = nodeP->value.firstChildP; crNodeP != NULL; crNodeP = crNodeP->next)
      {
        int crItemIx = 0;

        for (KjNode* crItemNodeP = nodeP->value.firstChildP->value.firstChildP; crItemNodeP != NULL; crItemNodeP = crItemNodeP->next)
        {
          if (strcmp(crItemNodeP->name, "attrs") == 0)
          {
            //
            // Populate the array registrationAttrV with the registered attributes
            //
            for (KjNode* attrItemP = crItemNodeP->value.firstChildP; attrItemP != NULL; attrItemP = attrItemP->next)
            {
              KjNode* nameP = kjLookup(attrItemP, "name");

              if (nameP == NULL)
              {
                LM_W(("FWD: 'name' field not found in attrs array item"));
                continue;
              }

              registrationAttrV[*registrationAttrsP] = nameP->value.s;
              *registrationAttrsP += 1;
            }
          }
          else if ((host[0] == 0) && (strcmp(crItemNodeP->name, "providingApplication") == 0))
          {
            if (urlParse(crItemNodeP->value.s, protocol, protocolSize, host, hostSize, portP, uriDirP, detailP) == false)
            {
              // Mark Error so that "Incomplete Response" is present in response?
              return false;
            }
          }

          ++crItemIx;
        }
      }
    }
  }

  return true;
}
