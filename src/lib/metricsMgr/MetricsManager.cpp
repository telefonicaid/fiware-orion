/*
*
* Copyright 2016 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include <stdint.h>   // int64_t et al
#include <sys/time.h>

#include <utility>
#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/JsonHelper.h"
#include "common/defaultValues.h"
#include "rest/rest.h"
#include "rest/RestService.h"
#include "metricsMgr/MetricsManager.h"



/* ****************************************************************************
*
* MetricsManager::MetricsManager -
*/
MetricsManager::MetricsManager(): on(false), semWaitStatistics(false), semWaitTime(0)
{
}



/* ****************************************************************************
*
* MetricsManager::init -
*
* NOTE
*   The semaphore is created even though the metrics manager is not turned on.
*   It's only one sys-call, and this way, the broker is prepared to receive 'on/off'
*   via REST.
*/
bool MetricsManager::init(bool _on, bool _semWaitStatistics)
{
  on                 = _on;
  semWaitStatistics  = _semWaitStatistics;

  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_E(("Runtime Error (error initializing 'metrics mgr' semaphore: %s)", strerror(errno)));
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* MetricsManager::semTake - 
*/
void MetricsManager::semTake(void)
{
  if (semWaitStatistics)
  {
    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);
    sem_wait(&sem);
    gettimeofday(&end, NULL);

    // Add semaphore waiting time to the accumulator (semWaitTime is in microseconds)
    semWaitTime += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
  }
  else
  {
    sem_wait(&sem);
  }
}



/* ****************************************************************************
*
* MetricsManager::semGive - 
*/
void MetricsManager::semGive(void)
{
  sem_post(&sem);
}



/* ****************************************************************************
*
* MetricsManager::semWaitTimeGet - 
*/
int64_t MetricsManager::semWaitTimeGet(void)
{
  return semWaitTime;
}



/* ****************************************************************************
*
* MetricsManager::add -
*/
void MetricsManager::add(const std::string& srv, const std::string& subServ, const std::string& metric, uint64_t value)
{
  if (on == false)
  {
    return;
  }

  // FIXME P4: See github issue #2781
  if (tenantCheck(srv) != "OK")
  {
    return;
  }

  // FIXME P4: See github issue #2781
  ConnectionInfo ci;
  ci.httpHeaders.servicePathReceived = true;
  if (servicePathCheck(&ci, subServ.c_str()) != 0)
  {
    return;
  }

  //
  // Exclude the first '/' from the Sub Service
  // But, only if it starts with a '/'
  //
  // Also, if service path ends in '/#', cancel out that part
  //
  // Note that for the 'cancel out' part we need to allocate a copy of
  // the service path (and frre it after usage).
  //
  char* subService;

  if (subServ.c_str()[0] == '/')
  {
    subService = strdup(&subServ.c_str()[1]);
  }
  else
  {
    subService = strdup(subServ.c_str());
  }

  //
  // Now, if service path ends in '/#', cancel out that part
  //
  int subServiceLen = strlen(subService);

  if ((subServiceLen >= 2) && (subService[subServiceLen - 1] == '#') && (subService[subServiceLen - 2] == '/'))
  {
    subService[subServiceLen - 2] = 0;
  }

  // Need to cover the '/#' case as well (only '#' left as the first '/' has been skipped already)
  if ((subService[0] == '#') && (subService[1] == 0))
  {
    subService[0] = 0;
  }

  semTake();


  // Do we have the service in the map?
  if (metrics.find(srv) == metrics.end())
  {
    // not found: create it
    metrics[srv] = new std::map<std::string, std::map<std::string, uint64_t>*>;
  }

  // Do we have the subservice in the map?
  if (metrics[srv]->find(subService) == metrics[srv]->end())
  {
    //
    // Not Found: create it
    //
    metrics[srv]->insert(std::pair<std::string, std::map<std::string, uint64_t>*>
                         (subService,
                          new std::map<std::string, uint64_t>));
  }

  // Do we have the metric in the map?
  if (metrics[srv]->at(subService)->find(metric) == metrics[srv]->at(subService)->end())
  {
    //
    // Not Found: create it
    //
    metrics[srv]->at(subService)->insert(std::pair<std::string, uint64_t>(metric, 0));
  }

  metrics[srv]->at(subService)->at(metric) += value;

  semGive();

  free(subService);
}



/* ****************************************************************************
*
* MetricsManager::_reset -
*/
void MetricsManager::_reset(void)
{
  //
  // Three iterators needed to iterate over the 'triple-map' metrics:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //   metricIter       to iterate over all metrics of a sub-service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, uint64_t>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, uint64_t>*>::iterator                          subServiceIter;
  std::map<std::string, uint64_t>::iterator                                                  metricIter;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    std::map<std::string, std::map<std::string, uint64_t>*>* servMap  = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      std::map<std::string, uint64_t>* metricMap  = subServiceIter->second;

      for (metricIter = metricMap->begin(); metricIter != metricMap->end(); ++metricIter)
      {
        metricIter->second = 0;
      }
    }
  }
}



/* ****************************************************************************
*
* metricsRender - 
*/
static std::string metricsRender(std::map<std::string, uint64_t>* metricsMap)
{
  std::map<std::string, uint64_t>::iterator  it;
  uint64_t                                   incomingTransactions = 0;
  uint64_t                                   totalServiceTime     = 0;
  JsonHelper                                 jh;

  for (it = metricsMap->begin();  it != metricsMap->end(); ++it)
  {
    std::string      metric = it->first;
    int64_t          value  = it->second;

    if (metric == _METRIC_TOTAL_SERVICE_TIME)
    {
      totalServiceTime = value;
    }
    else if (metric == METRIC_TRANS_IN)
    {
      incomingTransactions = value;
    }

    if ((totalServiceTime != 0) && (incomingTransactions != 0))
    {
      float mValue = (float) totalServiceTime / (float) (incomingTransactions * 1000000);

      jh.addFloat(METRIC_SERVICE_TIME, mValue);
      totalServiceTime     = 0;
      incomingTransactions = 0;
    }

    if (metric != _METRIC_TOTAL_SERVICE_TIME)
    {
      if (value != 0)
      {
        jh.addNumber(metric, value);
      }
    }
  }

  return jh.str();
}



/* ****************************************************************************
*
* MetricsManager::_toJson -
*/
std::string MetricsManager::_toJson(void)
{
  //
  // Three iterators needed to iterate over the 'triple-map' metrics:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //   metricIter       to iterate over all metrics of a sub-service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, uint64_t>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, uint64_t>*>::iterator                          subServiceIter;
  std::map<std::string, uint64_t>::iterator                                                  metricIter;
  JsonHelper                                                                                 top;
  JsonHelper                                                                                 services;
  std::map<std::string, uint64_t>                                                            sum;
  std::map<std::string, std::map<std::string, uint64_t> >                                    subServCrossTenant;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    JsonHelper                                                subServiceTop;
    JsonHelper                                                jhSubService;
    std::string                                               service        = serviceIter->first;
    std::map<std::string, std::map<std::string, uint64_t>*>*  servMap        = serviceIter->second;
    std::map<std::string, uint64_t>                           serviceSum;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      JsonHelper                        jhMetrics;
      std::string                       subService           = subServiceIter->first;
      std::map<std::string, uint64_t>*  metricMap            = subServiceIter->second;

      for (metricIter = metricMap->begin(); metricIter != metricMap->end(); ++metricIter)
      {
        std::string  metric = metricIter->first;
        int64_t      value  = metricIter->second;

        // Add to 'sum-maps'
        if (value != 0)
        {
          serviceSum[metric] += value;
          sum[metric]        += value;
          subServCrossTenant[subService][metric] += value;
        }
      }

      std::string subServiceString = metricsRender(metricMap);

      if (subServiceString != "{}")
      {
        if (subService != "")
        {
          jhSubService.addRaw(subService, subServiceString);
        }
        else
        {
          jhSubService.addRaw(DEFAULT_SUB_SERVICE_KEY_FOR_METRICS, subServiceString);
        }
      }
    }

    subServiceTop.addRaw("subservs", jhSubService.str());

    std::string serviceSumString = metricsRender(&serviceSum);

    subServiceTop.addRaw("sum", serviceSumString);

    if (service != "")
    {
      services.addRaw(service, subServiceTop.str());
    }
    else
    {
      services.addRaw(DEFAULT_SERVICE_KEY_FOR_METRICS, subServiceTop.str());
    }
  }

  //
  // Sum for grand total
  //
  JsonHelper   lastSum;
  JsonHelper   jhSubServ;

  std::map<std::string, std::map<std::string, uint64_t> >::iterator  it;
  for (it = subServCrossTenant.begin();  it != subServCrossTenant.end(); ++it)
  {
    JsonHelper   jhSubServCross;
    std::string  subService = it->first;
    std::string  subServiceString;

    subServiceString = metricsRender(&it->second);

    if (subService != "")
    {
      jhSubServ.addRaw(subService, subServiceString);
    }
    else
    {
      jhSubServ.addRaw(DEFAULT_SUB_SERVICE_KEY_FOR_METRICS, subServiceString);
    }
  }

  lastSum.addRaw("subservs", jhSubServ.str());

  std::string  sumString = metricsRender(&sum);
  lastSum.addRaw("sum", sumString);

  top.addRaw("services", services.str());
  top.addRaw("sum", lastSum.str());

  return top.str();
}



/* ****************************************************************************
*
* isOn - 
*/
bool MetricsManager::isOn(void)
{
  return on;
}



/* ****************************************************************************
*
* MetricsManager::semStateGet - 
*/
const char* MetricsManager::semStateGet(void)
{
  int value;

  if (sem_getvalue(&sem, &value) == -1)
  {
    return "error";
  }

  if (value == 0)
  {
    return "taken";
  }

  return "free";
}



/* ****************************************************************************
*
* MetricsManager::release -
*/
void MetricsManager::release(void)
{
  if (on == false)
  {
    return;
  }

  semTake();

  //
  // Two iterators needed to iterate over metrics, clearing all maps:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, uint64_t>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, uint64_t>*>::iterator                          subServiceIter;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    std::string                                              service  = serviceIter->first;
    std::map<std::string, std::map<std::string, uint64_t>*>* servMap  = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      std::string                       subService = subServiceIter->first;
      std::map<std::string, uint64_t>*  metricMap  = subServiceIter->second;

      metricMap->clear();
      delete metricMap;
    }
    servMap->clear();
    delete servMap;
  }
  metrics.clear();

  semGive();
}



/* ****************************************************************************
*
* MetricsManager::reset -
*/
void MetricsManager::reset(void)
{
  if (on == false)
  {
    return;
  }

  semTake();
  _reset();
  semGive();
}



/* ****************************************************************************
*
* MetricsManager::toJson -
*/
std::string MetricsManager::toJson(bool doReset)
{
  if (on == false)
  {
    return "";
  }

  semTake();

  std::string  s = _toJson();

  if (doReset)
  {
    _reset();
  }

  semGive();

  return s;
}
