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
#include <sys/types.h>

#include <string>
#include <map>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "alarmMgr/AlarmManager.h"



/* ****************************************************************************
*
* badInputSeen - 
*
* badInputSeen is a variable to keep track of whether a BadInput has already been issued
* for the current request.
* We only want ONE Bad Input per request.
*/
__thread bool badInputSeen = false;



/* ****************************************************************************
*
* AlarmManager::AlarmManager - 
*/
AlarmManager::AlarmManager()
:
  badInputs(0),
  badInputResets(0),
  notificationErrors(0),
  notificationErrorResets(0),
  forwardingErrors(0),
  forwardingErrorResets(0),
  mqttConnectionErrors(0),
  mqttConnectionResets(0),
  dbErrors(0),
  dbErrorResets(0),
  dbOk(true),
  notificationErrorLogAlways(false),
  forwardingErrorLogAlways(false),
  mqttConnectionErrorLogAlways(false),
  badInputLogAlways(false),
  dbErrorLogAlways(false)
{
}



/* ****************************************************************************
*
* AlarmManager::init - 
*/
void AlarmManager::init(bool logAlreadyRaisedAlarms)
{
  notificationErrorLogAlways   = logAlreadyRaisedAlarms;
  badInputLogAlways            = logAlreadyRaisedAlarms;
  dbErrorLogAlways             = logAlreadyRaisedAlarms;
  mqttConnectionErrorLogAlways = logAlreadyRaisedAlarms;
  forwardingErrorLogAlways     = logAlreadyRaisedAlarms;

  semInit();
}



/* ****************************************************************************
*
* AlarmManager::semInit - 
*/
void AlarmManager::semInit(void)
{
  if (sem_init(&sem, 0, 1) == -1)
  {
    LM_X(1, ("Fatal Error (error initializing 'alarm mgr' semaphore: %s)", strerror(errno)));
  }
}



/* ****************************************************************************
*
* AlarmManager::semGet - 
*/
const char* AlarmManager::semGet(void)
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
* AlarmManager::dbError - 
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::dbError(const std::string& details)
{
  if (dbOk == false)
  {
    if (dbErrorLogAlways)
    {
      LM_E(("Repeated Database Error: %s", details.c_str()));
    }
    else
    {
      // even if repeat alarms is off, this is a relevant event in debug level
      LM_T(LmtMongo, ("Repeated Database Error: %s", details.c_str()));
    }

    return false;
  }

  semTake();
  ++dbErrors;
  dbOk = false;
  semGive();

  LM_E(("Raising alarm DatabaseError: %s", details.c_str()));
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

  semTake();
  ++dbErrorResets;
  dbOk = true;
  semGive();

  LM_E(("Releasing alarm DatabaseError"));
  return true;
}



/* ****************************************************************************
*
* AlarmManager::dbErrorsGet - 
*
* NOTE
*   dbError active means there is a DB problem: dbOk is false
*
* ALSO NOTE
*   To read values, no semaphore is used.
*/
void AlarmManager::dbErrorsGet(bool* active, int64_t* raised, int64_t* released)
{
  *active    = (dbOk == false)? true : false;
  *raised    = dbErrors;
  *released  = dbErrorResets;
}



/* ****************************************************************************
*
* AlarmManager::notificationErrorGet - 
*
* NOTE
*   To read values, no semaphore is used.
*/
void AlarmManager::notificationErrorGet(int64_t* active, int64_t* raised, int64_t* released)
{
  *active    = notificationV.size();
  *raised    = notificationErrors;
  *released  = notificationErrorResets;
}



/* ****************************************************************************
*
* AlarmManager::forwardingErrorGet -
*
* NOTE
* To read values, no semaphore is used.
*/
void AlarmManager::forwardingErrorGet(int64_t* active, int64_t* raised, int64_t* released)
{
  *active    = forwardingErrorV.size();
  *raised    = forwardingErrors;
  *released  = forwardingErrorResets;
}



/* ****************************************************************************
*
* AlarmManager::badInputGet - 
*
* NOTE
*   To read values, no semaphore is used.
*/
void AlarmManager::badInputGet(int64_t* active, int64_t* raised, int64_t* released)
{
  *active    = badInputV.size();
  *raised    = badInputs;
  *released  = badInputResets;
}



/* ****************************************************************************
*
* AlarmManager::mqttConnectionErrorGet -
*
* NOTE
*   To read values, no semaphore is used.
*/
void AlarmManager::mqttConnectionErrorGet(int64_t* active, int64_t* raised, int64_t* released)
{
  *active    = mqttConnectionErrorV.size();
  *raised    = mqttConnectionErrors;
  *released  = mqttConnectionResets;
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
  semTake();

  std::map<std::string, int>::iterator iter = notificationV.find(url);

  if (iter != notificationV.end())  // Already exists - add to the 'url-specific' counter
  {
    iter->second += 1;

    if (notificationErrorLogAlways)
    {
      LM_W(("Repeated NotificationError %s: %s", url.c_str(), details.c_str()));
    }
    else
    {
      // even if repeat alarms is off, this is a relevant event in debug level
      LM_T(LmtNotifier, ("Repeated NotificationError %s: %s", url.c_str(), details.c_str()));
    }

    semGive();
    return false;
  }

  ++notificationErrors;

  notificationV[url] = 1;
  semGive();

  LM_W(("Raising alarm NotificationError %s: %s", url.c_str(), details.c_str()));

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
  semTake();

  if (notificationV.find(url) == notificationV.end())  // Doesn't exist
  {
    semGive();
    return false;
  }

  notificationV.erase(url);
  ++notificationErrorResets;
  semGive();

  LM_W(("Releasing alarm NotificationError %s", url.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::mqttConnectionError -
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*
* NOTE
* The number of mqttConnectionErrors per endpoint is maintained in the map.
* Right now that info is not used, but might be in the future.
* To do so, we'd need another counter as well, to not forget the accumulated
* number each time the notificationErrors are reset.
*/
bool AlarmManager::mqttConnectionError(const std::string& endpoint, const std::string& details)
{
  semTake();

  std::map<std::string, int>::iterator iter = mqttConnectionErrorV.find(endpoint);

  if (iter != mqttConnectionErrorV.end())  // Already exists - add to the 'url-specific' counter
  {
    iter->second += 1;

    if (mqttConnectionErrorLogAlways)
    {
      LM_W(("Repeated MqttConnectionError %s: %s", endpoint.c_str(), details.c_str()));
    }
    else
    {
      // even if repeat alarms is off, this is a relevant event in debug level
      LM_T(LmtMqttNotif, ("Repeated MqttConnectionError %s: %s", endpoint.c_str(), details.c_str()));
    }

    semGive();
    return false;
  }

  ++mqttConnectionErrors;

  mqttConnectionErrorV[endpoint] = 1;
  semGive();

  LM_W(("Raising alarm MqttConnectionError %s: %s", endpoint.c_str(), details.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::mqttConnectionReset -
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::mqttConnectionReset(const std::string& endpoint)
{
  semTake();

  if (mqttConnectionErrorV.find(endpoint) == mqttConnectionErrorV.end())  // Doesn't exist
  {
    semGive();
    return false;
  }

  mqttConnectionErrorV.erase(endpoint);
  ++mqttConnectionResets;
  semGive();

  LM_W(("Releasing alarm MqttConnectionError %s", endpoint.c_str()));

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
bool AlarmManager::badInput(const std::string& ip, const std::string& details, const std::string& extraInLog)
{
  if (badInputSeen == true)
  {
    return false;
  }

  badInputSeen = true;

  semTake();

  std::map<std::string, int>::iterator iter = badInputV.find(ip);

  if (iter != badInputV.end())  // Already exists - add to the 'ip-specific' counter
  {
    iter->second += 1;

    if (badInputLogAlways)
    {
      if (extraInLog.empty())
      {
        LM_W(("Repeated BadInput %s: %s", ip.c_str(), details.c_str()));
      }
      else
      {
        LM_W(("Repeated BadInput %s: %s (%s)", ip.c_str(), details.c_str(), extraInLog.c_str()));
      }
    }
    else
    {
      // in this case it is not clear which LM_T use... we leave this comment as placeholder
    }

    semGive();
    return false;
  }

  ++badInputs;

  badInputV[ip] = 1;
  semGive();

  if (extraInLog.empty())
  {
    LM_W(("Raising alarm BadInput %s: %s", ip.c_str(), details.c_str()));
  }
  else
  {
    LM_W(("Raising alarm BadInput %s: %s (%s)", ip.c_str(), details.c_str(), extraInLog.c_str()));
  }

  return true;
}



/* ****************************************************************************
*
* AlarmManager::forwardingError -
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*
* NOTE
* The number of forwardingErrors per url is maintained in the map.
* Right now that info is not used, but might be use in the future.
* To do so, we'd need another counter as well, to not forget the accumulated
* number each time the forwardingErrors are reset.
*/
bool AlarmManager::forwardingError(const std::string& url, const std::string& details)
{
  semTake();

  std::map<std::string, int>::iterator iter = forwardingErrorV.find(url);

  if (iter != forwardingErrorV.end())  // Already exists - add to the 'url-specific' counter
  {
    iter->second += 1;

    if (forwardingErrorLogAlways)
    {
      LM_W(("Repeated ForwardingError %s: %s", url.c_str(), details.c_str()));
    }
    else
    {
      // even if repeat alarms is off, this is a relevant event in debug level
      LM_T(LmtCPrForwardRequestPayload, ("Repeated ForwardingError %s: %s", url.c_str(), details.c_str()));
    }

    semGive();
    return false;
  }

  ++forwardingErrors;

  forwardingErrorV[url] = 1;
  semGive();

  LM_W(("Raising alarm ForwardingError %s: %s", url.c_str(), details.c_str()));

  return true;
}



/* ****************************************************************************
*
* AlarmManager::forwardingErrorReset -
*
* Returns false if no effective alarm transition occurs, otherwise, true is returned.
*/
bool AlarmManager::forwardingErrorReset(const std::string& url)
{
  semTake();

  if (forwardingErrorV.find(url) == forwardingErrorV.end())  // Doesn't exist
  {
    semGive();
    return false;
  }

  forwardingErrorV.erase(url);
  ++forwardingErrorResets;
  semGive();

  LM_W(("Releasing alarm forwardingError %s", url.c_str()));

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
  semTake();

  if (badInputV.find(ip) == badInputV.end())  // Doesn't exist
  {
    semGive();
    return false;
  }

  badInputV.erase(ip);
  ++badInputResets;
  semGive();

  LM_W(("Releasing alarm BadInput %s", ip.c_str()));

  return true;
}
