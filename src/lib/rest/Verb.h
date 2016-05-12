#ifndef VERB_H
#define VERB_H

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



/* ****************************************************************************
*
* Verb - 
*   The list ist of verbs taken from https://tools.ietf.org/html/rfc7231, section 4.3
*   and PATCH was added to that list (RFC 5789).
*/
typedef enum Verb
{
  GET,
  PUT,
  POST,
  DELETE,
  PATCH,
  HEAD,
  OPTIONS,
  TRACE,
  CONNECT,
  NOVERB
} Verb;



/* ****************************************************************************
*
* verbName - 
*/
extern const char* verbName(Verb verb);



/* ****************************************************************************
*
* stringToVerb - 
*/
extern Verb stringToVerb(const std::string& str);

#endif
