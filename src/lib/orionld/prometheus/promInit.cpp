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
extern "C"
{
#include "prometheus-client-c/prom/include/prom.h"          // Prometheus client lib
#include "prometheus-client-c/promhttp/include/promhttp.h"  // Prometheus client lib
}



// -----------------------------------------------------------------------------
//
// Prometheus counters, gauges and histograms
//
prom_counter_t*     promNgsildRequests;
prom_counter_t*     promNgsildRequestsFailed;
prom_counter_t*     promNotifications;
prom_counter_t*     promNotificationsFailed;
prom_gauge_t*       promTestGauge;
prom_histogram_t*   promTestHistogram;



// -----------------------------------------------------------------------------
//
// promInit - initialize the Prometheus client library
//
int promInit(unsigned short promPort)
{
  prom_collector_registry_default_init();

  promNgsildRequests       = prom_collector_registry_must_register_metric(prom_counter_new("ngsildRequests",       "# NGSILD Requests",        0, NULL));
  promNgsildRequestsFailed = prom_collector_registry_must_register_metric(prom_counter_new("ngsildRequestsFailed", "# Failed NGSILD Requests", 0, NULL));
  promNotifications        = prom_collector_registry_must_register_metric(prom_counter_new("notifications",        "# Notifications",          0, NULL));
  promNotificationsFailed  = prom_collector_registry_must_register_metric(prom_counter_new("notificationsFailed",  "# Failed Notifications",   0, NULL));

  promTestHistogram = prom_collector_registry_must_register_metric(prom_histogram_new(
                                                                     "promTestHistogram",
                                                                     "histogram under test",
                                                                     prom_histogram_buckets_linear(5.0, 5.0, 2),
                                                                     0,
                                                                     NULL));

  // Set the active registry for the HTTP handler
  promhttp_set_active_collector_registry(NULL);


  struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, promPort, NULL, NULL);

  return (daemon != NULL)? 0 : 1;
}
