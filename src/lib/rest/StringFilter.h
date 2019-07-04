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

#include "parse/CompoundValueNode.h"



/* ****************************************************************************
*
* Declaring structs for pointer-usage, to avoid include the headers
*/
struct ContextAttribute;
struct Metadata;



/* ****************************************************************************
*
* StringFilterType - 
*/
typedef enum StringFilterType
{
  SftQ,
  SftMq
} StringFilterType;



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
* MatchResult - see issue 2995 for an explanation of these four values
*/
typedef enum MatchResult
{
  MrDoesntExist = 0,   // E.g. q=a>2 if attribute 'a' doesn't exist in DB
  MrIncompatibleType,  // E.g. q=a>2 if attribute 'a' exists in the DB but is a boolean (can't be compared with the number 2)
  MrNoMatch,           // E.g. q=a>2 if attribute 'a' exists in the DB and is a Number but with a value of 1
  MrMatch              // E.g. q=a>2 if attribute 'a' exists in the DB and is a Number AND with a value of 3 (greater than 2)
} MatchResult;



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
*   attributeName        The name of the attribute, used for unary operators and for mq filters
*   metadataName         The name of the metadata, used for mqfilters
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
*   matchEquals          returns MrMatch if '== comparison' gives a match
*   matchPattern         returns MrMatch if '~= comparison' gives a match
*   matchGreaterThan     returns MrMatch if '> comparison' gives a match
*                        Note that '<= comparisons' use matchGreaterThan() != MrNoMatch
*   matchLessThan        returns MrMatch if '< comparison' gives a match
*                        Note that '>= comparisons' use matchLessThan() != MrNoMatch
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
  std::string               attributeName;  // Used for unary operators and for metadata filters
  std::string               metadataName;   // Used for metadata filters
  std::string               compoundPath;
  bool                      compiledPattern;
  StringFilterType          type;

  StringFilterItem();
  ~StringFilterItem();

  bool                      parse(char* qItem, std::string* errorStringP, StringFilterType _type);
#ifdef ORIONLD
  int                       render(char* buf, int bufLen);
  void                      valueAsString(char* buf, int bufLen);
#endif
  void                      lhsParse(void);
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
  MatchResult               matchEquals(ContextAttribute* caP);
  MatchResult               matchEquals(Metadata* mdP);
  MatchResult               matchEquals(orion::CompoundValueNode* cvP);
  MatchResult               matchPattern(ContextAttribute* caP);
  MatchResult               matchPattern(Metadata* mdP);
  MatchResult               matchPattern(orion::CompoundValueNode* cvP);
  MatchResult               matchGreaterThan(ContextAttribute* caP);
  MatchResult               matchGreaterThan(Metadata* mdP);
  MatchResult               matchGreaterThan(orion::CompoundValueNode* cvP);
  MatchResult               matchLessThan(ContextAttribute* caP);
  MatchResult               matchLessThan(Metadata* mdP);
  MatchResult               matchLessThan(orion::CompoundValueNode* cvP);
  bool                      fill(StringFilterItem* sfiP, std::string* errorStringP);

private:
  bool                      compatibleType(ContextAttribute* caP);
  bool                      compatibleType(Metadata* mdP);
  bool                      compatibleType(orion::CompoundValueNode* cvP);
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
  std::vector<StringFilterItem*>  filters;
  std::vector<mongo::BSONObj>     mongoFilters;
  StringFilterType                type;

  StringFilter(StringFilterType _type);
  ~StringFilter();

  bool  parse(const char* q, std::string* errorStringP);
#ifdef ORIONLD
  bool  render(char* buf, int bufLen, std::string* errorStringP);
#endif
  bool  mongoFilterPopulate(std::string* errorStringP);
  bool  match(ContextElementResponse* cerP);
  bool  qMatch(ContextElementResponse* cerP);
  bool  mqMatch(ContextElementResponse* cerP);

  StringFilter*  clone(std::string* errorStringP);
  bool           fill(StringFilter* sfP, std::string* errorStringP);
};

#endif  // SRC_LIB_REST_STRINGFILTERS_H_
