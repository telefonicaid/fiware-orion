#ifndef SRC_LIB_APITYPESV2_ATTRIBUTE_H_
#define SRC_LIB_APITYPESV2_ATTRIBUTE_H_

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
* Author: Orion dev team
*/
#include <string>
#include <vector>
#include <cstdlib>

#include "ngsi/ContextAttributeVector.h"
#include "ngsi/ContextAttribute.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* To avoid a problematic and not necessary include
*/
struct QueryContextResponse;



/* ****************************************************************************
*
* Attribute -
*/
class Attribute
{
 public:
  ContextAttribute*  pcontextAttribute;
  OrionError         oe;                    // Optional - mandatory if not 200-OK

  Attribute(): pcontextAttribute(0) {}
  std::string  render(bool                acceptedTextPlain,
                      bool                acceptedJson,
                      MimeType            outFormatSelection,
                      MimeType*           outMimeTypeP,
                      HttpStatusCode*     scP,
                      bool                keyValues,
                      const std::string&  metadataList,
                      RequestType         requestType,
                      bool                comma = false);
  void         fill(QueryContextResponse* qcrsP, std::string attrName);
};

#endif  // SRC_LIB_APITYPESV2_ATTRIBUTE_H_
