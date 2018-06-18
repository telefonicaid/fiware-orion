#ifndef SRC_LIB_ORIONTYPES_UPDATE_ACTION_TYPE_H
#define SRC_LIB_ORIONTYPES_UPDATE_ACTION_TYPE_H

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

#include "common/globals.h"



/* ****************************************************************************
*
* actionTypes enum (and rendering functions) -
*/
typedef enum ActionType
{
  ActionTypeUpdate,
  ActionTypeAppend,
  ActionTypeAppendStrict,
  ActionTypeDelete,
  ActionTypeReplace,
  ActionTypeUnknown
} ActionType;



/* ****************************************************************************
*
* actionTypeString -
*/
extern std::string actionTypeString(ApiVersion apiVersion, ActionType action);



/* ****************************************************************************
*
* parseActionTypeV1 -
*/
extern ActionType parseActionTypeV1(const std::string& action);



/* ****************************************************************************
*
* parseActionTypeV2 -
*/
extern ActionType parseActionTypeV2(const std::string& actionType);



#endif // SRC_LIB_ORIONTYPES_UPDATE_ACTION_TYPE_H
