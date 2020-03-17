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
#include <string.h>                                            // strchr

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/mqtt/mqttParse.h"                            // Own interface



// ----------------------------------------------------------------------------
//
// mqttParse -
//
bool mqttParse(char* mqtt, char** hostP, unsigned short* portP, char** topicP, char** detailP)
{
  LM_TMP(("MQTT: In mqttCheck: '%s'", mqtt));

  if (!SCOMPARE7(mqtt, 'm', 'q', 't', 't', ':', '/', '/'))
  {
    if (detailP != NULL)
      *detailP = (char*) "protocol doesn't start with 'mqtt://'";
    return false;
  }

  // Step over "mqtt://"
  mqtt = &mqtt[7];
  LM_TMP(("MQTT: In mqttCheck (after stepping over the protocol): '%s'", mqtt));

  char* slash = strchr(mqtt, '/');

  if (slash == NULL)
  {
    if (detailP != NULL)
      *detailP = (char*) "no separator between IP:PORT amd TOPIC found";
    return false;
  }
  *slash = 0;
  LM_TMP(("MQTT: In mqttCheck (after removing slash): '%s'", mqtt));
  *topicP = &slash[1];

  // If ':' is found, an integer < MAXSHORT must come after it
  char* colon = strchr(mqtt, ':');
  if (colon != NULL)
  {
    char* portString = &colon[1];
    char* psP        = portString;

    *colon = 0;

    if (*psP == 0)
    {
      // We have a colon, but no port ...
      LM_W(("Bad Input (Invalid MQTT endpoint port - there is a colon, but the port number is missing)"));
      if (detailP != NULL)
        *detailP = (char*) "Invalid MQTT endpoint port - there is a colon, but the port number is missing";
      return false;
    }

    while (*psP != 0)
    {
      if ((*psP < '0') || (*psP > '9'))
      {
        LM_W(("Bad Input (Invalid MQTT endpoint port - must be a number"));
        if (detailP != NULL)
          *detailP = (char*) "Invalid MQTT endpoint port - must be a number";
        return false;
      }

      ++psP;
    }

    *portP = atoi(portString);
  }
  *hostP = mqtt;

  LM_TMP(("MQTT: In mqttCheck:"));
  LM_TMP(("MQTT: host:  %s", *hostP));
  LM_TMP(("MQTT: port:  %d", *portP));
  LM_TMP(("MQTT: topic: %s", *topicP));

  return true;
}
