#ifndef SRC_LIB_COMMON_ERRORMESSAGES_H_
#define SRC_LIB_COMMON_ERRORMESSAGES_H_

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
* Author: Orion dev team
*/
#include "common/limits.h"



/* ****************************************************************************
*
* Auxiliar macros for stringification of integers.
* See http://stackoverflow.com/questions/5459868/c-preprocessor-concatenate-int-to-string
*/
#define STR2(x) #x
#define STR(x)  STR2(x)

/* FIXME P5: the current set of defines in this file doesn't cover all the possible cases we have in the code.
 * The unhardwiring work should continue, specially with regards to BadRequest errors (we have *a lot* of them
 * in the code.
 *
 * In addition, by the moment we are using this constants only for error payloads, but maybe they should be
 * also used for alarm error mensajes, e.g. alarmMgr.badInput(clientIp, ERROR_DESC_PARSE) instead of
 * alarmMgr.badInput(clientIp, "JSON parse error").
 */

#define ERROR_PARSE                                   "ParseError"
#define ERROR_DESC_PARSE                              "Errors found in incoming JSON buffer"

#define ERROR_BAD_REQUEST                             "BadRequest"
#define ERROR_DESC_BAD_REQUEST_INVALID_CHAR_URI       "invalid character in URI"
#define ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_ID        "entity id length: 0, min length supported: "      STR(MIN_ID_LEN)
#define ERROR_DESC_BAD_REQUEST_EMPTY_ATTR_NAME        "attribute name length: 0, min length supported: " STR(MIN_ID_LEN)
#define ERROR_DESC_BAD_REQUEST_EMPTY_ENTITY_TYPE      "entity type length: 0, min length supported: "    STR(MIN_ID_LEN)
#define ERROR_DESC_BAD_REQUEST_EMPTY_PAYLOAD          "empty payload"
#define ERROR_DESC_BAD_REQUEST_EMPTY_ENTITIES_VECTOR  "empty entities vector"
#define ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTID     "Invalid characters in entity id"
#define ERROR_DESC_BAD_REQUEST_INVALID_CHAR_ENTTYPE   "Invalid characters in entity type"
#define ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTID    "Invalid JSON type for entity id"
#define ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPE  "Invalid JSON type for entity type"
#define ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_SCOPE    "invalid JSON type for scope value: must be string"
#define ERROR_DESC_BAD_REQUEST_INVALID_ATTRSFORMAT    "invalid attrsFormat, accepted values: legacy, normalized, keyValues, values"
#define ERROR_DESC_BAD_REQUEST_INVALID_STATUS         "status is not valid: it has to be either active or inactive"
#define ERROR_DESC_BAD_REQUEST_INVALID_RANGE          "ranges only valid for equal and not equal ops"
#define ERROR_DESC_BAD_REQUEST_INVALID_LIST           "lists only valid for equal and not equal ops"
#define ERROR_DESC_BAD_REQUEST_PARTIAL_GEOEXPRESSION  "partial geo expression: geometry, georel and coords have to be provided together"

#define ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTIDPATTERN     "Invalid JSON type for entity idPattern"
#define ERROR_DESC_BAD_REQUEST_INVALID_JTYPE_ENTTYPEPATTERN   "Invalid JSON type for entity typePattern"
#define ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTIDPATTERN     "Invalid regex for entity idPattern"
#define ERROR_DESC_BAD_REQUEST_INVALID_REGEX_ENTTYPEPATTERN   "Invalid regex for entity typePattern"
#define ERROR_DESC_BAD_REQUEST_EMPTY_ENTTYPE                  "entity type length: 0, min length supported: 1"
#define ERROR_DESC_BAD_REQUEST_ID_AS_ATTR                     "invalid input, 'id' as attribute"
#define ERROR_DESC_BAD_REQUEST_ENTID_IN_PAYLOAD               "entity id specified in payload"
#define ERROR_DESC_BAD_REQUEST_ENTTYPE_IN_PAYLOAD             "entity type specified in payload"
#define ERROR_DESC_BAD_REQUEST_NOT_A_JSON_OBJECT              "not a JSON object"

#define ERROR_NOT_FOUND                               "NotFound"
#define ERROR_DESC_NOT_FOUND_ENTITY                   "The requested entity has not been found. Check type and id"
#define ERROR_DESC_NOT_FOUND_ENTITY_TYPE              "Entity type not found"
#define ERROR_DESC_NOT_FOUND_CONTEXT_ELEMENT          "No context element found"
#define ERROR_DESC_NOT_FOUND_ATTRIBUTE                "The entity does not have such an attribute"
#define ERROR_DESC_NOT_FOUND_SUBSCRIPTION             "The requested subscription has not been found. Check id"
#define ERROR_DESC_NOT_FOUND_REGISTRATION             "The requested registration has not been found. Check id"

#define ERROR_TOO_MANY                                "TooManyResults"
#define ERROR_DESC_TOO_MANY_ENTITIES                  "More than one matching entity. Please refine your query"

#define ERROR_DESC_BAD_VERB                           "method not allowed"

#endif  // SRC_LIB_COMMON_ERRORMESSAGES_H_
