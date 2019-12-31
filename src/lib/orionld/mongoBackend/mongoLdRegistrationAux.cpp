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
#include <string>                                                    // std::string
#include <vector>                                                    // std::vector

#include "mongo/client/dbclient.h"                                   // mongo::BSONObj, mongo::BSONElement

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "apiTypesV2/Registration.h"                                 // ngsiv2::Registration
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeFromBsonObj.h"  // mongoCppLegacyKjTreeFromBsonObj
#include "mongoBackend/dbConstants.h"                                // REG_ATTRS, ...
#include "mongoBackend/safeMongo.h"                                  // getFieldF
#include "orionld/mongoBackend/mongoLdRegistrationAux.h"             // Own interface



/* ****************************************************************************
*
* mongoSetLdPropertyV -
*/
void mongoSetLdPropertyV(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  std::vector<mongo::BSONElement> dbPropertyV = getFieldF(r, REG_ATTRS).Array();

  for (unsigned int ix = 0; ix < dbPropertyV.size(); ++ix)
  {
    mongo::BSONObj  pobj = dbPropertyV[ix].embeddedObject();
    std::string     type = getStringFieldF(pobj, REG_ATTRS_TYPE);
    std::string     propName;

    if (type == REG_PROPERTIES_TYPE)
    {
      propName = getStringFieldF(pobj, REG_PROPERTIES_NAME);

      if (propName != "")
      {
        reg->dataProvided.propertyV.push_back(propName);
      }
    }
  }
}



/* ****************************************************************************
*
* mongoSetLdRelationshipV -
*/
void mongoSetLdRelationshipV(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  std::vector<mongo::BSONElement> dbRelationshipV = getFieldF(r, REG_ATTRS).Array();

  for (unsigned int ix = 0; ix < dbRelationshipV.size(); ++ix)
  {
    mongo::BSONObj  robj = dbRelationshipV[ix].embeddedObject();
    std::string     type = getStringFieldF(robj, REG_ATTRS_TYPE);
    std::string     relName;

    if (type == REG_RELATIONSHIPS_TYPE)
    {
      relName = getStringFieldF(robj, REG_RELATIONSHIPS_NAME);

      if (relName != "")
      {
        reg->dataProvided.relationshipV.push_back(relName);
      }
    }
  }
}



/* ****************************************************************************
*
* mongoSetLdRegistrationId -
*/
void mongoSetLdRegistrationId(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  reg->id = getStringFieldF(r, "_id");
}



/* ****************************************************************************
*
* mongoSetLdName -
*/
void mongoSetLdName(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  reg->name = r.hasField(REG_NAME) ? getStringFieldF(r, REG_NAME) : "";
}



/* ****************************************************************************
*
* mongoSetLdObservationInterval
*/
void mongoSetLdObservationInterval(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  if (r.hasField(REG_OBSERVATION_INTERVAL))
  {
    mongo::BSONObj obj              = getObjectFieldF(r, REG_OBSERVATION_INTERVAL);

    reg->observationInterval.start  = getLongFieldF(obj, REG_INTERVAL_START);
    reg->observationInterval.end    = obj.hasField(REG_INTERVAL_END) ? getLongFieldF(obj, REG_INTERVAL_END) : -1;
  }
  else
  {
    reg->observationInterval.start = -1;
    reg->observationInterval.end   = -1;
  }
}



/* ****************************************************************************
*
* mongoSetLdManagementInterval
*/
void mongoSetLdManagementInterval(ngsiv2::Registration* reg, const mongo::BSONObj& r)
{
  if (r.hasField(REG_MANAGEMENT_INTERVAL))
  {
    mongo::BSONObj obj             = getObjectFieldF(r, REG_MANAGEMENT_INTERVAL);

    reg->managementInterval.start  = getLongFieldF(obj, REG_INTERVAL_START);
    reg->managementInterval.end    = obj.hasField(REG_INTERVAL_END) ? getLongFieldF(obj, REG_INTERVAL_END) : -1;
  }
  else
  {
    reg->managementInterval.start = -1;
    reg->managementInterval.end   = -1;
  }
}



// -----------------------------------------------------------------------------
//
// mongoSetLdTimestamp -
//
void mongoSetLdTimestamp(long long* timestampP, const char* name, const mongo::BSONObj& bobj)
{
  if (bobj.hasField(name))
    *timestampP = getIntFieldF(bobj, name);
  else
    *timestampP = -1;
}



// -----------------------------------------------------------------------------
//
// mongoSetLdTimeInterval -
//
bool mongoSetLdTimeInterval(OrionldGeoLocation* geoLocationP, const char* name, const mongo::BSONObj& bobj, char** titleP, char** detailP)
{
  if (bobj.hasField(name))
  {
    mongo::BSONObj   locBobj   = getObjectFieldF(bobj, name);
    KjNode*          tree      = mongoCppLegacyKjTreeFromBsonObj(&locBobj, titleP, detailP);

    for (KjNode* nodeP = tree->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      if (strcmp(nodeP->name, "type") == 0)
        geoLocationP->geoType = nodeP->value.s;
      else if (strcmp(nodeP->name, "coordinates") == 0)
        geoLocationP->coordsNodeP = nodeP;
    }

    if (geoLocationP->coordsNodeP == NULL)
    {
      LM_E(("Internal Error (%s: %s)", *titleP, *detailP));
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// mongoSetLdProperties -
//
bool mongoSetLdProperties(ngsiv2::Registration* regP, const char* name, const mongo::BSONObj& bobj, char** titleP, char** detailP)
{
  mongo::BSONObj  propertiesObj   = getObjectFieldF(bobj, name);

  regP->properties = mongoCppLegacyKjTreeFromBsonObj(&propertiesObj, titleP, detailP);

  if (regP->properties == NULL)
  {
    LM_E(("Internal Error (%s: %s)", *titleP, *detailP));
    return false;
  }

  return true;
}
