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

#include "ngsiNotify/senderThread.h"
#include "ngsiNotify/doNotify.h"


/* ****************************************************************************
*
* startSenderThread -
*/
void* startSenderThread(void* p)
{
  SenderThreadParams* paramsV = (SenderThreadParams*) p;

  // process paramV to send notification (freeing memory after use)
  // note NULL in queue statistics (it doesn't make sense in this case) and curl object (will be generated internally in doNotify)
  doNotify(paramsV, NULL, NULL, "sender-thread");

  pthread_exit(NULL);
  return NULL;
}
