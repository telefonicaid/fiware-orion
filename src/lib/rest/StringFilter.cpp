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
#include <vector>

extern "C"
{
#include "kbase/kFloatTrim.h"                       // kFloatTrim
#include "kalloc/kaStrdup.h"                        // kaStrdup
}

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"

#include "common/wsStrip.h"
#include "common/string.h"
#include "common/errorMessages.h"
#include "parse/forbiddenChars.h"
#include "parse/CompoundValueNode.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/Metadata.h"
#include "mongoBackend/dbConstants.h"

#ifdef ORIONLD
#include "orionld/common/orionldState.h"                // orionldState
#include "orionld/context/orionldContextItemExpand.h"   // orionldContextItemExpand
#endif
#include "rest/StringFilter.h"

using namespace mongo;



/* ****************************************************************************
*
* StringFilterItem::StringFilterItem -
*/
StringFilterItem::StringFilterItem() : compiledPattern(false)
{
  numberList.clear();
  stringList.clear();
}



/* ****************************************************************************
*
* StringFilterItem::fill -
*/
bool StringFilterItem::fill(StringFilterItem* sfiP, std::string* errorStringP)
{
  left                  = sfiP->left;
  op                    = sfiP->op;
  valueType             = sfiP->valueType;
  numberValue           = sfiP->numberValue;
  stringValue           = sfiP->stringValue;
  boolValue             = sfiP->boolValue;
  stringList            = sfiP->stringList;
  numberList            = sfiP->numberList;
  numberRangeFrom       = sfiP->numberRangeFrom;
  numberRangeTo         = sfiP->numberRangeTo;
  stringRangeFrom       = sfiP->stringRangeFrom;
  stringRangeTo         = sfiP->stringRangeTo;
  attributeName         = sfiP->attributeName;
  metadataName          = sfiP->metadataName;
  compiledPattern       = sfiP->compiledPattern;
  type                  = sfiP->type;
  compoundPath          = sfiP->compoundPath;

  if (compiledPattern)
  {
    //
    // FIXME P4: not very optimized to recalculate the regex.
    //
    // We don't know of a better way to copy the regex from sfiP, and have a question out on SOF:
    // http://stackoverflow.com/questions/36846426/best-way-of-cloning-compiled-regex-t-struct-in-c
    //
    if (regcomp(&patternValue, stringValue.c_str(), REG_EXTENDED) != 0)
    {
      *errorStringP = std::string("error compiling filter regex: '") + stringValue + "'";
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* StringFilterItem::~StringFilterItem -
*/
StringFilterItem::~StringFilterItem()
{
  stringList.clear();
  numberList.clear();

  if (compiledPattern == true)
  {
    regfree(&patternValue);  // If regcomp fails it frees up itself (see glibc sources for details)
    compiledPattern = false;
  }
}



/* ****************************************************************************
*
* StringFilterItem::valueParse -
*/
bool StringFilterItem::valueParse(char* s, std::string* errorStringP)
{
  bool b;

  b = valueGet(s, &valueType, &numberValue, &stringValue, &boolValue, errorStringP);
  if (b == false)
  {
    LM_E(("valueGet FAILED!"));
    return false;
  }

  //
  // Check that operator and type of value are compatible
  //
  if ((op == SfopEquals) || (op == SfopDiffers))  // ALL value types are valid for == and !=
  {
    return true;
  }
  else if ((op == SfopGreaterThan) || (op == SfopGreaterThanOrEqual) || (op == SfopLessThan) || (op == SfopLessThanOrEqual))
  {
    if ((valueType != SfvtNumber) && (valueType != SfvtDate) && (valueType != SfvtString))
    {
      *errorStringP = std::string("values of type /") + valueTypeName() + "/ not supported for operator /" + opName() + "/";
      return false;
    }

    return true;
  }

  if (op == SfopMatchPattern)
  {
    if (regcomp(&patternValue, stringValue.c_str(), REG_EXTENDED) != 0)
    {
      *errorStringP = std::string("error compiling filter regex: '") + stringValue + "'";
      return false;
    }
    compiledPattern = true;
  }

  return true;
}



/* ****************************************************************************
*
* StringFilterItem::rangeParse -
*/
bool StringFilterItem::rangeParse(char* s, std::string* errorStringP)
{
  if ((op != SfopEquals) && (op != SfopDiffers))
  {
    *errorStringP = ERROR_DESC_BAD_REQUEST_INVALID_RANGE;
    return false;
  }

  char* dotdot     = strstr(s, "..");
  char* fromString = s;
  char* toString;

  if (dotdot == NULL)  // We can't really get here - this check is done already
  {
    *errorStringP = "not a range";
    return false;
  }

  *dotdot  = 0;
  toString = &dotdot[2];

  toString   = wsStrip(toString);
  fromString = wsStrip(fromString);

  if ((*toString == 0) || (*fromString == 0))
  {
    *errorStringP = "empty item in range";
    return false;
  }

  StringFilterValueType vtFrom;
  StringFilterValueType vtTo;
  bool                  b;

  if (valueGet(fromString, &vtFrom, &numberRangeFrom, &stringRangeFrom, &b, errorStringP) == false)
  {
    return false;
  }
  if ((vtFrom != SfvtString) && (vtFrom != SfvtNumber) && (vtFrom != SfvtDate))
  {
    *errorStringP = "invalid data type for /range-from/";
    return false;
  }

  if (valueGet(toString, &vtTo, &numberRangeTo, &stringRangeTo, &b, errorStringP) == false)
  {
    return false;
  }
  if ((vtTo != SfvtString) && (vtTo != SfvtNumber) && (vtTo != SfvtDate))
  {
    *errorStringP = "invalid data type for /range-to/";
    return false;
  }
  if (vtTo != vtFrom)
  {
    *errorStringP = "invalid range: types mixed";
    return false;
  }

  if (vtFrom == SfvtNumber)
  {
    valueType = SfvtNumberRange;
  }
  else if (vtFrom == SfvtString)
  {
    valueType = SfvtStringRange;
  }
  else if (vtFrom == SfvtDate)
  {
    valueType = SfvtDateRange;
  }
  else  // We can't get here ...
  {
    *errorStringP = "invalid data type for range";
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* StringFilterItem::listItemAdd -
*/
bool StringFilterItem::listItemAdd(char* s, std::string* errorStringP)
{
  StringFilterValueType  vType;
  double                 d;
  std::string            str;
  bool                   b;

  s = wsStrip(s);
  if (*s == 0)
  {
    *errorStringP = "empty item in list";
    return false;
  }

  if (valueGet(s, &vType, &d, &str,  &b, errorStringP) == false)
  {
    return false;
  }

  // First item?
  if ((stringList.size() == 0) && (numberList.size() == 0))
  {
    if (vType == SfvtString)
    {
      valueType = SfvtStringList;
      stringList.push_back(str);
    }
    else if (vType == SfvtDate)
    {
      valueType = SfvtDateList;
      numberList.push_back(d);
    }
    else if (vType == SfvtNumber)
    {
      valueType = SfvtNumberList;
      numberList.push_back(d);
    }
    else
    {
      *errorStringP = "invalid JSON value type in list";
      return false;
    }
  }
  else  // NOT the first item
  {
    if ((vType == SfvtString) && (valueType == SfvtStringList))
    {
      stringList.push_back(str);
    }
    else if ((vType == SfvtDate) && (valueType == SfvtDateList))
    {
      numberList.push_back(d);
    }
    else if ((vType == SfvtNumber) && (valueType == SfvtNumberList))
    {
      numberList.push_back(d);
    }
    else
    {
      *errorStringP = "invalid JSON value type in list";
      stringList.clear();
      numberList.clear();
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* StringFilterItem::listParse -
*/
bool StringFilterItem::listParse(char* s, std::string* errorStringP)
{
  if ((op != SfopEquals) && (op != SfopDiffers))
  {
    *errorStringP = ERROR_DESC_BAD_REQUEST_INVALID_LIST;
    return false;
  }

  char* itemStart = wsStrip(s);
  char* cP        = itemStart;
  bool  inString  = false;

  while (*cP != 0)
  {
    if (*cP == '\'')
    {
      inString = (inString)? false : true;
    }

    if ((*cP == ',') && (inString == false))
    {
      *cP = 0;

      // forbiddenChars check is done inside listItemAdd
      if (listItemAdd(itemStart, errorStringP) == false)
      {
        return false;
      }

      ++cP;
      itemStart = cP;
      if (*itemStart == 0)
      {
        *errorStringP = "empty item in list";
        return false;
      }
    }
    else
    {
      ++cP;
    }
  }

  if (itemStart != cP)
  {
    if (listItemAdd(itemStart, errorStringP) == false)
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* StringFilterItem::valueGet -
*/
bool StringFilterItem::valueGet
(
  char*                   s,
  StringFilterValueType*  valueTypeP,
  double*                 doubleP,
  std::string*            stringP,
  bool*                   boolP,
  std::string*            errorStringP
)
{
  if (*s == '\'')
  {
    ++s;
    *valueTypeP = SfvtString;
    if (s[strlen(s) - 1] != '\'')
    {
      *errorStringP = "non-terminated forced string";
      return false;
    }

    s[strlen(s) - 1] = 0;
    *valueTypeP = SfvtString;
    *stringP    = s;

    if (strchr(s, '\'') != NULL)
    {
      *errorStringP = "quote in forced string";
      return false;
    }
  }
  else if (str2double(s, doubleP))
  {
    *valueTypeP = SfvtNumber;
  }
  else if ((*doubleP = parse8601Time(s)) != -1)
  {
    *valueTypeP = SfvtDate;
  }
  else if (strcmp(s, "true") == 0)
  {
    *valueTypeP = SfvtBool;
    *boolP      = true;
  }
  else if (strcmp(s, "false") == 0)
  {
    *valueTypeP = SfvtBool;
    *boolP      = false;
  }
  else if (strcmp(s, "null") == 0)
  {
    *valueTypeP = SfvtNull;
  }
  else  // must be a string then ...
  {
    *valueTypeP = SfvtString;
    *stringP = s;
  }

  if ((*valueTypeP == SfvtString) && (op != SfopMatchPattern))
  {
    if (forbiddenChars(s, ""))
    {
      LM_E(("forbidden characters in String Filter"));
      *errorStringP = std::string("forbidden characters in String Filter");
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* opFind - return the operator of the expression (and LHS + RHS as well)
*
* PARAMETERS:
*   expression:   (input)  the expression to parse (e.g. A==7  or  !a1  or  'b.c'.a>14)
*   lhsP:         (output) pointer to the string (char*) that references the Left Hand Side
*   rhsP:         (output) pointer to the string (char*) that references the Right Hand Side
*
* RETURN VALUE
*   The operator is returned. If 'no operator is found', then it is a unary expression for
*   existence and 'SfopExists' is returned.
*/
static StringFilterOp opFind(char* expression, char** lhsP, char** rhsP)
{
  char*           eP           = expression;
  bool            insideQuotes = false;
  StringFilterOp  op           = SfopExists;  // No operator found => SfopExists

  if (*eP == '!')  // Unary negation?
  {
    *lhsP = &eP[1];  // !a: a is LHS ...
    *rhsP = &eP[1];  // !a: a is RHS ... FIXME P1: Funny? yeah, a little. This is how it works

    return SfopNotExists;
  }

  *lhsP = expression;  // For all ops != Unary negation, LHS is the beginning of expression

  while (*eP != 0)
  {
    if (*eP == '\'')
    {
      insideQuotes = insideQuotes? false : true;
    }
    else if (!insideQuotes)
    {
      if (eP[1] == '=')
      {
        if      (*eP == '=') { *rhsP = &eP[2]; op = SfopEquals;             }
        else if (*eP == '~') { *rhsP = &eP[2]; op = SfopMatchPattern;       }
        else if (*eP == '!') { *rhsP = &eP[2]; op = SfopDiffers;            }
        else if (*eP == '>') { *rhsP = &eP[2]; op = SfopGreaterThanOrEqual; }
        else if (*eP == '<') { *rhsP = &eP[2]; op = SfopLessThanOrEqual;    }
      }
      else   if (*eP == '<') { *rhsP = &eP[1]; op = SfopLessThan;           }
      else   if (*eP == '>') { *rhsP = &eP[1]; op = SfopGreaterThan;        }
      else   if (*eP == ':') { *rhsP = &eP[1]; op = SfopEquals;             }

      if (op != SfopExists)  // operator found, RHS already set
      {
        // Mark the end of LHS - but not if op is Exists, where expression == LHS == RHS
        *eP = 0;

        return op;
      }
    }

    ++eP;
  }

  *rhsP = expression;

  return SfopExists;
}



/* ****************************************************************************
*
* StringFilterItem::parse -
*
* A StringFilterItem is a string of the form:
*
*   key OPERATOR value
*
* - The key can only contain alphanumeric chars
* - Valid operators:
*     - Binary operators
*       ==         Translates to EQUALS
*       ~=         Translates to MATCH PATTERN
*       !=         Translates to NOT EQUAL
*       >          Translates to GT
*       <          Translates to LT
*       >=         Translates to GTE
*       <=         Translates to LTE
*
*     - Unary operators:
*       !          Translates to DOES NOT EXIST (the string that comes after is the name of an attribute)
*       NOTHING:   Translates to EXISTS (the string is the name of an attribute)
*
*/
bool StringFilterItem::parse(char* qItem, std::string* errorStringP, StringFilterType _type)
{
  char* s       = strdup(qItem);
  char* toFree  = s;
  char* rhs     = NULL;
  char* lhs     = NULL;

  type = _type;

  s = wsStrip(s);
  if (*s == 0)
  {
    free(toFree);
    *errorStringP = "empty q-item";
    return false;
  }


  //
  // The start of left-hand-side is already found
  //
  lhs = s;

  //
  // 1. Find operator
  // 2. Is left-hand-side a valid string?
  // 3. Right-hand-side
  // 3.1. List
  // 3.2. Range
  // 3.3. Simple Value
  //

  //
  // 1. Find operator (which also gives up LHS and RHS)
  //
  op   = opFind(s, &lhs, &rhs);
  lhs  = wsStrip(lhs);
  rhs  = wsStrip(rhs);

  //
  // Check for invalid LHS
  // LHS can be empty ONLY if UNARY OP, i.e. SfopNotExists OR SfopExists
  //
  if ((*lhs == 0) && (op != SfopNotExists) && (op != SfopExists))
  {
    *errorStringP = "empty left-hand-side in q-item";
    free(toFree);
    return false;
  }

  if (forbiddenChars(lhs, "'"))
  {
    if (type == SftQ)
    {
      *errorStringP = "invalid character found in URI param /q/";
    }
    else
    {
      *errorStringP = "invalid character found in URI param /mq/";
    }

    free(toFree);
    return false;
  }

  // Now, finally, set the name of the left-hand-side in this object
  left = lhs;


  //
  // Now, is the left hand side just the name of an attribute/metadata?
  // Or, is it a path to an item in a complex value?
  //
  lhsParse();

#ifdef ORIONLD
  //
  // URI-expand the attribute name
  //
  if (orionldState.apiVersion == NGSI_LD_V1)
  {
    char* expanded = orionldContextItemExpand(orionldState.contextP, attributeName.c_str(), NULL, true, NULL);

    //
    // After expanding we need to replace all dots ('.') with equal signs ('='), because, that is how the attribute name is stored in mongo
    // But, we need to do this in a copy, to not destroy its real name, as 'expanded' points to the value inside the Context-Cache
    //
    expanded = kaStrdup(&orionldState.kalloc, expanded);

    char* cP = expanded;
    LM_TMP(("EQDOT: %s->%s", attributeName.c_str(), expanded));
    while (*cP != 0)
    {
      if (*cP == '.')
        *cP = '=';
      ++cP;
    }

    attributeName = expanded;
    LM_TMP(("EQDOT: attributeName: '%s'", attributeName.c_str()));
  }

#endif

  //
  // Check for empty RHS
  //
  if (*rhs == 0)
  {
    *errorStringP = "empty right-hand-side in q-item";
    free(toFree);
    return false;
  }

  //
  // Now, the right-hand-side, is it a RANGE, a LIST, a SIMPLE VALUE, or an attribute name?
  // And, what type of values?
  //
  bool b = true;
  if ((op == SfopNotExists) || (op == SfopExists))
  {
    if (forbiddenQuotes(rhs))
    {
      *errorStringP = std::string("forbidden characters in String Filter");
      free(toFree);
      return false;
    }

    if (type == SftMq)
    {
      if (metadataName == "")
      {
        *errorStringP = "no metadata in right-hand-side of q-item";
        free(toFree);
        return false;
      }
    }
  }
  else if (op == SfopMatchPattern)
  {
    //
    // RHS is a forced string for ~=
    // forbiddenChars to run over the entire RHS, no exceptions
    //
    if (forbiddenChars(rhs, ""))
    {
      *errorStringP = std::string("forbidden characters in String Filter");
      free(toFree);
      return false;
    }

    valueType   = SfvtString;
    stringValue = rhs;

    //
    // Can't call valueParse here, as the forced valueType 'SfvtString' will be knocked back to its 'default'.
    // So, instead we just perform the part of SfopMatchPattern of valueParse
    //
    if (regcomp(&patternValue, stringValue.c_str(), REG_EXTENDED) != 0)
    {
      *errorStringP = std::string("error compiling filter regex: '") + stringValue + "'";
      return false;
    }
    compiledPattern = true;
  }
  else
  {
    //
    // Forbidden char check is performed inside rangeParse, listParse, and valueParse
    // as only there the component of RHS are known
    //
    if      (strstr(rhs, "..") != NULL)   b = rangeParse(rhs, errorStringP);
    else if (strstr(rhs, ",")  != NULL)   b = listParse(rhs, errorStringP);
    else                                  b = valueParse(rhs, errorStringP);
  }

  free(toFree);
  return b;
}



/* ****************************************************************************
*
* StringFilterItem::valueAsString -
*/
void StringFilterItem::valueAsString(char* buf, int bufLen)
{
  switch (valueType)
  {
  case SfvtString:
    strncpy(buf, stringValue.c_str(), bufLen);
    break;

  case SfvtBool:
    if (SfvtBool)
      strncpy(buf, "true", bufLen);
    else
      strncpy(buf, "false", bufLen);
    break;

  case SfvtNumber:
    snprintf(buf, bufLen, "%f", numberValue);
    kFloatTrim(buf);
    break;

  case SfvtNull:
    strncpy(buf, "null", bufLen);
    break;

  case SfvtDate:
    snprintf(buf, bufLen, "%f", numberValue);
    break;

  case SfvtNumberRange:
  case SfvtDateRange:
  case SfvtStringRange:
    snprintf(buf, bufLen, "Ranges not implemented");
    break;

  case SfvtNumberList:
  case SfvtDateList:
  case SfvtStringList:
    snprintf(buf, bufLen, "Lists not implemented");
    break;
  }
}



/* ****************************************************************************
*
* StringFilterItem::render
*/
int StringFilterItem::render(char* buf, int bufLen)
{
  int  needChars        = 0;
  int  attributeNameLen = strlen(attributeName.c_str());
  char valueBuf[256];
  int  valueLen = 0;

  if (attributeNameLen >= bufLen)
  {
    LM_E(("Not enough room in rendering output buffer"));
    return -1;
  }

  if ((op != SfopExists) && (op != SfopNotExists))
  {
    valueAsString(valueBuf, sizeof(valueBuf));
    valueLen = strlen(valueBuf);
  }

  switch (op)
  {
  case SfopExists:
    needChars = attributeNameLen;
    break;

  case SfopNotExists:
    needChars =	attributeNameLen + 1;
    break;

  case SfopEquals:
  case SfopDiffers:
  case SfopGreaterThanOrEqual:
  case SfopLessThanOrEqual:
  case SfopMatchPattern:
    needChars = attributeNameLen + 2 + valueLen;
    break;

  case SfopGreaterThan:
  case SfopLessThan:
    needChars = attributeNameLen + 1 + valueLen;
    break;
  }

  if (needChars >= bufLen)
  {
    LM_E(("Not enough room in rendering output buffer"));
    return -1;
  }

  switch (op)
  {
  case SfopExists:
    snprintf(buf, bufLen, "%s", attributeName.c_str());
    break;

  case SfopNotExists:
    snprintf(buf, bufLen, "!%s", attributeName.c_str());
    break;

  case SfopEquals:
    snprintf(buf, bufLen, "%s==%s", attributeName.c_str(), valueBuf);
    break;

  case SfopDiffers:
    snprintf(buf, bufLen, "%s!=%s", attributeName.c_str(), valueBuf);
    break;

  case SfopGreaterThanOrEqual:
    snprintf(buf, bufLen, "%s>=%s", attributeName.c_str(), valueBuf);
    break;

  case SfopLessThanOrEqual:
    snprintf(buf, bufLen, "%s<=%s", attributeName.c_str(), valueBuf);
    break;

  case SfopMatchPattern:
    snprintf(buf, bufLen, "%s=%s", attributeName.c_str(), valueBuf);
    break;

  case SfopGreaterThan:
    snprintf(buf, bufLen, "%s>%s", attributeName.c_str(), valueBuf);
    break;

  case SfopLessThan:
    snprintf(buf, bufLen, "%s<%s", attributeName.c_str(), valueBuf);
    break;
  }

  return needChars;
}



/* ****************************************************************************
*
* lhsDotToEqualIfInsideQuote - change dots for equals, then remove all quotes
*/
static char* lhsDotToEqualIfInsideQuote(char* s)
{
  char* scopyP        = strdup(s);
  char* dotP          = scopyP;
  bool  insideQuotes  = false;

  //
  // Replace '.' for '=' if inside quotes
  //
  while (*dotP != 0)
  {
    if (*dotP == '\'')
    {
      insideQuotes = (insideQuotes == false)? true : false;
    }
    else if ((insideQuotes == true) && (*dotP == '.'))
    {
      *dotP = '=';
    }

    ++dotP;
  }

  //
  // Now that all '.' inside quotes are changed to '=', we can remove the quotes
  // As the resulting string is always smaller than the initial string (or equal if no quotes are present)
  // It is safe to use the inital buffer 's' to save the resulting string
  //
  int sIx = 0;  // Index of 's'
  int iIx = 0;  // Index of 'initial buffer', which is 'scopyP'

  while (scopyP[iIx] != 0)
  {
    if (scopyP[iIx] != '\'')
    {
      s[sIx] = scopyP[iIx];
      ++sIx;
    }
    ++iIx;
  }
  s[sIx] = 0;

  free(scopyP);

  return s;
}



/* ****************************************************************************
*
* StringFilterItem::lhsParse -
*/
void StringFilterItem::lhsParse(void)
{
  char* start = (char*) left.c_str();
  char* dotP  = start;

  start = lhsDotToEqualIfInsideQuote(start);

  attributeName = "";
  metadataName  = "";
  compoundPath  = "";

  dotP = strchr(start, '.');
  if (dotP == NULL)
  {
    attributeName = start;
    metadataName  = "";
    compoundPath  = "";

    return;
  }

  *dotP = 0;
  ++dotP;  // Step over the dot

  attributeName = start;

  // If MQ, a second dot must be found in order for LHS to be about compounds
  if (type == SftMq)
  {
    char* dot2P = strchr(dotP, '.');

    if (dot2P == NULL)
    {
      metadataName  = dotP;
      compoundPath  = "";
      return;
    }

    *dot2P = 0;
    ++dot2P;
    metadataName  = dotP;
    compoundPath  = dot2P;
  }
  else
  {
    //
    // So, now 'dotP' is pointing to the dot after attribute/metadata.
    // The rest of the 'path' in lhs is about an item in a compound value
    //
    compoundPath = dotP;
  }
}



/* ****************************************************************************
*
* StringFilterItem::opName -
*/
const char* StringFilterItem::opName(void)
{
  switch (op)
  {
  case SfopExists:              return "Exists";
  case SfopNotExists:           return "NotExists";
  case SfopEquals:              return "Equals";
  case SfopDiffers:             return "Differs";
  case SfopGreaterThan:         return "GreaterThan";
  case SfopGreaterThanOrEqual:  return "GreaterThanOrEqual";
  case SfopLessThan:            return "LessThan";
  case SfopLessThanOrEqual:     return "LessThanOrEqual";
  case SfopMatchPattern:        return "MatchPattern";
  }

  return "InvalidOperator";
}



/* ****************************************************************************
*
* StringFilterItem::valueTypeName -
*/
const char* StringFilterItem::valueTypeName(void)
{
  switch (valueType)
  {
  case SfvtString:              return "String";
  case SfvtBool:                return "Bool";
  case SfvtNumber:              return "Number";
  case SfvtNull:                return "Null";
  case SfvtDate:                return "Date";
  case SfvtNumberRange:         return "NumberRange";
  case SfvtDateRange:           return "DateRange";
  case SfvtStringRange:         return "StringRange";
  case SfvtNumberList:          return "NumberList";
  case SfvtDateList:            return "DateList";
  case SfvtStringList:          return "StringList";
  }

  return "InvalidValueType";
}



/* ****************************************************************************
*
* StringFilterItem::matchEquals(Metadata*) -
*/
MatchResult StringFilterItem::matchEquals(Metadata* mdP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (mdP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchEquals(compoundValueP);
  }


  if ((valueType == SfvtNumberRange) || (valueType == SfvtDateRange))
  {
    if ((mdP->numberValue < numberRangeFrom) || (mdP->numberValue > numberRangeTo))
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtStringRange)
  {
    if ((strcmp(mdP->stringValue.c_str(), stringRangeFrom.c_str()) < 0) ||
        (strcmp(mdP->stringValue.c_str(), stringRangeTo.c_str())   > 0))
    {
      return MrNoMatch;
    }
  }
  else if ((valueType == SfvtNumberList) || (valueType == SfvtDateList))
  {
    MatchResult match = MrNoMatch;

    // the metadata value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < numberList.size(); ++vIx)
    {
      if (mdP->numberValue == numberList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match != MrMatch)
    {
      return match;
    }
  }
  else if (valueType == SfvtStringList)
  {
    MatchResult match = MrNoMatch;

    // the metadata value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < stringList.size(); ++vIx)
    {
      if (mdP->stringValue == stringList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match != MrMatch)
    {
      return match;
    }
  }
  else if ((valueType == SfvtNumber) || (valueType == SfvtDate))
  {
    if (mdP->numberValue != numberValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtBool)
  {
    if (mdP->boolValue != boolValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtString)
  {
    if (mdP->stringValue != stringValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtNull)
  {
    if (mdP->valueType != orion::ValueTypeNull)
    {
      return MrNoMatch;
    }
  }
  else
  {
    LM_E(("Runtime Error (valueType '%s' is not treated)", valueTypeName()));
    return MrIncompatibleType;
  }

  return MrMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchEquals -
*
* FIXME P2: Note that the Date type doesn't exist in compound values, but as
*           this *might* be implemented some day, I leave the Data types just as for
*           non-compound comparisons. Doesn't really hurt.
*           Right now, 'Date' in compounds are treated as strings.
*/
MatchResult StringFilterItem::matchEquals(orion::CompoundValueNode* cvP)
{
  if ((valueType == SfvtNumberRange) || (valueType == SfvtDateRange))
  {
    if ((cvP->numberValue < numberRangeFrom) || (cvP->numberValue > numberRangeTo))
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtStringRange)
  {
    if ((strcmp(cvP->stringValue.c_str(), stringRangeFrom.c_str()) < 0) ||
        (strcmp(cvP->stringValue.c_str(), stringRangeTo.c_str())   > 0))
    {
      return MrNoMatch;
    }
  }
  else if ((valueType == SfvtNumberList) || (valueType == SfvtDateList))
  {
    MatchResult match = MrNoMatch;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < numberList.size(); ++vIx)
    {
      if (cvP->numberValue == numberList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match == MrNoMatch)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtStringList)
  {
    MatchResult match = MrNoMatch;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < stringList.size(); ++vIx)
    {
      if (cvP->stringValue == stringList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match != MrMatch)
    {
      return match;
    }
  }
  else if ((valueType == SfvtNumber) || (valueType == SfvtDate))
  {
    if (cvP->numberValue != numberValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtBool)
  {
    if (cvP->boolValue != boolValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtString)
  {
    if (cvP->stringValue != stringValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtNull)
  {
    if (cvP->valueType != orion::ValueTypeNull)
    {
      return MrNoMatch;
    }
  }
  else
  {
    LM_E(("Runtime Error (valueType '%s' is not treated)", valueTypeName()));
    return MrIncompatibleType;
  }

  return MrMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchEquals -
*/
MatchResult StringFilterItem::matchEquals(ContextAttribute* caP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (caP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchEquals(compoundValueP);
  }


  //
  // Comparison for non-compound attrribute values
  //
  if ((valueType == SfvtNumberRange) || (valueType == SfvtDateRange))
  {
    if ((caP->numberValue < numberRangeFrom) || (caP->numberValue > numberRangeTo))
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtStringRange)
  {
    //
    // man strcmp:
    //
    //   int strcmp(const char* s1, const char* s2);
    //
    //   The  strcmp()  function  compares  the  two  strings  s1 and s2.
    //   It returns an integer less than, equal to, or greater than zero if s1 is found,
    //   respectively, to be less than, to match, or be greater than s2.

    if ((strcmp(caP->stringValue.c_str(), stringRangeFrom.c_str()) < 0) ||
        (strcmp(caP->stringValue.c_str(), stringRangeTo.c_str())   > 0))
    {
      return MrNoMatch;
    }
  }
  else if ((valueType == SfvtNumberList) || (valueType == SfvtDateList))
  {
    MatchResult match = MrNoMatch;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < numberList.size(); ++vIx)
    {
      if (caP->numberValue == numberList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match != MrMatch)
    {
      return match;
    }
  }
  else if (valueType == SfvtStringList)
  {
    MatchResult match = MrNoMatch;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < stringList.size(); ++vIx)
    {
      if (caP->stringValue == stringList[vIx])
      {
        match = MrMatch;
        break;
      }
    }

    if (match != MrMatch)
    {
      return match;
    }
  }
  else if ((valueType == SfvtNumber) || (valueType == SfvtDate))
  {
    if (caP->numberValue != numberValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtBool)
  {
    if (caP->boolValue != boolValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtString)
  {
    if (caP->stringValue != stringValue)
    {
      return MrNoMatch;
    }
  }
  else if (valueType == SfvtNull)
  {
    if (caP->valueType != orion::ValueTypeNull)
    {
      return MrNoMatch;
    }
  }
  else
  {
    LM_E(("Runtime Error (valueType '%s' is not treated)", valueTypeName()));
    return MrIncompatibleType;
  }

  return MrMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchPattern -
*/
MatchResult StringFilterItem::matchPattern(ContextAttribute* caP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (caP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchPattern(compoundValueP);
  }


  // pattern evaluation only makes sense for attributes of type 'string'
  if (caP->valueType != orion::ValueTypeString)
  {
    return MrIncompatibleType;
  }

  return (regexec(&patternValue, caP->stringValue.c_str(), 0, NULL, 0) == 0)? MrMatch : MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchPattern -
*/
MatchResult StringFilterItem::matchPattern(orion::CompoundValueNode* cvP)
{
  // pattern evaluation only makes sense for attributes of type 'string'
  if (cvP->valueType != orion::ValueTypeString)
  {
    return MrIncompatibleType;
  }

  return (regexec(&patternValue, cvP->stringValue.c_str(), 0, NULL, 0) == 0)? MrMatch : MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchPattern -
*/
MatchResult StringFilterItem::matchPattern(Metadata* mdP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (mdP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchPattern(compoundValueP);
  }



  // pattern evaluation only makes sense for metadatas of type 'string'
  if (mdP->valueType != orion::ValueTypeString)
  {
    return MrIncompatibleType;
  }

  return (regexec(&patternValue, mdP->stringValue.c_str(), 0, NULL, 0) == 0)? MrMatch : MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::compatibleType -
*/
bool StringFilterItem::compatibleType(ContextAttribute* caP)
{
  if ((caP->valueType == orion::ValueTypeNumber) && ((valueType == SfvtNumber) || (valueType == SfvtDate)))
  {
    return true;
  }

  if ((caP->valueType == orion::ValueTypeString) && (valueType == SfvtString))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* StringFilterItem::compatibleType -
*/
bool StringFilterItem::compatibleType(Metadata* mdP)
{
  if ((mdP->valueType == orion::ValueTypeNumber) && ((valueType == SfvtNumber) || (valueType == SfvtDate)))
  {
    return true;
  }

  if ((mdP->valueType == orion::ValueTypeString) && (valueType == SfvtString))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* StringFilterItem::compatibleType -
*/
bool StringFilterItem::compatibleType(orion::CompoundValueNode* cvP)
{
  if ((cvP->valueType == orion::ValueTypeNumber) && ((valueType == SfvtNumber) || (valueType == SfvtDate)))
  {
    return true;
  }

  if ((cvP->valueType == orion::ValueTypeString) && (valueType == SfvtString))
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* StringFilterItem::matchGreaterThan -
*/
MatchResult StringFilterItem::matchGreaterThan(ContextAttribute* caP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (caP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchGreaterThan(compoundValueP);
  }

  if (!compatibleType(caP))
  {
    return MrIncompatibleType;
  }

  if (caP->valueType == orion::ValueTypeNumber)
  {
    if (caP->numberValue > numberValue)
    {
      return MrMatch;
    }
  }
  else if (caP->valueType == orion::ValueTypeString)
  {
    if (strcmp(caP->stringValue.c_str(), stringValue.c_str()) > 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchGreaterThan -
*/
MatchResult StringFilterItem::matchGreaterThan(orion::CompoundValueNode* cvP)
{
  if (!compatibleType(cvP))
  {
    return MrIncompatibleType;
  }

  if (cvP->valueType == orion::ValueTypeNumber)
  {
    if (cvP->numberValue > numberValue)
    {
      return MrMatch;
    }
  }
  else if (cvP->valueType == orion::ValueTypeString)
  {
    if (strcmp(cvP->stringValue.c_str(), stringValue.c_str()) > 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchGreaterThan -
*/
MatchResult StringFilterItem::matchGreaterThan(Metadata* mdP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (mdP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchGreaterThan(compoundValueP);
  }

  if (!compatibleType(mdP))
  {
    return MrIncompatibleType;
  }

  if (mdP->valueType == orion::ValueTypeNumber)
  {
    if (mdP->numberValue > numberValue)
    {
      return MrMatch;
    }
  }
  else if (mdP->valueType == orion::ValueTypeString)
  {
    if (strcmp(mdP->stringValue.c_str(), stringValue.c_str()) > 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchLessThan -
*/
MatchResult StringFilterItem::matchLessThan(ContextAttribute* caP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (caP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchLessThan(compoundValueP);
  }

  if (!compatibleType(caP))
  {
    return MrIncompatibleType;
  }

  if (caP->valueType == orion::ValueTypeNumber)
  {
    if (caP->numberValue < numberValue)
    {
      return MrMatch;
    }
  }
  else if (caP->valueType == orion::ValueTypeString)
  {
    if (strcmp(caP->stringValue.c_str(), stringValue.c_str()) < 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchLessThan -
*/
MatchResult StringFilterItem::matchLessThan(orion::CompoundValueNode* cvP)
{
  if (!compatibleType(cvP))
  {
    return MrIncompatibleType;
  }

  if (cvP->valueType == orion::ValueTypeNumber)
  {
    if (cvP->numberValue < numberValue)
    {
      return MrMatch;
    }
  }
  else if (cvP->valueType == orion::ValueTypeString)
  {
    if (strcmp(cvP->stringValue.c_str(), stringValue.c_str()) < 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilterItem::matchLessThan -
*/
MatchResult StringFilterItem::matchLessThan(Metadata* mdP)
{
  //
  // First of all, are we treating with a compound?
  // If so, check for errors and if all OK, delegate to other function
  //
  if (compoundPath.size() != 0)
  {
    orion::CompoundValueNode* compoundValueP;

    if (mdP->compoundItemExists(compoundPath, &compoundValueP) == false)
    {
      return MrDoesntExist;
    }

    return matchLessThan(compoundValueP);
  }

  if (!compatibleType(mdP))
  {
    return MrIncompatibleType;
  }

  if (mdP->valueType == orion::ValueTypeNumber)
  {
    if (mdP->numberValue < numberValue)
    {
      return MrMatch;
    }
  }
  else if (mdP->valueType == orion::ValueTypeString)
  {
    if (strcmp(mdP->stringValue.c_str(), stringValue.c_str()) < 0)
    {
      return MrMatch;
    }
  }

  return MrNoMatch;
}



/* ****************************************************************************
*
* StringFilter::StringFilter -
*/
StringFilter::StringFilter(StringFilterType _type):
  type(_type)
{
}



/* ****************************************************************************
*
* StringFilter::~StringFilter -
*/
StringFilter::~StringFilter()
{
  mongoFilters.clear();

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    delete filters[ix];
  }

  filters.clear();
}



/* ****************************************************************************
*
* StringFilter::parse -
*/
bool StringFilter::parse(const char* q, std::string* errorStringP)
{
  //
  // Initial Sanity check (of the entire string)
  // - Not empty             (empty q)
  // - Not just a single ';' (empty q)
  // - Not two ';' in a row  (empty q-item)
  //
  if ((q[0] == 0) || ((q[0] == ';') && (q[1] == 0)))
  {
    *errorStringP = "empty q";
    return false;
  }

  if (strstr(q, ";;") != NULL)
  {
    *errorStringP = "empty q-item";
    return false;
  }


  //
  // If q ends in single ';', just silently remove it to avoid error.
  // Note that before this is done, a check for ';;' anywhere in 'q' is performed
  // to detect empty items in 'q'
  //
  int   qLen      = strlen(q);
  char* lastCharP = (char*) &q[qLen - 1];

  if (*lastCharP == ';')
  {
    *lastCharP = 0;
  }

  char* str         = strdup(q);
  char* toFree      = str;
  char* saver;
  char* s           = wsStrip(str);
  int   len         = strlen(s);

  if (s[len - 1] == ';')
  {
    *errorStringP = "empty q-item";
    free(toFree);
    return false;
  }

  //
  // FIXME P6: Using strtok_r here is not very good, a ';' could be inside a string
  //           and strtok_r wouldn't recognize that.
  //
  while ((s = strtok_r(str, ";", &saver)) != NULL)
  {
    s = wsStrip(s);

    if (*s == 0)
    {
      *errorStringP = "empty q-item (only whitespace)";
      free(toFree);
      return false;
    }

    StringFilterItem* item = new StringFilterItem();

    if (item->parse(s, errorStringP, type) == true)
    {
      filters.push_back(item);
    }
    else
    {
      delete item;
      free(toFree);
      return false;
    }

    str = NULL;  // So that strtok_r continues eating the initial string
  }

  free(toFree);
  return mongoFilterPopulate(errorStringP);
}



/* ****************************************************************************
*
* StringFilter::render -
*/
bool StringFilter::render(char* buf, int bufLen, std::string* errorStringP)
{
  char* bufP       = buf;
  int   charsUsed  = 0;
  int   chars;

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem*  itemP = filters[ix];

    if (ix != 0)
    {
      *bufP = ';';
      ++charsUsed;
    }

    chars = itemP->render(bufP, bufLen - charsUsed);

    if (chars == -1)
    {
      *errorStringP = "Internal error - Not enough room in rendering output buffer of StringFilter";
      LM_E((errorStringP->c_str()));
      return false;
    }

    bufP = &bufP[chars];
    charsUsed += chars;
    buf[charsUsed] = 0;
  }

  return true;
}



/* ****************************************************************************
*
* StringFilter::mongoFilterPopulate -
*/
bool StringFilter::mongoFilterPopulate(std::string* errorStringP)
{
  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem*  itemP = filters[ix];
    std::string        k;
    BSONArrayBuilder   ba;
    BSONObjBuilder     bob;
    BSONObjBuilder     bb;
    BSONObjBuilder     bb2;
    BSONObj            f;
    std::string        left = std::string(itemP->left.c_str());

    //
    // Left hand side might have to change, in case of Metadata filters (mq)
    // The change consists in adding a '.md.' between attribute-name and metadata-name.
    // [ Assuming ENT_ATTRS_MD has the value 'md' ]
    //
    // This is to make the queries in mongo work.
    //
    if (type == SftMq)
    {
      if (itemP->attributeName == "")
      {
        *errorStringP = "no attribute name - not valid for metadata filters";
        return false;
      }

      if (itemP->metadataName == "")
      {
        *errorStringP = "no metadata name - not valid for metadata filters";
        return false;
      }

      left = itemP->attributeName + "." + ENT_ATTRS_MD + "." + itemP->metadataName;
    }
    else if ((itemP->op == SfopExists) || (itemP->op == SfopNotExists))
    {
      left = itemP->attributeName;
    }

    //
    // Also, if the left hand side contains a compound path, then we are
    // dealing with matching of a compound item.
    //
    if (itemP->compoundPath.size() != 0)
    {
      if (itemP->type == SftQ)
      {
        left = std::string(ENT_ATTRS) + "." + itemP->attributeName + "." + ENT_ATTRS_VALUE + "." + itemP->compoundPath;
        k = left;
      }
      else if (itemP->type == SftMq)
      {
        left = std::string(ENT_ATTRS) + "." +
               itemP->attributeName   + "." +
               ENT_ATTRS_MD           + "." +
               itemP->metadataName    + "." +
               ENT_ATTRS_MD_VALUE     + "." +
               itemP->compoundPath;
        k = left;
      }
    }
    else if (left == DATE_CREATED)
    {
      k = ENT_CREATION_DATE;
    }
    else if (left == DATE_MODIFIED)
    {
      k = ENT_MODIFICATION_DATE;
    }
    else if (left == itemP->attributeName + "." + ENT_ATTRS_MD "." + NGSI_MD_DATECREATED)
    {
      k = std::string(ENT_ATTRS) + "." + itemP->attributeName + "." + ENT_ATTRS_CREATION_DATE;
    }
    else if (left == itemP->attributeName + "." + ENT_ATTRS_MD "." + NGSI_MD_DATEMODIFIED)
    {
      k = std::string(ENT_ATTRS) + "." + itemP->attributeName + "." + ENT_ATTRS_MODIFICATION_DATE;
    }
    else
    {
      if (itemP->metadataName != "")
        k = std::string(ENT_ATTRS) + "." + left + "." ENT_ATTRS_VALUE;
      else
        k = std::string(ENT_ATTRS) + "." + itemP->attributeName + "." ENT_ATTRS_VALUE;
    }

    switch (itemP->op)
    {
    case SfopExists:
      bb.append("$exists", true);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopNotExists:
      bb.append("$exists", false);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopEquals:
      switch (itemP->valueType)
      {
      case SfvtNumberRange:
      case SfvtDateRange:
        bb.append("$gte", itemP->numberRangeFrom).append("$lte", itemP->numberRangeTo);
        break;

      case SfvtStringRange:
        bb.append("$gte", itemP->stringRangeFrom).append("$lte", itemP->stringRangeTo);
        break;

      case SfvtNumberList:
      case SfvtDateList:
        for (unsigned int ix = 0; ix < itemP->numberList.size(); ++ix)
        {
          ba.append(itemP->numberList[ix]);
        }
        bb.append("$in", ba.arr());
        break;

      case SfvtStringList:
        for (unsigned int ix = 0; ix < itemP->stringList.size(); ++ix)
        {
          ba.append(itemP->stringList[ix]);
        }
        bb.append("$in", ba.arr());
        break;

      case SfvtBool:
        bb.append("$exists", true).append("$in", BSON_ARRAY(itemP->boolValue));
        break;

      case SfvtNull:
        //
        // NOTE: $type 10 corresponds to NULL value
        // SEE:  https://docs.mongodb.com/manual/reference/bson-types/
        //
        bb.append("$exists", true).append("$type", 10);
        break;

      case SfvtNumber:
      case SfvtDate:
        bb.append("$exists", true).append("$in", BSON_ARRAY(itemP->numberValue));
        break;

      case SfvtString:
        bb.append("$in", BSON_ARRAY(itemP->stringValue));
        break;

      default:
        *errorStringP = "invalid valueType for q-item";
        return false;
        break;
      }

      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopDiffers:
      if ((itemP->valueType == SfvtNumberRange) || (itemP->valueType == SfvtDateRange))
      {
        bb.append("$gte", itemP->numberRangeFrom).append("$lte", itemP->numberRangeTo);
        bb2.append("$exists", true).append("$not", bb.obj());
        bob.append(k, bb2.obj());
        f = bob.obj();
      }
      else if ((itemP->valueType == SfvtNumberList) || (itemP->valueType == SfvtDateList))
      {
        for (unsigned int ix = 0; ix < itemP->numberList.size(); ++ix)
        {
          ba.append(itemP->numberList[ix]);
        }

        bb.append("$exists", true).append("$nin", ba.arr());
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtStringRange)
      {
        bb.append("$gte", itemP->stringRangeFrom).append("$lte", itemP->stringRangeTo);
        bb2.append("$exists", true).append("$not", bb.obj());
        bob.append(k, bb2.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtStringList)
      {
        for (unsigned int ix = 0; ix < itemP->stringList.size(); ++ix)
        {
          ba.append(itemP->stringList[ix]);
        }

        bb.append("$exists", true).append("$nin", ba.arr());
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtBool)
      {
        bb.append("$exists", true).append("$nin", BSON_ARRAY(itemP->boolValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtNull)
      {
        //
        // NOTE: $type 10 corresponds to NULL value
        // SEE:  https://docs.mongodb.com/manual/reference/bson-types/
        //
        bb.append("$exists", true).append("$not", BSON("$type" << 10));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if ((itemP->valueType == SfvtNumber) || (itemP->valueType == SfvtDate))
      {
        bb.append("$exists", true).append("$nin", BSON_ARRAY(itemP->numberValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtString)
      {
        bb.append("$exists", true).append("$nin", BSON_ARRAY(itemP->stringValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      break;

    case SfopGreaterThan:
      if (itemP->valueType == SfvtString)
      {
        bb.append("$gt", itemP->stringValue);
      }
      else
      {
        bb.append("$gt", itemP->numberValue);
      }
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopGreaterThanOrEqual:
      if (itemP->valueType == SfvtString)
      {
        bb.append("$gte", itemP->stringValue);
      }
      else
      {
        bb.append("$gte", itemP->numberValue);
      }
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopLessThan:
      if (itemP->valueType == SfvtString)
      {
        bb.append("$lt", itemP->stringValue);
      }
      else
      {
        bb.append("$lt", itemP->numberValue);
      }
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopLessThanOrEqual:
      if (itemP->valueType == SfvtString)
      {
        bb.append("$lte", itemP->stringValue);
      }
      else
      {
        bb.append("$lte", itemP->numberValue);
      }
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopMatchPattern:
      if (itemP->valueType != SfvtString)
      {
        // Pattern filter only makes sense with string value
        continue;
      }
      bb.append("$regex", itemP->stringValue);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;
    }

    mongoFilters.push_back(f);
  }

  return true;
}



/* ****************************************************************************
*
* StringFilter::match -
*
* This method is used in mongoBackend/MongoCommonUpdate.cpp, processSubscriptions()
* to help decide whether a Notification is to be sent after updating an entity
*/
bool StringFilter::match(ContextElementResponse* cerP)
{
  bool b;

  if (type == SftQ)
  {
    b = qMatch(cerP);
  }
  else
  {
    b = mqMatch(cerP);
  }

  return b;
}



/* ****************************************************************************
*
* StringFilter::mqMatch -
*
* NOTE
*   The filters are ANDed together, so, if a 'no match' is encountered, we can safely
*   return false (which means no match).
*   HOWEVER, the value 'true' can never be retuirned from within the loop over the filter items.
*   For the function to return 'true', ALL the filter items must be a match
*/
bool StringFilter::mqMatch(ContextElementResponse* cerP)
{
  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem*  itemP = filters[ix];
    ContextAttribute*  caP   = cerP->contextElement.getAttribute(itemP->attributeName);

    if ((itemP->op == SfopExists) || (itemP->op == SfopNotExists))
    {
      Metadata*  mdP = (caP == NULL)? NULL : caP->metadataVector.lookupByName(itemP->metadataName);

      if (itemP->compoundPath.size() == 0)
      {
        if ((itemP->op == SfopExists) && (mdP == NULL))
        {
          return false;
        }
        else if ((itemP->op == SfopNotExists) && (mdP != NULL))
        {
          return false;
        }
      }
      else  // compare with an item in the compound value
      {
        bool exists = (mdP != NULL) && (mdP->compoundItemExists(itemP->compoundPath) == true);

        if ((itemP->op == SfopExists) && (exists == false))
        {
          return false;
        }
        else if ((itemP->op == SfopNotExists) && (exists == true))
        {
          return false;
        }
      }
    }

    /* From here, the approach is very similar to the one used in qMatch() function */
    if (caP == NULL)
    {
      return false;
    }

    Metadata*  mdP = NULL;
    Metadata   md;

    if ((itemP->metadataName == NGSI_MD_DATECREATED) || (itemP->metadataName == NGSI_MD_DATEMODIFIED))
    {
      mdP            = &md;
      md.valueType   = orion::ValueTypeNumber;
      md.numberValue = (itemP->metadataName == NGSI_MD_DATECREATED)? caP->creDate : caP->modDate;
    }
    else if (itemP->op != SfopNotExists)
    {
      mdP = caP->metadataVector.lookupByName(itemP->metadataName);

      // If the metadata doesn't exist, no need to go further: filter fails
      if (mdP == NULL)
      {
        return false;
      }
    }

    if ((itemP->op == SfopEquals) && (itemP->matchEquals(mdP) != MrMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopDiffers) && (itemP->matchEquals(mdP) != MrNoMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopGreaterThan) && (itemP->matchGreaterThan(mdP) != MrMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopLessThan) && (itemP->matchLessThan(mdP) != MrMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopGreaterThanOrEqual) && (itemP->matchLessThan(mdP) != MrNoMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopLessThanOrEqual) && (itemP->matchGreaterThan(mdP) != MrNoMatch))
    {
      return false;
    }
    else if ((itemP->op == SfopMatchPattern) && (itemP->matchPattern(mdP) != MrMatch))
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* StringFilter::qMatch -
*
* NOTE
*   The filters are ANDed together, so, if a 'no match' is encountered, we can safely
*   return false (which means no match).
*   HOWEVER, the value 'true' can never be retuirned from within the loop over the filter items.
*   For the function to return 'true', ALL the filter items must be a match
*/
bool StringFilter::qMatch(ContextElementResponse* cerP)
{
  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem* itemP = filters[ix];

    // Unary operator?
    if ((itemP->op == SfopExists) || (itemP->op == SfopNotExists))
    {
      ContextAttribute* caP = cerP->contextElement.getAttribute(itemP->attributeName);

      if (itemP->compoundPath.size() == 0)
      {
        if ((itemP->op == SfopExists) && (caP == NULL))
        {
          return false;
        }
        else if ((itemP->op == SfopNotExists) && (caP != NULL))
        {
          return false;
        }
      }
      else  // compare with an item in the compound value
      {
        bool exists = (caP != NULL) && (caP->compoundItemExists(itemP->compoundPath) == true);

        if ((itemP->op == SfopExists) && (exists == false))
        {
          return false;
        }
        else if ((itemP->op == SfopNotExists) && (exists == true))
        {
          return false;
        }
      }
    }

    //
    // For binary operators, the left side is:
    //   - 'dateCreated'     (use the creation date of the contextElement)
    //   - 'dateModified'    (use the modification date of the contextElement)
    //   - attribute name    (use the value of the attribute)
    //   - compound path     (use the value of the item of the compound value)
    //
    ContextAttribute*  caP = NULL;
    ContextAttribute   ca;

    if ((itemP->left == DATE_CREATED) || (itemP->left == DATE_MODIFIED))
    {
      caP            = &ca;
      ca.valueType   = orion::ValueTypeNumber;
      ca.numberValue = (itemP->left == DATE_CREATED)? cerP->contextElement.entityId.creDate : cerP->contextElement.entityId.modDate;
    }
    else if (itemP->op != SfopNotExists)
    {
      caP = cerP->contextElement.getAttribute(itemP->attributeName);

      // If the attribute doesn't exist, no need to go further: filter fails
      if (caP == NULL)
      {
        return false;
      }
    }

    switch (itemP->op)
    {
    case SfopExists:
    case SfopNotExists:
      // Already treated, but needs to be in switch to avoid compilation problems
      break;

    case SfopEquals:
      if (itemP->matchEquals(caP) != MrMatch)
      {
        return false;
      }
      break;

    case SfopDiffers:
      if (itemP->matchEquals(caP) != MrNoMatch)
      {
        return false;
      }
      break;

    case SfopGreaterThan:
      if (itemP->matchGreaterThan(caP) != MrMatch)
      {
        return false;
      }
      break;

    case SfopLessThan:
      if (itemP->matchLessThan(caP) != MrMatch)
      {
        return false;
      }
      break;

    case SfopGreaterThanOrEqual:
      if (itemP->matchLessThan(caP) != MrNoMatch)
      {
        //
        // Double negation ... always complicated
        //
        // If matchLessThan() returns NOT COMPATIBLE: GreaterThanOrEqual is also NOT COMPATIBLE => Not a Match (FALSE)
        // If matchLessThan() returns DOESN'T EXIST:  GreaterThanOrEqual is also DOESN'T EXIST  => Not a Match (FALSE)
        // If matchLessThan() returns MATCH:          GreaterThanOrEqual is NOT MATCH           => Not a Match (FALSE)
        // If matchLessThan() returns NO MATCH:       GreaterThanOrEqual IS A MATCH             => MATCH (TRUE)
        //
        return false;
      }
      break;

    case SfopLessThanOrEqual:
      if (itemP->matchGreaterThan(caP) != MrNoMatch)
      {
        return false;
      }
      break;

    case SfopMatchPattern:
      if (itemP->matchPattern(caP) != MrMatch)
      {
        return false;
      }
      break;
    }
  }

  return true;
}



/* ****************************************************************************
*
* StringFilter::clone -
*/
StringFilter* StringFilter::clone(std::string* errorStringP)
{
  StringFilter* sfP = new StringFilter(type);

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem*  sfi = new StringFilterItem();

    if (!sfi->fill(filters[ix], errorStringP))
    {
      delete sfi;
      delete sfP;
      return NULL;
    }

    sfP->filters.push_back(sfi);
  }

  // Object copy
  sfP->mongoFilters = mongoFilters;

  return sfP;
}



/* ****************************************************************************
*
* StringFilter::fill -
*/
bool StringFilter::fill(StringFilter* sfP, std::string* errorStringP)
{
  for (unsigned int ix = 0; ix < sfP->filters.size(); ++ix)
  {
    StringFilterItem*  sfi = new StringFilterItem();

    if (!sfi->fill(sfP->filters[ix], errorStringP))
    {
      delete sfi;
      LM_E(("Runtime Error (error filling StringFilterItem: %s)", errorStringP->c_str()));
      return false;
    }

    filters.push_back(sfi);
  }

  // Object copy
  mongoFilters = sfP->mongoFilters;

  return true;
}
