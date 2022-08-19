#ifndef SRC_LIB_PARSE_FORBIDDENCHARS_H_
#define SRC_LIB_PARSE_FORBIDDENCHARS_H_

/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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



/* ****************************************************************************
*
* forbiddenChars - 
*/
extern bool forbiddenChars(const char* s, const char* exceptions = NULL);



/* ****************************************************************************
*
* forbiddenIdChars -
*/
extern bool forbiddenIdChars(int api, const char* s, const char* exceptions = NULL);



/* ****************************************************************************
*
* forbiddenIdCharsV2 -
*/
extern bool forbiddenIdCharsV2(const char* s, const char* exceptions = NULL);



/* ****************************************************************************
*
* forbiddenQuotes - any unauthorized quotes?
*/
extern bool forbiddenQuotes(char* s);



/* ****************************************************************************
*
* forbiddenMqttTopic -
*/
extern bool forbiddenMqttTopic(const char* s);

#endif  // SRC_LIB_PARSE_FORBIDDENCHARS_H_
