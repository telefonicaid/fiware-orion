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
#include "logMsg/logMsg.h"
#include "ngsi/ContextAttribute.h"
#include "parse/CompoundValueNode.h"
#include "rest/OrionError.h"

// FIXME P5: the following could be not necessary if we optimize the valueBson() thing. See
// the next FIXME P5 comment in this file
#include "mongoBackend/safeMongo.h"
#include "mongoBackend/dbConstants.h"
#include "mongoBackend/dateExpiration.h"



/* ****************************************************************************
*
* USING
*/
using mongo::BSONObjBuilder;
using mongo::BSONArrayBuilder;
using orion::CompoundValueNode;



/* ****************************************************************************
*
* getDateExpiration -
*
* Get the ISO8601 Expiration Date information for the given
* ContextAttribute provided as parameter.
*
* It returns true, except in the case of error (in which in addition errDetail gets filled)
*/
static bool getDateExpiration
(
  const ContextAttribute*  caP,
  mongo::Date_t*           dateExpiration,
  std::string*             errDetail
)
{
  if ((caP->type == DATE_TYPE) || (caP->type == DATE_TYPE_ALT))
  {
    *dateExpiration = mongo::Date_t(caP->numberValue*1000);

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
* date expiration for the entity. In that case, it fills dateExpiration pointer. If a date expiration attribute is not found, then
* dateExpiration is an empty pointer.
*
* This function always return true (no matter if the attribute was found or not), except in an
* error situation, in which case errorDetail is filled.
*/
bool processDateExpirationAtEntityCreation
(
  const ContextAttributeVector&  caV,
  mongo::Date_t*                 dateExpiration,
  std::string*                   errDetail,
  OrionError*                    oe
)
{
  for (unsigned ix = 0; ix < caV.size(); ++ix)
  {
    const ContextAttribute* caP = caV[ix];

    if (caP->name != ENT_EXPIRATION)
    {
      continue;
    }

    if (!getDateExpiration(caP, dateExpiration, errDetail))
    {
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* processDateExpirationAtUpdateAttribute -
*/
bool processDateExpirationAtUpdateAttribute
(
  const ContextAttribute*  targetAttr,
  mongo::Date_t*           dateExpiration,
  bool*                    replaceDate,
  std::string*             errDetail,
  OrionError*              oe
)
{

  /*
   * If the name of the target attribute is the date expiration
   * check for a number value to be used in the mongo::Date_t constructor.
   * If it is an empty value, is interpreted as a date expiration deletion and a NULL pointer is set
   * If valid value, the replaceDate boolean is set to true, in order to manage the new date value
    * in case of replace operation
   */

  if (targetAttr->name == ENT_EXPIRATION)
  {
    if (targetAttr->numberValue)
    {
	  if (!getDateExpiration(targetAttr, dateExpiration, errDetail))
	  {
	    oe->fill(SccBadRequest, *errDetail, "BadRequest");
	    return false;
	  }
	  else
	  {
        *replaceDate = true;
	  }
    }
    else
    {
      *dateExpiration = 0;
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
  mongo::Date_t*                 dateExpiration,
  const ContextAttribute*        targetAttr,
  bool                           actualAppend,
  std::string*                   errDetail,
  OrionError*                    oe
)
{
  if (targetAttr->numberValue)
  {
  if (!getDateExpiration(targetAttr, dateExpiration, errDetail))
    {
      oe->fill(SccBadRequest, *errDetail, "BadRequest");
      return false;
    }
  }
  else
  {
    delete dateExpiration;
    dateExpiration = NULL;
  }

  return true;
}
