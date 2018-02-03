#ifndef SRC_LIB_MONGOBACKEND_MONGOCOMMONREGISTER_H_
#define SRC_LIB_MONGOBACKEND_MONGOCOMMONREGISTER_H_

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
* Author: Fermín Galán
*/
#include <string>

#include "mongo/client/dbclient.h"

#include "ngsi9/RegisterContextRequest.h"
#include "ngsi9/RegisterContextResponse.h"
#include "apiTypesV2/Registration.h"



/* ****************************************************************************
*
* processRegisterContext -
*/
extern HttpStatusCode processRegisterContext
(
  RegisterContextRequest*   requestP,
  RegisterContextResponse*  responseP,
  mongo::OID*               id,
  const std::string&        tenant,
  const std::string&        servicePath,
  const std::string&        format,
  const std::string&        fiwareCorrelator
);



/* ****************************************************************************
*
* mongoRegistrationIdExtract -
*/
extern void mongoRegistrationIdExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r);



/* ****************************************************************************
*
* mongoDescriptionExtract -
*/
extern void mongoDescriptionExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel);



/* ****************************************************************************
*
* mongoProviderExtract -
*/
extern void mongoProviderExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel);



/* ****************************************************************************
*
* mongoEntitiesExtract -
*/
extern void mongoEntitiesExtract(ngsiv2::Registration* regP, const mongo::BSONObj& cr0, const char* dbLabel);



/* ****************************************************************************
*
* mongoAttributesExtract -
*/
extern void setAttributes(ngsiv2::Registration* regP, const mongo::BSONObj& cr0, const char* dbLabel);



/* ****************************************************************************
*
* mongoDataProvidedExtract -
*/
extern bool mongoDataProvidedExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, bool arrayAllowed, const char* dbLabel);



/* ****************************************************************************
*
* mongoExpiresExtract -
*/
extern void mongoExpiresExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel);



/* ****************************************************************************
*
* mongoStatusExtract -
*/
extern void mongoStatusExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel);



/* ****************************************************************************
*
* mongoForwardingInformationExtract -
*/
extern void mongoForwardingInformationExtract(ngsiv2::Registration* regP, const mongo::BSONObj& r, const char* dbLabel);

#endif  // SRC_LIB_MONGOBACKEND_MONGOCOMMONREGISTER_H_
