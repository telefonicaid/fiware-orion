#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDDEFAULTCONTEXT_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDDEFAULTCONTEXT_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
extern "C"
{
#include "kjson/KjNode.h"                               // KjNode
}

#include "orionld/context/OrionldContext.h"             // OrionldContext



// -----------------------------------------------------------------------------
//
// ORIONLD_DEFAULT_CONTEXT_URL - 
//
#define ORIONLD_DEFAULT_CONTEXT_URL (char*) "https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/coreContext/ngsi-ld-core-context.json"



// -----------------------------------------------------------------------------
//
// ORIONLD_DEFAULT_EXPANSION_URL_DIR - 
//
// If an expansion is not found for an Entity Type or Attribute Name/Type,
// use this prefix as default expansion and append the "not found" Name/Type.
//
// E.g., if the lookup the expansion for the Attribute Name "A1" returns NULL,
//       then use ORIONLD_DEFAULT_EXPANSION_URL_DIR_XXX + "A1" as expansion
//
#define ORIONLD_DEFAULT_EXPANSION_URL_DIR_ENTITY    "http://www.example.org/entities/"
#define ORIONLD_DEFAULT_EXPANSION_URL_DIR_ATTRIBUTE "http://www.example.org/attributes/"
#define ORIONLD_DEFAULT_EXPANSION_URL_DIR_DEFAULT   "http://www.example.org/default/"



// -----------------------------------------------------------------------------
//
// orionldDefaultContext
//
extern OrionldContext orionldDefaultContext;

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDDEFAULTCONTEXT_H_
