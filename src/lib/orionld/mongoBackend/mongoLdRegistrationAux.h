#ifndef SRC_LIB_MONGOBACKEND_MONGOLDREGISTRATIONAUX_H_
#define SRC_LIB_MONGOBACKEND_MONGOLDREGISTRATIONAUX_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin, Larysse Savanna and Gabriel Quaresma
*/
#include "mongo/client/dbclient.h"                         // mongo::BSONObj

#include "apiTypesV2/Registration.h"                       // ngsiv2::Registration



/* ****************************************************************************
*
* mongo LD functions
*/
extern void mongoSetLdPropertyV(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetLdRelationshipV(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetLdRegistrationId(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetLdName(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetExpiration(ngsiv2::Registration* regP, const mongo::BSONObj& r);
extern void mongoSetLdObservationInterval(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetLdManagementInterval(ngsiv2::Registration* reg, const mongo::BSONObj& r);
extern void mongoSetLdTimestamp(long long* timestampP, const char* name, const mongo::BSONObj& bobj);
extern bool mongoSetLdTimeInterval(OrionldGeoLocation* geoLocationP, const char* name, const mongo::BSONObj& bobj, char** titleP, char** detailP);
extern bool mongoSetLdProperties(ngsiv2::Registration* regP, const char* name, const mongo::BSONObj& bobj, char** titleP, char** detailP);

#endif  // SRC_LIB_MONGOBACKEND_MONGOLDREGISTRATIONAUX_H_
