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
  EntityId                entityId;         // Mandatory
  ContextAttributeVector  attributeVector;  // Optional

  std::string             servicePath;      // Not part of payload, just an internal field
  bool                    renderId;         // Should id and type be rendered in JSON?

  double                  creDate;          // used by dateCreated functionality in NGSIv2
  double                  modDate;          // used by dateModified functionality in NGSIv2

  std::vector<ngsiv2::Provider> providerList;      // Used internally for CPr forwarding functionality
  std::vector<std::string>      providerRegIdList; // Side vector to providerList, to hold the reg ids where they come (used for login purposes)

  Entity();
  Entity(const std::string& id, const std::string& idPattern, const std::string& type, const std::string& typePattern);

  ~Entity();

  std::string  toJson(RenderFormat                         renderFormat,
                      const std::vector<std::string>&      attrsFilter,
                      bool                                 blacklist,
                      const std::vector<std::string>&      metadataFilter,
                      bool                                 renderNgsiField    = false);

  std::string  toJson(RenderFormat                     renderFormat,
                      bool                             renderNgsiField = false);

  std::string  toString(void);

  std::string  check(RequestType requestType);

  void         applyUpdateOperators(void);

  void         release(void);

  void         fill(const EntityId&                entId,
                    const ContextAttributeVector&  caV,
                    double                         creDate,
                    double                         modDate);

  void         fill(const EntityId&     entId,
                    const std::string&  servicePath,
                    double              creDate = 0,
                    double              modDate = 0);

  void         fill(const EntityId& entId);

  void         fill(const Entity& en, bool useDefaultType = false, bool cloneCompounds = false);

  void         fill(const QueryContextResponse& qcrs, OrionError* oeP);

  void         hideIdAndType(bool hide = true);

  ContextAttribute* getAttribute(const std::string& attrName);

  bool         equal(Entity* eP);

 private:
  void filterAndOrderAttrs(const std::vector<std::string>&  attrsFilter,
                           bool                             blacklist,
                           std::vector<ContextAttribute*>*  orderedAttrs);

  void addAllAttrsExceptFiltered(std::vector<ContextAttribute*>*  orderedAttrs);

  std::string toJsonValues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonUniqueValues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonKeyvalues(const std::vector<ContextAttribute*>& orderedAttrs);
  std::string toJsonNormalized(const std::vector<ContextAttribute*>&  orderedAttrs,
                               const std::vector<std::string>&        metadataFilter,
                               bool                                   renderNgsiField   = false);

  std::string  checkId(RequestType requestType);
  std::string  checkIdPattern(RequestType requestType);
  std::string  checkType(RequestType requestType);
  std::string  checkTypePattern(RequestType requestType);
};

#endif  // SRC_LIB_APITYPESV2_ENTITY_H_
