#ifndef SRC_LIB_COMMON_RENDERFORMAT_H_
#define SRC_LIB_COMMON_RENDERFORMAT_H_

/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string>



/* ****************************************************************************
*
* DEFAULT_RENDER_FORMAT - 
*/
#define DEFAULT_RENDER_FORMAT         RF_NORMALIZED



/* ****************************************************************************
*
* RenderFormat - 
*/
typedef enum RenderFormat
{
  RF_NONE                           = 0,   // Invalid
  RF_LEGACY                         = 1,   // NGSIv1 only
  RF_NORMALIZED                     = 2,   // DEFAULT
  RF_CONCISE                        = 3,   // NGSI-LD Only - Concise format - as compact as possible while still lossless
  RF_KEYVALUES                      = 4,   // keyValues/Simplified
  RF_VALUES                         = 5,   // NGSIv2 only  - just the value, not even the key
  RF_UNIQUE_VALUES                  = 6,   // NGSIv2 only  - just the value, not even the key, and no repetition of values
  RF_CUSTOM                         = 7,   // NGSIv2 only  - custom format
  RF_CROSS_APIS_NORMALIZED          = 12,  // NGSI-LD Only - x-ngsiv2-normalized
  RF_CROSS_APIS_KEYVALUES           = 13,  // NGSI-LD Only - x-ngsiv2-keyValues
  RF_CROSS_APIS_NORMALIZED_COMPACT  = 22,  // NGSI-LD Only - x-ngsiv2-normalized-compacted
  RF_CROSS_APIS_KEYVALUES_COMPACT   = 23   // NGSI-LD Only - x-ngsiv2-keyValues-compacted
} RenderFormat;



/* ****************************************************************************
*
* renderFormatToString - 
*/
extern const char* renderFormatToString(RenderFormat format, bool noDefault = true, bool userLegacyWord = false);



/* ****************************************************************************
*
* stringToRenderFormat
*/
extern RenderFormat stringToRenderFormat(const char* s, bool noDefault = false);

#endif  // SRC_LIB_COMMON_RENDERFORMAT_H_
