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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <stdio.h>

#include "common/Format.h"
#include "common/globals.h"
#include "common/tag.h"
#include "ngsi/Request.h"
#include "ngsi/ContextElement.h"



/* ****************************************************************************
*
* ContextElement::render - 
*/
std::string ContextElement::render(RequestType requestType, Format format, std::string indent, bool comma)
{
  std::string  out                              = "";
  std::string  xmlTag                           = "contextElement";
  std::string  jsonTag                          = "contextElement";
  bool         attributeDomainNameRendered      = attributeDomainName.get() != "";
  bool         contextAttributeVectorRendered   = contextAttributeVector.size() != 0;
  bool         domainMetadataVectorRendered     = domainMetadataVector.size() != 0;

  bool         commaAfterDomainMetadataVector   = false; // Last element
  bool         commaAfterContextAttributeVector = domainMetadataVectorRendered;
  bool         commaAfterAttributeDomainName    = domainMetadataVectorRendered || contextAttributeVectorRendered;
  bool         commaAfterEntityId               = domainMetadataVectorRendered || contextAttributeVectorRendered || attributeDomainNameRendered;

  out += startTag(indent, xmlTag, jsonTag, format, false, true);

  out += entityId.render(format,               indent + "  ", commaAfterEntityId, false);
  out += attributeDomainName.render(format,    indent + "  ", commaAfterAttributeDomainName);
  out += contextAttributeVector.render(requestType, format, indent + "  ", commaAfterContextAttributeVector);
  out += domainMetadataVector.render(format,   indent + "  ", commaAfterDomainMetadataVector);

  out += endTag(indent, xmlTag, format, comma, false);

  return out;
}



/* ****************************************************************************
*
* ContextElement::check
*/
std::string ContextElement::check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter)
{
  std::string res;

  if ((res = entityId.check(requestType, format, indent, predetectedError, counter)) != "OK")                 return res;
  if ((res = attributeDomainName.check(requestType, format, indent, predetectedError, counter)) != "OK")      return res;
  if ((res = contextAttributeVector.check(requestType, format, indent, predetectedError, counter)) != "OK")   return res;
  if ((res = domainMetadataVector.check(requestType, format, indent, predetectedError, counter)) != "OK")     return res;

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
void ContextElement::present(std::string indent, int ix)
{
  if (ix == -1)
    PRINTF("%sContext Element:\n", indent.c_str());
  else
    PRINTF("%sContext Element %d:\n", indent.c_str(), ix);

  entityId.present(indent + "  ", -1);
  attributeDomainName.present(indent + "  ");
  contextAttributeVector.present(indent + "  ");
  domainMetadataVector.present("Domain", indent + "  ");
}
