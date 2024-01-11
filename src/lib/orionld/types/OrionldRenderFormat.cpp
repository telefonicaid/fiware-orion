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
#include <string.h>                                           // strcmp

#include "orionld/types/OrionldRenderFormat.h"                // Own interface + OrionldRenderFormat



// -----------------------------------------------------------------------------
//
// renderFormat -
//
const char* renderFormat(OrionldRenderFormat rf)
{
  switch (rf)
  {
  case RF_NONE:                           return "none";
  case RF_LEGACY:                         return "legacy";
  case RF_NORMALIZED:                     return "normalized";
  case RF_CONCISE:                        return "concise";
  case RF_SIMPLIFIED:                     return "simplified";
  case RF_VALUES:                         return "values";
  case RF_UNIQUE_VALUES:                  return "uniqueValues";
  case RF_CUSTOM:                         return "custom";
  case RF_CROSS_APIS_NORMALIZED:          return "normalized";
  case RF_CROSS_APIS_SIMPLIFIED:          return "simplified";
  case RF_CROSS_APIS_CONCISE:             return "concise";
  case RF_CROSS_APIS_NORMALIZED_COMPACT:  return "normalized";
  case RF_CROSS_APIS_SIMPLIFIED_COMPACT:  return "simplified";
  case RF_CROSS_APIS_CONCISE_COMPACT:     return "concise";
  }

  return "Unknown Render Format";
}



// -----------------------------------------------------------------------------
//
// renderFormatFromString -
//
OrionldRenderFormat renderFormat(const char* rf)
{
  if (strcmp(rf, "JSON")                          == 0) return RF_LEGACY;
  if (strcmp(rf, "legacy")                        == 0) return RF_LEGACY;
  if (strcmp(rf, "normalized")                    == 0) return RF_NORMALIZED;
  if (strcmp(rf, "keyValues")                     == 0) return RF_SIMPLIFIED;
  if (strcmp(rf, "simplified")                    == 0) return RF_SIMPLIFIED;
  if (strcmp(rf, "concise")                       == 0) return RF_CONCISE;
  if (strcmp(rf, "values")                        == 0) return RF_VALUES;
  if (strcmp(rf, "uniqueValues")                  == 0) return RF_UNIQUE_VALUES;
  if (strcmp(rf, "custom")                        == 0) return RF_CUSTOM;
  if (strcmp(rf, "x-ngsiv2")                      == 0) return RF_CROSS_APIS_NORMALIZED_COMPACT;
  if (strcmp(rf, "x-ngsiv2-normalized")           == 0) return RF_CROSS_APIS_NORMALIZED;
  if (strcmp(rf, "x-ngsiv2-keyValues")            == 0) return RF_CROSS_APIS_SIMPLIFIED;
  if (strcmp(rf, "x-ngsiv2-normalized-compacted") == 0) return RF_CROSS_APIS_NORMALIZED_COMPACT;
  if (strcmp(rf, "x-ngsiv2-keyValues-compacted")  == 0) return RF_CROSS_APIS_SIMPLIFIED_COMPACT;

  return RF_NONE;
}
