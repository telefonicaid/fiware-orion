#ifndef SRC_LIB_MONGOBACKEND_SAFEMONGO_H_
#define SRC_LIB_MONGOBACKEND_SAFEMONGO_H_

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
* Author: Fermin Galan Marquez
*/
#include <string>
#include <vector>

#include "ngsi/SubscriptionId.h"
#include "ngsi/RegistrationId.h"
#include "ngsi/StatusCode.h"

#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* Some macros to make the usage of these functions prettier
*/
#define getObjectFieldF(b, field)           getObjectField(b, field, __FUNCTION__, __LINE__)
#define getArrayFieldF(b, field)            getArrayField(b, field, __FUNCTION__, __LINE__)
#define getStringFieldF(b, field)           getStringField(b, field, __FUNCTION__, __LINE__)
#define getNumberFieldF(b, field)           getNumberField(b, field, __FUNCTION__, __LINE__)
#define getIntFieldF(b, field)              getIntField(b, field, __FUNCTION__, __LINE__)
#define getLongFieldF(b, field)             getLongField(b, field, __FUNCTION__, __LINE__)
#define getIntOrLongFieldAsLongF(b, field)  getIntOrLongFieldAsLong(b, field, __FUNCTION__, __LINE__)
#define getNumberFieldAsDoubleF(b, field)   getNumberFieldAsDouble(b, field, __FUNCTION__, __LINE__)
#define getBoolFieldF(b, field)             getBoolField(b, field, __FUNCTION__, __LINE__)
#define getFieldF(b, field)                 getField(b, field, __FUNCTION__,  __LINE__)
#define setStringVectorF(b, field, v)       setStringVector(b, field, v, __FUNCTION__,  __LINE__)
#define nextSafeOrErrorF(c, r, err)         nextSafeOrError(c, r, err, __FUNCTION__,  __LINE__)



/* ****************************************************************************
*
* getObjectField -
*/
extern mongo::BSONObj getObjectField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getArrayField -
*/
extern mongo::BSONArray getArrayField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getStringField -
*/
extern std::string getStringField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getNumberField -
*/
extern double getNumberField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller,
  int                    line
);



/* ****************************************************************************
*
* getIntField -
*/
extern int getIntField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getLongField -
*/
extern long long getLongField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line = 0
);



/* ****************************************************************************
*
* getIntOrLongFieldAsLong -
*/
extern long long getIntOrLongFieldAsLong
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getNumberFieldAsDouble -
*/
extern double getNumberFieldAsDouble
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getBoolField -
*/
extern bool getBoolField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const char*            caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* getField -
*/
extern mongo::BSONElement getField
(
  const mongo::BSONObj&  b,
  const std::string&     field,
  const std::string&     caller = "<none>",
  int                    line   = 0
);



/* ****************************************************************************
*
* setStringVector -
*/
extern void setStringVector
(
  const mongo::BSONObj&      b,
  const std::string&         field,
  std::vector<std::string>*  v,
  const std::string&         caller,
  int                        line
);



/* ****************************************************************************
*
* moreSafe -
*/
extern bool moreSafe(const std::auto_ptr<mongo::DBClientCursor>& cursor);



/* ****************************************************************************
*
* nextSafeOrError -
*/
extern bool nextSafeOrError
(
  const std::auto_ptr<mongo::DBClientCursor>&  cursor,
  mongo::BSONObj*                              r,
  std::string*                                 err,
  const char*                                  caller = "<none>",
  int                                          line   = 0
);



/* ****************************************************************************
*
* safeGetSubId -
*/
extern bool safeGetSubId(const SubscriptionId& subId, mongo::OID* id, StatusCode* sc);



/* ****************************************************************************
*
* safeGetRegId -
*/
extern bool safeGetRegId(const RegistrationId& regId, mongo::OID* id, StatusCode* sc);

#endif  // SRC_LIB_MONGOBACKEND_SAFEMONGO_H_
