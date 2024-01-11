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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>                                         // curl_version_info_data, curl_version_info, CURLversion
#include <boost/version.hpp>                                   // BOOST_LIB_VERSION
#include <microhttpd.h>                                        // MHD_VERSION (returns a number)
#include <openssl/opensslv.h>                                  // OPENSSL_VERSION_TEXT
#include <mongo/version.h>                                     // MONGOCLIENT_VERSION
#include <rapidjson/rapidjson.h>                               // RAPIDJSON_VERSION_STRING
#include <mongoc/mongoc.h>                                     // MONGOC_VERSION_S
#include <bson/bson.h>                                         // BSON_VERSION_S

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

#include "cache/subCache.h"                                    // subCacheItems

#include "orionld/common/orionldState.h"                       // orionldState, orionldVersion, postgresServerVersion, mongocServerVersion
#include "orionld/common/branchName.h"                         // ORIONLD_BRANCH
#include "orionld/common/pqHeader.h"                           // Postgres header
#include "orionld/context/orionldCoreContext.h"                // orionldCoreContextP
#include "orionld/troe/pgVersionGet.h"                         // pgVersionToString
#include "orionld/mqtt/mqttConnectionList.h"                   // Mqtt Connection List
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
bool orionldGetVersion(void)
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
  nodeP = kjString(orionldState.kjsonP, "based on orion", "1.15.0-next");
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

  // Direct libraries

  // microhttpd
  nodeP = kjString(orionldState.kjsonP, "microhttpd version", mhdVersion);
  kjChildAdd(orionldState.responseTree, nodeP);

  // rapidjson
  nodeP = kjString(orionldState.kjsonP, "rapidjson version", RAPIDJSON_VERSION_STRING);
  kjChildAdd(orionldState.responseTree, nodeP);

  // curl
  curlVersionP = curl_version_info(CURLVERSION_NOW);
  if (curlVersionP != NULL)
    nodeP = kjString(orionldState.kjsonP, "libcurl version", curlVersionP->version);
  else
    nodeP = kjString(orionldState.kjsonP, "libcurl version", "UNKNOWN");
  kjChildAdd(orionldState.responseTree, nodeP);

  // libuuid
  nodeP = kjString(orionldState.kjsonP, "libuuid version", "UNKNOWN");
  kjChildAdd(orionldState.responseTree, nodeP);

  // mongocpp
  nodeP = kjString(orionldState.kjsonP, "mongocpp version", mongo::client::kVersionString);
  kjChildAdd(orionldState.responseTree, nodeP);

  // mongoc
  nodeP = kjString(orionldState.kjsonP, "mongoc version", MONGOC_VERSION_S);
  kjChildAdd(orionldState.responseTree, nodeP);

  // bson
  nodeP = kjString(orionldState.kjsonP, "bson version", BSON_VERSION_S);
  kjChildAdd(orionldState.responseTree, nodeP);


  //
  // Mongo Server
  //
  nodeP = kjString(orionldState.kjsonP, "mongodb server version", mongocServerVersion);
  kjChildAdd(orionldState.responseTree, nodeP);


  // Libs needed by the "direct libs"
  nodeP = kjString(orionldState.kjsonP, "boost version", BOOST_LIB_VERSION);
  kjChildAdd(orionldState.responseTree, nodeP);
  nodeP = kjString(orionldState.kjsonP, "openssl version", OPENSSL_VERSION_TEXT);
  kjChildAdd(orionldState.responseTree, nodeP);

  if (troe)
  {
    //
    // Postgres Client Lib
    //
    int     pgLibVersion = PQlibVersion();
    char    pgLibVersionString[32];

    pgVersionToString(pgLibVersion, pgLibVersionString, sizeof(pgLibVersionString));
    nodeP = kjString(orionldState.kjsonP, "postgres libpq version", pgLibVersionString);
    kjChildAdd(orionldState.responseTree, nodeP);

    //
    // Postgres Server
    //
    nodeP = kjString(orionldState.kjsonP, "postgres server version", postgresServerVersion);
    kjChildAdd(orionldState.responseTree, nodeP);
  }

  // Branch
  nodeP = kjString(orionldState.kjsonP, "branch", ORIONLD_BRANCH);
  kjChildAdd(orionldState.responseTree, nodeP);

  // Number of items in the subscription cache
  nodeP = kjInteger(orionldState.kjsonP, "cached subscriptions", subCacheItems());
  kjChildAdd(orionldState.responseTree, nodeP);

  // Version of the core context
  nodeP = kjString(orionldState.kjsonP, "Core Context", orionldCoreContextP->url);
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

  int mqttConnections = 0;
  for (int ix = 0; ix < mqttConnectionListIx; ix++)
  {
    if (mqttConnectionList[ix].host != NULL)
      ++mqttConnections;
  }

  if (mqttConnections > 0)
  {
    nodeP = kjInteger(orionldState.kjsonP, "MQTT Connections", mqttConnections);
    kjChildAdd(orionldState.responseTree, nodeP);
  }

  // This request is ALWAYS returned with pretty-print
  orionldState.uriParams.prettyPrint     = true;
  orionldState.kjsonP->spacesPerIndent   = 2;
  orionldState.kjsonP->nlString          = (char*) "\n";
  orionldState.kjsonP->stringBeforeColon = (char*) "";
  orionldState.kjsonP->stringAfterColon  = (char*) " ";

  orionldState.noLinkHeader              = true;  // We don't want the Link header for version requests

  return true;
}
