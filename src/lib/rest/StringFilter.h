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
#include <regex.h>

#include "mongo/client/dbclient.h"

struct ContextAttribute;

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
  SfopLessThanOrEqual,     // <=
  SfopMatchPattern         // ~=
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
*
* FIELDS
*   left                 the left-hand-side of a filter-item
*   op                   the operator of the filter-item
*   valueType            the type of the right-hand-side
*   numberValue          if 'valueType' is 'numeric', the value is stored here
*   stringValue          if 'valueType' is 'string', the value isstored here
*   boolValue            if 'valueType' is 'boolean', the value is stored here
*   stringList           vector/list of string values
*   numberList           vector/list of numeric values
*   numberRangeFrom      lower limit for numeric ranges
*   numberRangeTo        upper limit for numeric ranges
*   stringRangeFrom      lower limit for string ranges
*   stringRangeTo        upper limit for string ranges
*   attributeName        The name of the attribute, used for unary operators only
*
* METHODS
*   parse                parse a string, like 'a>14' into a StringFilterItem
*   opName               help method to translate the 'op' field into a string
*   valueTypeName        help method to translate the 'valueType' field into a string
*   valueGet             extract the value of RHS (right hand side), including type, etc.
*   valueParse           extract the value of RHS and check its validity
*   rangeParse           parse a range 'xxx..yyy' and check its validity
*   listParse            parse a list 'a,b,c,d' and check its validity
*   listItemAdd          add an item to a list - used by listParse
*   matchEquals          returns true if '== comparison' with ContextAttribute gives a match
*   matchPattern         returns true if '~= comparison' with ContextAttribute gives a match
*   matchGreaterThan     returns true if '> comparison' with ContextAttribute gives a match
*                        Note that '<= comparisons' use !matchGreaterThan()
*   matchLessThan        returns true if '< comparison' with ContextAttribute gives a match
*                        Note that '>= comparisons' use !matchLessThan()
*/
class StringFilterItem
{
public:
  std::string               left;
  StringFilterOp            op;
  StringFilterValueType     valueType;
  double                    numberValue;
  std::string               stringValue;
  regex_t                   patternValue;
  bool                      boolValue;
  std::vector<std::string>  stringList;
  std::vector<double>       numberList;
  double                    numberRangeFrom;
  double                    numberRangeTo;
  std::string               stringRangeFrom;
  std::string               stringRangeTo;
  std::string               attributeName;  // Used for unary operators only

  bool                      compiledPattern;

  StringFilterItem();
  ~StringFilterItem();

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
  bool                      matchPattern(ContextAttribute* caP);
  bool                      matchGreaterThan(ContextAttribute* caP);
  bool                      matchLessThan(ContextAttribute* caP);
  bool                      fill(StringFilterItem* sfiP, std::string* errorStringP);

private:
  bool                      compatibleType(ContextAttribute* caP);
};



class ContextElementResponse;
/* ****************************************************************************
*
* StringFilter - 
*
* FIELDS
*   filters       vector of filter-items, see StringFilterItem comment header
*   mongoFilters  the filter-items translated to filters that can be used by mongo
*                 Note that the method 'mongoFilterPopulate' must be called to translate
*                 'filters' to 'mongoFilters'
*
* METHODS
*   parse                 parse the string filter string in 'q' and create the filters.
*                         This is the main method of StringFilter and before this method has been executed,
*                         the string filter is empty.
*                         THIS METHOD MUST BE USED BEFORE ANY OTHER.
*   mongoFilterPopulate   translate 'filters' into mongo-understandable 'mongoFilters'.
*                         Of course, parse() must be called before mongoFilterPopulate(), so that there are
*                         filters to translate.
*   match                 Compare a ContextElementResponse to the entire StringFilter and return TRUE if
*                         a match exists.
*                         It is an 'AND-match', so ALL StringFilterItems in 'filters' must match the ContextElementResponse
*                         in order for 'match' to return TRUE.
*                         Also here, parse() must be called before match() can be used.
*/
class StringFilter
{
public:
  std::vector<StringFilterItem>  filters;
  std::vector<BSONObj>           mongoFilters;

  StringFilter();
  ~StringFilter();

  bool  parse(const char* q, std::string* errorStringP);
  bool  mongoFilterPopulate(std::string* errorStringP);
  bool  match(ContextElementResponse* cerP);

  StringFilter*  clone(std::string* errorStringP);
  bool           fill(StringFilter* sfP, std::string* errorStringP);
};

#endif  // SRC_LIB_REST_STRINGFILTERS_H_
