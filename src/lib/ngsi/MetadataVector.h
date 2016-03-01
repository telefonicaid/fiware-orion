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

#include "ngsi/Metadata.h"



/* ****************************************************************************
*
* MetadataVector - 
*/
typedef struct MetadataVector
{
  std::vector<Metadata*>  vec;

  std::string             keyName;        // Help variable for the 'render' method

  MetadataVector(const std::string& _keyName = "registrationMetadata");

  void            keyNameSet(const std::string& key);
  std::string     render(const std::string& indent, bool comma = false);
  std::string     toJson(bool isLastElement);
  std::string     check(ConnectionInfo* ciP,
                      RequestType requestType,
                      const std::string& indent,
                      const std::string& predetectedError,
                      int counter);

  void            present(const std::string& metadataType, const std::string& indent);
  void            push_back(Metadata* item);
  unsigned int    size(void) const;
  Metadata*       lookupByName(const std::string& _name);
  void            release();
  void            fill(MetadataVector* mV);
 
  
  Metadata* operator[](unsigned int ix) const;

  
} MetadataVector;

#endif  // SRC_LIB_NGSI_METADATAVECTOR_H_
