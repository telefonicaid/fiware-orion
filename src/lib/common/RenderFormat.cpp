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
const char* renderFormatToString(RenderFormat format)
{
  switch (format)
  {
  case NGSI_V2_NORMALIZED:          return "normalized";
  case NGSI_V2_KEYVALUES:           return "keyValues";
  case NGSI_V2_VALUES:              return "values";
  case NGSI_V2_UNIQUE_VALUES:       return "uniqueValues";
  case NGSI_V2_CUSTOM:              return "custom";
  case NGSI_V2_SIMPLIFIEDNORMALIZED:return "simplifiedNormalized";
  case NGSI_V2_SIMPLIFIEDKEYVALUES: return "simplifiedKeyValues";
  case NO_FORMAT:                   return "no render format";
  }

  return "Unknown render format";
}



/* ****************************************************************************
*
* stringToRenderFormat -
*/
RenderFormat stringToRenderFormat(const std::string& s, bool noDefault)
{
  if (s == "normalized")   { return NGSI_V2_NORMALIZED;    }
  if (s == "keyValues")    { return NGSI_V2_KEYVALUES;     }
  if (s == "values")       { return NGSI_V2_VALUES;        }
  if (s == "uniqueValues") { return NGSI_V2_UNIQUE_VALUES; }
  if (s == "custom")       { return NGSI_V2_CUSTOM;        }
  if (s == "simplifiedNormalized") { return NGSI_V2_SIMPLIFIEDNORMALIZED;    }
  if (s == "simplifiedKeyValues")  { return NGSI_V2_SIMPLIFIEDKEYVALUES;     }
  
  return (noDefault == false)? DEFAULT_RENDER_FORMAT : NO_FORMAT;
}
