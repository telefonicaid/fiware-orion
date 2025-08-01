#ifndef SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_
#define SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_

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
#include <vector>

#include "common/RenderFormat.h"
#include "common/globals.h"
#include "common/MimeType.h"
#include "orionTypes/OrionValueType.h"
#include "ngsi/MetadataVector.h"
#include "ngsi/Request.h"
#include "parse/CompoundValueNode.h"
#include "rest/HttpStatusCode.h"
#include "mongoDriver/BSONObjBuilder.h"
#include "expressions/ExprContext.h"
#include "apiTypesV2/Registration.h"



/* ****************************************************************************
*
* ContextAttribute -
*/
typedef struct ContextAttribute
{
public:
  std::string     name;                    // Mandatory
  std::string     type;                    // Optional
  MetadataVector  metadataVector;          // Optional

  orion::ValueType           valueType;    // Type of value: taken from JSON parse
  std::string                stringValue;  // "value" as a String
  double                     numberValue;  // "value" as a Number
  bool                       boolValue;    // "value" as a Boolean  

  ngsiv2::Provider           provider;                // Used internally for CPr forwarding functionality
  std::string                providerRegId;          // Side vector to provider, to hold the reg ids where they come (used for login purposes)

  bool                       found;                   // Used internally for CPr forwarding functionality (update case)
                                                      // It means attribute found either locally or remotely in providing application

  bool                       skip;                    // For internal use in mongoBackend - in case of 'op=append' and the attribute already exists
  orion::CompoundValueNode*  compoundValueP;
  bool                       typeGiven;               // Was 'type' part of the incoming payload?

  double                     creDate;                 // used by dateCreated functionality in NGSIv2
  double                     modDate;                 // used by dateModified functionality in NGSIv2

  std::string                actionType;              // Used by special metadata in notifications functionality
  ContextAttribute*          previousValue;           // Used by special metadata in notifications functionality
                                                      // (Note we are forced to use a pointer for this, as we are using
                                                      // ContextAttribute field in the ContextAttribute type declaration)


  bool                      onlyValue;                // Used when only the value is meaningful in v2 updates of value, without regarding metadata
  bool                      shadowed;                 // shadowed true means that the attribute is rendered only if explicitly required
                                                      // in attrs filter (typically for builtin attributes)

  std::string               rawValue;                 // Pre-calculated value
  bool                      rawValueCalculated;       // true if rawValue has been calculated, false otherwise


  ~ContextAttribute();
  ContextAttribute();
  ContextAttribute(ContextAttribute* caP, bool useDefaultType = false, bool cloneCompound = false);
  ContextAttribute(const std::string& _name, const std::string& _type, const char* _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, const std::string& _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, double _value, bool _found = true);
  ContextAttribute(const std::string& _name, const std::string& _type, bool _value, bool _found = true);

  /* Check if attribute means a location  */
  bool  getLocation(orion::BSONObj* attrsP) const;

  double getEvalPriority(void);

  std::string  toJson(const std::vector<std::string>&  metadataFilter, bool renderNgsiField = false, ExprContextObject* exprContextObjectP = NULL);

  std::string  toJsonValue(void);

  void         setRaw(ExprContextObject* exprContextObjectP);

  std::string  getValueResponse(bool             acceptedTextPlain,
                                bool             acceptedJson,
                                MimeType         outFormatSelection,
                                MimeType*        outMimeTypeP,
                                HttpStatusCode*  scP);

  void         addToContext(ExprContextObject* exprContextObjectP, bool legacy);

  void         release(void);
  std::string  getName(void);

  /* Used to render attribute value to BSON */
  bool valueBson(const std::string&      valueKey,
                 orion::BSONObjBuilder*  bsonAttr,
                 const std::string&      attrType) const;

  /* Helper method to be use in some places wher '%s' is needed */
  std::string  getValue(void) const;

  std::string  check(bool asValue, bool relaxForbiddenCheck = false);
  bool              compoundItemExists(const std::string& compoundPath, orion::CompoundValueNode** compoundItemPP = NULL);

private:
  void filterAndOrderMetadata(const std::vector<std::string>&  metadataFilter,
                              std::vector<Metadata*>*          orderedMetadata);

  void bsonAppendAttrValue(const std::string&      valueKey,
                           orion::BSONObjBuilder*  bsonAttr,
                           const std::string&      attrType) const;

  bool hasIgnoreType(void) const;

  bool calculateOperator(const std::string&         valueKey,
                         orion::CompoundValueNode*  upOp,
                         orion::BSONObjBuilder*     bsonAttr) const;

} ContextAttribute;

#endif  // SRC_LIB_NGSI_CONTEXTATTRIBUTE_H_
