/*
*
* Copyright 2026 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "common/JsonHelper.h"

#include "apiTypesV2/ContextReference.h"

#include "mongoBackend/dbConstants.h"

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONElement.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "mongoDriver/BSONArrayBuilder.h"
#include "mongoDriver/safeMongo.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* ContextReference::toJson -
*/
std::string ContextReference::toJson()
{
  JsonObjectHelper jh;

  jh.addString("id",   this->id);
  jh.addString("type", this->type);

  return jh.str();
}



/* ****************************************************************************
*
* contextReferencesFill -
*/
void contextReferencesFill(const orion::BSONObj& bo, std::vector<ContextReference>* crV)
{
  crV->clear();

  if (!bo.hasField(CSUB_CONTEXT_REFERENCES))
  {
    return;
  }

  orion::BSONElement be = getFieldF(bo, CSUB_CONTEXT_REFERENCES);
  if (be.type() != orion::Array)
  {
    LM_E(("Runtime Error (csub contextReferences field must be Array but is %s)", orion::bsonType2String(be.type())));
    return;
  }

  std::vector<orion::BSONElement> crElements = be.Array();
  for (unsigned int ix = 0; ix < crElements.size(); ++ix)
  {
    if (crElements[ix].type() != orion::Object)
    {
      LM_E(("Runtime Error (csub contextReferences item must be Object but is %s)", orion::bsonType2String(crElements[ix].type())));
      continue;
    }

    orion::BSONObj   crObj = crElements[ix].embeddedObject();
    ContextReference cr;

    cr.id   = crObj.hasField(CSUB_ENTITY_ID)?   getStringFieldF(crObj, CSUB_ENTITY_ID)   : "";
    cr.type = crObj.hasField(CSUB_ENTITY_TYPE)? getStringFieldF(crObj, CSUB_ENTITY_TYPE) : "";

    crV->push_back(cr);
  }
}



/* ****************************************************************************
*
* contextReferencesToBson -
*/
orion::BSONArray contextReferencesToBson(const std::vector<ContextReference>& crV)
{
  orion::BSONArrayBuilder ba;

  for (unsigned int ix = 0; ix < crV.size(); ++ix)
  {
    orion::BSONObjBuilder bob;

    bob.append(CSUB_ENTITY_ID,   crV[ix].id);
    bob.append(CSUB_ENTITY_TYPE, crV[ix].type);

    ba.append(bob.obj());
  }

  return ba.arr();
}
}
