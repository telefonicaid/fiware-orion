/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include "orionld/rest/OrionLdRestService.h"                         // ORIONLD_URIPARAM_*
#include "orionld/rest/uriParamName.h"                               // Own interface



// -----------------------------------------------------------------------------
//
// uriParamName -
//
const char* uriParamName(uint32_t bit)
{
  switch (bit)
  {
  case ORIONLD_URIPARAM_OPTIONS:             return "options";
  case ORIONLD_URIPARAM_LIMIT:               return "limit";
  case ORIONLD_URIPARAM_OFFSET:              return "offset";
  case ORIONLD_URIPARAM_COUNT:               return "count";
  case ORIONLD_URIPARAM_IDLIST:              return "id";
  case ORIONLD_URIPARAM_TYPELIST:            return "type";
  case ORIONLD_URIPARAM_IDPATTERN:           return "idPattern";
  case ORIONLD_URIPARAM_ATTRS:               return "attrs";
  case ORIONLD_URIPARAM_Q:                   return "q";
  case ORIONLD_URIPARAM_GEOREL:              return "georel";
  case ORIONLD_URIPARAM_GEOMETRY:            return "geometry";
  case ORIONLD_URIPARAM_COORDINATES:         return "coordinates";
  case ORIONLD_URIPARAM_GEOPROPERTY:         return "geoproperty";
  case ORIONLD_URIPARAM_GEOMETRYPROPERTY:    return "geometryProperty";
  case ORIONLD_URIPARAM_CSF:                 return "csf";
  case ORIONLD_URIPARAM_DATASETID:           return "datasetId";
  case ORIONLD_URIPARAM_TIMEPROPERTY:        return "timeproperty";
  case ORIONLD_URIPARAM_TIMEREL:             return "timerel";
  case ORIONLD_URIPARAM_TIMEAT:              return "timeAt";
  case ORIONLD_URIPARAM_ENDTIMEAT:           return "endTimeAt";
  case ORIONLD_URIPARAM_DETAILS:             return "details";
  }

  return "unknown URI parameter";
}
