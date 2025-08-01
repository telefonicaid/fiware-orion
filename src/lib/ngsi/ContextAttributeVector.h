#ifndef SRC_LIB_NGSI_CONTEXTATTRIBUTEVECTOR_H_
#define SRC_LIB_NGSI_CONTEXTATTRIBUTEVECTOR_H_

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
#include <vector>

#include "common/RenderFormat.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/StringList.h"

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONArrayBuilder.h"


/* ****************************************************************************
*
* ContextAttributeVector - 
*/
struct ContextAttributeVector;
typedef struct ContextAttributeVector
{
  std::vector<ContextAttribute*>  vec;

  ContextAttributeVector();

  void                     push_back(ContextAttribute* item);
  void                     push_back(const ContextAttributeVector& caV, bool cloneCompound = false);
  unsigned int             size(void) const;
  void                     release(void);
  void                     fill(const ContextAttributeVector& caV, bool useDefaultType = false, bool cloneCompounds = false);
  void                     fill(const orion::BSONObj& attrs, const StringList& attrL);
  void                     fill(const orion::BSONObj& attrs);
  int                      get(const std::string& attributeName) const;

  ContextAttribute*  operator[](unsigned int ix) const;


  std::string        check(bool asValue);

  std::string        toJsonTypes(void);

  void               toBson(double                    now,
                            orion::BSONObjBuilder*    attrsToAdd,
                            orion::BSONArrayBuilder*  attrNamesToAdd) const;

  void               applyUpdateOperators(void);

} ContextAttributeVector;

#endif  // SRC_LIB_NGSI_CONTEXTATTRIBUTEVECTOR_H_
