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
#include <stdlib.h>                                              // calloc, free
#include <semaphore.h>                                           // sem_init
#include <mongoc/mongoc.h>                                       // MongoDB C Client Driver

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/common/orionldState.h"                         // orionldState, mongocPool, ...
#include "orionld/common/tenantList.h"                           // tenant0 - the default tenant
#include "orionld/mongoc/mongocTenantsGet.h"                     // mongocTenantsGet
#include "orionld/mongoc/mongocGeoIndexInit.h"                   // mongocGeoIndexInit
#include "orionld/mongoc/mongocIdIndexCreate.h"                  // mongocIdIndexCreate
#include "orionld/mongoc/mongocInit.h"                           // Own interface



// -----------------------------------------------------------------------------
//
// mongocLog -
//
static void mongocLog
(
  mongoc_log_level_t  level,
  const char*         domain,
  const char*         msg,
  void*               userData
)
{
  if (level == MONGOC_LOG_LEVEL_CRITICAL)
    LM_E(("MONGOC[%s]: %s", domain, msg));  // Perhaps even LM_X ?
  else if (level == MONGOC_LOG_LEVEL_ERROR)
    LM_E(("MONGOC[%s]: %s", domain, msg));
  else if (level == MONGOC_LOG_LEVEL_WARNING)
    LM_W(("MONGOC[%s]: %s", domain, msg));
  else if (level == MONGOC_LOG_LEVEL_MESSAGE)
    LM_M(("MONGOC[%s]: %s", domain, msg));
  else if (level == MONGOC_LOG_LEVEL_INFO)
    LM_I(("MONGOC[%s]: %s", domain, msg));
  else if ((level == MONGOC_LOG_LEVEL_DEBUG) || (level ==  MONGOC_LOG_LEVEL_TRACE))
  {
    // if (mongo log is on)
    //   LM_M(("MONGOC[%s]: %s", domain, msg));
  }
}



// -----------------------------------------------------------------------------
//
// uriCompose -
//
// All options for connection to the built-in mongoc connection pool are CLI options for Orion-LD
// and thus global variables in orionldState.h/cpp:
// - dbHost
// - dbUser
// - dbPwd
// - dbAuthDb
// - rplSet
// - dbAuthMech
// - dbSSL
//
// The URI for the connection looks like this:
//   mongodb://[dbUser:dbPwd@]dbHost/[dbAuthDb]? [replicaSet=dbReplicaSet] [&authMechanism=dbAuthMech] [&tls=true&tlsAllowInvalidCertificates=true]
//
// Note that dbHost can be a comma-separated list of hostnames (and ports)
//
//
// About Connecting to a server over TLS
//
// Orion does this:
//   if (dbSSL)
//     uri += optionPrefix + "tls=true&tlsAllowInvalidCertificates=true";
//
// So, no certificate is used.
// Not sure that's the best idea though ...
//
// http://mongoc.org/libmongoc/current/advanced-connections.html:
// -----------------------------------------------------------------------------------------------------------------------------------------------
//   MongoDB requires client certificates by default, unless the --tlsAllowConnectionsWithoutCertificates is provided.
//
//   The C Driver can be configured to present a client certificate using the URI option tlsCertificateKeyFile,
//   which may be referenced through the constant MONGOC_URI_TLSCERTIFICATEKEYFILE:
//
//     mongoc_client_t*  client = NULL;
//     mongoc_uri_t*     uri    = mongoc_uri_new("mongodb://localhost:27017/?tls=true");
//     mongoc_uri_set_option_as_utf8(uri, MONGOC_URI_TLSCERTIFICATEKEYFILE, "client.pem");  // "client.pem" would be a CLI for Orion-LD
//     client = mongoc_client_new_from_uri(uri);
// -----------------------------------------------------------------------------------------------------------------------------------------------
//
//
// Apart from what I've implemented so far:
// * Connecting to a UNIX Domain Socket:    mongoc_uri_new("mongodb://%2Ftmp%2Fmongodb-27017.sock");
// * Compressing data to and from MongoDB:  mongoc_client_new("mongodb://localhost:27017/?compressors=snappy,zlib,zstd");
// * Disable Retry Writes                   mongodb://localhost/?retryWrites=false
// * Connection timeout                     mongodb://localhost/?connectTimeoutMS=<millisecs>
// * SRV record:                            mongodb+srv://localhost ...
// * And another 100 options or so:         See complete list here: http://mongoc.org/libmongoc/current/mongoc_uri_t.html
//
// So, perhaps I should just NOT try to fix the URI myself, just use it as is and let the users define the URI ...
// Makes no sense to implement that myself, and check for everything - will just add bugs, for nothing
// Default would be "mongodb://localhost" (port 27017 is already default) and if a user wants something else,
// just pass the URI when starting the broker.
//
// BTW, Orion's tlsallowinvalidcertificates is DEPRECATED.
//
static char* uriCompose
(
  char* dbURI,
  char* dbHost,
  char* dbUser,
  char* dbPwd,
  char* dbAuthDb,
  char* dbReplicaSet,
  char* dbAuthMechanism,
  bool  dbSSL,
  char* tlsCertificateFilePath
)
{
  char* compV[50];
  int   compNo = 0;

#if 0
  LM(("dbURI:           '%s'", dbURI));
  LM(("dbHost:          '%s'", dbHost));
  LM(("dbUser:          '%s'", dbUser));
  LM(("dbPwd:           '%s'", dbPwd));
  LM(("dbAuthDb:        '%s'", dbAuthDb));
  LM(("dbReplicaSet:    '%s'", dbReplicaSet));
  LM(("dbAuthMechanism: '%s'", dbAuthMechanism));
  LM(("dbSSL:           '%s'", (dbSSL == true)? "true" : "false"));
  LM(("tlsCertificate:  '%s'", tlsCertificateFilePath));
#endif

  if (dbURI[0] != 0)
  {
    //
    // OK, the complete URI comes in dbURI
    //
    // Is "${PWD}" present?
    // If so, split dbURI into two parts - before and after "${PWD}" and add dbPwd in between
    //
    char* pwdP = strstr(dbURI, "${PWD}");
    if (pwdP != NULL)
    {
      if (dbPwd[0] == 0)
        LM_X(1, ("Invalid Command Line Options: -dbURI is used with a password substitution, but no password (-dbPwd) is supplied"));

      compV[compNo++] = dbURI;
      *pwdP = 0;
      compV[compNo++] = dbPwd;
      compV[compNo++] = &pwdP[6];
    }
    else
      compV[compNo++] = dbURI;

    compV[compNo++] = (char*) "&";
  }
  else
  {
    compV[compNo++] = (char*) "mongodb://";

    if ((dbUser != NULL) && (dbUser[0] != 0))
    {
      compV[compNo++] = dbUser;
      compV[compNo++] = (char*) ":";
      compV[compNo++] = dbPwd;
      compV[compNo++] = (char*) "@";
    }

    compV[compNo++] = dbHost;
    compV[compNo++] = (char*) "/";

    if ((dbAuthDb != NULL) && (dbAuthDb[0] != 0))
      compV[compNo++] = dbAuthDb;

    compV[compNo++] = (char*) "?";

    if ((dbReplicaSet != NULL) && (dbReplicaSet[0] != 0))
    {
      compV[compNo++] = (char*) "replicaSet=";
      compV[compNo++] = dbReplicaSet;
      compV[compNo++] = (char*) "&";
    }

    if ((dbAuthMechanism != NULL) && (dbAuthMechanism[0] != 0))
    {
      compV[compNo++] = (char*) "authMechanism=";
      compV[compNo++] = dbAuthMechanism;
      compV[compNo++] = (char*) "&";
    }

    if (dbSSL == true)
    {
      if (tlsCertificateFilePath == NULL)
        compV[compNo++] = (char*) "&tls=true&tlsAllowConnectionsWithoutCertificates=true&";
      else
      {
        compV[compNo++] = (char*) "&tls=true&tlsCertificateKeyFile=";
        compV[compNo++] = tlsCertificateFilePath;
      }
    }
  }


  //
  // Count all string length and get the total length of the URI
  //
  int uriLen = 0;

  for (int ix = 0; ix < compNo; ix++)
  {
    uriLen += strlen(compV[ix]);
  }

  char* uri = (char*) calloc(1, uriLen + 1);
  if (uri == NULL)
    LM_X(1, ("Out of memory allocating a mongo connection URI of length %d", uriLen + 1));

  int len = 0;
  for (int ix = 0; ix < compNo; ix++)
  {
    strncpy(&uri[len], compV[ix], uriLen - len);
    len += strlen(compV[ix]);
  }

  // LM(("mongo uri: %s", uri));
  return uri;
}



// -----------------------------------------------------------------------------
//
// mongocInit -
//
void mongocInit
(
  char* dbURI,
  char* dbHost,
  char* dbUser,
  char* dbPwd,
  char* dbAuthDb,
  char* dbReplicaSet,
  char* dbAuthMechanism,
  bool  dbSSL,
  char* tlsCertificateFilePath
)
{
  //
  // Redirect mongoc log messages from stdout to OrionLD's own log file
  //
  mongoc_log_set_handler(mongocLog, NULL);

  //
  // Initialize libmongoc's internals
  //
  mongoc_init();

  //
  // Create a MongoDB URI object from the given connection parameters
  //
  char*         mongoUri = uriCompose(dbURI, dbHost, dbUser, dbPwd, dbAuthDb, dbReplicaSet, dbAuthMechanism, dbSSL, tlsCertificateFilePath);
  bson_error_t  mongoError;

  LM_K(("Connecting to mongo for the C driver"));
  mongocUri = mongoc_uri_new_with_error(mongoUri, &mongoError);
  if (mongocUri == NULL)
    LM_X(1, ("Unable to connect to mongo(URI: %s): %s", mongoUri, mongoError.message));

  //
  // Initialize the connection pool
  //
  mongocPool = mongoc_client_pool_new(mongocUri);

  //
  // We want the newer, better, error handling
  //
  mongoc_client_pool_set_error_api(mongocPool, 2);


  //
  // Semaphore for the 'contexts' collection on DB 'orionld' - hopefully not needed in the end ...
  //
  sem_init(&mongocContextsSem, 0, 1);  // 0: shared between threads of the same process. 1: free to be taken

  if (mongocTenantsGet() == false)
    LM_X(1, ("Unable to extract tenants from the database - fatal error"));

  if (mongocGeoIndexInit() == false)
    LM_X(1, ("Unable to initialize geo indices in database - fatal error"));

  if (mongocIdIndexCreate(&tenant0) == false)
    LM_W(("Unable to create the index on Entity ID on the default database"));

  // Free the uri, allocated by uriCompose
  free(mongoUri);
}
