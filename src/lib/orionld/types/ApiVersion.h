#ifndef SRC_LIB_ORIONLD_TYPES_APIVERSION_H_
#define SRC_LIB_ORIONLD_TYPES_APIVERSION_H_

/*
*
* Copyright 2024 FIWARE Foundation e.V.
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



/* ****************************************************************************
*
*  API version -
*/
typedef enum ApiVersion
{
  API_VERSION_NONE       = -1,
  API_VERSION_ADMIN      =  0,
  API_VERSION_NGSI_V1    =  1,
  API_VERSION_NGSI_V2    =  2,
  API_VERSION_NGSILD_V1  =  3
} ApiVersion;

#endif  // SRC_LIB_ORIONLD_TYPES_APIVERSION_H_
