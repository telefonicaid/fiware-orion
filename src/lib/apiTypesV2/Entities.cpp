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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>
#include <map>

#include "logMsg/traceLevels.h"
#include "logMsg/logMsg.h"
#include "ngsi10/QueryContextResponse.h"
#include "apiTypesV2/Entities.h"



/* ****************************************************************************
*
* Entities::Entities -
*/
Entities::Entities()
{
  oe.fill(SccNone, "", "");
}



/* ****************************************************************************
*
* Entities::~Entities -
*/
Entities::~Entities()
{
  release();
}



/* ****************************************************************************
*
* Entities::render -
*
*/
std::string Entities::render
(
  std::map<std::string, bool>&         uriParamOptions,
  std::map<std::string, std::string>&  uriParam
)
{
  return vec.render(uriParamOptions, uriParam);
}



/* ****************************************************************************
*
* Entities::check -
*
* NOTE
*   The 'check' method is normally only used to check that incoming payload is correct.
*   For now (at least), the Entities type is only used as outgoing payload ...
*/
std::string Entities::check(RequestType requestType)
{
  return vec.check(requestType);
}



/* ****************************************************************************
*
* Entities::present -
*/
void Entities::present(const std::string& indent)
{
  LM_T(LmtPresent, ("%s%d Entities:", indent.c_str(), vec.size()));
  vec.present(indent + "  ");
}



/* ****************************************************************************
*
* Entities::release -
*/
void Entities::release(void)
{
  vec.release();
}



/* ****************************************************************************
*
* Entities::fill -
*
* NOTE
*   The errorCode field from qcrsP is not used at all if errorCode::code equals SccOk.
*   This means that e.g. the "Count:" in errorCode::details (from v1 logic) will not be
*   present in the Entities for v2 (that number is in the HTTP header Fiware-Total-Count for v2).
*   Other values for "details" are lost as well, if errorCode::code equals SccOk.
*/
void Entities::fill(QueryContextResponse* qcrsP)
{
  if (qcrsP->errorCode.code == SccContextElementNotFound)
  {
    //
    // If no entities are found, we respond with a 200 OK
    // and an empty vector of entities ( [] )
    //

    oe.fill(SccOk, "", "OK");
    return;
  }
  else if (qcrsP->errorCode.code != SccOk)
  {
    //
    // If any other error - use the error for the response
    //

    oe.fill(qcrsP->errorCode.code, qcrsP->errorCode.details, qcrsP->errorCode.reasonPhrase);
    return;
  }

  for (unsigned int ix = 0; ix < qcrsP->contextElementResponseVector.size(); ++ix)
  {
    ContextElement* ceP = &qcrsP->contextElementResponseVector[ix]->contextElement;
    StatusCode* scP = &qcrsP->contextElementResponseVector[ix]->statusCode;

    if (scP->code == SccReceiverInternalError)
    {
      // FIXME P4: Do we need to release the memory allocated in 'vec' before returning? I don't
      // think so, as the releasing logic in the upper layer will deal with that but
      // let's do anyway just in case... (we don't have a ft covering this, so valgrind suite
      // cannot help here and it is better to ensure)
      oe.fill(SccReceiverInternalError, scP->details, "InternalServerError");
      vec.release();
      return;
    }
    else
    {
      Entity*         eP  = new Entity();

      eP->id        = ceP->entityId.id;
      eP->type      = ceP->entityId.type;
      eP->isPattern = ceP->entityId.isPattern;
      eP->creDate   = ceP->entityId.creDate;
      eP->modDate   = ceP->entityId.modDate;

      eP->attributeVector.fill(&ceP->contextAttributeVector);
      vec.push_back(eP);
    }
  }
}
