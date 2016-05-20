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
    else if ((vec[ix]->valueType == orion::ValueTypeObject) || (vec[ix]->valueType == orion::ValueTypeVector))
    {
      if (vec[ix]->compoundValueP)
      {
        if (vec[ix]->compoundValueP->valueType == orion::ValueTypeVector)
        {
          *valueP = "[" + vec[ix]->compoundValueP->toJson(true) + "]";
        }
        else if (vec[ix]->compoundValueP->valueType == orion::ValueTypeObject)
        {
          *valueP = "{" + vec[ix]->compoundValueP->toJson(true) + "}";
        }
        else
        {
          LM_E(("Runtime Error (attribute is of object type but its compound is of invalid type)"));
          *valueP = "";
        }
      }
      else
      {
        LM_E(("Runtime Error (attribute is of object type but has no compound)"));
        *valueP = "";
      }
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
*
* An initial out-buffer of CHUNK_SIZE is allocated, and later on during the substitution if the
* out-buffer is not big enough, it is realloced with another CHUNK_SIZE.
*
* 1024 is set as CHUNK_SIZE and hopefully for most substitutions it will be enough with ONE allocation.
* Allocation is very costly operations ...
* This function could be a lot faster is we took the first 1024 bytes from the stack instead of using calloc.
* However, the function gets a little more complicated like that as the first realloc would have to be a normal malloc and a memcpy.
*
* Variables used:
*   - toP:     pointer to the beginning of the 'to string' where to add the new characters from the 'from string'
*   - toIx:    the index of the 'to string', always at the end of the string, where to append characters.
*              toIx also serves as the running size of the 'to string'
*   - toLen:   Total size of the 'to string'
*   - fromP:   pointer to the currently analysed character in the 'from string'
*
*/
#define CHUNK_SIZE 1024
void macroSubstitute(std::string* to, const std::string& from, const ContextElement& ce)
{
  char*        toP     = (char*) calloc(1, CHUNK_SIZE);
  int          toIx    = 0;
  int          toLen   = CHUNK_SIZE;

  if (toP == NULL)
  {
    LM_E(("Runtime Error (out of memory)"));
    *to = "";
    return;
  }

  // We need to do a copy, or the fromP processing logic will destroy incoming function argument
  char*       fromToFreeP = strdup(from.c_str());
  const char* fromP       = fromToFreeP;

  if (fromP == NULL)
  {
    LM_E(("Runtime Error (out of memory)"));
    *to = "";
    return;
  }

  while (*fromP != 0)
  {
    //
    // If the current position in the 'from string' (fromP), points to the two characters
    // "${", then (if an ending '}' is found) a variable substitution is to be performed.
    //
    // Otherwise, just copy the character in 'from string' to 'to string'.
    //
    if ((fromP[0] == '$') && (fromP[1] == '{'))
    {
      //
      // varP points to the character after "${", i.e. the first character of the variable name.
      // E.g. if 'from string' is "${abc}", varP would point to the 'a'
      //
      char* varP = (char*) &fromP[2];

      // Set fromP to point to the chatactrer after "${" - the first character of the variable name
      fromP += 2;

      // Find the closing '}'
      while ((fromP[0] != 0) && (*fromP != '}'))
      {
        ++fromP;
      }

      // If the 'from string' ends without finding a closing '}', ERROR
      if (fromP[0] == 0)
      {
        break;  // ERROR: non-terminated variable
      }

      // Terminate the variable name, by replacing the '}' with a ZERO
      *((char*) fromP) = 0;

      // Then make fromP point to the character AFTER the closing '}'
      ++fromP;

      // Variable found, get its value
      char*        substitute    = NULL;
      int          substituteLen = 0;
      std::string  value;

      if (strcmp(varP, "id") == 0)
      {
        //
        // ${id} => substitute with ID of the entity
        //
        substitute = (char*) ce.entityId.id.c_str();
      }
      else if (strcmp(varP, "type") == 0)
      {
        //
        // ${type} => substitute with TYPE of the entity
        //
        substitute = (char*) ce.entityId.type.c_str();
      }
      else  // attribute
      {
        //
        // Neither ${id} nor ${type} => must be a name of an attribute ( ${ATTR_NAME} )
        // The value of the attribute ATTR_NAME is extracted by the function attributeValue
        // The variable 'substitute' is set to point to it
        //
        attributeValue(&value, ce.contextAttributeVector.vec, varP);
        substitute = (char*) value.c_str();
      }

      //
      // If substitution found, get its string length
      // If not found, substituteLen is ZERO already
      //
      if (substitute != NULL)
      {
        substituteLen = strlen(substitute);
      }

      //
      // Room enough?  If not, realloc
      //   toIx            is the current length of the 'to string'.
      //   substituteLen   is the length of the value to put in instead of ${XXX}
      //   toLen           is the TOTAL length of the 'to string'
      //
      // So, if toIx + substituteLen is bigger than THE TOTAL LENGTH of the 'to string'
      // we will have to get more memory before we insert the 'substitute' in the 'to string'
      // [ The '- 1' is for the zero-termination of the 'in string'.
      //
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

      //
      // Now, if a substitute was found, append it to 'to string' and add the length of the
      // substitute to 'toIx', so that toP[toIx] point to the last non-used char of the 'to string'
      //
      // If no substitution was found (attribute with that name does not exist), then the ${XXX}
      // is substituted with NOTHING - it is silently consumed.
      //
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
      //
      // Room enough?  If not, realloc
      // See detaild description of the realloc step above
      //
      if (toIx >= toLen - 1)
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

      //
      // Simply copy the char from 'from string' into 'to string'
      // and step over that char to prepare for the next one.
      //
      toP[toIx++] = fromP[0];
      ++fromP;
    }
  }

  //
  // Set the incoming std::string to the value of 'to string' and free the 'to string'
  //
  *to = toP;
  free(toP);
  free(fromToFreeP);
}
