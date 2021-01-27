#ifndef TEST_UNITTESTS_TESTINIT_H_
#define TEST_UNITTESTS_TESTINIT_H_

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
* Author: Fermin Galan
*/
#include "ngsi10/NotifyContextRequest.h"
#include "mongoBackend/safeMongo.h"



/* Collection names used for testing */
#define DBPREFIX                    "utest"
#define REGISTRATIONS_COLL          DBPREFIX ".registrations"
#define ENTITIES_COLL               DBPREFIX ".entities"
#define SUBSCRIBECONTEXT_COLL       DBPREFIX ".csubs"



/* Some useful macros to avoid to long and verbose lines in asserts */
#define RES_CNTX_REG(i)         res.responseVector[i]->contextRegistration
#define RES_CNTX_REG_ATTR(i, j) res.responseVector[i]->contextRegistration.contextRegistrationAttributeVector[j]
#define RES_CER(i)              res.contextElementResponseVector[i]->entity
#define RES_CER_STATUS(i)       res.contextElementResponseVector[i]->statusCode
#define RES_CER_ATTR(i, j)      res.contextElementResponseVector[i]->entity.attributeVector[j]

#define C_STR_FIELD(b, f)       getStringField(b, f).c_str()



/* ****************************************************************************
*
* setupDatabase -
*
* This function (which is called before every test) cleans the database and set
* collection names
*/
extern void setupDatabase(void);



/* ****************************************************************************
*
* matchNotifyContextRequest -
*/
extern bool matchNotifyContextRequest(NotifyContextRequest* expected, NotifyContextRequest* arg);


#endif  // TEST_UNITTESTS_TESTINIT_H_
