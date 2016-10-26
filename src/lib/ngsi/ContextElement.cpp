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
#include "ngsi/ContextElement.h"
#include "ngsi/EntityId.h"
#include "ngsi/Request.h"
#include "rest/ConnectionInfo.h"



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
std::string ContextElement::render(ConnectionInfo* ciP, RequestType requestType, const std::string& indent, bool comma, bool omitAttributeValues)
{
  std::string  out                              = "";
  std::string  key                              = "contextElement";
  bool         attributeDomainNameRendered      = attributeDomainName.get() != "";
  bool         contextAttributeVectorRendered   = contextAttributeVector.size() != 0;
  bool         domainMetadataVectorRendered     = domainMetadataVector.size() != 0;

  bool         commaAfterDomainMetadataVector   = false;  // Last element
  bool         commaAfterContextAttributeVector = domainMetadataVectorRendered;
  bool         commaAfterAttributeDomainName    = domainMetadataVectorRendered  || contextAttributeVectorRendered;
  bool         commaAfterEntityId               = commaAfterAttributeDomainName || attributeDomainNameRendered;

  if (requestType == UpdateContext)
  {
    out += startTag2(indent, key, false, false);
  }
  else
  {
    out += startTag2(indent, key, false, true);
  }

  out += entityId.render(indent + "  ", commaAfterEntityId, false);
  out += attributeDomainName.render(indent + "  ", commaAfterAttributeDomainName);
  out += contextAttributeVector.render(ciP, requestType, indent + "  ", commaAfterContextAttributeVector, omitAttributeValues);
  out += domainMetadataVector.render(indent + "  ", commaAfterDomainMetadataVector);

  out += endTag(indent, comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElement::toJson - 
*/
std::string ContextElement::toJson
(
  RenderFormat                     renderFormat,
  const std::vector<std::string>&  attrsFilter,
  const std::vector<std::string>&  metadataFilter,
  bool                             blacklist
) const
{
  std::string out;

  if (renderFormat != NGSI_V2_VALUES)
  {
    out += entityId.toJson();
    if (contextAttributeVector.size() != 0)
    {
      out += ",";
    }
  }

  if (contextAttributeVector.size() != 0)
  {
    out += contextAttributeVector.toJson(renderFormat, attrsFilter, metadataFilter, blacklist);
  }

  return out;
}



/* ****************************************************************************
*
* ContextElement::getAttribute
*/
ContextAttribute* ContextElement::getAttribute(const std::string& attrName)
{
  for (unsigned int ix = 0; ix < contextAttributeVector.size(); ++ix)
  {
    ContextAttribute* ca = contextAttributeVector[ix];

    if (ca->name == attrName)
    {
      return ca;
    }
  }

  return NULL;
}



/* ****************************************************************************
*
* ContextElement::check
*/
std::string ContextElement::check
(
  ConnectionInfo*     ciP,
  RequestType         requestType,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  std::string res;

  if ((res = entityId.check(ciP, requestType, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = attributeDomainName.check(requestType, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = contextAttributeVector.check(ciP, requestType, indent, predetectedError, counter)) != "OK")
  {
    return res;
  }

  if ((res = domainMetadataVector.check(ciP, requestType, indent, predetectedError, counter)) != "OK")
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
* ContextElement::present - 
*/
void ContextElement::present(const std::string& indent, int ix)
{
  if (ix == -1)
  {
    LM_T(LmtPresent, ("%sContext Element:", indent.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sContext Element %d:", indent.c_str(), ix));
  }

  entityId.present(indent + "  ", -1);
  attributeDomainName.present(indent + "  ");
  contextAttributeVector.present(indent + "  ");
  domainMetadataVector.present("Domain", indent + "  ");
  for (unsigned int ix = 0; ix < providingApplicationList.size(); ++ix)
  {
    LM_T(LmtPresent, ("%s  PA: %s (%s)", 
		      indent.c_str(), 
		      providingApplicationList[ix].get().c_str(), 
		      mimeTypeToString(providingApplicationList[ix].getMimeType())));
    LM_T(LmtPresent, ("%s  providingApplication: %s", 
		      indent.c_str(), 
		      providingApplicationList[ix].c_str()));
  }
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
