#ifndef SRC_LIB_REST_HTTPHEADERS_H_
#define SRC_LIB_REST_HTTPHEADERS_H_

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



/* ****************************************************************************
*
* HTTP Headers -
*/
#define HTTP_ACCEPT                        "Accept"
#define HTTP_ALLOW                         "Allow"
#define HTTP_ACCESS_CONTROL_ALLOW_ORIGIN   "Access-Control-Allow-Origin"
#define HTTP_ACCESS_CONTROL_ALLOW_HEADERS  "Access-Control-Allow-Headers"
#define HTTP_ACCESS_CONTROL_ALLOW_METHODS  "Access-Control-Allow-Methods"
#define HTTP_ACCESS_CONTROL_MAX_AGE        "Access-Control-Max-Age"
#define HTTP_ACCESS_CONTROL_EXPOSE_HEADERS "Access-Control-Expose-Headers"
#define HTTP_CONNECTION                    "Connection"
#define HTTP_CONTENT_LENGTH                "Content-Length"
#define HTTP_CONTENT_TYPE                  "Content-Type"
#define HTTP_EXPECT                        "Expect"
#define HTTP_FIWARE_CORRELATOR             "Fiware-Correlator"
#define HTTP_FIWARE_SERVICE                "Fiware-Service"
#define HTTP_FIWARE_SERVICEPATH            "Fiware-Servicepath"
#define HTTP_FIWARE_TOTAL_COUNT            "Fiware-Total-Count"
#define HTTP_HOST                          "Host"
#define HTTP_NGSIV2_ATTRSFORMAT            "Ngsiv2-AttrsFormat"
#define HTTP_RESOURCE_LOCATION             "Location"
#define HTTP_LINK                          "Link"
#define HTTP_ORIGIN                        "Origin"
#define HTTP_USER_AGENT                    "User-Agent"
#define HTTP_X_AUTH_TOKEN                  "X-Auth-Token"
#define HTTP_X_REAL_IP                     "X-Real-IP"
#define HTTP_X_FORWARDED_FOR               "X-Forwarded-For"
#define HTTP_LINK                          "Link"



/* ****************************************************************************
*
* CORS Allowed Headers -
*/
#define CORS_ALLOWED_HEADERS HTTP_CONTENT_TYPE ", " HTTP_FIWARE_SERVICE ", " HTTP_FIWARE_SERVICEPATH ", " HTTP_NGSIV2_ATTRSFORMAT ", " HTTP_FIWARE_CORRELATOR ", " HTTP_X_FORWARDED_FOR ", " HTTP_X_REAL_IP ", " HTTP_X_AUTH_TOKEN



/* ****************************************************************************
*
* CORS Exposed Headers -
*/
#define CORS_EXPOSED_HEADERS HTTP_FIWARE_CORRELATOR ", " HTTP_FIWARE_TOTAL_COUNT ", " HTTP_RESOURCE_LOCATION

#endif  // SRC_LIB_REST_HTTPHEADERS_H_
