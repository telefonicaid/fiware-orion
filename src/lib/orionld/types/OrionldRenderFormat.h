#ifndef SRC_LIB_ORIONLD_TYPES_ORIONLDRENDERFORMAT_H_
#define SRC_LIB_ORIONLD_TYPES_ORIONLDRENDERFORMAT_H_

/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
* OrionldRenderFormat -
*/
typedef enum OrionldRenderFormat
{
  RF_NONE                           = 0,   // Invalid
  RF_LEGACY                         = 1,   // NGSIv1 only
  RF_NORMALIZED                     = 2,   // DEFAULT
  RF_CONCISE                        = 3,   // NGSI-LD Only - Concise format - as compact as possible while still lossless
  RF_SIMPLIFIED                     = 4,   // simplified/Simplified
  RF_KEYVALUES                      = 4,   // keyValues (same as Simplified)
  RF_VALUES                         = 5,   // NGSIv2 only  - just the value, not even the key
  RF_UNIQUE_VALUES                  = 6,   // NGSIv2 only  - just the value, not even the key, and no repetition of values
  RF_CUSTOM                         = 7,   // NGSIv2 only  - custom format
  RF_CROSS_APIS_NORMALIZED          = 12,  // NGSI-LD Only - x-ngsiv2-normalized
  RF_CROSS_APIS_SIMPLIFIED          = 13,  // NGSI-LD Only - x-ngsiv2-simplified
  RF_CROSS_APIS_CONCISE             = 14,  // NGSI-LD Only - x-ngsiv2-concise
  RF_CROSS_APIS_NORMALIZED_COMPACT  = 22,  // NGSI-LD Only - x-ngsiv2-normalized-compacted
  RF_CROSS_APIS_SIMPLIFIED_COMPACT  = 23,  // NGSI-LD Only - x-ngsiv2-simplified-compacted
  RF_CROSS_APIS_CONCISE_COMPACT     = 24   // NGSI-LD Only - x-ngsiv2-concise-compacted
} OrionldRenderFormat;



// -----------------------------------------------------------------------------
//
// RF_DEFAULT - default render format
//
#define RF_DEFAULT RF_NORMALIZED



// -----------------------------------------------------------------------------
//
// renderFormat -
//
extern const char* renderFormat(OrionldRenderFormat renderFormat);



// -----------------------------------------------------------------------------
//
// renderFormatFromString -
//
extern OrionldRenderFormat renderFormat(const char* renderFormat);

#endif  // SRC_LIB_ORIONLD_TYPES_ORIONLDRENDERFORMAT_H_
