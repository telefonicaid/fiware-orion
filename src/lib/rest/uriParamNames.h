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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/



/* ****************************************************************************
*
* Names of the URI parameters
*/
#define URI_PARAM_NOTIFY_FORMAT       "notifyFormat"
#define URI_PARAM_PAGINATION_OFFSET   "offset"
#define URI_PARAM_PAGINATION_LIMIT    "limit"
#define URI_PARAM_PAGINATION_DETAILS  "details"
#define URI_PARAM_COLLAPSE            "collapse"
#define URI_PARAM_TYPE                "entity::type"
#define URI_PARAM_NOT_EXIST           "!exist"



/* ****************************************************************************
*
* Values for URI parameters
*/
#define DEFAULT_PAGINATION_OFFSET       "0"
#define DEFAULT_PAGINATION_OFFSET_INT   0

#define DEFAULT_PAGINATION_LIMIT        "20"
#define DEFAULT_PAGINATION_LIMIT_INT    20
#define MAX_PAGINATION_LIMIT            "1000"

#define DEFAULT_PARAM_NOTIFY_FORMAT     ""
#define DEFAULT_PAGINATION_DETAILS      "off"

#endif
