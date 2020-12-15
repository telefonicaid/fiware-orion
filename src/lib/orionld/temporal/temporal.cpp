/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/temporal/temporal.h"                         // Own interface



// -----------------------------------------------------------------------------
//
// temporalMode -
//
const char* temporalMode(TemporalMode opMode)
{
  switch (opMode)
  {
  case TEMPORAL_ENTITY_CREATE:      return "Create";
  case TEMPORAL_ENTITY_UPDATE:      return "Update";
  case TEMPORAL_ENTITY_REPLACE:     return "Replace";
  case TEMPORAL_ENTITY_DELETE:      return "Delete";
  case TEMPORAL_ATTRIBUTE_APPEND:   return "Create";
  case TEMPORAL_ATTRIBUTE_UPDATE:   return "Update";
  case TEMPORAL_ATTRIBUTE_REPLACE:  return "Replace";
  case TEMPORAL_ATTRIBUTE_DELETE:   return "Delete";
  }

  LM_E(("Invalid TemporalMode: %d", opMode));
  return "INVALID";
}
