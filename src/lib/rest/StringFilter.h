#ifndef SRC_LIB_REST_STRINGFILTERS_H_
#define SRC_LIB_REST_STRINGFILTERS_H_

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

#include "ngsi/ContextAttribute.h"

using namespace mongo;



/* ****************************************************************************
*
* StringFilterOp - 
*/
typedef enum StringFilterOp
{
  SfopExists,              //
  SfopNotExists,           // !
  SfopEquals,              // ==
  SfopDiffers,             // !=
  SfopGreaterThan,         // >
  SfopGreaterThanOrEqual,  // >=
  SfopLessThan,            // <
  SfopLessThanOrEqual      // <=
} StringFilterOp;



/* ****************************************************************************
*
* StringFilterValueType - 
*/
typedef enum StringFilterValueType
{
  SfvtString,
  SfvtBool,
  SfvtNumber,
  SfvtNull,
  SfvtDate,
  SfvtNumberRange,
  SfvtDateRange,
  SfvtStringRange,
  SfvtNumberList,
  SfvtDateList,
  SfvtStringList
} StringFilterValueType;



/* ****************************************************************************
*
* StringFilterItem - 
*/
class StringFilterItem
{
public:
  std::string               left;
  StringFilterOp            op;
  StringFilterValueType     valueType;
  double                    numberValue;
  std::string               stringValue;
  bool                      boolValue;
  std::vector<std::string>  stringList;
  std::vector<double>       numberList;
  double                    numberRangeFrom;
  double                    numberRangeTo;
  std::string               stringRangeFrom;
  std::string               stringRangeTo;
  std::string               attributeName;  // Used for unary operators only

  bool                      parse(char* qItem, std::string* errorStringP);
  const char*               opName(void);
  const char*               valueTypeName(void);

  bool                      valueGet(char*                   s,
                                     StringFilterValueType*  valueTypeP,
                                     double*                 doubleP,
                                     std::string*            stringP,
                                     bool*                   boolP,
                                     std::string* errorStringP);

  bool                      valueParse(char* s, std::string* errorStringP);
  bool                      rangeParse(char* s, std::string* errorStringP);
  bool                      listParse(char* s, std::string* errorStringP);
  bool                      listItemAdd(char* s, std::string* errorStringP);
  bool                      matchEquals(ContextAttribute* caP);
  bool                      matchGreaterThan(ContextAttribute* caP);
  bool                      matchLessThan(ContextAttribute* caP);
};



class ContextElementResponse;
/* ****************************************************************************
*
* StringFilter - 
*/
class StringFilter
{
public:
  std::vector<StringFilterItem>  filters;
  std::vector<BSONObj>           mongoFilters;
  std::string                    plainString;

  bool  parse(const char* q, std::string* errorStringP);
  bool  mongoFilterPopulate(std::string* errorStringP);
  bool  match(ContextElementResponse* cerP);
};

#endif  // SRC_LIB_REST_STRINGFILTERS_H_
