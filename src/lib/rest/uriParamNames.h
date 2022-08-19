#ifndef URI_PARAM_NAMES_H
#define URI_PARAM_NAMES_H

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
#include "ngsi/Scope.h"    // SCOPE_VALUE_ENTITY_TYPE: "entity::type"



/* ****************************************************************************
*
* Names of URI parameters
*/
#define URI_PARAM_PAGINATION_OFFSET       "offset"
#define URI_PARAM_PAGINATION_LIMIT        "limit"
#define URI_PARAM_PAGINATION_DETAILS      "details"
#define URI_PARAM_COLLAPSE                "collapse"
#define URI_PARAM_ENTITY_TYPE             SCOPE_VALUE_ENTITY_TYPE
#define URI_PARAM_NOT_EXIST               "!exist"
#define URI_PARAM_EXIST                   "exist"
#define URI_PARAM_ATTRIBUTES_FORMAT       "attributesFormat"
#define URI_PARAM_ATTRIBUTE_FORMAT        "attributeFormat"
#define URI_PARAM_OPTIONS                 "options"
#define URI_PARAM_TYPE                    "type"
#define URI_PARAM_SORTED                  "orderBy"
#define URI_PARAM_Q                       "q"
#define URI_PARAM_MQ                      "mq"
#define URI_PARAM_METADATA                "metadata"
#define URI_PARAM_ATTRS                   "attrs"


// URI parameters for 'admin' requests
#define URI_PARAM_LEVEL                   "level"



/* ****************************************************************************
*
* Values for URI parameters
*/
#define DEFAULT_PAGINATION_OFFSET       "0"
#define DEFAULT_PAGINATION_OFFSET_INT   0

#define DEFAULT_PAGINATION_DETAILS      "off"

#endif
