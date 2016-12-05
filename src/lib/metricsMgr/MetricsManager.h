#ifndef SRC_LIB_METRICSMGR_METRICSMANAGER_H_
#define SRC_LIB_METRICSMGR_METRICSMANAGER_H_

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
* Author: Fermín Galán
*/

#include <string>
#include <map>

/* ****************************************************************************
*
* MetricsManager -
*
* FIXME PR: summary of things to improve
*
* 1. Check alternatives to "triple std::map" from the point of view of performance
*    (probably difficult to beat) and syntax (current one is a bit awkward)
* 2. In order to be homogeneous, probably 'metrics' should be a pointer (and the
*    initial map created at constructor time)
* 3. Destroy method, releasing all the maps in cascade (probably never used, as the
*    singleton object in CB using this class will be destroyed at the end, but do it
*    for class completeness)
* 4. reset() method implementation (not delete maps, only set metrics to 0)
* 5. toJson() to be split into 3 methods (2 of them private)
* 6. Multi-thread safeness. Probably the same sem-based strategy used in AlarmManager
*    could be used.
* 7. Use 'long long' instead of 'int'
* 8. (Usure) We could need maps for metrics different for int. In that case, implement
*    it (and the accumulate method) using templates, to avoid repeating the same implementation
*    N times
*/
class MetricsManager
{
 private:
  std::map<std::string, std::map<std::string, std::map<std::string, int>*>*>  metrics;

 public:
  MetricsManager();

  void        accumulate(const std::string& srv, const std::string& subServ, const std::string& metric, int value);
  void        reset(void);
  std::string toJson(void);
};

#endif  // SRC_LIB_METRICSMGR_METRICSMANAGER_H_
