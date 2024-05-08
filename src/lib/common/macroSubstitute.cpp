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

#include "common/string.h"
#include "common/limits.h"
#include "common/globals.h"
#include "common/JsonHelper.h"
#include "common/macroSubstitute.h"

#include "expressions/exprMgr.h"


/* ****************************************************************************
*
* smartStringValue -
*
* Returns the effective string value, taking into account replacements
*
*/
std::string smartStringValue(const std::string stringValue, ExprContextObject* exprContextObjectP, const std::string notFoundDefault)
{
  // This code is pretty similar to the one in CompoundValueNode::toJson()
  // The program logic branching is the same, but the result at the end of each if-else
  // is different, which makes difficult to unify both them
  if ((exprContextObjectP != NULL) && (stringValue.rfind("${") == 0) && (stringValue.rfind("}", stringValue.size()) == stringValue.size() - 1))
  {
    // "Full replacement" case. In this case, the result is not always a string
    // len("${") + len("}") = 3
    std::string macroName = stringValue.substr(2, stringValue.size() - 3);

    ExprResult r = exprMgr.evaluate(exprContextObjectP, macroName);
    std::string result;
    if (r.valueType == orion::ValueTypeNull)
    {
      result = notFoundDefault;
    }
    else
    {
      // in basic mode an extra remove quotes step is needed, to avoid "1" instead of 1 for number, etc.
      if (exprContextObjectP->isBasic())
      {
        result = removeQuotes(r.toString());
      }
      else
      {
        result = r.toString();
      }
    }
    r.release();
    return result;
  }
  else if (exprContextObjectP != NULL)
  {
    // "Partial replacement" case. In this case, the result is always a string
    std::string effectiveValue;
    if (!macroSubstitute(&effectiveValue, stringValue, exprContextObjectP, "null", true))
    {
      // error already logged in macroSubstitute, using stringValue itself as failsafe
      effectiveValue = stringValue;
    }

    // toJsonString will stringfly JSON values in macros
    return '"' + toJsonString(effectiveValue) + '"';
  }
  else
  {
    // No replacement applied, just return the original string
    return '"' + toJsonString(stringValue) + '"';
  }
}



/* ****************************************************************************
*
* stringValueOrNothing -
*
*/
static std::string stringValueOrNothing(ExprContextObject* exprContextObjectP, const std::string key, const std::string& notFoundDefault, bool raw)
{
  ExprResult r = exprMgr.evaluate(exprContextObjectP, key);
  std::string result;

  if (r.valueType == orion::ValueTypeNull)
  {
    result = notFoundDefault;
  }
  else
  {
    std::string s = r.toString();

    // in basic mode an extra remove quotes step is needed, to avoid "1" instead of 1 for number, etc.
    if (exprContextObjectP->isBasic())
    {
      s = removeQuotes(s);
    }

    if (raw)
    {
      // This means that the expression is in the middle of the string (i.e. partial replacement and not full replacement),
      // so double quotes have to be be removed
      result = removeQuotes(s);
    }
    else
    {
      result = s;
    }
  }

  r.release();
  return result;
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
* It can be found checking out the following commit (the last one before changing implementation):
*
*   commit f8c91bf16e192388824c3786a76b203b83354d13
*   Date:   Mon Jun 19 16:33:29 2017 +0200
*
*/
bool macroSubstitute(std::string* to, const std::string& from, ExprContextObject* exprContextObjectP, const std::string& notFoundDefault, bool raw)
{
  // Initial size check: is the string to convert too big?
  //
  // If the string to convert is bigger than the maximum allowed buffer size (outReqMsgMaxSize),
  // then there is an important probability that the resulting string after substitution is also > outReqMsgMaxSize.
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
  if (from.size() > outReqMsgMaxSize)
  {
    LM_E(("Runtime Error (too large initial string, before substitution)"));
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
      LM_W(("macro end not found, syntax error, aborting substitution"));
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

  // Calculate resulting size (if > outReqMsgMaxSize, then reject)
  unsigned long  toReduce = 0;
  unsigned long  toAdd    = 0;

  for (std::map<std::string, unsigned int>::iterator it = macroNames.begin(); it != macroNames.end(); ++it)
  {
    std::string   macroName = it->first;
    unsigned int  times     = it->second;

    // The +3 is due to "${" and "}"
    toReduce += (macroName.length() + 3) * times;
    toAdd += stringValueOrNothing(exprContextObjectP, macroName, notFoundDefault, raw).length() * times;
  }

  if (from.length() + toAdd - toReduce > outReqMsgMaxSize)
  {
    LM_E(("Runtime Error (too large final string, after substitution)"));
    *to = "";
    return false;
  }

  // Macro replace
  *to = from;
  for (std::map<std::string, unsigned int>::iterator it = macroNames.begin(); it != macroNames.end(); ++it)
  {
    std::string macroName = it->first;
    unsigned int times    = it->second;

    std::string macro = "${" + macroName + "}";
    std::string value = stringValueOrNothing(exprContextObjectP, macroName, notFoundDefault, raw);

    // We have to do the replace operation as many times as macro occurrences
    for (unsigned int ix = 0; ix < times; ix++)
    {
      to->replace(to->find(macro), macro.length(), value);
    }
  }

  return true;
}
