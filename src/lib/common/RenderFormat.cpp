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

#include "orionld/types/OrionldRenderFormat.h"
#include "common/RenderFormat.h"



/* ****************************************************************************
*
* renderFormatToString - 
*/
const char* renderFormatToString(OrionldRenderFormat format, bool noDefault, bool useLegacyWord)
{
  switch (format)
  {
  case RF_LEGACY:                         return useLegacyWord ? "legacy" : "JSON";
  case RF_NORMALIZED:                     return "normalized";
  case RF_SIMPLIFIED:                     return "keyValues";
  case RF_VALUES:                         return "values";
  case RF_UNIQUE_VALUES:                  return "uniqueValues";
  case RF_CUSTOM:                         return "custom";
  case RF_CONCISE:                        return "concise";
  case RF_CROSS_APIS_NORMALIZED:          return "normalized";
  case RF_CROSS_APIS_SIMPLIFIED:          return "keyValues";
  case RF_CROSS_APIS_NORMALIZED_COMPACT:  return "normalized";
  case RF_CROSS_APIS_SIMPLIFIED_COMPACT:  return "keyValues";
  case RF_NONE:
    if      (noDefault == true)           return "no render format";
    else if (useLegacyWord == true)       return "legacy";
    else                                  return "JSON";
  default:
    return "Unknown render format";
  }

  return "Unknown render format";
}



/* ****************************************************************************
*
* stringToRenderFormat -
*/
OrionldRenderFormat stringToRenderFormat(const char* s, bool noDefault)
{
  if (strcmp(s, "JSON")         == 0) return RF_LEGACY;          // DB content for NGSIv1 rendering due to legacy reasons
  if (strcmp(s, "legacy")       == 0) return RF_LEGACY;
  if (strcmp(s, "normalized")   == 0) return RF_NORMALIZED;
  if (strcmp(s, "keyValues")    == 0) return RF_SIMPLIFIED;
  if (strcmp(s, "simplified")   == 0) return RF_SIMPLIFIED;
  if (strcmp(s, "concise")      == 0) return RF_CONCISE;
  if (strcmp(s, "values")       == 0) return RF_VALUES;
  if (strcmp(s, "uniqueValues") == 0) return RF_UNIQUE_VALUES;
  if (strcmp(s, "custom")       == 0) return RF_CUSTOM;

  if (strcmp(s, "x-ngsiv2")                      == 0) return RF_CROSS_APIS_NORMALIZED_COMPACT;
  if (strcmp(s, "x-ngsiv2-normalized")           == 0) return RF_CROSS_APIS_NORMALIZED;
  if (strcmp(s, "x-ngsiv2-keyValues")            == 0) return RF_CROSS_APIS_SIMPLIFIED;
  if (strcmp(s, "x-ngsiv2-normalized-compacted") == 0) return RF_CROSS_APIS_NORMALIZED_COMPACT;
  if (strcmp(s, "x-ngsiv2-keyValues-compacted")  == 0) return RF_CROSS_APIS_SIMPLIFIED_COMPACT;

  return (noDefault == false)? DEFAULT_RENDER_FORMAT : RF_NONE;
}
