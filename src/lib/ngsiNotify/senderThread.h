#ifndef SRC_LIB_NGSINOTIFY_SENDERTHREAD_H_
#define SRC_LIB_NGSINOTIFY_SENDERTHREAD_H_

/* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán Márquez
*/
#include <string>
#include <vector>
#include <map>

#include "common/MimeType.h"



/* ****************************************************************************
*
* QUEUE_MSG_NOTIF
* QUEUE_MSG_KILL
*/
#define QUEUE_MSG_NOTIF 0
#define QUEUE_MSG_KILL  1



/* ****************************************************************************
*
* SenderThreadParams - 
*/
typedef struct SenderThreadParams
{
  unsigned short                     type; // 0 -> regular notif, 1 -> kill thread
  std::string                        from;
  std::string                        ip;
  unsigned short                     port;
  std::string                        protocol;  // used to disinguish between mqtt and http/https notifications
  std::string                        verb;
  std::string                        tenant;
  long long                          maxFailsLimit;
  long long                          failsCounter;
  std::string                        servicePath;
  std::string                        xauthToken;
  std::string                        resource;      // path for HTTP notifications, topic for MQTT notifications
  unsigned int                       qos;           // used only in MQTT notifications
  std::string                        user;          // for user/pass auth connections (only MQTT at the present moment)
  std::string                        passwd;        // for user/pass auth connections (only MQTT at the present moment)
  std::string                        content_type;
  std::string                        content;
  char                               transactionId[64];
  MimeType                           mimeType;
  std::string                        renderFormat;
  std::string                        fiwareCorrelator;
  struct timespec                    timeStamp;
  std::map<std::string, std::string> extraHeaders;
  std::string                        subscriptionId;
  bool                               registration;
  long long                          timeout;
} SenderThreadParams;



/* ****************************************************************************
*
* startSenderThread -
*/
extern void* startSenderThread(void* paramsV);

#endif  // SRC_LIB_NGSINOTIFY_SENDERTHREAD_H_
