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
#include <sys/time.h>
#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "metricsMgr/MetricsManager.h"
#include "common/JsonHelper.h"



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
long long MetricsManager::semWaitTimeGet(void)
{
  return semWaitTime;
}



/* ****************************************************************************
*
* MetricsManager::add -
*/
void MetricsManager::add(const std::string& srv, const std::string& subServ, const std::string& metric, unsigned long long value)
{
  if (on == false)
  {
    return;
  }

  //
  // Exclude the first '/' from the Sub Service
  // But, only if if starts with a '/'
  const char* subService = subServ.c_str();

  if (subService[0] == '/')
  {
    subService = &subService[1];
  }

  semTake();

  // Do we have the service in the map?
  if (metrics.find(srv) == metrics.end())
  {
    // not found: create it
    metrics[srv] = new std::map<std::string, std::map<std::string, unsigned long long>*>;
  }

  // Do we have the subservice in the map?
  if (metrics[srv]->find(subService) == metrics[srv]->end())
  {
    //
    // not found: create it
    // FIXME PR: this syntax should be simpler, closer to
    // metrics[srv][subService] = new std::map<std::string, unsigned long long>;
    //
    metrics[srv]->insert(std::pair<std::string, std::map<std::string, unsigned long long>*>(subService, new std::map<std::string, unsigned long long>));
  }

  // Do we have the metric in the map?
  if (metrics[srv]->at(subService)->find(metric) == metrics[srv]->at(subService)->end())
  {
    //
    // not found: create it
    // FIXME PR: I don't like the at() and pair() syntax, I'd prefer a syntax closer to:
    // metrics[srv][subService][metric] = 0;
    //
    metrics[srv]->at(subService)->insert(std::pair<std::string, unsigned long long>(metric, 0));
  }

  metrics[srv]->at(subService)->at(metric) += value;

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

  //
  // Three iterators needed to iterate over the 'triple-map' metrics:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //   metricIter       to iterate over all metrics of a sub-service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, unsigned long long>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, unsigned long long>*>::iterator                          subServiceIter;
  std::map<std::string, unsigned long long>::iterator                                                  metricIter;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    std::map<std::string, std::map<std::string, unsigned long long>*>* servMap  = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      std::map<std::string, unsigned long long>* metricMap  = subServiceIter->second;

      for (metricIter = metricMap->begin(); metricIter != metricMap->end(); ++metricIter)
      {
        metricIter->second = 0;
      }
    }
  }

  semGive();
}



/* ****************************************************************************
*
* MetricsManager::toJson -
*
* FIXME PR: needs a refactor (see .h)
*/
std::string MetricsManager::toJson(void)
{
  if (on == false)
  {
    return "";
  }

  semTake();

  //
  // Three iterators needed to iterate over the 'triple-map' metrics:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //   metricIter       to iterate over all metrics of a sub-service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, unsigned long long>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, unsigned long long>*>::iterator                          subServiceIter;
  std::map<std::string, unsigned long long>::iterator                                                  metricIter;
  JsonHelper                                                                                           top;
  JsonHelper                                                                                           services;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    JsonHelper                                                         subServiceTop;
    JsonHelper                                                         jhSubService;
    std::string                                                        service        = serviceIter->first;
    std::map<std::string, std::map<std::string, unsigned long long>*>* servMap        = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      JsonHelper                                  jhMetrics;
      std::string                                 subServ              = subServiceIter->first;
      std::map<std::string, unsigned long long>*  metricMap            = subServiceIter->second;
      unsigned long long                          incomingTransactions = 0;
      unsigned long long                          totalServiceTime     = 0;

      for (metricIter = metricMap->begin(); metricIter != metricMap->end(); ++metricIter)
      {
        std::string  metric = metricIter->first;
        int          value  = metricIter->second;

        if (metric == METRIC_TOTAL_SERVICE_TIME)
        {
          totalServiceTime = value;
        }
        else if (metric == METRIC_TRANS_IN)
        {
          incomingTransactions = value;
        }

        if (metric != METRIC_TOTAL_SERVICE_TIME)
        {
          jhMetrics.addNumber(metric, value);
        }

        if ((totalServiceTime != 0) && (incomingTransactions != 0))
        {
          float mValue = (float) totalServiceTime / (float) (incomingTransactions * 1000000);
          jhMetrics.addFloat(METRIC_SERVICE_TIME, mValue);
          totalServiceTime     = 0;
          incomingTransactions = 0;
        }
      }

      jhSubService.addRaw(subServ, jhMetrics.str());
    }

    subServiceTop.addRaw("subservs", jhSubService.str());
    services.addRaw(service, subServiceTop.str());
  }

  top.addRaw("services", services.str());
  semGive();

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
  std::map<std::string, std::map<std::string, std::map<std::string, unsigned long long>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, unsigned long long>*>::iterator                          subServiceIter;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    std::string                                                        service        = serviceIter->first;
    std::map<std::string, std::map<std::string, unsigned long long>*>* servMap        = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      std::string                                 subServ    = subServiceIter->first;
      std::map<std::string, unsigned long long>*  metricMap  = subServiceIter->second;

      metricMap->clear();
      delete metricMap;
    }
    servMap->clear();
    delete servMap;
  }
  metrics.clear();

  semGive();
}
