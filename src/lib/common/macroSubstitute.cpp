/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string>

#include "ngsi/ContextElement.h"
#include "common/macroSubstitute.h"



/* ****************************************************************************
*
* attributeValue - return value of attribute as a string
*/
static void attributeValue(std::string* valueP, const std::vector<ContextAttribute*>& vec, char* attrName)
{
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    if (vec[ix]->name != attrName)
    {
      continue;
    }

    if (vec[ix]->valueType == orion::ValueTypeString)
    {
      *valueP = vec[ix]->stringValue;
    }
    else if (vec[ix]->valueType == orion::ValueTypeNumber)
    {
      char i[STRING_SIZE_FOR_INT];

      snprintf(i, sizeof(i), "%f", vec[ix]->numberValue);
      *valueP = i;
    }
    else if (vec[ix]->valueType == orion::ValueTypeBoolean)
    {
      *valueP = (vec[ix]->boolValue == true)? "true" : "false";
    }
    else if (vec[ix]->valueType == orion::ValueTypeNone)
    {
      *valueP = "null";
    }
    else if (vec[ix]->valueType == orion::ValueTypeVector)
    {
      *valueP = vec[ix]->compoundValueP->toJson(true);
    }
    else if (vec[ix]->valueType == orion::ValueTypeObject)
    {
      *valueP = vec[ix]->compoundValueP->toJson(true);
    }
    else
    {
      LM_E(("Runtime Error (unknown value type for attribute)"));
      *valueP = "";
    }

    return;
  }

  *valueP = "";
}



/* ****************************************************************************
*
* macroSubstitute - 
*/
#define CHUNK_SIZE 1024
void macroSubstitute(std::string* to, const std::string& from, const ContextElement& ce)
{
  char*        toP     = (char*) calloc(1, CHUNK_SIZE);
  int          toIx    = 0;
  int          toLen   = CHUNK_SIZE;
  const char*  fromP   = (char*) from.c_str();

  if (toP == NULL)
  {
    LM_E(("Runtime Error (out of memory)"));
    *to = "";
    return;
  }

  while (*fromP != 0)
  {
    if ((*fromP == '$') && (fromP[1] == '{'))
    {
      char* varP = (char*) &fromP[2];

      fromP += 2;
      while ((*fromP != 0) && (*fromP != '}'))
      {
        ++fromP;
      }

      if (*fromP == 0)
      {
        break;  // ERROR: non-terminated variable
      }

      *((char*) fromP) = 0;
      ++fromP;

      // Variable found, get its value
      LM_W(("KZ: Got a var, so substitute: '%s'", varP));
      char* substitute    = NULL;
      int   substituteLen = 0;

      if (strcmp(varP, "id") == 0)
      {
        substitute = (char*) ce.entityId.id.c_str();
      }
      else if (strcmp(varP, "type") == 0)
      {
        substitute = (char*) ce.entityId.type.c_str();
      }
      else  // attribute
      {
        std::string value;

        attributeValue(&value, ce.contextAttributeVector.vec, varP);
        substitute = (char*) value.c_str();
      }

      if (substitute != NULL)
      {
        substituteLen = strlen(substitute);
      }

      // Room enough?  If not, realloc
      if (toIx + substituteLen >= toLen - 1)
      {
        toP    = (char*) realloc(toP, toLen + CHUNK_SIZE);
        toLen += CHUNK_SIZE;

        if (toP == NULL)
        {
          LM_E(("Runtime Error (out of memory)"));
          *to = "";
          return;
        }
      }

      if (substitute != NULL)
      {
        // Concatenate 'substitute' to toP
        if (substitute != NULL)
        {
          strcat(&toP[toIx], substitute);
          toIx += substituteLen;
        }
      }
    }
    else
    {
      toP[toIx++] = *fromP;
      ++fromP;
    }
  }

  *to = toP;
  free(toP);
}
