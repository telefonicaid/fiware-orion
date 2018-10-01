#ifndef SRC_LIB_NGSI_METADATAVECTOR_H_
#define SRC_LIB_NGSI_METADATAVECTOR_H_

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

#include "common/globals.h"

#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* MetadataVector - 
*/
typedef struct MetadataVector
{
public:
  std::vector<Metadata*>  vec;

  MetadataVector(void);

  std::string     toJsonV1(const std::vector<Metadata*>& orderedMetadata, bool comma);
  std::string     toJson(const std::vector<Metadata*>& orderedMetadata);
  std::string     check(ApiVersion apiVersion);

  void            push_back(Metadata* item);
  unsigned int    size(void) const;
  Metadata*       lookupByName(const std::string& _name);
  void            release();
  void            fill(MetadataVector* mV);
 
  
  Metadata* operator[](unsigned int ix) const;

private:
  bool matchFilter(const std::string& mdName, const std::vector<std::string>& metadataFilter);
  
} MetadataVector;

#endif  // SRC_LIB_NGSI_METADATAVECTOR_H_
