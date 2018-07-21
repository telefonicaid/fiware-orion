/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "alarmMgr/alarmMgr.h"

#include "orionTypes/OrionValueType.h"
#include "ngsi/ParseData.h"
#include "parse/CompoundValueNode.h"
#include "parse/compoundValue.h"



namespace orion
{
/* ****************************************************************************
*
* compoundValueStart - 
*
* This function is called when the first compound node is encountered, so not
* only must the root be created, but also the first node of the compound tree
* must be taken care of. This is done by calling compoundValueMiddle.
*/
void compoundValueStart
(
    ConnectionInfo*     ciP,
    const std::string&  path,
    const std::string&  name,
    const std::string&  value,
    const std::string&  rest,
    orion::ValueType    type,
    bool                fatherIsVector
)
{
  ciP->inCompoundValue   = true;
  ciP->compoundValueP    = new orion::CompoundValueNode(orion::ValueTypeObject);
  ciP->compoundValueRoot = ciP->compoundValueP;

  LM_T(LmtCompoundValueContainer, ("Set current container to '%s' (%s)",
                                   ciP->compoundValueP->path.c_str(),
                                   ciP->compoundValueP->name.c_str()));


  if (fatherIsVector)
  {
    ciP->compoundValueP->valueType = orion::ValueTypeVector;
  }

  //
  // In the parsing routines, in all context attributes that can accept Compound values,
  // a pointer to the one where we are right now is saved in ParseData.
  //
  // If this pointer is not set, it is a fatal error and the broker dies, because of the
  // following LM_X, that does an exit
  // It is better to exit here and clearly see the error, than to continue and get strange
  // outputs that will be difficult to trace back to here.
  //
  if (ciP->parseDataP->lastContextAttribute == NULL)
    orionExitFunction(1, "No pointer to last ContextAttribute");

  ciP->compoundValueVector.push_back(ciP->compoundValueP);
  LM_T(LmtCompoundValueAdd, ("Created new toplevel element"));
  compoundValueMiddle(ciP, rest, name, value, type);
}



/* ****************************************************************************
*
* compoundValueMiddle - 
*
* containerType: vector/object/string
*/
void compoundValueMiddle
(
  ConnectionInfo*     ciP,
  const std::string&  relPath,
  const std::string&  name,
  const std::string&  value,
  orion::ValueType    type
)
{
  LM_T(LmtCompoundValue, ("Compound MIDDLE %s: %s: NAME: '%s', VALUE: '%s'",
                          relPath.c_str(),
                          orion::valueTypeName(type),
                          name.c_str(),
                          value.c_str()));

  if ((type == orion::ValueTypeVector) || (type == orion::ValueTypeObject))
  {
    // If we enter a vector or an object, the container must change (so that we add to this container from now on).
    // ciP->compoundValueP points to the current compound container
    ciP->compoundValueP = ciP->compoundValueP->add(type, name, "");

    LM_T(LmtCompoundValueContainer, ("Set current container to '%s' (%s)",
                                     ciP->compoundValueP->path.c_str(),
                                     ciP->compoundValueP->name.c_str()));
  }
  else
  {
    ciP->compoundValueP->add(type, name, value);
  }
}



/* ****************************************************************************
*
* compoundValueEnd - 
*/
void compoundValueEnd(ConnectionInfo* ciP, ParseData* parseDataP)
{
  LM_T(LmtCompoundValue, ("Compound END"));

  // Finish the compound value - error check included
  std::string status = ciP->compoundValueRoot->finish();

  // Any problems in 'finish'?
  // If so, mark as erroneous
  if (status != "OK")
  {
    ciP->httpStatusCode = SccBadRequest;
    ciP->answer = std::string("compound value error: ") + status;
    alarmMgr.badInput(clientIp, ciP->answer);
  }

  //
  // Give the root pointer of this Compound to the active ContextAttribute
  // lastContextAttribute is set in the JSON v1 parsing routines, to point at the
  // latest contextAttribute, i.e. the attribute whose 'contextValue' is the
  // owner of this compound value tree.
  //

  LM_T(LmtCompoundValue, ("Set compoundValueP (%p) for attribute at %p",
                          ciP->compoundValueRoot,
                          parseDataP->lastContextAttribute));

  //
  // Special case for updateContextAttributeRequest. This payload has no
  // ContextAttribute to point to by lastContextAttribute, as the whole payload
  // is a part of a ContextAttribute.
  //
  if (strcmp(ciP->payloadWord, "updateContextAttributeRequest") == 0)
#if 0
    if ((ciP->requestType = AttributeValueInstance)              ||
        (ciP->requestType = AttributeValueInstanceWithTypeAndId) ||
        (ciP->requestType = IndividualContextEntityAttribute)    ||
        (ciP->requestType = IndividualContextEntityAttributeWithTypeAndId))
#endif
  {
    parseDataP->upcar.res.compoundValueP = ciP->compoundValueRoot;
  }
  else
  {
    parseDataP->lastContextAttribute->compoundValueP = ciP->compoundValueRoot;
  }

  // Reset the Compound stuff in ConnectionInfo
  ciP->compoundValueRoot = NULL;
  ciP->compoundValueP    = NULL;
  LM_T(LmtCompoundValueContainer, ("Set current container to NULL"));
  ciP->inCompoundValue   = false;
}
}
