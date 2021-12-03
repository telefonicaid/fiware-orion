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
* NOTIFICATION_WAIT_MODE - 
*/
#define NOTIFICATION_WAIT_MODE false



/* ****************************************************************************
*
* SenderThreadParams - 
*/
typedef struct SenderThreadParams
{
  std::string                        ip;
  unsigned short                     port;
  std::string                        protocol;
  std::string                        verb;
  std::string                        tenant;
  std::string                        servicePath;
  std::string                        xauthToken;
  std::string                        resource;
  std::string                        content_type;
  std::string                        content;
  char                               transactionId[68];
  MimeType                           mimeType;
  char*                              toFree;             // Payload Data buffer has been allocated
  std::string                        renderFormat;
  std::string                        fiwareCorrelator;
  struct timespec                    timeStamp;
  std::map<std::string, std::string> extraHeaders;
  std::string                        subscriptionId;
  bool                               registration;
  int                                mqttQoS;
  char                               mqttVersion[18];
  char                               mqttUserName[130];
  char                               mqttPassword[130];
} SenderThreadParams;



/* ****************************************************************************
*
* startSenderThread -
*/
extern void* startSenderThread(void* paramsV);

#endif  // SRC_LIB_NGSINOTIFY_SENDERTHREAD_H_
