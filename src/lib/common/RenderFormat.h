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
#define DEFAULT_RENDER_FORMAT         NGSI_V2_NORMALIZED



/* ****************************************************************************
*
* RenderFormat - 
*/
typedef enum RenderFormat
{
  NO_FORMAT                         = 0,
  NGSI_V1_LEGACY                    = 1,
  NGSI_V2_NORMALIZED                = 2,
  NGSI_V2_KEYVALUES                 = 3,
  NGSI_V2_VALUES                    = 4,
  NGSI_V2_UNIQUE_VALUES             = 5,
  NGSI_V2_CUSTOM                    = 6,
  NGSI_LD_V1_NORMALIZED             = 10,  // normalized (default) - but inside an NGSI-LD subscription
  NGSI_LD_V1_KEYVALUES              = 11,  // keyValues - but inside an NGSI-LD subscription
  NGSI_LD_V1_V2_NORMALIZED          = 12,  // x-ngsiv2-normalized
  NGSI_LD_V1_V2_KEYVALUES           = 13,  // x-ngsiv2-keyValues
  NGSI_LD_V1_V2_NORMALIZED_COMPACT  = 14,  // x-ngsiv2-normalized-compacted
  NGSI_LD_V1_V2_KEYVALUES_COMPACT   = 15   // x-ngsiv2-keyValues-compacted
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
