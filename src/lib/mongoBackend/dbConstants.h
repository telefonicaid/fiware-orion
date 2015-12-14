#ifndef SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_
#define SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_

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
* Author: Fermín Galán
*/

/*****************************************************************************
* Constant string for collections names */
#define COL_ENTITIES       "entities"
#define COL_REGISTRATIONS  "registrations"
#define COL_CSUBS          "csubs"
#define COL_CASUBS         "casubs"

/*****************************************************************************
* Constant string for field names in collection (first characters
* are the code name: REG_, ENT_, ASUB_, CSUB */

#define REG_FWS_REGID               "fwdRegId"
#define REG_CONTEXT_REGISTRATION    "contextRegistration"
#define REG_PROVIDING_APPLICATION   "providingApplication"
#define REG_ENTITIES                "entities"
#define REG_ATTRS                   "attrs"
#define REG_EXPIRATION              "expiration"
#define REG_ENTITY_ID               "id"
#define REG_ENTITY_TYPE             "type"
#define REG_ATTRS_NAME              "name"
#define REG_ATTRS_TYPE              "type"
#define REG_ATTRS_ISDOMAIN          "isDomain"
#define REG_SERVICE_PATH            "servicePath"
#define REG_FORMAT                  "format"

#define ENT_ATTRS                    "attrs"
#define ENT_ATTRNAMES                "attrNames"
#define ENT_ENTITY_ID                "id"
#define ENT_ENTITY_TYPE              "type"
#define ENT_SERVICE_PATH             "servicePath"
#define ENT_ATTRS_TYPE               "type"
#define ENT_ATTRS_VALUE              "value"
#define ENT_ATTRS_CREATION_DATE      "creDate"
#define ENT_ATTRS_MODIFICATION_DATE  "modDate"
#define ENT_ATTRS_MD                 "md"
#define ENT_ATTRS_MD_NAME            "name"
#define ENT_ATTRS_MD_TYPE            "type"
#define ENT_ATTRS_MD_VALUE           "value"
#define ENT_CREATION_DATE            "creDate"
#define ENT_MODIFICATION_DATE        "modDate"
#define ENT_LOCATION                 "location"
#define ENT_LOCATION_ATTRNAME        "attrName"
#define ENT_LOCATION_COORDS          "coords"

#define CSUB_EXPIRATION         "expiration"
#define CSUB_LASTNOTIFICATION   "lastNotification"
#define CSUB_REFERENCE          "reference"
#define CSUB_CONDITIONS         "conditions"
#define CSUB_CONDITIONS_TYPE    "type"
#define CSUB_CONDITIONS_VALUE   "value"
#define CSUB_CONDITIONS_EXPR   "expression"
#define CSUB_CONDITIONS_Q       "q"
#define CSUB_CONDITIONS_GEO     "geometry"
#define CSUB_CONDITIONS_COORDS  "coords"

#define CSUB_THROTTLING         "throttling"
#define CSUB_ENTITIES           "entities"
#define CSUB_ATTRS              "attrs"
#define CSUB_ENTITY_ID          "id"
#define CSUB_ENTITY_TYPE        "type"
#define CSUB_ENTITY_ISPATTERN   "isPattern"
#define CSUB_COUNT              "count"
#define CSUB_FORMAT             "format"
#define CSUB_SERVICE_PATH       "servicePath"

#define CASUB_EXPIRATION        "expiration"
#define CASUB_REFERENCE         "reference"
#define CASUB_ENTITIES          "entities"
#define CASUB_ATTRS             "attrs"
#define CASUB_ENTITY_ID         "id"
#define CASUB_ENTITY_TYPE       "type"
#define CASUB_ENTITY_ISPATTERN  "isPattern"
#define CASUB_LASTNOTIFICATION  "lastNotification"
#define CASUB_COUNT             "count"
#define CASUB_FORMAT            "format"

#define EARTH_RADIUS_METERS     6371000

#define LOCATION_WGS84          "WGS84"
#define LOCATION_WGS84_LEGACY   "WSG84"    /* We fixed the right string at 0.17.0, but the old one needs to be mantained */


#endif // SRC_LIB_MONGOBACKEND_DBCONSTANTS_H_

