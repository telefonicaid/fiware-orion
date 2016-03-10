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

#include "mongo/client/dbclient.h"

#include "logMsg/logMsg.h"

#include "common/wsStrip.h"
#include "common/string.h"
#include "parse/forbiddenChars.h"
#include "rest/StringFilter.h"
#include "ngsi/ContextElementResponse.h"
#include "mongoBackend/dbConstants.h"

using namespace mongo;



/* ****************************************************************************
*
* StringFilterItem::valueParse - 
*/
bool StringFilterItem::valueParse(char* s, std::string* errorStringP)
{
  bool b;

  LM_W(("KZ: Calling valueGet"));
  b = valueGet(s, &valueType, &numberValue, &stringValue, &boolValue, errorStringP);
  LM_W(("KZ: op == %s, valueType == %s", opName(), valueTypeName()));

  if (b == false)
  {
    LM_W(("KZ: valueGet returned false: %s", errorStringP->c_str()));
    return b;
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
    LM_W(("KZ: op == %s, valueType == %s", opName(), valueTypeName()));
    if ((valueType != SfvtNumber) && (valueType != SfvtDate))
    {
      *errorStringP = std::string("values of type /") + valueTypeName() + "/ not supported for operator /" + opName() + "/";
      return false;
    }
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
    *errorStringP = "ranges only valid for == and != ops";
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

  LM_W(("KZ: -----------------------"));
  LM_W(("KZ: Got a %s range", valueTypeName()));
  if (valueType == SfvtNumberRange)
  {
    LM_W(("KZ:   From: %f", numberRangeFrom));
    LM_W(("KZ:   To:   %f", numberRangeTo));
  }
  LM_W(("KZ: -----------------------"));

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

  LM_W(("KZ: listItemAdd: s == '%s'", s));
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
    *errorStringP = "lists only valid for == and != ops";
    return false;
  }

  LM_W(("KZ: list to parse: '%s", s));
  
  char* itemStart = wsStrip(s);
  char* cP        = itemStart;
  bool  inString  = false;

  while (*cP != 0)
  {
    if (*cP == '\'')
    {
      if (!inString)
      {
        ++itemStart;
        inString = true;
      }
      else
      {
        inString = false;
        *cP = 0;
        LM_W(("KZ: list item item with stripped \'s: %s", itemStart));
      }
      ++cP;
    }
    else if ((*cP == ',') && (inString == false))
    {
      *cP = 0;

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

    LM_W(("KZ: String value with quotes"));
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

  return true;
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
*       !=         Translates to NOT EQUAL
*       >          Translates to GT
*       <          Translates to LT
*       >=         Translates to GTE
*       <=         Translates to LTE
*
*     -Unary operators:
*       NOTHING:   Translates to EXISTS
*       !          Translates to DOES NOT EXIST
*/
bool StringFilterItem::parse(char* qItem, std::string* errorStringP)
{
  char* s       = strdup(qItem);
  char* toFree  = s;
  char* rhs     = NULL;
  char* lhs     = NULL;

  s = wsStrip(s);
  if (*s == 0)
  {
    free(toFree);
    *errorStringP = "empty q-item";
    return false;
  }

  //
  // If string starts with single-quote it must also end with single-quoite and it is
  // a string, of course.
  // Also, the resulting string after removing the quotes cannot contain any quotes ... ?
  //
  if (*s == '\'')
  {
    ++s;

    if (s[strlen(s) - 1] != '\'')
    {
      free(toFree);
      *errorStringP = "non-terminated forced string";
      return false;
    }
    s[strlen(s) - 1] = 0;

    if (strchr(s, '\'') != NULL)
    {
      *errorStringP = "quote in forced string";
      return false;
    }

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
  // 1. Find operator
  //
  // if 'No-Operator-Found' => Test for EXISTENCE of an attribute
  //
  char* opP = NULL;

  if      ((opP = strstr(s, "==")) != NULL)  { op  = SfopEquals;              rhs = &opP[2];        }
  else if ((opP = strstr(s, "!=")) != NULL)  { op  = SfopDiffers;             rhs = &opP[2];        }
  else if ((opP = strstr(s, "<=")) != NULL)  { op  = SfopLessThanOrEqual;     rhs = &opP[2];        }
  else if ((opP = strstr(s, "<"))  != NULL)  { op  = SfopLessThan;            rhs = &opP[1];        }
  else if ((opP = strstr(s, ">=")) != NULL)  { op  = SfopGreaterThanOrEqual;  rhs = &opP[2];        }
  else if ((opP = strstr(s, ">"))  != NULL)  { op  = SfopGreaterThan;         rhs = &opP[1];        }
  else if (*s == '!')                        { op  = SfopNotExists;           rhs = &s[1]; opP = s; }
  else                                       { op  = SfopExists;              rhs = s;              }

  // Mark the end of LHS
  if (opP != NULL)
  {
    *opP = 0;
  }
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
  if (forbiddenChars(lhs, ""))
  {
    *errorStringP = "invalid character found in URI param /q/";
    free(toFree);
    return false;
  }

  // Now, finally, set the name of the left-hand-side in this object
  left = lhs;

  //
  // Check for empty RHS
  //
  if (*rhs == 0)
  {
    *errorStringP = "empty right-hand-side in q-item";
    free(toFree);
    return false;
  }


  LM_W(("KZ: LHS: '%s'", lhs));
  LM_W(("KZ: OP:  '%s'", opName()));
  LM_W(("KZ: RHS: '%s'", rhs));
  LM_W(("KZ: OR:   %s %s %s", lhs, opName(), rhs));
  LM_W(("KZ: -------------------------"));
  LM_W(("KZ"));

  //
  // Now, the right-hand-side, is it a RANGE, a LIST, a SIMPLE VALUE, or an attribute name?
  // And, what type of values?
  //
  bool b = true;
  if ((op == SfopNotExists) || (op == SfopExists))
  {
    attributeName = rhs;
  }
  else
  {
    if      (strstr(rhs, "..") != NULL)   b = rangeParse(rhs, errorStringP);
    else if (strstr(rhs, ",")  != NULL)   b = listParse(rhs, errorStringP);
    else                                  b = valueParse(rhs, errorStringP);
  }

  LM_W(("KZ: returning %s", b? "true":"false"));
  return b;
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
* StringFilterItem::matchEquals - 
*/
bool StringFilterItem::matchEquals(ContextAttribute* caP)
{
  LM_W(("KZ: valueType == %s", valueTypeName()));

  if ((valueType == SfvtNumberRange) || (valueType == SfvtDateRange))
  {
    LM_W(("KZ: NumberRange or DateRange"));
    LM_W(("KZ: caP->numberValue == %f", caP->numberValue));
    LM_W(("KZ: numberRangeFrom  == %f", numberRangeFrom));
    LM_W(("KZ: numberRangeTo    == %f",numberRangeTo));

    if ((caP->numberValue < numberRangeFrom) || (caP->numberValue > numberRangeTo))
    {
      LM_W(("KZ: matchEquals: FALSE"));
      return false;
    }
    LM_W(("KZ: matchEquals: OK"));
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
      return false;
    }
  }
  else if ((valueType == SfvtNumberList) || (valueType == SfvtDateList))
  {
    bool match = false;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < numberList.size(); ++vIx)
    {
      if (caP->numberValue == numberList[vIx])
      {
        match = true;
        break;
      }
    }

    if (match == false)
    {
      return false;
    }
  }
  else if (valueType == SfvtStringList)
  {
    bool match = false;

    // the attribute value has to match at least one of the elements in the vector (OR)
    for (unsigned int vIx = 0; vIx < stringList.size(); ++vIx)
    {
      if (caP->stringValue == stringList[vIx])
      {
        match = true;
        break;
      }
    }

    if (match == false)
    {
      return false;
    }
  }
  else if ((valueType == SfvtNumber) || (valueType == SfvtDate))
  {
    if (caP->numberValue != numberValue)
    {
      return false;
    }
  }
  else if (valueType == SfvtString)
  {
    if (caP->stringValue != stringValue)
    {
      return false;
    }
  }
  else
  {
    LM_E(("Runtime Error (valueType '%s' is not treated)", valueTypeName()));
    return false;
  }

  LM_W(("KZ: return TRUE"));
  return true;
}



/* ****************************************************************************
*
* StringFilterItem::matchGreaterThan - 
*/
bool StringFilterItem::matchGreaterThan(ContextAttribute* caP)
{
  if (caP->valueType != orion::ValueTypeNumber)
  {
    return false;
  }
  if ((valueType != SfvtNumber) && (valueType!= SfvtDate))
  {
    return false;
  }

  if (caP->numberValue > numberValue)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* StringFilterItem::matchLessThan - 
*/
bool StringFilterItem::matchLessThan(ContextAttribute* caP)
{
  if (caP->valueType != orion::ValueTypeNumber)
  {
    return false;
  }
  if ((valueType != SfvtNumber) && (valueType!= SfvtDate))
  {
    return false;
  }

  if (caP->numberValue < numberValue)
  {
    return true;
  }

  return false;
}



/* ****************************************************************************
*
* StringFilterItem::present - 
*/
void StringFilterItem::present(std::string prefix)
{
  LM_W(("%sStringFilterItem '%s':", prefix.c_str(), left.c_str()));
  LM_W(("%s  OP:  %s", prefix.c_str(), opName()));
  LM_W(("%s  RHS: %s", prefix.c_str(), valueTypeName()));

  if (valueType == SfvtString)
  {
    LM_W(("%s  '%s'", prefix.c_str(), stringValue.c_str()));
  }
  else if (valueType == SfvtBool)
  {
    LM_W(("%s  %s", prefix.c_str(), (boolValue == true)? "true" : "false"));
  }
  else if (valueType == SfvtNumber)
  {
    LM_W(("%s  %f", prefix.c_str(), numberValue));
  }
  else if (valueType == SfvtNull)
  {
    LM_W(("%s  NULL", prefix.c_str()));
  }
  else if (valueType == SfvtDate)
  {
    LM_W(("%s  %f", prefix.c_str(), numberValue));
  }
  else if (valueType == SfvtNumberRange)
  {
    LM_W(("%s  %f - %f", prefix.c_str(), numberRangeFrom, numberRangeTo));
  }
  else if (valueType == SfvtDateRange)
  {
    LM_W(("%s  %f - %f", prefix.c_str(), numberRangeFrom, numberRangeTo));
  }
  else if (valueType == SfvtStringRange)
  {
    LM_W(("%s  '%s' - '%s'", prefix.c_str(), stringRangeFrom.c_str(), stringRangeTo.c_str()));
  }
  else if (valueType == SfvtNumberList)
  {
    LM_W(("%s  %f, %f, ... (%d values in vector)", prefix.c_str(), numberList[0], numberList[1], numberList.size()));
  }
  else if (valueType == SfvtDateList)
  {
    LM_W(("%s  %f, %f, ... (%d values in vector)", prefix.c_str(), numberList[0], numberList[1], numberList.size()));
  }
  else if (valueType == SfvtStringList)
  {
    LM_W(("%s  '%s', '%s', ... (%d values in vector)", prefix.c_str(), stringList[0].c_str(), stringList[1].c_str(), stringList.size()));
  }
  else
  {
    LM_W(("%s  Invalid value-type", prefix.c_str()));
  }
}



/* ****************************************************************************
*
* StringFilter::parse - 
*/
bool StringFilter::parse(const char* q, std::string* errorStringP)
{
  plainString = q;

  LM_W(("KZ: parse q '%s'", q));

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

    StringFilterItem item;

    if (item.parse(s, errorStringP) == true)
    {
      filters.push_back(item);
    }
    else
    {
      LM_W(("KZ: ERROR: %s", errorStringP->c_str()));
      free(toFree);
      return false;
    }

    str = NULL;  // So that strtok_r continues eating the initial string
  }

  LM_W(("KZ: StringFilter parse was successful"));
  free(toFree);
  LM_W(("KZ: Calling mongoFilterPopulate"));
  return mongoFilterPopulate(errorStringP);
}



/* ****************************************************************************
*
* StringFilter::mongoFilterPopulate - 
*/
bool StringFilter::mongoFilterPopulate(std::string* errorStringP)
{
  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem*  itemP = &filters[ix];
    std::string        k;
    BSONArrayBuilder   ba;
    BSONObjBuilder     bob;
    BSONObjBuilder     bb;
    BSONObjBuilder     bb2;
    BSONObj            f;

    if (itemP->left == DATE_CREATED)
    {
      k = ENT_CREATION_DATE;
    }
    else if (itemP->left == DATE_MODIFIED)
    {
      k = ENT_MODIFICATION_DATE;
    }
    else
    {
      k = std::string(ENT_ATTRS) + "." + itemP->left + "." ENT_ATTRS_VALUE;
    }


    switch (itemP->op)
    {
    case SfopExists:
      k = std::string(ENT_ATTRS) + "." + itemP->attributeName;
      bb.append("$exists", true);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopNotExists:
      k = std::string(ENT_ATTRS) + "." + itemP->attributeName;
      bb.append("$exists", false);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopEquals:
      if ((itemP->valueType == SfvtNumberRange) || (itemP->valueType == SfvtDateRange))
      {
        bb.append("$gte", itemP->numberRangeFrom).append("$lte", itemP->numberRangeTo);
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtStringRange)
      {
        *errorStringP = "string range is not supported";
        LM_W(("KZ: %s", (*errorStringP).c_str()));
        return false;
      }
      else if ((itemP->valueType == SfvtNumberList) || (itemP->valueType == SfvtDateList))
      {
        for (unsigned int ix = 0; ix < itemP->numberList.size(); ++ix)
        {
          ba.append(itemP->numberList[ix]);
        }

        bb.append("$in", ba.arr());
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtStringList)
      {
        for (unsigned int ix = 0; ix < itemP->stringList.size(); ++ix)
        {
          ba.append(itemP->stringList[ix]);
        }

        bb.append("$in", ba.arr());
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtBool)
      {
        bb.append("$exists", true).append("$in", BSON_ARRAY(itemP->boolValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtNull)
      {
        *errorStringP = "null values not supported";
        LM_W(("KZ: %s", (*errorStringP).c_str()));
        return false;
      }
      else if ((itemP->valueType == SfvtNumber) || (itemP->valueType == SfvtDate))
      {
        bb.append("$exists", true).append("$in", BSON_ARRAY(itemP->numberValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtString)
      {
        bb.append("$in", BSON_ARRAY(itemP->stringValue));
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else
      {
        LM_W(("KZ: invalid valueType (%d) for q-item", itemP->valueType));
        *errorStringP = "invalid valueType for q-item";
        LM_W(("KZ: %s", (*errorStringP).c_str()));
        return false;
      }
      break;

    case SfopDiffers:
      LM_W(("KZ: NOT EQUAL"));
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

        bb.append("$nin", ba.arr());
        bob.append(k, bb.obj());
        f = bob.obj();
      }
      else if (itemP->valueType == SfvtStringRange)
      {
        *errorStringP = "string range is not supported";
        LM_W(("KZ: %s", (*errorStringP).c_str()));
        return false;
      }
      else if (itemP->valueType == SfvtStringList)
      {
        for (unsigned int ix = 0; ix < itemP->stringList.size(); ++ix)
        {
          LM_W(("KZ: NOT EQUAL to %s", itemP->stringList[ix].c_str()));
          ba.append(itemP->stringList[ix]);
        }

        bb.append("$nin", ba.arr());
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
        *errorStringP = "null values not supported";
        LM_W(("KZ: %s", (*errorStringP).c_str()));
        return false;
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
      bb.append("$gt", itemP->numberValue);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopGreaterThanOrEqual:
      bb.append("$gte", itemP->numberValue);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopLessThan:
      bb.append("$lt", itemP->numberValue);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;

    case SfopLessThanOrEqual:
      bb.append("$lte", itemP->numberValue);
      bob.append(k, bb.obj());
      f = bob.obj();
      break;
    }

    mongoFilters.push_back(f);
  }

  LM_W(("KZ: pushed %d filters to mongoFilters vector", mongoFilters.size()));
  LM_W(("KZ: mongoFilter 0: %s", mongoFilters[0].toString().c_str()));
  return true;
}



/* ****************************************************************************
*
* StringFilter::match - 
*/
bool StringFilter::match(ContextElementResponse* cerP)
{
  LM_W(("KZ: In StringFilter::match"));

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    StringFilterItem* itemP = &filters[ix];

    LM_W(("KZ: StringFilter %d: op == %s", ix, itemP->opName()));
    // Unary operator?
    if ((itemP->op == SfopExists) || (itemP->op == SfopNotExists))
    {
      ContextAttribute* caP = cerP->contextElement.getAttribute(itemP->attributeName);

      LM_W(("KZ: attribute '%s' at %p", itemP->attributeName.c_str(), caP));
      if ((itemP->op == SfopExists) && (caP == NULL))
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      else if ((itemP->op == SfopNotExists) && (caP != NULL))
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      
      LM_W(("KZ: OK from a exist/not-exist point of view for attribute '%s'", itemP->attributeName.c_str()));
    }

    //
    // For binary operators, the left side is:
    //   - 'dateCreated'     (use the creation date of the contextElement)
    //   - 'dateModified'    (use the modification date of the contextElement)
    //   - attribute name    (use the value of the attribute)
    //
    ContextAttribute*  caP = NULL;
    ContextAttribute   ca;

    if ((itemP->left == DATE_CREATED) || (itemP->left == DATE_MODIFIED))
    {
      LM_W(("KZ: DATE_CREATED or DATE_MODIFIED"));
      caP            = &ca;
      ca.valueType   = orion::ValueTypeNumber;
      ca.numberValue = (itemP->left == DATE_CREATED)? cerP->contextElement.creDate : cerP->contextElement.modDate;
      LM_W(("KZ: ca.numberValue: %f", ca.numberValue));
    }
    else if (itemP->op != SfopNotExists)
    {
      caP = cerP->contextElement.getAttribute(itemP->left);

      // If the attribute doesn't exist, no need to go further: filter fails
      if (caP == NULL)
      {
        LM_W(("KZ: From StringFilter::match: false"));
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
      LM_W(("KZ: Equals"));
      if (itemP->matchEquals(caP) == false)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;

    case SfopDiffers:
      if (itemP->matchEquals(caP) == true)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;

    case SfopGreaterThan:
      if (itemP->matchGreaterThan(caP) == false)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;

    case SfopLessThan:
      if (itemP->matchLessThan(caP) == false)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;

    case SfopGreaterThanOrEqual:
      if (itemP->matchLessThan(caP) == true)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;

    case SfopLessThanOrEqual:
      if (itemP->matchGreaterThan(caP) == true)
      {
        LM_W(("KZ: From StringFilter::match: false"));
        return false;
      }
      break;
    }
  }

  LM_W(("KZ: From StringFilter::match: true"));
  return true;
}



/* ****************************************************************************
*
* StringFilter::present - 
*/
void StringFilter::present(std::string prefix)
{
  LM_W(("%sStringFilter '%s' [%d items]", prefix.c_str(), plainString.c_str(), filters.size()));

  for (unsigned int ix = 0; ix < filters.size(); ++ix)
  {
    filters[ix].present(prefix + "  ");
  }
}
