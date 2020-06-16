/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
#include <boost/version.hpp>                                   // BOOST_LIB_VERSION
#include <microhttpd.h>                                        // MHD_VERSION (returns a number)
#include <openssl/opensslv.h>                                  // OPENSSL_VERSION_TEXT
#include <mongo/version.h>                                     // MONGOCLIENT_VERSION
#include <rapidjson/rapidjson.h>                               // RAPIDJSON_VERSION_STRING
#include <curl/curl.h>                                         // curl_version_info_data, curl_version_info, CURLversion
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C"
{
#include "kbase/version.h"                                     // kbaseVersion
#include "kalloc/version.h"                                    // kallocVersion
#include "khash/version.h"                                     // khashVersion
#include "kjson/version.h"                                     // kjsonVersion
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "serviceRoutines/versionTreat.h"                      // versionGet
#include "cache/subCache.h"                                    // subCacheItems
#include "orionld/common/orionldState.h"                       // orionldState, orionldVersion
#include "orionld/common/branchName.h"                         // ORIONLD_BRANCH
#include "orionld/serviceRoutines/orionldGetVersion.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// mhdVersionGet -
//
void mhdVersionGet(char* buff, int buflen, int iVersion)
{
  char major;
  char minor;
  char bugfix;
  char revision;

  major    = iVersion >> 24;
  minor    = (iVersion >> 16) & 0xFF;
  bugfix   = (iVersion >> 8)  & 0xFF;
  revision = iVersion & 0xFF;

  snprintf(buff, buflen, "%x.%x.%x-%x", major, minor, bugfix, revision);
}




// ----------------------------------------------------------------------------
//
// orionldGetVersion -
//
bool orionldGetVersion(ConnectionInfo* ciP)
{
  KjNode*                  nodeP;
  char                     mhdVersion[32];
  curl_version_info_data*  curlVersionP;

  mhdVersionGet(mhdVersion, sizeof(mhdVersion), MHD_VERSION);

  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

  // Orion-LD version
  nodeP = kjString(orionldState.kjsonP, "Orion-LD version", orionldVersion);
  kjChildAdd(orionldState.responseTree, nodeP);

  // Orion version
  nodeP = kjString(orionldState.kjsonP, "based on orion", versionGet());
  kjChildAdd(orionldState.responseTree, nodeP);

  // K-Lib versions
  nodeP = kjString(orionldState.kjsonP, "kbase version", kbaseVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "kalloc version", kallocVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "khash version", khashVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "kjson version", kjsonVersion);
  kjChildAdd(orionldState.responseTree, nodeP);

  // Lib versions
  nodeP = kjString(orionldState.kjsonP, "boost version", BOOST_LIB_VERSION);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "microhttpd version", mhdVersion);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "openssl version", OPENSSL_VERSION_TEXT);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "mongo version", mongo::client::kVersionString);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "rapidjson version", RAPIDJSON_VERSION_STRING);
  kjChildAdd(orionldState.responseTree, nodeP);

  //
  // CURL lib
  //
  curlVersionP = curl_version_info(CURLVERSION_NOW);
  if (curlVersionP != NULL)
    nodeP = kjString(orionldState.kjsonP, "libcurl version", curlVersionP->version);
  else
    nodeP = kjString(orionldState.kjsonP, "libcurl version", "UNKNOWN");

  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "libuuid version", "UNKNOWN");
  kjChildAdd(orionldState.responseTree, nodeP);

  // Branch
  nodeP = kjString(orionldState.kjsonP, "branch", ORIONLD_BRANCH);
  kjChildAdd(orionldState.responseTree, nodeP);

  // Number of item in the subscription cache
  nodeP = kjInteger(orionldState.kjsonP, "cached subscriptions", subCacheItems());
  kjChildAdd(orionldState.responseTree, nodeP);

  //
  // Opening and closing an arbitrary file (/etc/passwd) only to see the next non-open
  // file descriptor.
  // This is to detect fd-leakage.
  //
  int fd;
  fd = open("/etc/passwd", O_RDONLY);
  nodeP = kjInteger(orionldState.kjsonP, "Next File Descriptor", fd);
  kjChildAdd(orionldState.responseTree, nodeP);
  close(fd);

  // This request is ALWAYS returned with pretty-print
  orionldState.kjsonP->spacesPerIndent   = 2;
  orionldState.kjsonP->nlString          = (char*) "\n";
  orionldState.kjsonP->stringBeforeColon = (char*) "";
  orionldState.kjsonP->stringAfterColon  = (char*) " ";

  orionldState.noLinkHeader              = true;  // We don't want the Link header for version requests

  return true;
}
