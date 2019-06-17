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
#include "kbase/version.h"                                     // kbaseVersion
#include "kalloc/version.h"                                    // kallocVersion
#include "kjson/version.h"                                     // kjsonVersion
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/branchName.h"                         // ORIONLD_BRANCH
#include "orionld/serviceRoutines/orionldGetVersion.h"         // Own Interface

#include <boost/version.hpp>                                   // BOOST_LIB_VERSION
#include <microhttpd.h>                                        // MHD_VERSION (returns a number)
#include <openssl/opensslv.h>                                  // OPENSSL_VERSION_NUMBER
#include <mongo/version.h>                                     // MONGOCLIENT_VERSION
#include <rapidjson/rapidjson.h>                               // RAPIDJSON_VERSION_STRING
// libcurl version not found, keep searching
// libuuid version not found, keep searching
// gtest and gmock don't contain any function or header to check versions

// ----------------------------------------------------------------------------
//
// orionldGetVersion -
//
bool orionldGetVersion(ConnectionInfo* ciP)
{
  KjNode* nodeP;

  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

  nodeP = kjString(orionldState.kjsonP, "branch", ORIONLD_BRANCH);
  kjChildAdd(orionldState.responseTree, nodeP);

  nodeP = kjString(orionldState.kjsonP, "kbase version", kbaseVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "kalloc version", kallocVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "kjson version", kjsonVersion);
  kjChildAdd(orionldState.responseTree, nodeP);

  // Libs versions
  nodeP = kjString(orionldState.kjsonP, "boost version", BOOST_LIB_VERSION);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjInteger(orionldState.kjsonP, "microhttpd version", MHD_VERSION); // (FIX ME)
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjInteger(orionldState.kjsonP, "openssl version", OPENSSL_VERSION_NUMBER); // (FIX ME)
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjInteger(orionldState.kjsonP, "mongo version", MONGOCLIENT_VERSION);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "rapidjson version", RAPIDJSON_VERSION_STRING);
  kjChildAdd(orionldState.responseTree, nodeP);


  // This request is ALWAYS returned with pretty-print
  orionldState.kjsonP->spacesPerIndent   = 2;
  orionldState.kjsonP->nlString          = (char*) "\n";
  orionldState.kjsonP->stringBeforeColon = (char*) "";
  orionldState.kjsonP->stringAfterColon  = (char*) " ";

  orionldState.useLinkHeader = false;  // We don't want the Link header for version requests

  return true;
}
