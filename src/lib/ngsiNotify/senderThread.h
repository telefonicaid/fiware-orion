#ifndef SENDERTHREAD_H
#define SENDERTHREAD_H

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

#include "common/Format.h"



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
  std::string     ip;
  unsigned short  port;
  std::string     protocol;
  std::string     verb;
  std::string     tenant;
  std::string     servicePath;
  std::string     xauthToken;
  std::string     resource;
  std::string     content_type;
  std::string     content;
  char            transactionId[64];
  Format          format;
  std::string     renderFormat;
  std::string     fiwareCorrelator;
  struct timespec timeStamp;
} SenderThreadParams;



/* ****************************************************************************
*
* startSenderThread -
*/
extern void* startSenderThread(void* params);

#endif
