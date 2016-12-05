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
#include <string>
#include <map>

#include "metricsMgr/MetricsManager.h"
#include "common/JsonHelper.h"



/* ****************************************************************************
*
* MetricsManager::MetricsManager -
*/
MetricsManager::MetricsManager()
{
}



/* ****************************************************************************
*
* MetricsManager::accumulate -
*
*/
void MetricsManager::accumulate(const std::string& srv, const std::string& subServ, const std::string& metric, int value)
{
  // Do we have the service in the map?
  if (metrics.find(srv) == metrics.end())
  {
    // not found: create it
    metrics[srv] = new std::map<std::string, std::map<std::string, int>*>;
  }

  // Do we have the subservice in the map?
  if (metrics[srv]->find(subServ) == metrics[srv]->end())
  {
    // not found: create it
    // FIXME PR: this syntax should be simpler, closer to
    // metrics[srv][subServ] = new std::map<std::string, int>;

    metrics[srv]->insert(std::pair<std::string, std::map<std::string, int>*>(subServ, new std::map<std::string, int>));
  }

  // Do we have the metric in the map?
  if (metrics[srv]->at(subServ)->find(metric) == metrics[srv]->at(subServ)->end())
  {
    // not found: create it
    // FIXME PR: I don't like the at() and pair() syntax, I'd prefer a syntax closer to:
    // metrics[srv][subServ][metric] = 0;
    metrics[srv]->at(subServ)->insert(std::pair<std::string, int>(metric, 0));
  }

  metrics[srv]->at(subServ)->at(metric) += value;
}



/* ****************************************************************************
*
* MetricsManager::reset -
*/
void MetricsManager::reset(void)
{
  // FIXME PR (see .h)
}



/* ****************************************************************************
*
* MetricsManager::toJson -
*
* FIXME PR: needs a refactor (see .h)
*/
std::string MetricsManager::toJson(void)
{
  //
  // Three iterators needed to iterate over the 'triple-map' metrics:
  //   serviceIter      to iterate over all services
  //   subServiceIter   to iterate over all sub-services of a service
  //   metricIter       to iterate over all metrics of a sub-service
  //
  std::map<std::string, std::map<std::string, std::map<std::string, int>*>*>::iterator  serviceIter;
  std::map<std::string, std::map<std::string, int>*>::iterator                          subServiceIter;
  std::map<std::string, int>::iterator                                                  metricIter;
  JsonHelper                                                                            jh;

  for (serviceIter = metrics.begin(); serviceIter != metrics.end(); ++serviceIter)
  {
    JsonHelper                                          jhSubService;
    std::string                                         service        = serviceIter->first;
    std::map<std::string, std::map<std::string, int>*>* servMap        = serviceIter->second;

    for (subServiceIter = servMap->begin(); subServiceIter != servMap->end(); ++subServiceIter)
    {
      JsonHelper                  jhMetrics;
      std::string                 subServ    = subServiceIter->first;
      std::map<std::string, int>* metricMap  = subServiceIter->second;

      for (metricIter = metricMap->begin(); metricIter != metricMap->end(); ++metricIter)
      {
        std::string  metric = metricIter->first;
        int          value  = metricIter->second;

        jhMetrics.addNumber(metric, value);
      }

      jhSubService.addRaw(subServ, jhMetrics.str());
    }

    jh.addRaw(service, jhSubService.str());
  }

  return jh.str();
}
