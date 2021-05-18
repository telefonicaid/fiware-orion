#ifndef SRC_LIB_MONGODRIVER_SAFEMONGO_H_
#define SRC_LIB_MONGODRIVER_SAFEMONGO_H_

/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <vector>

#include "ngsi/SubscriptionId.h"
#include "ngsi/RegistrationId.h"
#include "ngsi/StatusCode.h"

#include "mongoDriver/DBCursor.h"
#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONArray.h"
#include "mongoDriver/BSONElement.h"
#include "mongoDriver/OID.h"



/* ****************************************************************************
*
* Some macros to make the usage of these functions prettier
*
*/
#define getObjectFieldF(b, field)           orion::getObjectField(b, field, __FUNCTION__, __LINE__)
#define getArrayFieldF(b, field)            orion::getArrayField(b, field, __FUNCTION__, __LINE__)
#define getStringFieldF(b, field)           orion::getStringField(b, field, __FUNCTION__, __LINE__)
#define getNumberFieldF(b, field)           orion::getNumberField(b, field, __FUNCTION__, __LINE__)
#define getIntFieldF(b, field)              orion::getIntField(b, field, __FUNCTION__, __LINE__)
#define getLongFieldF(b, field)             orion::getLongField(b, field, __FUNCTION__, __LINE__)
#define getIntOrLongFieldAsLongF(b, field)  orion::getIntOrLongFieldAsLong(b, field, __FUNCTION__, __LINE__)
#define getBoolFieldF(b, field)             orion::getBoolField(b, field, __FUNCTION__, __LINE__)
#define getFieldF(b, field)                 orion::getField(b, field, __FUNCTION__,  __LINE__)
#define setStringVectorF(b, field, v)       orion::setStringVector(b, field, v, __FUNCTION__,  __LINE__)

namespace orion
{
/* ****************************************************************************
*
* getObjectField -
*/
extern BSONObj getObjectField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* getArrayField -
*/
extern BSONArray getArrayField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* getStringField -
*/
extern std::string getStringField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* getNumberField -
*/
extern double getNumberField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller,
  int                 line
);



/* ****************************************************************************
*
* getIntField -
*/
extern int getIntField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* getLongField -
*/
extern long long getLongField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line = 0
);



/* ****************************************************************************
*
* getIntOrLongFieldAsLong -
*/
extern long long getIntOrLongFieldAsLong
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* getBoolField -
*/
extern bool getBoolField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                line   = 0
);



/* ****************************************************************************
*
* getField -
*/
extern BSONElement getField
(
  const BSONObj&      b,
  const std::string&  field,
  const std::string&  caller = "<none>",
  int                 line   = 0
);



/* ****************************************************************************
*
* setStringVector -
*/
extern void setStringVector
(
  const BSONObj&             b,
  const std::string&         field,
  std::vector<std::string>*  v,
  const std::string&         caller,
  int                        line
);
}

#endif  // SRC_LIB_MONGODRIVER_SAFEMONGO_H_
