/*
*
* Copyright 2022 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <curl/curl.h>                                           // CURL, curl_easy_getinfo

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/curlToBrokerStrerror.h"                 // Own interface



// ----------------------------------------------------------------------------
//
// curlToBrokerStrerror -
//
const char* curlToBrokerStrerror(CURL* curlHandle, int curlErrorCode, int* statusCodeP)
{
  if (curlErrorCode == 5)
  {
    *statusCodeP = 504;
    return "Unable to resolve proxy";
  }
  else if (curlErrorCode == 6)
  {
    *statusCodeP = 504;
    LM_E(("Unable to resolve host name of registrant"));
    return "Unable to resolve host name of registrant";
  }
  else if (curlErrorCode == 7)
  {
    *statusCodeP = 504;
    LM_E(("Unable to connect to registrant"));
    return "Unable to connect to registrant";
  }
  else if (curlErrorCode == 22)
  {
    long long httpStatus = 500;
    curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &httpStatus);

    *statusCodeP = httpStatus;
    if (httpStatus == 409)
      return "Entity already exists";
    else
    {
      LM_W(("Forwarded request response is of HTTP Status %d", httpStatus));
      return "Entity was not created externally, and it did not previously exist";
    }
  }

  *statusCodeP = 500;
  return "Other CURL Error";
}
