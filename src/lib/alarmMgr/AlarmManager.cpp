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
dbOk(true),
notificationErrorLogInterval(0),
badInputLogInterval(0)
{
}



/* ****************************************************************************
*
* AlarmManager::AlarmManager - 
*/
AlarmManager::AlarmManager(int _notificationErrorLogInterval, int _badInputLogInterval):
dbOk(true),
notificationErrorLogInterval(_notificationErrorLogInterval),
badInputLogInterval(_badInputLogInterval)
{
}  



/* ****************************************************************************
*
* AlarmManager::notificationErrorLogIntervalSet - 
*/
void AlarmManager::notificationErrorLogIntervalSet(int _notificationErrorLogInterval)
{
  notificationErrorLogInterval = _notificationErrorLogInterval;
}



/* ****************************************************************************
*
* AlarmManager::badInputLogIntervalSet - 
*/
void AlarmManager::badInputLogIntervalSet(int _badInputLogInterval)
{
  badInputLogInterval = _badInputLogInterval;
}



/* ****************************************************************************
*
* AlarmManager::dbError - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::dbError(const std::string& details)
{
  if (dbOk == false)
  {
    return false;
  }

  LM_E(("DB in an erroneous state (%s)", details.c_str()));
  dbOk = false;
  return true;
}



/* ****************************************************************************
*
* AlarmManager::dbErrorReset - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::dbErrorReset(void)
{
  if (dbOk == true)
  {
    return false;
  }

  LM_E(("DB is ok again"));
  dbOk = true;
  return true;
}



/* ****************************************************************************
*
* AlarmManager::notificationError - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::notificationError(const std::string& url, const std::string& details)
{
  std::map<std::string, int>::iterator iter = notificationV.find(url);

  if (iter != notificationV.end())  // Already exists
  {
    if (notificationErrorLogInterval != 0)
    {
      iter->second += 1;

      if ((iter->second % notificationErrorLogInterval) == 0)
      {
        LM_W(("Notification Errors for %s (%d times): %s", url.c_str(), iter->second, details.c_str()));
      }
    }

    return false;
  }

  notificationV[url] = 1;
  LM_W(("Notification Error for %s: %s", url.c_str(), details.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::notificationErrorReset - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::notificationErrorReset(const std::string& url)
{
  if (notificationV.find(url) == notificationV.end())  // Doesn't exist
    return false;
  
  notificationV.erase(url);
  LM_W(("Notification OK for %s", url.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::badInput - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::badInput(const std::string& ip, const std::string& details)
{
  std::map<std::string, int>::iterator iter = badInputV.find(ip);

  if (iter != badInputV.end())  // Already exists
  {
    if (badInputLogInterval != 0)
    {
      iter->second += 1;

      if ((iter->second % badInputLogInterval) == 0)
      {
        LM_W(("Bad Input for %s (%d times); %s", ip.c_str(), iter->second, details.c_str()));
      }
    }

    return false;
  }

  badInputV[ip] = 1;
  LM_W(("Bad Input for %s (%s)", ip.c_str(), details.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::badInputReset - 
*
* Returns false if no action is taken/necessary, otherwise, true is returned.
*/
bool AlarmManager::badInputReset(const std::string& ip)
{
  if (badInputV.find(ip) == badInputV.end())  // Doesn't exist
  {
    return false;
  }

  badInputV.erase(ip);
  LM_W(("Bad Input stopped for %s", ip.c_str()));

  return true;
}
