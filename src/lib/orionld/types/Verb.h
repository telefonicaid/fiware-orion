#ifndef SRC_LIB_ORIONLD_TYPES_VERB_H_
#define SRC_LIB_ORIONLD_TYPES_VERB_H_

/*
*
* Copyright 2023 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/



// -----------------------------------------------------------------------------
//
// Verb -
//
typedef enum Verb
{
  HTTP_GET,
  HTTP_PUT,
  HTTP_POST,
  HTTP_DELETE,
  HTTP_PATCH,
  HTTP_HEAD,
  HTTP_OPTIONS,
  HTTP_TRACE,
  HTTP_CONNECT,
  HTTP_NOVERB,
  HTTP_UNKNOWNVERB
} Verb;



// -----------------------------------------------------------------------------
//
// verbToString -
//
const char* verbToString(Verb verb);



// -----------------------------------------------------------------------------
//
// verbFromString -
//
Verb verbFromString(const char* verb);

#endif  // SRC_LIB_ORIONLD_TYPES_VERB_H_
