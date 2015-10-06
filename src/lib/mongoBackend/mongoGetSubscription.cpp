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
* Author: Orion dev team
*/

#include "common/sem.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "mongoBackend/mongoGetSubscription.h"
#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/collectionOperations.h"

#include "mongo/client/dbclient.h"

using namespace ngsiv2;


/* ****************************************************************************
*
* mongoGetSubscription -
*/
void mongoGetSubscription
(
  ngsiv2::Subscription                *sub,
  OrionError                          *oe,
  const std::string&                  idSub,
  std::map<std::string, std::string>& uriParam,
  const std::string&                  tenant
)
{

  EntID ei;
  Subscription s1;
  ei.id ="EID1";
  s1.id = idSub;
  s1.subject.entities.push_back(ei);
  s1.subject.condition.attributes.push_back("A \nsc sub 1");
  s1.subject.condition.attributes.push_back("B \tsc sub 1");
  s1.subject.condition.attributes.push_back("Z \fsc sub 2");
  s1.notification.attributes.push_back("notification attributes  1");
  s1.notification.attributes.push_back("notification attributes 2");

  *sub = s1;

  oe->code  = SccOk;
  return;
}

