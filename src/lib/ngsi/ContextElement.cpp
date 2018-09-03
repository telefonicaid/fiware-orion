/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdio.h>
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/MimeType.h"
#include "common/globals.h"
#include "common/tag.h"
#include "common/string.h"
#include "common/JsonHelper.h"

#include "ngsi/EntityId.h"
#include "ngsi/Request.h"
#include "ngsi/ContextElement.h"

#include "mongoBackend/dbFieldEncoding.h"


/* ****************************************************************************
*
* ContextElement::ContextElement - 
*/
ContextElement::ContextElement()
{
  entityId.fill("", "", "false");
}



/* ****************************************************************************
*
* ContextElement::ContextElement - 
*/
ContextElement::ContextElement(EntityId* eP)
{
  entityId.fill(eP);
}



/* ****************************************************************************
*
* ContextElement::ContextElement - 
*/
ContextElement::ContextElement(const std::string& id, const std::string& type, const std::string& isPattern)
{
  entityId.fill(id, type, isPattern);
}



/* ****************************************************************************
*
* ContextElement::render - 
*/
std::string ContextElement::render
(
  ApiVersion   apiVersion,
  bool         asJsonObject,
  RequestType  requestType,
  bool         comma,
  bool         omitAttributeValues
)
{
  std::string  out                              = "";
  bool         attributeDomainNameRendered      = attributeDomainName.get() != "";
  bool         contextAttributeVectorRendered   = contextAttributeVector.size() != 0;
  bool         domainMetadataVectorRendered     = domainMetadataVector.size() != 0;

  bool         commaAfterDomainMetadataVector   = false;  // Last element
  bool         commaAfterContextAttributeVector = domainMetadataVectorRendered;
  bool         commaAfterAttributeDomainName    = domainMetadataVectorRendered  || contextAttributeVectorRendered;
  bool         commaAfterEntityId               = commaAfterAttributeDomainName || attributeDomainNameRendered;

  out += startTag(requestType != UpdateContext? "contextElement" : "");

  out += entityId.render(commaAfterEntityId, false);
  out += attributeDomainName.render(commaAfterAttributeDomainName);
  out += contextAttributeVector.render(apiVersion, asJsonObject, requestType, commaAfterContextAttributeVector, omitAttributeValues);
  out += domainMetadataVector.render(commaAfterDomainMetadataVector);

  out += endTag(comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElement::filterAttributes -
*
* Filter attributes vector in order to get the effective attribute vector to
* render.
*
* dateCreatedOption and dateModifiedOption are due to deprecated ways of requesting
* date in response. If used, the date is added at the end.
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
void ContextElement::filterAttributes(const std::vector<std::string>&  attrsFilter, bool blacklist)
{

  if (attrsFilter.size () != 0)
  {
    if (std::find(attrsFilter.begin(), attrsFilter.end(), ALL_ATTRS) != attrsFilter.end())
    {
      // No filtering, just adding dateCreated and dateModified if needed (only in no blacklist case)
      //
      // The (contextAttributeVector.lookup(DATE_XXXX) == -1) check is to give preference to user
      // defined attributes (see
      // https://fiware-orion.readthedocs.io/en/master/user/ngsiv2_implementation_notes/index.html#datemodified-and-datecreated-attributes)

      if (!blacklist)
      {
        if ((entityId.creDate != 0) && (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_CREATED) != attrsFilter.end()) && (contextAttributeVector.lookup(DATE_CREATED) == -1))
        {
          ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, entityId.creDate);
          contextAttributeVector.push_back(caP);
        }
        if ((entityId.modDate != 0) &&  (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_MODIFIED) != attrsFilter.end()) && (contextAttributeVector.lookup(DATE_MODIFIED) == -1))
        {
          ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, entityId.modDate);
          contextAttributeVector.push_back(caP);
        }
      }
    }
    else
    {
      // Processing depend on blacklist
      //
      // 1. If blacklist == true, go through the contextAttributeVector, taking only its elements
      //    not in attrsFilter
      // 2. If blacklist == false, reorder attributes in the same order they are in attrsFilter, excluding
      //    the ones not there (i.e. filtering them out) and giving special treatment to creation
      //    and modification dates

      if (blacklist)
      {
        std::vector<ContextAttribute*> caNewV;
        for (unsigned int ix = 0; ix < contextAttributeVector.size(); ix++)
        {
          ContextAttribute* caP = contextAttributeVector[ix];
          if (std::find(attrsFilter.begin(), attrsFilter.end(), caP->name) == attrsFilter.end())
          {
            caNewV.push_back(caP);
          }
          else
          {
            caP->release();
          }
        }
        contextAttributeVector.vec = caNewV;
      }
      else
      {
        std::vector<ContextAttribute*> caNewV;

        for (unsigned int ix = 0; ix < attrsFilter.size(); ix++)
        {
          std::string attrsFilterItem = attrsFilter[ix];
          if ((entityId.creDate != 0) && (attrsFilterItem == DATE_CREATED) && (contextAttributeVector.lookup(DATE_CREATED) == -1))
          {
            ContextAttribute* caP = new ContextAttribute(DATE_CREATED, DATE_TYPE, entityId.creDate);
            caNewV.push_back(caP);
          }
          else if ((entityId.modDate != 0) && (attrsFilterItem == DATE_MODIFIED) && (contextAttributeVector.lookup(DATE_MODIFIED) == -1))
          {
            ContextAttribute* caP = new ContextAttribute(DATE_MODIFIED, DATE_TYPE, entityId.modDate);
            caNewV.push_back(caP);
          }
          // Actual attribute filtering only takes place if '*' was not used
          else
          {
            int found = contextAttributeVector.lookup(attrsFilterItem);
            if (found != -1)
            {
              caNewV.push_back(contextAttributeVector.vec[found]);
              contextAttributeVector.vec.erase(contextAttributeVector.vec.begin() + found);
            }
          }
        }

        // All the remainder elements in attributeVector need to be released,
        // before overriding the vector with caNewV
        contextAttributeVector.release();

        contextAttributeVector.vec = caNewV;

      }
    }
  }

  // Removing dateExpires if not explictely included in the filter
  bool includeDateExpires = (std::find(attrsFilter.begin(), attrsFilter.end(), DATE_EXPIRES) != attrsFilter.end());
  int found;
  if (!blacklist && !includeDateExpires && ((found = contextAttributeVector.lookup(DATE_EXPIRES)) != -1))
  {
    contextAttributeVector.vec[found]->release();
    contextAttributeVector.vec.erase(contextAttributeVector.vec.begin() + found);
  }
}


/* ****************************************************************************
*
* ContextElement::toJson - 
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
std::string ContextElement::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  const std::vector<std::string>&  metadataFilter,
  bool                             blacklist
)
{
  // Get the effective vector of attributes to render
  filterAttributes(attrsFilter, blacklist);

  std::string out;
  switch (renderFormat)
  {
  case NGSI_V2_VALUES:
    out = toJsonValues();
    break;
  case NGSI_V2_UNIQUE_VALUES:
    out = toJsonUniqueValues();
    break;
  case NGSI_V2_KEYVALUES:
    out = toJsonKeyvalues();
    break;
  default:  // NGSI_V2_NORMALIZED
    out = toJsonNormalized(metadataFilter);
    break;
  }

  return out;
}


/* ****************************************************************************
*
* ContextElement::toJsonValues -
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
std::string ContextElement::toJsonValues(void)
{
  std::string out = "[";

  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = contextAttributeVector[ix];
    out += caP->toJsonValue();

    if (ix != contextAttributeVector.size() - 1)
    {
      out += ",";
    }
  }

  out += "]";

  return out;
}



/* ****************************************************************************
*
* ContextElement::toJsonUniqueValues -
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
std::string ContextElement::toJsonUniqueValues(void)
{
  std::string out = "[";

  std::map<std::string, bool>  uniqueMap;

  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = contextAttributeVector[ix];

    std::string value = caP->toJsonValue();

    if (uniqueMap[value] == true)
    {
      // Already rendered. Skip.
      continue;
    }
    else
    {
      out += value;
      uniqueMap[value] = true;
    }

    if (ix != contextAttributeVector.size() - 1)
    {
      out += ",";
    }
  }

  out += "]";

  return out;
}



/* ****************************************************************************
*
* ContextElement::toJsonKeyvalues -
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
std::string ContextElement::toJsonKeyvalues(void)
{
  JsonHelper jh;

  jh.addString("id", entityId.id);

  /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
  jh.addString("type", (entityId.type != "")? entityId.type : DEFAULT_ENTITY_TYPE);

  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = contextAttributeVector[ix];
    jh.addRaw(caP->name, caP->toJsonValue());
  }

  return jh.str();
}



/* ****************************************************************************
*
* ContextElement::toJsonNormalized -
*
* FIXME PR: lot of duplicated code from Entity NGSIv2 class but we don't care
* as NGSIv1 code is deprecated. The duplication will be solved when NGSIv1 gets removed
*/
std::string ContextElement::toJsonNormalized(const std::vector<std::string>&  metadataFilter)
{
  JsonHelper jh;

  jh.addString("id", entityId.id);

  /* This is needed for entities coming from NGSIv1 (which allows empty or missing types) */
  jh.addString("type", (entityId.type != "")? entityId.type : DEFAULT_ENTITY_TYPE);

  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ix++)
  {
    ContextAttribute* caP = contextAttributeVector[ix];
    jh.addRaw(caP->name, caP->toJson(metadataFilter));
  }

  return jh.str();
}


/* ****************************************************************************
*
* ContextElement::getAttribute
*/
ContextAttribute* ContextElement::getAttribute(const std::string& attrName)
{
  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ++ix)
  {
    ContextAttribute* caP = contextAttributeVector[ix];

    if (dbDotEncode(caP->name) == attrName)
    {
      return caP;
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* ContextElement::check
*/
std::string ContextElement::check(ApiVersion apiVersion, RequestType requestType)
{
  std::string res;

  if ((res = entityId.check(requestType)) != "OK")
  {
    return res;
  }

  if ((res = attributeDomainName.check()) != "OK")
  {
    return res;
  }

  if ((res = contextAttributeVector.check(apiVersion, requestType)) != "OK")
  {
    return res;
  }

  if ((res = domainMetadataVector.check(apiVersion)) != "OK")
  {
    return res;
  }

  return "OK";
}



/* ****************************************************************************
*
* ContextElement::release - 
*/
void ContextElement::release(void)
{
  entityId.release();
  attributeDomainName.release();
  contextAttributeVector.release();
  domainMetadataVector.release();
}



/* ****************************************************************************
*
* ContextElement::fill - 
*/
void ContextElement::fill(const struct ContextElement& ce)
{
  entityId.fill(&ce.entityId);
  attributeDomainName.fill(ce.attributeDomainName);
  contextAttributeVector.fill((ContextAttributeVector*) &ce.contextAttributeVector);
  domainMetadataVector.fill((MetadataVector*) &ce.domainMetadataVector);
  /* Note that according to http://www.cplusplus.com/reference/vector/vector/operator=/, it is
   * safe to copy vectors of std::string using '=' */
  providingApplicationList = ce.providingApplicationList;
}



/* ****************************************************************************
*
* ContextElement::fill - 
*/
void ContextElement::fill(ContextElement* ceP, bool useDefaultType)
{
  entityId.fill(&ceP->entityId, useDefaultType);
  attributeDomainName.fill(ceP->attributeDomainName);
  contextAttributeVector.fill((ContextAttributeVector*) &ceP->contextAttributeVector, useDefaultType);
  domainMetadataVector.fill((MetadataVector*) &ceP->domainMetadataVector);
  /* Note that according to http://www.cplusplus.com/reference/vector/vector/operator=/, it is
   * safe to copy vectors of std::string using '=' */
  providingApplicationList = ceP->providingApplicationList;
}
