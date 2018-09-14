#ifndef SRC_LIB_NGSI_CONTEXTELEMENT_H_
#define SRC_LIB_NGSI_CONTEXTELEMENT_H_

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
#include <string>

#include "ngsi/Request.h"
#include "common/RenderFormat.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/MetadataVector.h"



/* ****************************************************************************
*
* ContextElement -
*/
typedef struct ContextElement
{
  EntityId                 entityId;                // Mandatory
  ContextAttributeVector   contextAttributeVector;  // Optional

  std::vector<ProvidingApplication> providingApplicationList;    // Not part of NGSI, used internally for CPr forwarding functionality

  ContextElement();
  ContextElement(const std::string& id, const std::string& type, const std::string& isPattern);
  ContextElement(EntityId* eP);

  std::string  render(bool asJsonObject, RequestType requestType, bool comma, bool omitAttributeValues = false);
  std::string  toJson(RenderFormat                     renderFormat,
                      const std::vector<std::string>&  metadataFilter);
  void         release(void);
  void         fill(const struct ContextElement& ce);
  void         fill(ContextElement* ceP, bool useDefaultType = false);

  ContextAttribute* getAttribute(const std::string& attrName);

  std::string  check(ApiVersion apiVersion, RequestType requestType);

  void filterAttributes(const std::vector<std::string>&  attrsFilter, bool blacklist);

private:
  std::string toJsonValues(void);
  std::string toJsonUniqueValues(void);
  std::string toJsonKeyvalues(void);
  std::string toJsonNormalized(const std::vector<std::string>&  metadataFilter);
} ContextElement;

#endif  // SRC_LIB_NGSI_CONTEXTELEMENT_H_
