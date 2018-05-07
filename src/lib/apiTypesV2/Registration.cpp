/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán Márquez
*/
#include <string>

#include "apiTypesV2/Registration.h"
#include "common/JsonHelper.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* ForwardingInformation::ForwardingInformation -
*/
ForwardingInformation::ForwardingInformation(): lastFailure(0), lastSuccess(0), timesSent(0), lastForwarding(0)
{
}



/* ****************************************************************************
*
* Registration::Registration -
*/
Registration::Registration(): descriptionProvided(false), expires(-1)
{
}



/* ****************************************************************************
*
* Registration::~Registration -
*/
Registration::~Registration()
{
}



/* ****************************************************************************
*
* Registration::toJson -
*/
std::string Registration::toJson(void)
{
  JsonHelper jh;

  jh.addString("id", id);

  if (description != "")
  {
    jh.addString("description", description);
  }

  if (expires != -1)
  {
    jh.addDate("expires", expires);
  }

  jh.addRaw("dataProvided", dataProvided.toJson());
  jh.addRaw("provider", provider.toJson());
  jh.addString("status", (status != "")? status : "active");

  //
  // FIXME P6: once forwarding is implemented for APIv2, include this call
  // jh.addRaw("forwardingInformation", forwardingInformation.toJson());
  //

  return jh.str();
}



/* ****************************************************************************
*
* DataProvided::toJson -
*/
std::string DataProvided::toJson(void)
{
  JsonHelper jh;

  jh.addRaw("entities", vectorToJson(entities));
  jh.addRaw("attrs", vectorToJson(attributes));

  return jh.str();
}



/* ****************************************************************************
*
* Provider::toJson -
*/
std::string Provider::toJson(void)
{
  JsonHelper   jh;
  std::string  urlAsJson = "{\"url\": \"" + http.url + "\"}";

  jh.addRaw("http", urlAsJson);
  jh.addString("supportedForwardingMode", forwardingModeToString(supportedForwardingMode));
  jh.addBool("legacyForwarding", legacyForwardingMode? "true" : "false");

  return jh.str();
}



/* ****************************************************************************
*
* ForwardingInformation::toJson -
*/
std::string ForwardingInformation::toJson()
{
  JsonHelper  jh;

  jh.addNumber("timesSent", timesSent);

  if (lastSuccess > 0)
  {
    jh.addDate("lastSuccess", lastSuccess);
  }

  if (lastFailure > 0)
  {
    jh.addDate("lastFailure", lastFailure);
  }

  if (lastForwarding > 0)
  {
    jh.addDate("lastForwarding", lastForwarding);
  }

  return jh.str();
}
}
