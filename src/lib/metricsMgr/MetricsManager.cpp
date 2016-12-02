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

#include "MetricsManager.h"

#include "common/JsonHelper.h"

/* ****************************************************************************
*
* MetricsManager::MetricsManager -
*/
MetricsManager::MetricsManager()
{
  // Nothing to do so far
}

/* ****************************************************************************
*
* MetricsManager::accum -
*
*/
void MetricsManager::accum(const std::string& srv, const std::string& subServ, const std::string& metric, int value)
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
    // intMetrics[srv][subServ] = new std::map<std::string, int>;

    metrics[srv]->insert(std::pair<std::string, std::map<std::string, int>*>(subServ, new std::map<std::string, int>));
  }

  // Do we have the metric in the map?
  if (metrics[srv]->at(subServ)->find(metric) == metrics[srv]->at(subServ)->end())
  {
    // not found: create it
    // FIXME PR: I don't like the at() and pair() syntax, I'd prefer a syntax closer to:
    // intMetrics[srv][subServ][metric] = 0;
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
  // TBD
}

/* ****************************************************************************
*
* MetricsManager::toJson -
*/
std::string MetricsManager::toJson(void)
{
  // For all services...
  JsonHelper jhServ;

  for(std::map<std::string, std::map<std::string, std::map<std::string, int>*>* >::iterator ix = metrics.begin();
      ix != metrics.end();
      ++ix)
  {
    // For all subservices ...
    JsonHelper jhSubserv;

    std::string service                                         = ix->first;
    std::map<std::string, std::map<std::string, int>*>* servMap = ix->second;

    for(std::map<std::string, std::map<std::string, int>*>::iterator jx = servMap->begin();
        jx != servMap->end();
        ++jx)
    {
      // For all metrics...
      JsonHelper jhMetrics;

      std::string subServ                   = jx->first;
      std::map<std::string, int>* metricMap = jx->second;

      for(std::map<std::string, int>::iterator kx = metricMap->begin();
          kx != metricMap->end();
          ++kx)
      {
        // ... print it!
        std::string metric = kx->first;
        int value          = kx->second;

        jhMetrics.addNumber(metric, value);

      }

      jhSubserv.addRaw(subServ, jhMetrics.str());
    }

    jhServ.addRaw(service, jhSubserv.str());
  }

  return jhServ.str();
}


