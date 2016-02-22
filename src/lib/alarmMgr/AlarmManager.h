#ifndef SRC_LIB_ALARMMGR_ALARMMANAGER_H_
#define SRC_LIB_ALARMMGR_ALARMMANAGER_H_

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

#include <string>
#include <map>



/* ****************************************************************************
*
* AlarmManager - 
*/
class AlarmManager
{
private:
  long long                   badInputs;
  long long                   badInputResets;
  long long                   notificationErrors;
  long long                   notificationErrorResets;
  long long                   dbErrors;
  long long                   dbErrorResets;
  bool                        dbOk;
  std::map<std::string, int>  notificationV;
  std::map<std::string, int>  badInputV;
  bool                        notificationErrorLogAlways;
  bool                        badInputLogAlways;
  bool                        dbErrorLogAlways;
  sem_t                       sem;

public:
  AlarmManager();

  int  init(bool logAlreadyRaisedAlarms);
  void semTake(void);
  void semGive(void);

  void notificationErrorLogAlwaysSet(bool _notificationErrorLogAlways);
  void badInputLogAlwaysSet(bool _badInputLogAlways);
  void dbErrorLogAlwaysSet(bool _dbErrorLogAlways);

  bool dbError(const std::string& details);
  bool dbErrorReset(void);

  bool notificationError(const std::string& url, const std::string& details);
  bool notificationErrorReset(const std::string& url);

  bool badInput(const std::string& ip, const std::string& details);
  bool badInputReset(const std::string& ip);

  // Methods for Log Summary
  void dbErrorsGet(bool* active, long long* raised, long long* released);
  void badInputGet(long long* active, long long* raised, long long* released);
  void notificationErrorGet(long long* active, long long* raised, long long* released);

private:
  int  semInit(void);
};

#endif  // SRC_LIB_ALARMMGR_ALARMMANAGER_H_
