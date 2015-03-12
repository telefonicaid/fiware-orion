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

#include "common/Format.h"
#include "orionTypes/areas.h"
#include "ngsi/AttributeAssociation.h"
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



/* ****************************************************************************
*
* RegisterContextData - output data and help pointers for RegisterContextRequest
*/
typedef struct RegisterContextData
{
  RegisterContextRequest         res;
  ContextRegistration*           crP;
  EntityId*                      entityIdP;
  ContextRegistrationAttribute*  attributeP;
  Metadata*                      attributeMetadataP;
  Metadata*                      registrationMetadataP;
  AttributeAssociation*          attributeAssociationP;
} RegisterContextData;



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
typedef struct DiscoverContextAvailabilityData
{
  DiscoverContextAvailabilityRequest  res;
  EntityId*                           entityIdP;
  Scope*                              scopeP;
} DiscoverContextAvailabilityData;



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
typedef struct QueryContextData
{
  QueryContextRequest  res;
  EntityId*            entityIdP;
  Scope*               scopeP;
  orion::Point*        vertexP;
} QueryContextData;



/* ****************************************************************************
*
* QueryContextResponseData - 
*/
typedef struct QueryContextResponseData
{
  QueryContextResponse     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;
  Metadata*                domainMetadataP;
} QueryContextResponseData;



/* ****************************************************************************
*
* SubscribeContextAvailabilityData - 
*/
typedef struct SubscribeContextAvailabilityData
{
  SubscribeContextAvailabilityRequest  res;
  EntityId*                            entityIdP;
  Scope*                               scopeP;
} SubscribeContextAvailabilityData;



/* ****************************************************************************
*
* SubscribeContextData -
*/
typedef struct SubscribeContextData
{
  SubscribeContextRequest        res;
  EntityId*                      entityIdP;
  Metadata*                      attributeMetadataP;
  Restriction*                   restrictionP;
  NotifyCondition*               notifyConditionP;
  Scope*                         scopeP;
  orion::Point*                  vertexP;
} SubscribeContextData;



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
typedef struct NotifyContextData
{
  NotifyContextRequest     res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                attributeMetadataP;
  Metadata*                domainMetadataP;
} NotifyContextData;



/* ****************************************************************************
*
* NotifyContextAvailabilityData -
*/
typedef struct NotifyContextAvailabilityData
{
  NotifyContextAvailabilityRequest     res;
  ContextRegistrationResponse*         crrP;
  EntityId*                            entityIdP;
  ContextRegistrationAttribute*        craP;
  Metadata*                            attributeMetadataP;
  Metadata*                            regMetadataP;
} NotifyContextAvailabilityData;



/* ****************************************************************************
*
* UpdateContextAvailabilitySubscriptionData -
*/
typedef struct UpdateContextAvailabilitySubscriptionData
{
  UpdateContextAvailabilitySubscriptionRequest  res;
  EntityId*                                     entityIdP;
  Scope*                                        scopeP;
} UpdateContextAvailabilitySubscriptionData;



/* ****************************************************************************
*
* UpdateContextData -
*/
typedef struct UpdateContextData
{
  UpdateContextRequest   res;
  ContextElement*        ceP;
  EntityId*              entityIdP;
  ContextAttribute*      attributeP;
  Metadata*              contextMetadataP;
  Metadata*              domainMetadataP;
} UpdateContextData;



/* ****************************************************************************
*
* UpdateContextResponseData - 
*/
typedef struct UpdateContextResponseData
{
  UpdateContextResponse    res;
  ContextElementResponse*  cerP;
  ContextAttribute*        attributeP;
  Metadata*                metadataP;
  Metadata*                domainMetadataP;
} UpdateContextResponseData;



/* ****************************************************************************
*
* UpdateContextSubscriptionData - 
*/
typedef struct UpdateContextSubscriptionData
{
  UpdateContextSubscriptionRequest  res;
  NotifyCondition*                  notifyConditionP;
  Scope*                            scopeP;
  orion::Point*                     vertexP;
} UpdateContextSubscriptionData;



/* ****************************************************************************
*
* RegisterProviderRequestData -
*/
typedef struct RegisterProviderRequestData
{
  RegisterProviderRequest  res;
  Metadata*                metadataP;
} RegisterProviderRequestData;



/* ****************************************************************************
*
* UpdateContextElementData -
*/
typedef struct UpdateContextElementData
{
  UpdateContextElementRequest  res;
  ContextAttribute*            attributeP;
  Metadata*                    metadataP;
} UpdateContextElementData;



/* ****************************************************************************
*
* AppendContextElementData -
*/
typedef struct AppendContextElementData
{
  AppendContextElementRequest  res;
  ContextAttribute*            attributeP;
  Metadata*                    metadataP;
  Metadata*                    domainMetadataP;
} AppendContextElementData;



/* ****************************************************************************
*
* UpdateContextAttributeData -
*/
typedef struct UpdateContextAttributeData
{
  UpdateContextAttributeRequest  res;
  Metadata*                      metadataP;
  ContextAttribute               attribute;
} UpdateContextAttributeData;



/* ****************************************************************************
*
* ParseData -
*/
typedef struct ParseData
{
  ParseData()
  {
    lastContextAttribute = NULL;
  }

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
} ParseData;

#endif  // SRC_LIB_NGSI_PARSEDATA_H_
