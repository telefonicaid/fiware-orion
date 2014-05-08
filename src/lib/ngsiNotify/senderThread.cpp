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
 * fermin at tid dot es
 *
 * Author: Fermín Galán Márquez
 */

#include "senderThread.h"
#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "rest/clientSocketHttp.h"

/* ****************************************************************************
*
* startSenderThread -
*/
void* startSenderThread(void* p)
{
    SenderThreadParams* params = (SenderThreadParams*) p;

    LM_T(LmtNotifier, ("sending to: host='%s', port=%d, verb=%s, tenant='%s', path='%s', content-type: %s", 
                       params->ip.c_str(),
                       params->port,
                       params->verb.c_str(),
                       params->tenant.c_str(),
                       params->resource.c_str(),
                       params->content_type.c_str()));

    sendHttpSocket(params->ip,
                   params->port,
                   params->verb,
                   params->tenant,
                   params->resource,
                   params->content_type,
                   params->content,
                   NOTIFICATION_WAIT_MODE);

    /* Delete the parameters after using them */
    delete params;

    pthread_exit(NULL);
    return NULL;
}
