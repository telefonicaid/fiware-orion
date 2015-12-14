/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan
*/
#include <string>
#include <stdio.h>
#include <unistd.h>      // needed for sleep in Debian 7

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/sem.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/mongoOntimeintervalOperations.h"
#include "ngsi/Duration.h"
#include "ngsi/NotifyCondition.h"
#include "ngsiNotify/ContextSubscriptionInfo.h"
#include "ngsiNotify/Notifier.h"
#include "ngsiNotify/onTimeIntervalThread.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* doNotification -
*/
static void doNotification(OnIntervalThreadParams* params, const std::string& tenant)
{
    std::string err;

    /* Get document from database. Note that although we can include some parameters in
     * OnIntervalThreadParams coming for the csubs document at thread creation time
     * from mongoBackend, we always needs that to get fresh lastNotification */
    ContextSubscriptionInfo csi;
    if (mongoGetContextSubscriptionInfo(params->subId, &csi, &err, tenant) != SccOk)
    {
      //
      // FIXME P6: mongoGetContextSubscriptionInfo ALWAYS returns SccOk
      //           github issue #575.
      //           If it didn't we would have a leak right here, as mongoGetContextSubscriptionInfo
      //           allocates entities and pushes them to 'csi', which is lost leaving this function.
      //
      alarmMgr.dbError("error invoking mongoGetContextSubscriptionInfo");
      return;
    }
    alarmMgr.dbErrorReset();

    int current = getCurrentTime();

    /* Send notification (only if subscription is not expired and there is actual data)*/
    if (current < csi.expiration)
    {
        /* Throttling check (only if throttling is used and at least one notification has been sent) */
        if (csi.throttling < 0 || csi.lastNotification < 0 || csi.lastNotification + csi.throttling <  current)
        {
            /* Query database for data */
            NotifyContextRequest ncr;

            // FIXME P7: mongoGetContextElementResponses ALWAYS returns SccOk !!!
            if (mongoGetContextElementResponses(csi.entityIdVector, csi.attributeList, &(ncr.contextElementResponseVector), &err, tenant) != SccOk)
            {
              csi.release();
              ncr.contextElementResponseVector.release();
              alarmMgr.dbError("error invoking mongoGetContextElementResponses");
              return;
            }
            alarmMgr.dbErrorReset();

            if (ncr.contextElementResponseVector.size() > 0)
            {
                /* Complete NotifyContextRequest */
                // FIXME: implement a proper originator string
                ncr.originator.set("localhost");
                ncr.subscriptionId.set(params->subId);

                // Update database fields due to new notification
                HttpStatusCode s = mongoUpdateCsubNewNotification(params->subId, &err, tenant);
                if (s == SccOk)
                {
                  // FIXME P6: Note that the X-Auth-Token is left blank in this case.
                  //           In the future, the ONTIMEINTERVAL notification struture *could* include it
                  //           (and a TRUST_TOKEN to re-negotiate X-Auth-Token if it gets expired)"
                  //
                  params->notifier->sendNotifyContextRequest(&ncr, csi.url, tenant, "", csi.format);
                }

                ncr.contextElementResponseVector.release();
                csi.release();
            }
            else
            {
              LM_T(LmtNotifier, ("notification not sent due to empty context elements response vector)"));
            }
        }
        else
        {
          LM_T(LmtNotifier, ("notification not sent due to throttling (current time: %d)", current));
        }
    }

    //
    // FIXME P3: mongoGetContextSubscriptionInfo allocates an EntityId and pushes it to csi.
    //           github issue #576.
    //           Here that entity is released, but this is running in a separate thread.
    //           Sometimes, when the broker is killed, this thread is inside the mongoGetContextSubscriptionInfo
    //           and the new entityId is considered a memory leak by valgrind.
    //           This is no mempry leak, Everything is under control, but as Jenkins complains about a leak,
    //           it is annoying.
    //           What we need to do is to make the broker die in a more controlled manner, making the threads
    //           exit and wait for them to exit, before the broker itself exits.
    //           This should be done when receiving an /exit (only valid for DEBUG compilations).
    //
    csi.release();
}

/* ****************************************************************************
*
* startOnIntervalThread -
*
* About pthread_setcancelstate:
* The problem with this ontimeinterval subscription thread is that during valgrind tests,
* the broker if started and killed and if this thread is running, it it sometimes cancelled 
* with buffers allocated and valgrind will flag this as a memory leak.
* Using the function pthread_setcancelstate, we can decide when the thread is cancelable and
* thus avoid to be cancelled with memory allocated.
* The function 'doNotification' allocates and releases its own memory buffers so if this thread
* is never cancelled inside 'doNotification', this "false leak" should disappear.
* 
*/
void* startOnIntervalThread(void* p)
{
  OnIntervalThreadParams* params = (OnIntervalThreadParams*) p;
  int                     oldState;

  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &oldState);

  while (true)
  {
    strncpy(transactionId, "N/A", sizeof(transactionId));
    LM_T(LmtNotifier, ("ONTIMEINTERVAL thread wakes up (%s)", params->subId.c_str()));

    // Do the work (we put this in a function as error conditions would produce an
    // early interruption of the process)
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldState);
    doNotification(params, params->tenant);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldState);

    sleep(params->interval);
  }

  return NULL;
}
