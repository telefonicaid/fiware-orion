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
#include <vector>

#include "logMsg/logMsg.h"

#include "common/MimeType.h"
#include "rest/HttpHeaders.h"



/* ****************************************************************************
*
* HttpHeaders::HttpHeaders - 
*/
HttpHeaders::HttpHeaders(): gotHeaders(false), servicePathReceived(false), contentLength(0), acceptJson(false), acceptJsonld(false)
{
}



/* ****************************************************************************
*
* release - 
*/
void HttpHeaders::release(void)
{
  for (unsigned int ix = 0; ix < acceptHeaderV.size(); ++ix)
  {
    delete acceptHeaderV[ix];
  }

  acceptHeaderV.clear();
}



/* ****************************************************************************
*
* HttpHeaders::accepted - 
*/
bool HttpHeaders::accepted(const std::string& mime)
{
  for (unsigned int ix = 0; ix < acceptHeaderV.size(); ++ix)
  {
    if (acceptHeaderV[ix]->mediaRange == mime)
    {
      return true;
    }

    if (acceptHeaderV[ix]->mediaRange == "*/*")
    {
      return true;
    }

    if ((mime == "application/json") && (acceptHeaderV[ix]->mediaRange == "application/*"))
    {
      return true;
    }

    if ((mime == "text/plain") && (acceptHeaderV[ix]->mediaRange == "text/*"))
    {
      return true;
    }
  }

  return false;
}



/* ****************************************************************************
*
* HttpHeaders::outformatSelect - 
*/
MimeType HttpHeaders::outformatSelect(void)
{
  std::string  outformat;
  double       q;

  // The mime types come in order. First one to be picked unless 'q' says otherwise
  outformat = acceptHeaderV[0]->mediaRange;
  q         = acceptHeaderV[0]->qvalue;

  for (unsigned int ix = 1; ix < acceptHeaderV.size(); ++ix)
  {
    HttpAcceptHeader* haP = acceptHeaderV[ix];

    if (haP->qvalue > q)
    {
      outformat = haP->mediaRange;
      q         = haP->qvalue;
    }
  }

  if ((outformat == "*/*") || (outformat == "application/*") || (outformat == "application/json"))
  {
    return JSON;
  }

  if ((outformat == "text/*") || (outformat == "text/plain"))
  {
    return TEXT;
  }

  return NOMIMETYPE;
}
