/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/

/* Common function used by serveral modules within the serviceRoutinesV2 library */

#include "serviceRoutinesV2/serviceRoutinesCommon.h"

#include <string>
#include <map>

#include "common/string.h"
#include "common/RenderFormat.h"
#include "ngsi/StringList.h"
#include "rest/uriParamNames.h"



/* ****************************************************************************
*
* setFilters -
*
*/
void setFilters
(
  std::map<std::string, std::string>&  uriParam,
  std::map<std::string, bool>&         uriParamOptions,
  StringList*                          attrsFilter,
  StringList*                          metadataFilter
)
{
  // Add metadata filter
  if (uriParam[URI_PARAM_METADATA] != "")
  {
    stringSplit(uriParam[URI_PARAM_METADATA], ',', metadataFilter->stringV);
  }

  // Add attributes filter
  if (uriParam[URI_PARAM_ATTRS] != "")
  {
    stringSplit(uriParam[URI_PARAM_ATTRS], ',', attrsFilter->stringV);
  }

  // dateCreated and dateModified options are still supported although deprecated.
  // Note that we check for attr list emptyness, as in that case the "*" needs
  // to be added to print also user attributes
  if (uriParamOptions[OPT_DATE_CREATED])
  {
    if (attrsFilter->size() == 0)
    {
      attrsFilter->push_back(ALL_ATTRS);
    }

    // Safe measure to avoid duplicates in attrsFilter
    if (!attrsFilter->lookup(DATE_CREATED))
    {
      attrsFilter->push_back(DATE_CREATED);
    }
  }

  if (uriParamOptions[OPT_DATE_MODIFIED])
  {
    if (attrsFilter->size() == 0)
    {
      attrsFilter->push_back(ALL_ATTRS);
    }

    // Safe measure to avoid duplicates in attrsFilter
    if (!attrsFilter->lookup(DATE_MODIFIED))
    {
      attrsFilter->push_back(DATE_MODIFIED);
    }
  }
}



/* ****************************************************************************
*
* renderFormat -
*
*/
RenderFormat getRenderFormat(std::map<std::string, bool>&  uriParamOptions)
{
  RenderFormat  renderFormat = NGSI_V2_NORMALIZED;

  if      (uriParamOptions[OPT_KEY_VALUES]    == true)  { renderFormat = NGSI_V2_KEYVALUES;     }
  else if (uriParamOptions[OPT_VALUES]        == true)  { renderFormat = NGSI_V2_VALUES;        }
  else if (uriParamOptions[OPT_UNIQUE_VALUES] == true)  { renderFormat = NGSI_V2_UNIQUE_VALUES; }

  return renderFormat;
}
