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
* Author: Ken Zangelin
*/
#include "logMsg/traceLevels.h"         // Own interface



/* ****************************************************************************
*
* traceLevelName - 
*/
char* traceLevelName(TraceLevels level)
{
  switch (level)
  {
  case LmtFormat:            return (char*) "Format";
  case LmtDuration:          return (char*) "Duration";
  case LmtEntityId:          return (char*) "EntityId";
  case LmtAttribute:         return (char*) "Attribute";
  case LmtMetadata:          return (char*) "Metadata";

  case LmtRcrParse:          return (char*) "Register Context Request Parse";
  case LmtDcarParse:         return (char*) "Discover Context Availability Request Parse";
  case LmtScarParse:         return (char*) "Subscribe Context Availability Request Parse";
  case LmtUcasParse:         return (char*) "Update Context Availability Subscription Request Parse";
  case LmtUcarParse:         return (char*) "Unsubscribe Context Availability Request Parse";
  case LmtNcarParse:         return (char*) "Notify Context Availability Request Parse";

  case LmtQcrParse:          return (char*) "Query Context Request Parse";
  case LmtScrParse:          return (char*) "Subscribe Context Request Parse";
  case LmtUcsrParse:         return (char*) "Update Context Subscription Request Parse";
  case LmtUncrParse:         return (char*) "Unsubscribe Context Request Parse";
  case LmtNcrParse:          return (char*) "Notify Context Request Parse";
  case LmtUpcrParse:         return (char*) "Update Context Request Parse";

  case LmtRcrDump:          return (char*) "Register Context Request Dump";
  case LmtDcarDump:         return (char*) "Discover Context Availability Request Dump";
  case LmtScarDump:         return (char*) "Subscribe Context Availability Request Dump";
  case LmtUcasDump:         return (char*) "Update Context Availability Subscription Request Dump";
  case LmtUcarDump:         return (char*) "Unsubscribe Context Availability Request Dump";
  case LmtNcarDump:         return (char*) "Notify Context Availability Request Dump";

  case LmtQcrDump:          return (char*) "Query Context Request Dump";
  case LmtScrDump:          return (char*) "Subscribe Context Request Dump";
  case LmtUcsrDump:         return (char*) "Update Context Subscription Request Dump";
  case LmtUncrDump:         return (char*) "Unsubscribe Context Request Dump";
  case LmtNcrDump:          return (char*) "Notify Context Request Dump";
  case LmtUpcrDump:         return (char*) "Update Context Request Dump";

  case LmtNullNode:          return (char*) "NULL Node";
  }

  return (char*) 0;
}
