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
#include <string.h>
#include <string>
#include <sstream>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/RenderFormat.h"
#include "common/wsStrip.h"



/* ****************************************************************************
*
* renderFormatToString - 
*/
const char* renderFormatToString(RenderFormat format, bool noDefault, bool useLegacyWord)
{
  switch (format)
  {
  case NGSI_V1_LEGACY:                    return useLegacyWord ? "legacy" : "JSON";
  case NGSI_V2_NORMALIZED:                return "normalized";
  case NGSI_V2_KEYVALUES:                 return "keyValues";
  case NGSI_V2_VALUES:                    return "values";
  case NGSI_V2_UNIQUE_VALUES:             return "uniqueValues";
  case NGSI_V2_CUSTOM:                    return "custom";
  case NGSI_LD_V1_NORMALIZED:             return "normalized";
  case NGSI_LD_V1_KEYVALUES:              return "keyValues";
  case NGSI_LD_V1_V2_NORMALIZED:          return "x-ngsiv2-normalized";
  case NGSI_LD_V1_V2_KEYVALUES:           return "x-nsgiv2-keyValues";
  case NGSI_LD_V1_V2_NORMALIZED_COMPACT:  return "x-ngsiv2-normalized-compacted";
  case NGSI_LD_V1_V2_KEYVALUES_COMPACT:   return "x-nsgiv2-keyValues-compacted";
  case NO_FORMAT:
    if (noDefault == true)
    {
      return "no render format";
    }
    else
    {
      return useLegacyWord ? "legacy" : "JSON";
    }
  }

  return "Unknown render format";
}



/* ****************************************************************************
*
* stringToRenderFormat -
*/
RenderFormat stringToRenderFormat(const char* s, bool noDefault)
{
  if (strcmp(s, "JSON")         == 0) return NGSI_V1_LEGACY;          // DB content for NGSIv1 rendering due to legacy reasons
  if (strcmp(s, "legacy")       == 0) return NGSI_V1_LEGACY;
  if (strcmp(s, "normalized")   == 0) return NGSI_V2_NORMALIZED;
  if (strcmp(s, "keyValues")    == 0) return NGSI_V2_KEYVALUES;
  if (strcmp(s, "values")       == 0) return NGSI_V2_VALUES;
  if (strcmp(s, "uniqueValues") == 0) return NGSI_V2_UNIQUE_VALUES;
  if (strcmp(s, "custom")       == 0) return NGSI_V2_CUSTOM;

  if (strcmp(s, "x-ngsiv2")                      == 0) return NGSI_LD_V1_V2_NORMALIZED_COMPACT;
  if (strcmp(s, "x-ngsiv2-normalized")           == 0) return NGSI_LD_V1_V2_NORMALIZED;
  if (strcmp(s, "x-ngsiv2-keyValues")            == 0) return NGSI_LD_V1_V2_KEYVALUES;
  if (strcmp(s, "x-ngsiv2-normalized-compacted") == 0) return NGSI_LD_V1_V2_NORMALIZED_COMPACT;
  if (strcmp(s, "x-ngsiv2-keyValues-compacted")  == 0) return NGSI_LD_V1_V2_KEYVALUES_COMPACT;

  return (noDefault == false)? DEFAULT_RENDER_FORMAT : NO_FORMAT;
}
