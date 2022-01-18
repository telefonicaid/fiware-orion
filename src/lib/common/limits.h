#ifndef SRC_LIB_COMMON_LIMITS_H_
#define SRC_LIB_COMMON_LIMITS_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Service/Tenant definitions -
*/
#define SERVICE_NAME_MAX_LEN            50
#define SERVICE_NAME_MAX_LEN_STRING    "50"



/* ****************************************************************************
*
* Service Path definitions -
*
* The formula for SERVICE_PATH_MAX_TOTAL needs a little explanation. Considering
*
* Fiware-ServicePath: /A/B, /C/D
*
* The +1 if for the '/' in each level, the +2 is the extra characters assuming that the
* separation between tokens is ', ' (this is just an heuristic, as HTTP header could include
* more whitespace, but probably the final value is so large that it suffices most of the
* cases and don't mind in the case of truncated very long service path, from a logs point
* of view).
*
*/
#define SERVICE_PATH_MAX_COMPONENTS       10
#define SERVICE_PATH_MAX_LEVELS           10
#define SERVICE_PATH_MAX_COMPONENT_LEN    50
#define SERVICE_PATH_MAX_TOTAL            (((SERVICE_PATH_MAX_COMPONENT_LEN + 1) * SERVICE_PATH_MAX_LEVELS) + 2) * SERVICE_PATH_MAX_COMPONENTS



/* ****************************************************************************
*
* Others -
*/
#define IP_LENGTH_MAX           64     // Must put *some* limit on the hostname ...
#define STRING_SIZE_FOR_INT     16     // Room enough for a 32 bit integer
#define STRING_SIZE_FOR_LONG    20     // Room enough for a 64 bit integer
#define STRING_SIZE_FOR_DOUBLE  64     // Room enough for a double
#define CORRELATOR_ID_SIZE      36     // Max size of a UUIDv4 string
#define MAX_PORT                65535  // Max port number (== 0xFFFF)



/* *****************************************************************************
*
* DB_AND_SERVICE_NAME_MAX_LEN -
*
* Max name length of a database name for a tenant:
* Remember the database name is created like this:
*   orion-<tenant>
*
* 'orion' is the default database name prefix, but this
* string is configurable via CLI.
* However, its maximum allowed length is SERVICE_NAME_MAX_LEN, so
* this we can always use.
*
* Now:
*   DB_NAME_MAX_LEN + 1 + SERVICE_NAME_MAX_LEN + 1:
*
*     DB_NAME_MAX_LEN:      'orion'
*     1:                    '-'
*     SERVICE_NAME_MAX_LEN: tenant name, via HTTP header 'Fiware-Service'
*     1:                    zero-termination of the string
*/
#define DB_AND_SERVICE_NAME_MAX_LEN     (DB_NAME_MAX_LEN + 1 + SERVICE_NAME_MAX_LEN + 1)



/* ****************************************************************************
*
* DEFAULT_IN_REQ_PAYLOAD_MAX_SIZE -
*/
#define DEFAULT_IN_REQ_PAYLOAD_MAX_SIZE (1 * 1024 * 1024)  // 1 MB default max size of incoming request payload (see CLI -inReqPayloadMaxSize)



/* ****************************************************************************
*
* DEFAULT_OUT_REQ_MSG_MAX_SIZE -
*/
#define DEFAULT_OUT_REQ_MSG_MAX_SIZE (8 * 1024 * 1024)  // 8 MB default max size of any outgoing request message (see CLI -outReqMsgMaxSize)



/* ****************************************************************************
*
* IP -
*/
#define  LOCAL_IP_V6  "::"
#define  LOCAL_IP_V4  "0.0.0.0"



/* ****************************************************************************
*
* STATIC_BUFFER_SIZE - to avoid mallocs for "smaller" requests
*/
#define STATIC_BUFFER_SIZE (32 * 1024) // 32 KB



/* ****************************************************************************
*
* HTTP header maximum lengths
*/
#define CURL_VERSION_MAX_LENGTH             128
#define HTTP_HEADER_USER_AGENT_MAX_LENGTH   256
#define HTTP_HEADER_HOST_MAX_LENGTH         256



/* ****************************************************************************
*
* Default timeout - 5000 milliseconds
*/
#define DEFAULT_TIMEOUT     5000



/* ****************************************************************************
*
* MAX_HTTP_TIMEOUT - 1800000 milliseconds, 30 minutes
*/
#define MAX_HTTP_TIMEOUT     1800000


/* ****************************************************************************
*
* Pagination definitions -
*/
#define MAX_PAGINATION_LIMIT            "1000"



/* ****************************************************************************
*
* MAX_LEN_IP -
*/
#define MAX_LEN_IP  64



/* ****************************************************************************
*
* Values for URI parameters
*/
#define DEFAULT_PAGINATION_LIMIT        "20"
#define DEFAULT_PAGINATION_LIMIT_INT     20



/* ****************************************************************************
*
* DB_NAME_MAX_LEN - max length of database name
*/
#define DB_NAME_MAX_LEN  10



/* ****************************************************************************
*
* MAX_ID_LEN - max length of id and type for entities, attributes and metadata
*/
#ifndef MAX_ID_LEN
#define MAX_ID_LEN 256
#endif



/* ****************************************************************************
*
* MIN_ID_LEN - min length of id and type for entities, attributes and metadata
*/
#ifndef MIN_ID_LEN
#define MIN_ID_LEN 1
#endif



/* ****************************************************************************
*
* MAX_DESCRIPTION_LENGTH -
*/
#define MAX_DESCRIPTION_LENGTH  1024



/* ****************************************************************************
*
* MAX_JSON_NESTING -
*
* 50 seems to be a reasonable limit (note that MongoDB sets this limit in 100
* see https://docs.mongodb.com/manual/reference/limits/#Nested-Depth-for-BSON-Documents
*/
#define MAX_JSON_NESTING  50



#endif  // SRC_LIB_COMMON_LIMITS_H_
