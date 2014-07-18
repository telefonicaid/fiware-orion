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
* fermin at tid dot es
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

/* ****************************************************************************
*
* doNotification -
*/
static void doNotification(OnIntervalThreadParams* params, const std::string& tenant) {

    std::string err;

    /* Get document from database. Note that although we can include some parameters in
     * OnIntervalThreadParams coming for the csubs document at thread creation time
     * from mongoBackend, we always needs that to get fresh lastNotification */
    ContextSubscriptionInfo csi;
    if (mongoGetContextSubscriptionInfo(params->subId, &csi, &err, tenant) != SccOk)
    {
      LM_E(("Database Error (error invoking mongoGetContextSubscriptionInfo"));
      return;
    }

    int current = getCurrentTime();

    /* Send notification (only if subscription is not expired and there is actual data)*/
    if (current < csi.expiration) {

        /* Throttling check (only if throttling is used and at least one notification has been sent) */
        if (csi.throttling < 0 || csi.lastNotification < 0 || csi.lastNotification + csi.throttling <  current) {

            /* Query database for data */
            NotifyContextRequest ncr;
            // FIXME P7: mongoGetContextElementResponses ALWAYS returns SccOk !!!
            if (mongoGetContextElementResponses(csi.entityIdVector, csi.attributeList, &(ncr.contextElementResponseVector), &err, tenant) != SccOk)
            {
              csi.release();
              ncr.contextElementResponseVector.release();
              LM_E(("Database Error (error invoking mongoGetContextElementResponses"));
              return;
            }

            if (ncr.contextElementResponseVector.size() > 0)
            {
                // New transactionId for each notification
                transactionIdSet();

                /* Complete NotifyContextRequest */
                // FIXME: implement a proper originator string
                ncr.originator.set("localhost");
                ncr.subscriptionId.set(params->subId);

                LM_I(("Starting transaction to %s", csi.url.c_str()));
                // Update database fields due to new notification
                HttpStatusCode s = mongoUpdateCsubNewNotification(params->subId, &err, tenant);
                if (s == SccOk)
                {
                  params->notifier->sendNotifyContextRequest(&ncr, csi.url, tenant, csi.format);
                }

                ncr.contextElementResponseVector.release();
                csi.release();
            }
            else {
                LM_T(LmtNotifier, ("notification not sent due to empty context elements response vector)"));
            }
        }
        else {
            LM_T(LmtNotifier, ("notification not sent due to throttling (current time: %d)", current));
        }
    }

    csi.release();
}

/* ****************************************************************************
*
* startOnIntervalThread -
*/
void* startOnIntervalThread(void* p)
{
    OnIntervalThreadParams* params = (OnIntervalThreadParams*) p;

    while (true)
    {
        strncpy(transactionId, "N/A", sizeof(transactionId));
        LM_T(LmtNotifier, ("ONTIMEINTERVAL thread wakes up (%s)", params->subId.c_str()));

        // Do the work (we put this in a function as error conditions would produce an
        // early interruption of the process)
        doNotification(params, params->tenant);

        // Sleeps for interval
        sleep(params->interval);
    }

    return NULL;
}
