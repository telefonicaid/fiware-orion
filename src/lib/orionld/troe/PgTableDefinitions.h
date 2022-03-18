#ifndef SRC_LIB_ORIONLD_TROE_PGTABLEDEFINITIONS_H_
#define SRC_LIB_ORIONLD_TROE_PGTABLEDEFINITIONS_H_

/*
*
* Copyright 2021 FIWARE Foundation e.V.
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



// ----------------------------------------------------------------------------
//
// PG_ENTITY_INSERT_START
//
#define PG_ENTITY_INSERT_START "INSERT INTO entities("      \
  "instanceId,"                                             \
  "ts,"                                                     \
  "opMode,"                                                 \
  "id,"                                                     \
  "type) VALUES "                                           \



// ----------------------------------------------------------------------------
//
// PG_ATTRIBUTE_INSERT_START
//
#define PG_ATTRIBUTE_INSERT_START "INSERT INTO attributes(" \
  "instanceId,"                                             \
  "id,"                                                     \
  "opMode,"                                                 \
  "entityId,"                                               \
  "observedAt,"                                             \
  "subProperties,"                                          \
  "unitCode,"                                               \
  "datasetId,"                                              \
  "valueType,"                                              \
  "text,"                                                   \
  "boolean,"                                                \
  "number,"                                                 \
  "datetime,"                                               \
  "compound,"                                               \
  "geoPoint,"                                               \
  "geoMultiPoint,"                                          \
  "geoPolygon,"                                             \
  "geoMultiPolygon,"                                        \
  "geoLineString,"                                          \
  "geoMultiLineString,"                                     \
  "ts) VALUES "



// ----------------------------------------------------------------------------
//
// PG_SUB_ATTRIBUTE_INSERT_START
//
#define PG_SUB_ATTRIBUTE_INSERT_START "INSERT INTO subAttributes(" \
  "instanceId,"                                                    \
  "id,"                                                            \
  "entityId,"                                                      \
  "attrInstanceId,"                                                \
  "attrDatasetId,"                                                 \
  "observedAt,"                                                    \
  "unitCode,"                                                      \
  "valueType,"                                                     \
  "text,"                                                          \
  "boolean,"                                                       \
  "number,"                                                        \
  "datetime,"                                                      \
  "compound,"                                                      \
  "geoPoint,"                                                      \
  "geoMultiPoint,"                                                 \
  "geoPolygon,"                                                    \
  "geoMultiPolygon,"                                               \
  "geoLineString,"                                                 \
  "geoMultiLineString,"                                            \
  "ts) VALUES "

#endif  // SRC_LIB_ORIONLD_TROE_PGTABLEDEFINITIONS_H_
