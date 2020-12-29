#ifndef SRC_LIB_ORIONLD_TEMPORAL_TEMPORAL_H_
#define SRC_LIB_ORIONLD_TEMPORAL_TEMPORAL_H_

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



// -----------------------------------------------------------------------------
//
// TemporalMode -
//
typedef enum TemporalMode
{
  TEMPORAL_ENTITY_CREATE,
  TEMPORAL_ENTITY_UPDATE,
  TEMPORAL_ENTITY_REPLACE,
  TEMPORAL_ENTITY_DELETE,
  TEMPORAL_ATTRIBUTE_APPEND,
  TEMPORAL_ATTRIBUTE_UPDATE,
  TEMPORAL_ATTRIBUTE_REPLACE,
  TEMPORAL_ATTRIBUTE_DELETE
} TemporalMode;



// -----------------------------------------------------------------------------
//
// temporalMode -
//
extern const char* temporalMode(TemporalMode opMode);

#endif  // SRC_LIB_ORIONLD_TEMPORAL_TEMPORAL_H_
