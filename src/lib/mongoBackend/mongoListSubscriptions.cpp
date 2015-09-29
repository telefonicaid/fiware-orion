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

#include "mongoBackend/mongoListSubscriptions.h"

OrionError mongoListSubscriptions
(
  std::vector<Subscription>& subs,
  std::map<std::string, std::string>&  uriParam,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV
)
{
  OrionError oe;
  oe.code = SccOk;

  Subscription s1,s2;
  EntID ei;
  ei.id ="ID1";
  s1.id ="s1";
  s1.subject.entities.push_back(ei);
  s1.subject.condition.attributes.push_back("A \nsc sub 1");
  s1.subject.condition.attributes.push_back("B \tsc sub 1");
  s1.subject.condition.attributes.push_back("Z \fsc sub 2");
  s1.notification.attributes.push_back("notification attributes  1");
  s1.notification.attributes.push_back("notification attributes 2");

  subs.push_back(s1);

  ei.idPattern ="IDP2";
  ei.id = "";
  ei.type ="FENOTIPO";
  s2.id = "s2";
  s2.subject.entities.push_back(ei);
  s2.subject.condition.expression.q ="Q";
  s2.subject.condition.expression.geometry ="G";
  s2.subject.condition.expression.coords ="C";

  subs.push_back(s2);

  return oe;
}
