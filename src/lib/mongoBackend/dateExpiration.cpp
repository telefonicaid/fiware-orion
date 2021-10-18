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
*
*/
#include <string>
#include <vector>

#include "common/string.h"
#include "common/globals.h"
#include "common/errorMessages.h"
#include "logMsg/logMsg.h"
#include "ngsi/ContextAttribute.h"

// FIXME P5: the following could be not necessary if we optimize the valueBson() thing. See
// the next FIXME P5 comment in this file
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dateExpiration.h"



/* ****************************************************************************
*
* USING
*/
using orion::CompoundValueNode;



/* ****************************************************************************
*
* getDateExpiration -
*
* Get the ISO8601 Expiration Date information for the given
* ContextAttribute provided as parameter, in order to construct the corresponding Mongo date object.
*
* It returns true, except in the case of error (in which in addition errDetail gets filled)
*/
static bool getDateExpiration
(
  const ContextAttribute*  caP,
  orion::BSONDate*         dateExpiration,
  std::string*             errDetail
)
{
  if ((caP->type == DATE_TYPE) || (caP->type == DATE_TYPE_ALT))
  {
    *dateExpiration = orion::BSONDate(caP->numberValue*1000);

    return true;
  }

  LM_E(("Runtime Error (attribute detected as date expiration but invalid type: %s)", caP->type.c_str()));
  *errDetail = "error processing date expiration attribute, see log traces";

  return false;
}



/* ****************************************************************************
*
* processDateExpirationAtEntityCreation -
*
* This function process the context attribute vector, searching for an attribute meaning
* date expiration for the entity. In that case, it fills dateExpiration value.
* If a date expiration attribute is not found dateExpiration keeps empty value set by the caller function.
*
* This function always return true (no matter if the attribute was found or not), except in an
* error situation, in which case errorDetail is filled.
*/
bool processDateExpirationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  orion::BSONDate*               dateExpiration,
  std::string*                   errDetail,
  OrionError*                    oe
)
{
  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    ContextAttribute* caP = caV[ix];

    if (caP->name == DATE_EXPIRES)
    {
      if (!getDateExpiration(caP, dateExpiration, errDetail))
      {
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
        return false;
      }

      // dateExpires is not rendered except if explicitly requested (i.e. shadow = true)
      // (note we are in update entity code but the attribute could be rendered in a triggered notification)
      caP->shadowed = true;
      return true;
    }
  }

  return true;
}



/* ****************************************************************************
*
* processDateExpirationAtUpdateAttribute -
*
* If the name of the target attribute is the date expiration,
* check for a number value that will be used in the orion::BSONDate constructor.
* If it is an empty value, is interpreted as a date expiration deletion and the ACTUAL date value
* is set to 0, in order to signal the caller function.
* If valid value, also dateExpirationInPayload boolean is set to true, in order to manage the new date value
* in case the update is a replace operation.
*/
bool processDateExpirationAtUpdateAttribute
(
  const ContextAttribute*  targetAttr,
  orion::BSONDate*         dateExpiration,
  bool*                    dateExpirationInPayload,
  std::string*             errDetail,
  OrionError*              oe
)
{
  if (targetAttr->name == DATE_EXPIRES)
  {
    if (targetAttr->numberValue != NO_EXPIRATION_DATE)
    {
      if (!getDateExpiration(targetAttr, dateExpiration, errDetail))
      {
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
        return false;
      }
      else
      {
        *dateExpirationInPayload = true;
      }
    }
    else
    {
      *dateExpiration = orion::BSONDate(NO_EXPIRATION_DATE);
    }
  }

  return true;
}



/* ****************************************************************************
*
* processDateExpirationAtAppendAttribute -
*/
bool processDateExpirationAtAppendAttribute
(
  orion::BSONDate*               dateExpiration,
  const ContextAttribute*        targetAttr,
  bool                           actualAppend,
  std::string*                   errDetail,
  OrionError*                    oe
)
{
  if (targetAttr->name == DATE_EXPIRES)
  {
    if (targetAttr->numberValue != NO_EXPIRATION_DATE)
    {
      if (!getDateExpiration(targetAttr, dateExpiration, errDetail))
      {
        oe->fill(SccBadRequest, *errDetail, ERROR_BAD_REQUEST);
        return false;
      }
    }
    else
    {
      *dateExpiration = orion::BSONDate(0);
    }
  }

  return true;
}
