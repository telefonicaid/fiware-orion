/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "orionld/context/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldCoreContext.h"                // Own interface



// -----------------------------------------------------------------------------
//
// orionldCoreContext -
//
OrionldContext orionldCoreContext;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlContext -
//
OrionldContext orionldDefaultUrlContext;



// -----------------------------------------------------------------------------
//
// orionldDefaultContext
//
OrionldContext orionldDefaultContext;



// -----------------------------------------------------------------------------
//
// orionldCoreContextString - to avoid download during functest
//
const char* orionldCoreContextString = " {  \
  \"@context\": {  \
  \"ngsi-ld\": \"http://uri.etsi.org/ngsi-ld/\",      \
  \"id\": \"@id\",  \
  \"type\": \"@type\",  \
  \"value\": \"http://uri.etsi.org/ngsi-ld/hasValue\",  \
  \"object\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/hasObject\",  \
  \"@type\":\"@id\"  \
  },  \
  \"Property\": \"http://uri.etsi.org/ngsi-ld/Property\",  \
  \"Relationship\": \"http://uri.etsi.org/ngsi-ld/Relationship\",  \
  \"DateTime\": \"http://uri.etsi.org/ngsi-ld/DateTime\",  \
  \"Date\": \"http://uri.etsi.org/ngsi-ld/Date\",  \
  \"Time\": \"http://uri.etsi.org/ngsi-ld/Time\",  \
  \"createdAt\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/createdAt\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"modifiedAt\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/modifiedAt\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"observedAt\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/observedAt\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"datasetId\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/datasetId\",  \
  \"@type\": \"@id\"  \
  },  \
  \"instanceId\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/instanceId\",  \
  \"@type\": \"@id\"  \
  },  \
  \"unitCode\": \"http://uri.etsi.org/ngsi-ld/unitCode\",  \
  \"location\": \"http://uri.etsi.org/ngsi-ld/location\",  \
  \"observationSpace\": \"http://uri.etsi.org/ngsi-ld/observationSpace\",  \
  \"operationSpace\": \"http://uri.etsi.org/ngsi-ld/operationSpace\",  \
  \"GeoProperty\": \"http://uri.etsi.org/ngsi-ld/GeoProperty\",  \
  \"TemporalProperty\": \"http://uri.etsi.org/ngsi-ld/TemporalProperty\",  \
  \"ContextSourceRegistration\": \"http://uri.etsi.org/ngsi-ld/ContextSourceRegistration\",  \
  \"Subscription\": \"http://uri.etsi.org/ngsi-ld/Subscription\",   \
  \"Notification\": \"http://uri.etsi.org/ngsi-ld/Notification\",  \
  \"ContextSourceNotification\": \"http://uri.etsi.org/ngsi-ld/ContextSourceNotification\",  \
  \"title\": \"http://uri.etsi.org/ngsi-ld/title\",  \
  \"detail\": \"http://uri.etsi.org/ngsi-ld/detail\",  \
  \"idPattern\": \"http://uri.etsi.org/ngsi-ld/idPattern\",  \
  \"name\": \"http://uri.etsi.org/ngsi-ld/name\",  \
  \"description\": \"http://uri.etsi.org/ngsi-ld/description\",  \
  \"information\": \"http://uri.etsi.org/ngsi-ld/information\",  \
  \"observationInterval\": \"http://uri.etsi.org/ngsi-ld/observationInterval\",  \
  \"managementInterval\": \"http://uri.etsi.org/ngsi-ld/managementInterval\",  \
  \"expires\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/expires\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"endpoint\": \"http://uri.etsi.org/ngsi-ld/endpoint\",  \
  \"entities\": \"http://uri.etsi.org/ngsi-ld/entities\",  \
  \"properties\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/properties\",  \
  \"@type\": \"@vocab\"  \
  },  \
  \"relationships\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/relationships\",  \
  \"@type\": \"@vocab\"  \
  },  \
  \"start\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/start\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"end\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/end\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"watchedAttributes\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/watchedAttributes\",  \
  \"@type\": \"@vocab\"  \
  },  \
  \"timeInterval\": \"http://uri.etsi.org/ngsi-ld/timeInterval\",  \
  \"q\": \"http://uri.etsi.org/ngsi-ld/q\",  \
  \"geoQ\": \"http://uri.etsi.org/ngsi-ld/geoQ\",  \
  \"csf\": \"http://uri.etsi.org/ngsi-ld/csf\",  \
  \"isActive\": \"http://uri.etsi.org/ngsi-ld/isActive\",  \
  \"notification\": \"http://uri.etsi.org/ngsi-ld/notification\",  \
  \"status\": \"http://uri.etsi.org/ngsi-ld/status\",  \
  \"throttling\": \"http://uri.etsi.org/ngsi-ld/throttling\",  \
  \"temporalQ\": \"http://uri.etsi.org/ngsi-ld/temporalQ\",  \
  \"geometry\": \"http://uri.etsi.org/ngsi-ld/geometry\",  \
  \"coordinates\": \"http://uri.etsi.org/ngsi-ld/coordinates\",  \
  \"georel\": \"http://uri.etsi.org/ngsi-ld/georel\",  \
  \"geoproperty\": \"http://uri.etsi.org/ngsi-ld/geoproperty\",  \
  \"attributes\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/attributes\",  \
  \"@type\": \"@vocab\"  \
  },  \
  \"format\": \"http://uri.etsi.org/ngsi-ld/format\",  \
  \"timesSent\": \"http://uri.etsi.org/ngsi-ld/timesSent\",  \
  \"lastNotification\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/lastNotification\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"lastFailure\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/lastFailure \",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"lastSuccess\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/lastSuccess\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"uri\": \"http://uri.etsi.org/ngsi-ld/uri\",  \
  \"accept\": \"http://uri.etsi.org/ngsi-ld/accept\",  \
  \"success\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/success\",  \
  \"@type\": \"@id\"  \
  },  \
  \"errors\": \"http://uri.etsi.org/ngsi-ld/errors\",  \
  \"error\": \"http://uri.etsi.org/ngsi-ld/error\",  \
  \"entityId\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/entityId\",  \
  \"@type\": \"@id\"  \
  },  \
  \"updated\": \"http://uri.etsi.org/ngsi-ld/updated\",  \
  \"unchanged\": \"http://uri.etsi.org/ngsi-ld/unchanged\",  \
  \"attributeName\": \"http://uri.etsi.org/ngsi-ld/attributeName\",  \
  \"reason\": \"http://uri.etsi.org/ngsi-ld/reason\",  \
  \"timerel\": \"http://uri.etsi.org/ngsi-ld/timerel\",  \
  \"time\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/time\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"endTime\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/endTime\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"timeproperty\": \"http://uri.etsi.org/ngsi-ld/timeproperty\",  \
  \"subscriptionId\": {  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/subscriptionId\",  \
  \"@type\": \"@id\"  \
  },  \
  \"notifiedAt\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/notifiedAt\",  \
  \"@type\": \"DateTime\"  \
  },  \
  \"data\": \"http://uri.etsi.org/ngsi-ld/data\",  \
  \"triggerReason\": \"http://uri.etsi.org/ngsi-ld/triggerReason\",  \
  \"values\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/hasValues\",  \
  \"@container\": \"@list\"  \
  },  \
  \"objects\":{  \
  \"@id\": \"http://uri.etsi.org/ngsi-ld/hasObjects\",  \
  \"@type\": \"@id\",  \
  \"@container\": \"@list\"  \
  }  \
  }  \
}";



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlContextString - to avoid download during functest
//
const char* orionldDefaultUrlContextString = "{  \
  \"@context\": {  \
    \"@vocab\": \"http://example.org/ngsi-ld/default/\"  \
  }  \
}";




// -----------------------------------------------------------------------------
//
// orionldDefaultContextString - to avoid download during functest
//
const char* orionldDefaultContextString = " {  \
  \"@context\": [  \
    \"https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/defaultContext/defaultContextVocab.jsonld\",  \
    \"https://forge.etsi.org/gitlab/NGSI-LD/NGSI-LD/raw/master/coreContext/ngsi-ld-core-context.jsonld\"  \
  ]  \
}";



// -----------------------------------------------------------------------------
//
// orionldDefaultUrl -
//
char* orionldDefaultUrl    = (char*) "DEFAULT URL";
int   orionldDefaultUrlLen = -1;
