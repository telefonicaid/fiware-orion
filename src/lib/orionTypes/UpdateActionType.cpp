/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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

#include <string>
#include <string.h>

#include "common/globals.h"
#include "orionTypes/UpdateActionType.h"



/* ****************************************************************************
*
* actionTypeString -
*/
std::string actionTypeString(ApiVersion apiVersion, ActionType action)
{
  if (apiVersion == V2)
  {
    switch (action)
    {
    case ActionTypeUpdate:       return "update";
    case ActionTypeAppend:       return "append";
    case ActionTypeAppendStrict: return "appendStrict";
    case ActionTypeDelete:       return "delete";
    case ActionTypeReplace:      return "replace";
    default:                     return "unknownAction";
    }
  }
  else
  {
    switch (action)
    {
    case ActionTypeUpdate:       return "UPDATE";
    case ActionTypeAppend:       return "APPEND";
    case ActionTypeAppendStrict: return "APPEND_STRICT";
    case ActionTypeDelete:       return "DELETE";
    case ActionTypeReplace:      return "REPLACE";
    default:                     return "UNKNOWN_ACTION";
    }
  }
}



/* ****************************************************************************
*
* parseActionTypeV1 -
*/
ActionType parseActionTypeV1(const std::string& action)
{
  if (strcasecmp(action.c_str(), "update") == 0)
  {
    return ActionTypeUpdate;
  }
  else if (strcasecmp(action.c_str(), "append") == 0)
  {
    return ActionTypeAppend;
  }
  else if (strcasecmp(action.c_str(), "append_strict") == 0)
  {
    return ActionTypeAppendStrict;
  }
  else if (strcasecmp(action.c_str(), "delete") == 0)
  {
    return ActionTypeDelete;
  }
  else if (strcasecmp(action.c_str(), "replace") == 0)
  {
    return ActionTypeReplace;
  }
  else
  {
    return ActionTypeUnknown;
  }
}



/* ****************************************************************************
*
* parseActionTypeV2 -
*/
ActionType parseActionTypeV2(const std::string& actionType)
{
  if (actionType == "update")
  {
    return ActionTypeUpdate;
  }
  else if (actionType == "append")
  {
    return ActionTypeAppend;
  }
  else if (actionType == "appendStrict")
  {
    return ActionTypeAppendStrict;
  }
  else if (actionType == "delete")
  {
    return ActionTypeDelete;
  }
  else if (actionType == "replace")
  {
    return ActionTypeReplace;
  }
  else
  {
    // Last resort: try NGSIv1 based parsing. This could be removed in the future
    return parseActionTypeV1(actionType);
  }
}

