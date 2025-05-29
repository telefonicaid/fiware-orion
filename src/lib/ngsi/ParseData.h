#ifndef SRC_LIB_NGSI_PARSEDATA_H_
#define SRC_LIB_NGSI_PARSEDATA_H_

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
#include <string>

#include "orionTypes/areas.h"
#include "ngsi/EntityId.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/Metadata.h"
#include "ngsi/QueryContextRequest.h"
#include "ngsi/QueryContextResponse.h"
#include "ngsi/UpdateContextRequest.h"
#include "ngsi/UpdateContextResponse.h"
#include "ngsi/NotifyContextRequest.h"
#include "apiTypesV2/Entity.h"
#include "apiTypesV2/BatchQuery.h"
#include "apiTypesV2/BatchUpdate.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "apiTypesV2/Registration.h"



/* ****************************************************************************
*
* QueryContextData -
*/
struct QueryContextData
{
  QueryContextData(): entityIdP(NULL), scopeP(NULL), vertexP(NULL),pointNo(0), coords(0) {}
  QueryContextRequest  res;
  EntityId*            entityIdP;
  Scope*               scopeP;
  orion::Point*        vertexP;
  int                  pointNo;
  int                  coords;
};



/* ****************************************************************************
*
* QueryContextResponseData - 
*/
struct QueryContextResponseData
{
  QueryContextResponseData(): cerP(NULL), attributeP(NULL), metadataP(NULL) {}
  QueryContextResponse     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;

};



/* ****************************************************************************
*
* NotifyContextData -
*/
struct NotifyContextData
{
  NotifyContextData(): cerP(NULL), attributeP(NULL), attributeMetadataP(NULL) {}
  NotifyContextRequest     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                attributeMetadataP;
};



/* ****************************************************************************
*
* UpdateContextData -
*/
struct UpdateContextData
{
  UpdateContextData(): eP(NULL), entityIdP(NULL), attributeP(NULL), contextMetadataP(NULL) {}
  UpdateContextRequest   res;
  Entity*                eP;
  EntityId*              entityIdP;
  ContextAttribute*      attributeP;
  Metadata*              contextMetadataP;
};



/* ****************************************************************************
*
* UpdateContextResponseData - 
*/
struct UpdateContextResponseData
{
  UpdateContextResponseData(): cerP(NULL), attributeP(NULL), metadataP(NULL) {}
  UpdateContextResponse    res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;
};



/* ****************************************************************************
*
* EntityData - 
*/
typedef struct EntityData
{
  Entity  res;
} EntityData;



/* ****************************************************************************
 *
* AttributeData - 
*/
typedef struct AttributeData
{
  ContextAttribute attribute;
} AttributeData;


/* ****************************************************************************
*
* AttributeValueData - 
*/
typedef struct AttributeValueData
{
  ContextAttribute attribute;
} AttributeValueData;



/* ****************************************************************************
*
* BatchQueryData - 
*/
typedef struct BatchQueryData
{
  BatchQuery res;
} BatchQueryData;



/* ****************************************************************************
*
* BatchUpdateData - 
*/
typedef struct BatchUpdateData
{
  BatchUpdate res;
} BatchUpdateData;



/* ****************************************************************************
*
* ParseData -
*
*/
typedef struct ParseData
{
  // filled by jsonRequestTreat() function in jsonRequestTreat.cpp
  EntityData                                  ent;
  AttributeData                               attr;
  AttributeValueData                          av;
  BatchQueryData                              bq;
  BatchUpdateData                             bu;
  ngsiv2::SubscriptionUpdate                  sub;
  ngsiv2::Registration                        reg;
  NotifyContextData                           ncr;

    // Used in postQueryContext() function for the forwarding logic
  QueryContextData                            qcr;
  QueryContextResponseData                    qcrs;

  // Used in postUpdateContext() function for the forwarding logic
  UpdateContextData                           upcr;
  UpdateContextResponseData                   upcrs;
} ParseData;

#endif  // SRC_LIB_NGSI_PARSEDATA_H_
