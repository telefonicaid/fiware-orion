#ifndef SRC_LIB_APITYPESV2_ENTITY_H_
#define SRC_LIB_APITYPESV2_ENTITY_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <vector>
#include <map>

#include "ngsi/ContextAttributeVector.h"
#include "ngsi/EntityId.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* To avoid a problematic and not necessary include
*/
struct QueryContextResponse;



/* ****************************************************************************
*
* Entity - 
*/
class Entity
{
 public:
  std::string             id;               // Mandatory
  std::string             type;             // Optional
  std::string             isPattern;        // Optional
  bool                    isTypePattern;
  ContextAttributeVector  attributeVector;  // Optional
  OrionError              oe;               // Optional - mandatory if not 200-OK

  std::string             servicePath;      // Not part of payload, just an internal field
  bool                    typeGiven;        // Was 'type' part of the incoming payload?
  bool                    renderId;         // Should id and type be rendered in JSON?

  double                  creDate;          // used by dateCreated functionality in NGSIv2
  double                  modDate;          // used by dateModified functionality in NGSIv2

  std::vector<ProvidingApplication> providingApplicationList;    // Not part of NGSI, used internally for CPr forwarding functionality

  Entity();
  Entity(const std::string& id, const std::string& type, const std::string& isPattern);
  explicit Entity(EntityId* eP);
  explicit Entity(Entity* eP);

  ~Entity();

  std::string  toJsonV1(bool                             asJsonObject,
                        RequestType                      requestType,
                        const std::vector<std::string>&  attrsFilter,
                        bool                             blacklist,
                        const std::vector<std::string>&  metadataFilter,
                        bool                             comma,
                        bool                             omitAttributeValues = false);

  std::string  toJson(RenderFormat                     renderFormat,
                      const std::vector<std::string>&  attrsFilter,
                      bool                             blacklist,
                      const std::vector<std::string>&  metadataFilter);

  std::string  toString(bool useIsPattern = false, const std::string& delimiter = ", ");

  std::string  check(ApiVersion apiVersion, RequestType requestType);

  void         release(void);

  void         fill(const std::string&             id,
                    const std::string&             type,
                    const std::string&             isPattern,
                    const ContextAttributeVector&  caV,
                    double                         creDate,
                    double                         modDate);

  void         fill(const std::string&  id,
                    const std::string&  type,
                    const std::string&  isPattern,
                    const std::string&  servicePath,
                    double              creDate = 0,
                    double              modDate = 0);

  void         fill(const std::string&  id,
                    const std::string&  type,
                    const std::string&  isPattern);

  void         fill(const Entity& en, bool useDefaultType = false);

  void         fill(QueryContextResponse* qcrsP);

  void         hideIdAndType(bool hide = true);

  ContextAttribute* getAttribute(const std::string& attrName);

  bool         equal(Entity* eP);

 private:
  void filterAndOrderAttrs(const std::vector<std::string>&  attrsFilter,
                           bool                             blacklist,
                           std::vector<ContextAttribute*>*  orderedAttrs);

  void addAllExceptShadowed(std::vector<ContextAttribute*>*  orderedAttrs);

  std::string toJsonValues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonUniqueValues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonKeyvalues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonNormalized(const std::vector<ContextAttribute*>& orderedAttrs, const std::vector<std::string>&  metadataFilter);
};

#endif  // SRC_LIB_APITYPESV2_ENTITY_H_
