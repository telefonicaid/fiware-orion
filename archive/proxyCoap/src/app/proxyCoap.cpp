/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: TID Developer
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <vector>
#include <unistd.h>                             // getppid, for, setuid, etc.
#include <fcntl.h>                              // open
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <boost/thread.hpp>

#include "parseArgs/parseArgs.h"
#include "parseArgs/paConfig.h"
#include "parseArgs/paBuiltin.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "rest/rest.h"

#include "common/sem.h"
#include "common/globals.h"
#include "common/Timer.h"
#include "common/compileInfo.h"


#include "common/string.h"

#include "proxyCoap/CoapController.h"
#include "proxyCoap/version.h"

/* ****************************************************************************
*
* Option variables
*/
bool            fg;
char            bindAddress[MAX_LEN_IP];
int             port;
char            cbHost[64];
int             cbPort;
bool            useOnlyIPv4;
bool            useOnlyIPv6;

/* ****************************************************************************
*
* parse arguments
*/
PaArgument paArgs[] =
{
  { "-fg",           &fg,           "FOREGROUND",      PaBool,   PaOpt, false,          false,  true,  "don't start as daemon"                     },
  { "-localIp",      bindAddress,   "LOCALIP",         PaString, PaOpt, _i "0.0.0.0",   PaNL,   PaNL,  "IP to receive new connections"             },
  { "-port",         &port,         "PORT",            PaInt,    PaOpt, 5683,           PaNL,   PaNL,  "port to receive new connections"           },

  { "-cbHost",       cbHost,         "FWD_HOST",       PaString, PaOpt, _i "localhost", PaNL,   PaNL,  "host for forwarding CoAP requests"         },
  { "-cbPort",       &cbPort,        "FWD_PORT",       PaInt,    PaOpt, 1026,              0,  65000,  "HTTP port for forwarding CoAP requests"    },

  { "-ipv4",         &useOnlyIPv4,  "USEIPV4",         PaBool,   PaOpt, false,          false,  true,  "use ip v4 only"                            },
  { "-ipv6",         &useOnlyIPv6,  "USEIPV6",         PaBool,   PaOpt, false,          false,  true,  "use ip v6 only"                            },


  PA_END_OF_ARGS
};

/* ****************************************************************************
*
* sigHandler -
*/
void sigHandler(int sigNo)
{
  LM_T(("In sigHandler - caught signal %d", sigNo));

  switch (sigNo)
  {
  case SIGINT:
  case SIGTERM:
    LM_X(1, ("Received signal %d", sigNo));
    break;
  }
}

const char* description =
   "\n"
   "proxyCoap version details:\n"
   "  version:            " PROXYCOAP_VERSION"\n"
   "  git hash:           " GIT_HASH         "\n"
   "  compile time:       " COMPILE_TIME     "\n"
   "  compiled by:        " COMPILED_BY      "\n"
   "  compiled in:        " COMPILED_IN      "\n";



/* ****************************************************************************
*
* daemonize -
*/
void daemonize(void)
{
  pid_t  pid;
  pid_t  sid;

  // already daemon
  if (getppid() == 1)
    return;

  pid = fork();
  if (pid == -1)
    LM_X(1, ("fork: %s", strerror(errno)));

  // Exiting father process
  if (pid > 0)
    exit(0);

  // Change the file mode mask */
  umask(0);

  // Removing the controlling terminal
  sid = setsid();
  if (sid == -1)
    LM_X(1, ("setsid: %s", strerror(errno)));

  // Change current working directory.
  // This prevents the current directory from being locked; hence not being able to remove it.
  if (chdir("/") == -1)
    LM_X(1, ("chdir: %s", strerror(errno)));
}



/* ****************************************************************************
*
* main -
*/
int main(int argC, char* argV[])
{

  signal(SIGINT,  sigHandler);
  signal(SIGTERM, sigHandler);

  paConfig("remove builtin", "-d");
  paConfig("remove builtin", "-r");
  paConfig("remove builtin", "-w");
  paConfig("remove builtin", "-F");
  paConfig("remove builtin", "-B");
  paConfig("remove builtin", "-b");
  paConfig("remove builtin", "-?");
  paConfig("remove builtin", "-toDo");
  paConfig("remove builtin", "-lmnc");
  paConfig("remove builtin", "-lmca");
  paConfig("remove builtin", "-lmkl");
  paConfig("remove builtin", "-lmll");
  paConfig("remove builtin", "-assert");
  paConfig("remove builtin", "-version");
  paConfig("remove builtin", "-h");
  paConfig("remove builtin", "-help");

  paConfig("man synopsis",                  (void*) "[options]");
  paConfig("man shortdescription",          (void*) "Options:");
  paConfig("man description",               (void*) description);
  paConfig("man author",                    (void*) "Telefonica I+D");
  paConfig("man exitstatus",                (void*) "proxyCoap is a daemon. If it exits, something is wrong ...");
  paConfig("man version",                   (void*) PROXYCOAP_VERSION);
  paConfig("log to screen",                 (void*) true);
  paConfig("log to file",                   (void*) true);
  paConfig("log file line format",          (void*) "TYPE:DATE:EXEC-AUX/FILE[LINE] FUNC: TEXT");
  paConfig("screen line format",            (void*) "TYPE@TIME  FUNC[LINE]: TEXT");
  paConfig("builtin prefix",                (void*) "PROXYCOAP_");
  paConfig("usage and exit on any warning", (void*) true);

  paParse(paArgs, argC, (char**) argV, 1, false);

  if (fg == false)
    daemonize();

  CoapController* cc = new CoapController(bindAddress, cbPort, port);
  cc->serve();
}

