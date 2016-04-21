#ifndef SRC_LIB_COMMON_TAG_H_
#define SRC_LIB_COMMON_TAG_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/Format.h"

/* FIXME P2: this file (along with some other places around all the code) uses
 * the old term "tag", coming from the XML days. Now we only support JSON and
 * all the terminology should use "key" or "keyName". We have changed terminology
 * in many places, but there are yet some remains (not so important, anyway).
 */


/* ****************************************************************************
*
* Macros for JSON rendering
*/
#define JSON_STR(value)                std::string("\"" + std::string(value) + "\"")
#define JSON_NUMBER(value)             std::string(value)
#define JSON_BOOL(bvalue)              std::string((bvalue == true)? "true" : "false") 

#define JSON_PROP(name)                std::string("\"" + std::string(name) + "\":")
#define JSON_VALUE(name, value)        std::string(JSON_PROP(name) + JSON_STR(value))
#define JSON_VALUE_NUMBER(name, value) std::string(JSON_PROP(name) + JSON_NUMBER(value))
#define JSON_VALUE_BOOL(name, value)   std::string(JSON_PROP(name) + ((value == true)? "true" : "false"))



/* ****************************************************************************
*
* htmlEscape - 
*/
extern char* htmlEscape(const char* s);

/* ****************************************************************************
*
* jsonInvalidCharsTransformation -
*
* FIXME P5: this is a quick fix for #1172. A better fix should be developed.
*/
extern std::string jsonInvalidCharsTransformation(const std::string& input);



/* ****************************************************************************
*
* startTag -  
*
* FIXME P3: in the past, this entire family of functions was named just startTag().
* However, changes in the function signature due to XML format removal have caused
* that different startTag() instances end with the same (or conflicting, due to
* optional parameters) signatures. Thus, we use startTag1() and startTag2() to
* distinguish. I know they are awful names :), we should find better ones, but given
* this is NGSIv1 functionality that will be deprecated in the future, the priority
* to do this is low.
*/
extern std::string startTag1
(
  const std::string&  indent,
  const std::string&  key,
  bool                showKey    = true,
  bool                isToplevel = false
);

extern std::string startTag2
(
  const std::string&  indent,
  const std::string&  key,
  bool                isVector         = false,
  bool                showKey          = true
);



/* ****************************************************************************
*
* endTag -  
*/
extern std::string endTag
(
  const std::string&  indent,
  bool                comma      = false,
  bool                isVector   = false,
  bool                nl         = true,
  bool                isToplevel = false
);



/* ****************************************************************************
*
* valueTag -  
*
* FIXME P3: in the past, this entire family of functions was named just valueTag().
* However, changes in the function signature due to XML format removal have caused
* that different valueTag() instances end with the same (or conflicting, due to
* optional parameters) signatures. Thus, we use valueTag() and valueTag() to
* distinguish. I know they are awful names :), we should find better ones, but given
* this is NGSIv1 functionality that will be deprecated in the future, the priority
* to do this is low.
*/
extern std::string valueTag1
(
  const std::string&  indent,
  const std::string&  key,
  const std::string&  value,
  bool                showComma           = false,
  bool                isVectorElement     = false,
  bool                withoutQuotes       = false
);

extern std::string valueTag
(
  const std::string&  indent,
  const std::string&  key,
  int                 value,
  bool                showComma     = false
);

extern std::string valueTag2
(
  const std::string&  indent,
  const std::string&  key,
  const std::string&  value,
  bool                showComma     = false,
  bool                withoutQuotes = false
);



/* ****************************************************************************
*
* startArray -
*/
extern std::string startArray
(
  const std::string&  indent,
  const std::string&  key,
  bool                showKey = true
);



/* ****************************************************************************
*
* endArray -
*/
extern std::string endArray(const std::string& indent, const std::string& key);

#endif  // SRC_LIB_COMMON_TAG_H_
