/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/
#include <string>
#include <vector>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "mongoBackend/compoundResponses.h"
#include "mongoBackend/dbFieldEncoding.h"



/* ****************************************************************************
*
* addCompoundNode -
*
*/
static void addCompoundNode(orion::CompoundValueNode* cvP, const orion::BSONElement& e)
{
  if ((e.type() != orion::String)       &&
      (e.type() != orion::Bool)         &&
      (e.type() != orion::NumberDouble) &&
      (e.type() != orion::jstNULL)      &&
      (e.type() != orion::Object)       &&
      (e.type() != orion::Array))
  {
    LM_E(("Runtime Error (unknown BSON type: %d)", e.type()));
    return;
  }

  orion::CompoundValueNode* child = new orion::CompoundValueNode(orion::ValueTypeObject);
  child->name = dbDotDecode(e.fieldName());

  switch (e.type())
  {
  case orion::String:
    child->valueType  = orion::ValueTypeString;
    child->stringValue = e.String();
    break;

  case orion::Bool:
    child->valueType  = orion::ValueTypeBoolean;
    child->boolValue = e.Bool();
    break;

  case orion::NumberDouble:
    child->valueType  = orion::ValueTypeNumber;
    child->numberValue = e.Number();
    break;

  case orion::jstNULL:
    child->valueType  = orion::ValueTypeNull;
    break;

  case orion::Object:
    compoundObjectResponse(child, e);
    break;

  case orion::Array:
    compoundVectorResponse(child, e);
    break;

  default:
    //
    // We need the default clause to avoid 'enumeration value X not handled in switch' errors
    // due to -Werror=switch at compilation time
    //
    break;
  }

  cvP->add(child);
}


/* ****************************************************************************
*
* compoundObjectResponse -
*/
void compoundObjectResponse(orion::CompoundValueNode* cvP, const orion::BSONElement& be)
{
  orion::BSONObj obj = be.embeddedObject();

  cvP->valueType = orion::ValueTypeObject;
  std::vector<orion::BSONElement> vBe;
  obj.toElementsVector(&vBe);
  for (unsigned int ix = 0; ix < vBe.size(); ix++)
  {
    addCompoundNode(cvP, vBe[ix]);
  }
}


/* ****************************************************************************
*
* compoundVectorResponse -
*/
void compoundVectorResponse(orion::CompoundValueNode* cvP, const orion::BSONElement& be)
{
  std::vector<orion::BSONElement> vec = be.Array();

  cvP->valueType = orion::ValueTypeVector;
  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    orion::BSONElement e = vec[ix];
    addCompoundNode(cvP, e);
  }
}
