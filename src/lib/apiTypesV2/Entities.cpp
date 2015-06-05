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

#include "ngsi10/QueryContextResponse.h"
#include "apiTypesV2/Entities.h"



/* ****************************************************************************
*
* Entities::Entities - 
*/
Entities::Entities()
{
  errorCode.fill(SccOk);
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
* If no error reported in errorCode, render the vector of entities.
* Otherwise, render the errorCode.
*/
std::string Entities::render(ConnectionInfo* ciP, RequestType requestType)
{
  if ((errorCode.code == SccOk) || (errorCode.code == SccNone))
  {
    return vec.render(ciP, requestType, false);
  }

  return errorCode.render(JSON, "", false, false);
} 



/* ****************************************************************************
*
* Entities::check - 
*/
std::string Entities::check(ConnectionInfo* ciP, RequestType requestType)
{
  return "OK";
}



/* ****************************************************************************
*
* Entities::present - 
*/
void Entities::present(const std::string& indent, const std::string& caller)
{
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
*/
void Entities::fill(QueryContextResponse* qcrsP)
{
  if (qcrsP->errorCode.code != SccOk)
  {
    errorCode.fill(qcrsP->errorCode);
  }
  else
  {
    unsigned int ix;

    for (ix = 0; ix < qcrsP->contextElementResponseVector.size(); ++ix)
    {
      ContextElement* ceP = &qcrsP->contextElementResponseVector[ix]->contextElement;
      Entity*         eP  = new Entity();

      eP->id        = ceP->entityId.id;
      eP->type      = ceP->entityId.type;
      eP->isPattern = ceP->entityId.isPattern;

      eP->attributeVector.fill(&ceP->contextAttributeVector);
      vec.push_back(eP);
    }
  }
}
