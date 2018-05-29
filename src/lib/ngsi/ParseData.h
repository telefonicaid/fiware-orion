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
#include "ngsi/ContextRegistrationAttribute.h"
#include "ngsi/ContextElementResponse.h"
#include "ngsi/Metadata.h"
#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"
#include "ngsi9/DiscoverContextAvailabilityRequest.h"
#include "ngsi9/DiscoverContextAvailabilityResponse.h"
#include "ngsi9/SubscribeContextAvailabilityRequest.h"
#include "ngsi9/UnsubscribeContextAvailabilityRequest.h"
#include "ngsi9/UpdateContextAvailabilitySubscriptionRequest.h"
#include "ngsi9/NotifyContextAvailabilityRequest.h"
#include "ngsi10/SubscribeContextRequest.h"
#include "ngsi10/QueryContextRequest.h"
#include "ngsi10/QueryContextResponse.h"
#include "ngsi10/UnsubscribeContextRequest.h"
#include "ngsi10/UpdateContextRequest.h"
#include "ngsi10/UpdateContextResponse.h"
#include "ngsi10/UpdateContextSubscriptionRequest.h"
#include "ngsi10/NotifyContextRequest.h"
#include "convenience/RegisterProviderRequest.h"
#include "convenience/UpdateContextElementRequest.h"
#include "convenience/AppendContextElementRequest.h"
#include "convenience/UpdateContextAttributeRequest.h"
#include "apiTypesV2/Entity.h"
#include "apiTypesV2/BatchQuery.h"
#include "apiTypesV2/BatchUpdate.h"
#include "apiTypesV2/SubscriptionUpdate.h"
#include "apiTypesV2/Registration.h"



/* ****************************************************************************
*
* RegisterContextData - output data and help pointers for RegisterContextRequest
*/
struct RegisterContextData
{
  RegisterContextData(): crP(NULL), entityIdP(NULL), attributeP(NULL), attributeMetadataP(NULL), registrationMetadataP(NULL) {}
  RegisterContextRequest         res;
  ContextRegistration*           crP;
  EntityId*                      entityIdP;
  ContextRegistrationAttribute*  attributeP;
  Metadata*                      attributeMetadataP;
  Metadata*                      registrationMetadataP;
};



/* ****************************************************************************
*
* RegisterContextResponseData - output data and help pointers for RegisterContextRequest
*/
typedef struct RegisterContextResponseData
{
  RegisterContextResponse        res;
} RegisterContextResponseData;



/* ****************************************************************************
*
* DiscoverContextAvailabilityData -
*/
struct DiscoverContextAvailabilityData
{
  DiscoverContextAvailabilityData(): entityIdP(NULL), scopeP(NULL) {}
  DiscoverContextAvailabilityRequest  res;
  EntityId*                           entityIdP;
  Scope*                              scopeP;
};



/* ****************************************************************************
*
* DiscoverContextAvailabilityResponseData -
*/
typedef struct DiscoverContextAvailabilityResponseData
{
  DiscoverContextAvailabilityResponse  res;
} DiscoverContextAvailabilityResponseData;



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
  QueryContextResponseData(): cerP(NULL), attributeP(NULL), metadataP(NULL), domainMetadataP(NULL) {}
  QueryContextResponse     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;
  Metadata*                domainMetadataP;
};



/* ****************************************************************************
*
* SubscribeContextAvailabilityData - 
*/
struct SubscribeContextAvailabilityData
{
  SubscribeContextAvailabilityData(): entityIdP(NULL), scopeP(NULL) {}
  SubscribeContextAvailabilityRequest  res;
  EntityId*                            entityIdP;
  Scope*                               scopeP;
};



/* ****************************************************************************
*
* SubscribeContextData -
*/
struct SubscribeContextData
{
  SubscribeContextData():entityIdP(NULL), attributeMetadataP(NULL), restrictionP(NULL), notifyConditionP(NULL), scopeP(NULL), vertexP(NULL) {}
  SubscribeContextRequest        res;
  EntityId*                      entityIdP;
  Metadata*                      attributeMetadataP;
  Restriction*                   restrictionP;
  NotifyCondition*               notifyConditionP;
  Scope*                         scopeP;
  orion::Point*                  vertexP;
};



/* ****************************************************************************
*
* UnsubscribeContextAvailabilityData -
*/
typedef struct UnsubscribeContextAvailabilityData
{
  UnsubscribeContextAvailabilityRequest        res;
} UnsubscribeContextAvailabilityData;



/* ****************************************************************************
*
* UnsubscribeContextData -
*/
typedef struct UnsubscribeContextData
{
  UnsubscribeContextRequest res;
} UnsubscribeContextData;



/* ****************************************************************************
*
* NotifyContextData -
*/
struct NotifyContextData
{
  NotifyContextData(): cerP(NULL), attributeP(NULL), attributeMetadataP(NULL), domainMetadataP(NULL) {}
  NotifyContextRequest     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                attributeMetadataP;
  Metadata*                domainMetadataP;
};



/* ****************************************************************************
*
* NotifyContextAvailabilityData -
*/
struct NotifyContextAvailabilityData
{
  NotifyContextAvailabilityData(): crrP(NULL), entityIdP(NULL), craP(NULL), attributeMetadataP(NULL), regMetadataP(NULL) {}
  NotifyContextAvailabilityRequest     res;
  ContextRegistrationResponse*         crrP;
  EntityId*                            entityIdP;
  ContextRegistrationAttribute*        craP;
  Metadata*                            attributeMetadataP;
  Metadata*                            regMetadataP;
};



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionData -
*/
struct UpdateContextAvailabilitySubscriptionData
{
  UpdateContextAvailabilitySubscriptionData(): entityIdP(NULL), scopeP(NULL) {}
  UpdateContextAvailabilitySubscriptionRequest  res;
  EntityId*                                     entityIdP;
  Scope*                                        scopeP;
};



/* ****************************************************************************
*
* UpdateContextData -
*/
struct UpdateContextData
{
  UpdateContextData(): ceP(NULL), entityIdP(NULL), attributeP(NULL), contextMetadataP(NULL), domainMetadataP(NULL) {}
  UpdateContextRequest   res;
  ContextElement*        ceP;
  EntityId*              entityIdP;
  ContextAttribute*      attributeP;
  Metadata*              contextMetadataP;
  Metadata*              domainMetadataP;
};



/* ****************************************************************************
*
* UpdateContextResponseData - 
*/
struct UpdateContextResponseData
{
  UpdateContextResponseData(): cerP(NULL), attributeP(NULL), metadataP(NULL), domainMetadataP(NULL) {}
  UpdateContextResponse    res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;
  Metadata*                domainMetadataP;
};



/* ****************************************************************************
*
* UpdateContextSubscriptionData - 
*/
struct UpdateContextSubscriptionData
{
  UpdateContextSubscriptionData(): notifyConditionP(NULL), scopeP(NULL), vertexP(NULL) {}
  UpdateContextSubscriptionRequest  res;
  NotifyCondition*                  notifyConditionP;
  Scope*                            scopeP;
  orion::Point*                     vertexP;
};



/* ****************************************************************************
*
* RegisterProviderRequestData -
*/
struct RegisterProviderRequestData
{
  RegisterProviderRequestData(): metadataP(NULL) {}
  RegisterProviderRequest  res;
  Metadata*                metadataP;
};



/* ****************************************************************************
*
* UpdateContextElementData -
*/
struct UpdateContextElementData
{
  UpdateContextElementData(): attributeP(NULL), metadataP(NULL) {}
  UpdateContextElementRequest  res;
  ContextAttribute*            attributeP;
  Metadata*                    metadataP;
};



/* ****************************************************************************
*
* AppendContextElementData -
*/
struct AppendContextElementData
{
  AppendContextElementData(): attributeP(NULL), metadataP(NULL), domainMetadataP(NULL) {}
  AppendContextElementRequest  res;
  ContextAttribute*            attributeP;
  Metadata*                    metadataP;
  Metadata*                    domainMetadataP;
};



/* ****************************************************************************
*
* UpdateContextAttributeData -
*/
struct UpdateContextAttributeData
{
  UpdateContextAttributeData(): metadataP(NULL) {}
  UpdateContextAttributeRequest  res;
  Metadata*                      metadataP;
  ContextAttribute               attribute;
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
*/
typedef struct ParseData
{
  ParseData():  lastContextAttribute(NULL) { }

  std::string                                 errorString;
  ContextAttribute*                           lastContextAttribute;
  RegisterContextData                         rcr;
  DiscoverContextAvailabilityData             dcar;
  SubscribeContextAvailabilityData            scar;
  UnsubscribeContextAvailabilityData          ucar;
  UpdateContextAvailabilitySubscriptionData   ucas;

  QueryContextData                            qcr;
  SubscribeContextData                        scr;
  UnsubscribeContextData                      uncr;
  UpdateContextData                           upcr;
  UpdateContextSubscriptionData               ucsr;
  NotifyContextData                           ncr;
  NotifyContextAvailabilityData               ncar;

  RegisterProviderRequestData                 rpr;
  UpdateContextElementData                    ucer;
  AppendContextElementData                    acer;
  UpdateContextAttributeData                  upcar;

  RegisterContextResponseData                 rcrs;
  DiscoverContextAvailabilityResponseData     dcars;
  QueryContextResponseData                    qcrs;
  UpdateContextResponseData                   upcrs;

  EntityData                                  ent;
  AttributeData                               attr;
  AttributeValueData                          av;
  BatchQueryData                              bq;
  BatchUpdateData                             bu;

  ngsiv2::SubscriptionUpdate                  subsV2;
  ngsiv2::Registration                        reg;
} ParseData;

#endif  // SRC_LIB_NGSI_PARSEDATA_H_
