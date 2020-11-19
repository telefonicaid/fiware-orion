#ifndef SRC_LIB_ORIONLD_KJTREE_KJTREEREGISTRATIONINFOEXTRACT_H_
#define SRC_LIB_ORIONLD_KJTREE_KJTREEREGISTRATIONINFOEXTRACT_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}



// -----------------------------------------------------------------------------
//
// kjTreeRegistrationInfoExtract -
//
extern bool kjTreeRegistrationInfoExtract
(
  KjNode*    registrationP,
  char*      protocol,
  int        protocolSize,
  char*      host,
  int        hostSize,
  uint16_t*  portP,
  char**     uriDirP,
  char*      registrationAttrV[],
  int        registrationAttrSize,
  int*       registrationAttrsP,
  char**     detailP
);

#endif  // SRC_LIB_ORIONLD_KJTREE_KJTREEREGISTRATIONINFOEXTRACT_H_
