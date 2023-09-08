/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include <unistd.h>                                              // close
#include <sys/uio.h>                                             // iovec, writev

#include "logMsg/logMsg.h"                                       // LM

#include "orionld/common/orionldServerConnect.h"                 // orionldServerConnect
#include "orionld/notifications/notificationFailure.h"           // notificationFailure
#include "cache/CachedSubscription.h"                            // CachedSubscription



// -----------------------------------------------------------------------------
//
// httpNotify - send a notification over http
//
int httpNotify
(
  CachedSubscription*  cSubP,
  PernotSubscription*  pSubP,
  const char*          subscriptionId,
  const char*          ip,
  unsigned short       port,
  const char*          path,
  struct iovec*        ioVec,
  int                  ioVecLen,
  double               notificationTime
)
{
  // Connect
  LM_T(LmtNotificationSend, ("%s: Connecting to notification '%s:%d' receptor for HTTP notification", subscriptionId, ip, port));
  int fd = orionldServerConnect(ip, port);

  if (fd == -1)
  {
    LM_E(("Internal Error (unable to connect to server for notification for subscription '%s': %s)", subscriptionId, strerror(errno)));
    notificationFailure(cSubP, pSubP, "Unable to connect to notification endpoint", notificationTime);
    return -1;
  }

  LM_T(LmtNotificationSend, ("%s: Connected to notification receptor '%s:%d' on fd %d", subscriptionId, ip, port, fd));

  if (lmTraceIsSet(LmtNotificationHeaders) == true)
  {
    for (int ix = 0; ix < ioVecLen - 1; ix++)
    {
      LM_T(LmtNotificationHeaders, ("%s: Notification Request Header: '%s'", subscriptionId, ioVec[ix].iov_base));
    }
  }

  LM_T(LmtNotificationBody, ("%s: Notification Request Body: %s", subscriptionId, ioVec[ioVecLen - 1].iov_base));

  // Send
  int nb;
  if ((nb = writev(fd, ioVec, ioVecLen)) == -1)
  {
    close(fd);

    LM_E(("Internal Error (unable to send to server for notification for subscription '%s' (fd: %d): %s", subscriptionId, fd, strerror(errno)));
    notificationFailure(cSubP, pSubP, "Unable to write to notification endpoint", notificationTime);
    return -1;
  }

  LM_T(LmtNotificationSend, ("%s: Written %d bytes to fd %d of %s:%d", subscriptionId, nb, fd, ip, port));

  return fd;
}
