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
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/kjTree/kjAttributeNormalizedToSimplified.h"    // Own interface



// ----------------------------------------------------------------------------
//
// kjAttributeNormalizedToSimplified -
//
// FIXME - steal stuff from kjEntityNormalizedToSimplified
//
void kjAttributeNormalizedToSimplified(KjNode* attrP, const char* lang)
{
  KjNode* valueP       = kjLookup(attrP, "value");
  KjNode* objectP      = kjLookup(attrP, "object");
  KjNode* languageMapP = kjLookup(attrP, "languageMap");

  if (valueP == NULL) valueP = objectP;
  if (valueP == NULL) valueP = languageMapP;

  LM_TMP(("In kjAttributeNormalizedToSimplified"));

  if (valueP != NULL)
  {
    attrP->type  = valueP->type;
    attrP->value = valueP->value;
  }
  else
    LM_TMP(("Nothing done - no value field found"));
}
