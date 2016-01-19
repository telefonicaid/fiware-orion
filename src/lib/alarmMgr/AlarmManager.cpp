/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <semaphore.h>
#include <errno.h>

#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/AlarmManager.h"



/* ****************************************************************************
*
* AlarmManager::AlarmManager - 
*/
AlarmManager::AlarmManager():
  badInputs(0),
  badInputResets(0),
  notificationErrors(0),
  notificationErrorResets(0),
  dbErrors(0),
  dbErrorResets(0),
  dbOk(true),
  notificationErrorLogSampling(0),
  badInputLogSampling(0)
{
  semInit();
}



/* ****************************************************************************
*
* AlarmManager::AlarmManager - 
*/
AlarmManager::AlarmManager(int _notificationErrorLogSampling, int _badInputLogSampling):
  badInputs(0),
  badInputResets(0),
  notificationErrors(0),
  notificationErrorResets(0),
  dbErrors(0),
  dbErrorResets(0),
  dbOk(true),
  notificationErrorLogSampling(_notificationErrorLogSampling),
  badInputLogSampling(_badInputLogSampling)
{
  semInit();
}  



/* ****************************************************************************
*
* AlarmManager::semInit - 
*/
int AlarmManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_E(("Runtime Error (error initializing 'alarm mgr' semaphore: %s)", strerror(errno)));
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* AlarmManager::semTake - 
*/
void AlarmManager::semTake(void)
{
  sem_wait(&sem);
}



/* ****************************************************************************
*
* AlarmManager::semGive - 
*/
void AlarmManager::semGive(void)
{
  sem_post(&sem);
}



/* ****************************************************************************
*
* AlarmManager::notificationErrorLogSamplingSet - 
*/
void AlarmManager::notificationErrorLogSamplingSet(int _notificationErrorLogSampling)
{
  notificationErrorLogSampling = _notificationErrorLogSampling;
}



/* ****************************************************************************
*
* AlarmManager::badInputLogSamplingSet - 
*/
void AlarmManager::badInputLogSamplingSet(int _badInputLogSampling)
{
  badInputLogSampling = _badInputLogSampling;
}



/* ****************************************************************************
*
* AlarmManager::dbError - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::dbError(const std::string& details)
{
  if (dbOk == false)
  {
    return false;
  }

  ++dbErrors;

  LM_E(("Raising alarm DatabaseError: %s", details.c_str()));
  dbOk = false;
  return true;
}



/* ****************************************************************************
*
* AlarmManager::dbErrorReset - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::dbErrorReset(void)
{
  if (dbOk == true)
  {
    return false;
  }

  ++dbErrorResets;
  LM_E(("Releasing alarm DatabaseError"));
  dbOk = true;
  return true;
}



/* ****************************************************************************
*
* AlarmManager::dbErrorsGet - 
*
* NOTE
*  dbError active means there is a DB problem: dbOk is false
*/
void AlarmManager::dbErrorsGet(bool* active, long long* raised, long long* released)
{
  *active    = (dbOk == false)? true : false;
  *raised    = dbErrors;
  *released  = dbErrorResets;
}



/* ****************************************************************************
*
* AlarmManager::notificationErrorGet - 
*/
void AlarmManager::notificationErrorGet(long long* active, long long* raised, long long* released)
{
  *active    = notificationV.size();
  *raised    = notificationErrors;
  *released  = notificationErrorResets;
}



/* ****************************************************************************
*
* AlarmManager::badInputGet - 
*/
void AlarmManager::badInputGet(long long* active, long long* raised, long long* released)
{
  *active    = badInputV.size();
  *raised    = badInputs;
  *released  = badInputResets;
}



/* ****************************************************************************
*
* AlarmManager::notificationError - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*
* NOTE
* The number of notificationErrors per url is maintained in the map.
* Right now that info is not used, but might be in the future.
* To do so, we'd need another counter as well, to not forget the accumulated 
* number each time the notificationErrors are reset.
*/
bool AlarmManager::notificationError(const std::string& url, const std::string& details)
{
  std::map<std::string, int>::iterator iter = notificationV.find(url);

  ++notificationErrors;

  if (iter != notificationV.end())  // Already exists - add to the 'url-specific' counter
  {
    iter->second += 1;

    if ((notificationErrorLogSampling != 0) && ((notificationErrors % notificationErrorLogSampling) == 1))
    {
      LM_W(("Repeated NotificationError %s: %s", url.c_str(), details.c_str()));
    }

    return false;
  }

  LM_W(("Raising alarm NotificationError %s: %s", url.c_str(), details.c_str()));
  notificationV[url] = 1;

  return true;
}



/* ****************************************************************************
*
* AlarmManager::notificationErrorReset - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::notificationErrorReset(const std::string& url)
{
  if (notificationV.find(url) == notificationV.end())  // Doesn't exist
  {
    return false;
  }
  
  notificationV.erase(url);
  LM_W(("Releasing alarm NotificationError %s", url.c_str()));

  ++notificationErrorResets;
  return true;
}



/* ****************************************************************************
*
* AlarmManager::badInput - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*
* NOTE
* The number of badInputs per IP is maintained in the map.
* Right now that info is not used, but might be in the future.
* To do so, we'd need another counter as well, to not forget the accumulated 
* number each time the badInputs are reset.
*/
bool AlarmManager::badInput(const std::string& ip, const std::string& details)
{
  std::map<std::string, int>::iterator iter = badInputV.find(ip);

  ++badInputs;

  if (iter != badInputV.end())  // Already exists - add to the 'ip-specific' counter
  {
    iter->second += 1;

    if ((badInputLogSampling != 0) && ((badInputs % badInputLogSampling) == 1))
    {
      LM_W(("Repeated BadInput %s: %s", ip.c_str(), details.c_str()));
    }

    return false;
  }

  LM_W(("Raising alarm BadInput %s: %s", ip.c_str(), details.c_str()));
  badInputV[ip] = 1;

  return true;
}



/* ****************************************************************************
*
* AlarmManager::badInputReset - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::badInputReset(const std::string& ip)
{
  if (badInputV.find(ip) == badInputV.end())  // Doesn't exist
  {
    return false;
  }

  badInputV.erase(ip);
  LM_W(("Releasing alarm BadInput %s", ip.c_str()));

  ++badInputResets;
  return true;
}
