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

#include "logMsg/logMsg.h"
#include "ngsi/ContextElement.h"

#include "common/string.h"
#include "common/limits.h"
#include "common/macroSubstitute.h"


/* ****************************************************************************
*
* attributeValue - return value of attribute as a string
*/
static void attributeValue(std::string* valueP, const std::vector<ContextAttribute*>& vec, const char* attrName)
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
      *valueP = toString(vec[ix]->numberValue);
    }
    else if (vec[ix]->valueType == orion::ValueTypeBoolean)
    {
      *valueP = (vec[ix]->boolValue == true)? "true" : "false";
    }
    else if (vec[ix]->valueType == orion::ValueTypeNull)
    {
      *valueP = "null";
    }
    else if (vec[ix]->valueType == orion::ValueTypeNotGiven)
    {
      LM_E(("Runtime Error (value not given for attribute)"));
      *valueP = "";
    }
    else if ((vec[ix]->valueType == orion::ValueTypeObject) || (vec[ix]->valueType == orion::ValueTypeVector))
    {
      if (vec[ix]->compoundValueP)
      {
        if (vec[ix]->compoundValueP->valueType == orion::ValueTypeVector)
        {
          *valueP = "[" + vec[ix]->compoundValueP->toJson(true, true) + "]";
        }
        else if (vec[ix]->compoundValueP->valueType == orion::ValueTypeObject)
        {
          *valueP = "{" + vec[ix]->compoundValueP->toJson(true, true) + "}";
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
* An old version of this function was based in char processing. However, we faced
* weird crashing problems after fixing that implementation to support >1KB payloads.
*
* We didn't know the actual cause of these problems but after changing the implementation
* to the current one based on std::string, it seems stable. The most probable causes
* of the problem were:
*
* 1) The old macroSubstitute() function had some bug managing memory which we weren't able
*    to find.
* 2) The old macroSubstitute() function was ok, but the way it managed the memory was in a way
*    it makes more probable some memory corruption bug in other place, compared with the current
*    implementation. If this theory is correct, we haven't been able to "raise" the memory corruption
*    bug with the current implementation.
*
* However, the old version is still available at git repository, for the records.
* It can be found checking out the following commit (the last one before chaning implementation):
*
*   commit f8c91bf16e192388824c3786a76b203b83354d13
*   Date:   Mon Jun 19 16:33:29 2017 +0200
*
*/
bool macroSubstitute(std::string* to, const std::string& from, const ContextElement& ce)
{
  // Initial size check: is the string to convert too big?
  //
  // If the string to convert is bigger than the maximum allowed buffer size (MAX_DYN_MSG_SIZE),
  // then there is an important probability that the resulting string after substitution is also > MAX_DYN_MSG_SIZE.
  //
  // This check avoids to copy 8MB to later only throw it away and return an error
  // That is the advantage.
  //
  // There is an inconvenience as well.
  //
  // The inconvenience is for buffers that are larger before substitution than they are after substitution.
  // Those buffers aren't let through this check, and end up in an error.
  //
  // We assume that this second case is more than rare
  //
  if (from.size() > MAX_DYN_MSG_SIZE)
  {
    LM_W(("Runtime Error (too large initial string, before substitution)"));
    *to = "";
    return false;
  }

  // Look for macros (using a hash map to count how many times each macro appears)
  std::map<std::string, unsigned int>  macroNames;  // holds all the positions that each sub occur inside 'from'
  size_t                               macroStart = from.find("${", 0);

  while (macroStart != std::string::npos)
  {
    size_t macroEnd = from.find("}", macroStart);

    if (macroEnd == std::string::npos)
    {
      LM_W(("Runtime Error (macro end not found, syntax error, aborting substitution)"));
      *to = "";
      return false;
    }

    std::string macroName = from.substr(macroStart + 2, macroEnd - (macroStart + 2));
    if (macroNames.find(macroName) != macroNames.end())
    {
      macroNames[macroName]++;
    }
    else
    {
      macroNames[macroName] = 1;
    }

    macroStart = from.find("${", macroEnd + 1);
  }

  // Calculate resulting size (if > MAX_DYN_MSG_SIZE, then reject)
  unsigned long  toReduce = 0;
  unsigned long  toAdd    = 0;

  for (std::map<std::string, unsigned int>::iterator it = macroNames.begin(); it != macroNames.end(); ++it)
  {
    std::string   macroName = it->first;
    unsigned int  times     = it->second;

    // The +3 is due to "${" and "}"
    toReduce += (macroName.length() + 3) * times;

    if (macroName == "id")
    {
      toAdd += ce.entityId.id.length() * times;
    }
    else if (macroName == "type")
    {
      toAdd += ce.entityId.type.length() * times;
    }
    else
    {
      std::string value;

      attributeValue(&value, ce.contextAttributeVector.vec, macroName.c_str());
      toAdd += value.length() * times;
    }
  }

  if (from.length() + toAdd - toReduce > MAX_DYN_MSG_SIZE)
  {
    LM_W(("Runtime Error (too large final string, after substitution)"));
    *to = "";
    return false;
  }

  // Macro replace
  *to = from;
  for (std::map<std::string, unsigned int>::iterator it = macroNames.begin(); it != macroNames.end(); ++it)
  {
    std::string macroName = it->first;
    unsigned int times    = it->second;

    std::string macro = "${" + it->first + "}";
    std::string value;

    if (macroName == "id")
    {
      value = ce.entityId.id;
    }
    else if (macroName == "type")
    {
      value = ce.entityId.type;
    }
    else
    {
      attributeValue(&value, ce.contextAttributeVector.vec, macroName.c_str());
    }

    // We have to do the replace operation as many times as macro occurrences
    for (unsigned int ix = 0; ix < times; ix++)
    {
      to->replace(to->find(macro), macro.length(), value);
    }
  }

  return true;
}
