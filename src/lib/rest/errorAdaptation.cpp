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
* Author: Fermin Galan
*/

#include "rest/errorAdaptation.h"
#include "rest/ConnectionInfo.h"


/* ****************************************************************************
*
* setStatusCodeAndSmartRender -
*/
std::string setStatusCodeAndSmartRender(ConnectionInfo* ciP, OrionError& oe)
{
  if (ciP->apiVersion == "v2")
  {
    ciP->httpStatusCode = oe.code;
  }
  return oe.smartRender(ciP->apiVersion);
}


#if 0
/* ****************************************************************************
*
* errorStringForV2 -
*
* NGSIv2 uses "compacted" error descriptions, e.g. "BadRequest" instead of
* "Bad Request". This function makes the conversion.
*/
std::string errorStringForV2(const std::string& _reasonPhrase)
{
  if (_reasonPhrase == "Bad Request")
  {
    return "BadRequest";
  }
  else if (_reasonPhrase == "Content Length Required")
  {
    return "ContentLengthRequired";
  }
  else if (_reasonPhrase == "Request Entity Too Large")
  {
    return "RequestEntityTooLarge";
  }
  else if (_reasonPhrase == "Unsupported Media Type")
  {
    return "UnsupportedMediaType";
  }
  else if (_reasonPhrase == "Invalid Modification")
  {
    return "InvalidModification";
  }
  else if (_reasonPhrase == "Too Many Results")
  {
    return "TooManyResults";
  }
  else if (_reasonPhrase == "No context element found")
  {
    return "NotFound";
  }

  return _reasonPhrase;
}

/* ****************************************************************************
*
* invalidParameterForNgsiv2 -
*
* NGSIv1 -> NGSIv2 error conversion is a little crazy as the same SccInvalidParameter
* at v1 level could match two different error conditions at v2 level, making harder
* the logic and we need to look into the error details string. The idea of this
* function is to hide all that ugly stuff to others to be easily removed once
* NGSIv1 get deprecated and we can make this in a more convenient way.
*
* This function resturns true if the OrionError object passed as argument has been
* filled, false otherwise.
*
*/
bool invalidParameterForNgsiv2(const std::string& details, OrionError* oe)
{
  if (details.find("geo") != std::string::npos)
  {
    if ((details.find("more than one") != std::string::npos) ||
        (details.find("another one has been previously defined") != std::string::npos))
    {
      // Special case, for the error related withmore than one attribute acting as entity location
      // The following "test strings" come from processLocation*() functions at mongoBackend/location.h
      oe->fill(SccRequestEntityTooLarge, "No more than one geo-location attribute allowed", "NoResourcesAvailable");
    }
    else
    {
      // In the other "geo error" cases, just used comming from mongoBackend
      oe->fill(SccBadRequest, details);
    }
    return true;
  }

  return false;
}
#endif
