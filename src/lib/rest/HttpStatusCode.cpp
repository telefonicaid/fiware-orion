/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: developer
*/
#include "HttpStatusCode.h"



/* ****************************************************************************
*
* httpStatusCodeString -
*/
std::string httpStatusCodeString(HttpStatusCode code)
{
  switch (code)
  {
  case SccOk:                                return "OK";
  case SccCreated:                           return "Created";
  case SccBadRequest:                        return "Bad Request";
  case SccForbidden:                         return "Forbidden";
  case SccContextElementNotFound:            return "No context element found"; // Standard HTTP for 404: "Not Found"
  case SccBadVerb:                           return "Method Not Allowed";
  case SccNotAcceptable:                     return "Not Acceptable";
  case SccConflict:                          return "Too Many Results";
  case SccContentLengthRequired:             return "Content Length Required";
  case SccRequestEntityTooLarge:             return "Request Entity Too Large";
  case SccUnsupportedMediaType:              return "Unsupported Media Type";
  case SccInvalidModification:               return "Invalid Modification";
  case SccSubscriptionIdNotFound:            return "subscriptionId does not correspond to an active subscription"; // FI-WARE
  case SccMissingParameter:                  return "parameter missing in the request";                             // FI-WARE
  case SccInvalidParameter:                  return "request parameter is invalid/not allowed";                     // FI-WARE
  case SccErrorInMetadata:                   return "Generic error in metadata";
  case SccEntityIdReNotAllowed:              return "Regular Expression for EntityId is not allowed by receiver";
  case SccEntityTypeRequired:                return "EntityType required by the receiver";
  case SccAttributeListRequired:             return "Attribute List required by the receiver";
  case SccReceiverInternalError:             return "Internal Server Error";
  case SccNotImplemented:                    return "Not Implemented";
  default:                                   return "Undefined";
  }
}
