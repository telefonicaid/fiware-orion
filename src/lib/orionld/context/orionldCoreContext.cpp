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
* Author: Ken Zangelin
*/
#include "orionld/types/OrionldContext.h"                    // OrionldContext
#include "orionld/context/orionldCoreContext.h"              // Own interface



// -----------------------------------------------------------------------------
//
// orionldCoreContextP -
//
OrionldContext* orionldCoreContextP = (OrionldContext*) 0;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrl -
//
char* orionldDefaultUrl;



// -----------------------------------------------------------------------------
//
// orionldDefaultUrlLen -
//
int orionldDefaultUrlLen;



// -----------------------------------------------------------------------------
//
// builtinCoreContext
//
const char* builtinCoreContextUrl = ORIONLD_CORE_CONTEXT_URL_V1_0;
const char* builtinCoreContext    = "{ \"@context\": {"
  "\"ngsi-ld\": \"https://uri.etsi.org/ngsi-ld/\","
  "\"id\": \"@id\","
  "\"type\": \"@type\","
  "\"value\": \"https://uri.etsi.org/ngsi-ld/hasValue\","
  "\"object\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/hasObject\","
    "\"@type\":\"@id\""
  "},"
  "\"Property\": \"https://uri.etsi.org/ngsi-ld/Property\","
  "\"Relationship\": \"https://uri.etsi.org/ngsi-ld/Relationship\","
  "\"DateTime\": \"https://uri.etsi.org/ngsi-ld/DateTime\","
  "\"Date\": \"https://uri.etsi.org/ngsi-ld/Date\","
  "\"Time\": \"https://uri.etsi.org/ngsi-ld/Time\","
  "\"createdAt\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/createdAt\","
    "\"@type\": \"DateTime\""
  "},"
  "\"modifiedAt\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/modifiedAt\","
    "\"@type\": \"DateTime\""
  "},"
  "\"observedAt\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/observedAt\","
    "\"@type\": \"DateTime\""
  "},"
  "\"datasetId\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/datasetId\","
    "\"@type\": \"@id\""
  "},"
  "\"instanceId\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/instanceId\","
    "\"@type\": \"@id\""
  "},"
  "\"unitCode\": \"https://uri.etsi.org/ngsi-ld/unitCode\","
  "\"location\": \"https://uri.etsi.org/ngsi-ld/location\","
  "\"observationSpace\": \"https://uri.etsi.org/ngsi-ld/observationSpace\","
  "\"operationSpace\": \"https://uri.etsi.org/ngsi-ld/operationSpace\","
  "\"GeoProperty\": \"https://uri.etsi.org/ngsi-ld/GeoProperty\","
  "\"TemporalProperty\": \"https://uri.etsi.org/ngsi-ld/TemporalProperty\","
  "\"ContextSourceRegistration\": \"https://uri.etsi.org/ngsi-ld/ContextSourceRegistration\","
  "\"Subscription\": \"https://uri.etsi.org/ngsi-ld/Subscription\", "
  "\"Notification\": \"https://uri.etsi.org/ngsi-ld/Notification\","
  "\"ContextSourceNotification\": \"https://uri.etsi.org/ngsi-ld/ContextSourceNotification\","
  "\"title\": \"https://uri.etsi.org/ngsi-ld/title\","
  "\"detail\": \"https://uri.etsi.org/ngsi-ld/detail\","
  "\"idPattern\": \"https://uri.etsi.org/ngsi-ld/idPattern\","
  "\"name\": \"https://uri.etsi.org/ngsi-ld/name\","
  "\"description\": \"https://uri.etsi.org/ngsi-ld/description\","
  "\"information\": \"https://uri.etsi.org/ngsi-ld/information\","
  "\"observationInterval\": \"https://uri.etsi.org/ngsi-ld/observationInterval\","
  "\"managementInterval\": \"https://uri.etsi.org/ngsi-ld/managementInterval\","
  "\"expires\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/expires\","
    "\"@type\": \"DateTime\""
  "},"
  "\"endpoint\": \"https://uri.etsi.org/ngsi-ld/endpoint\","
  "\"entities\": \"https://uri.etsi.org/ngsi-ld/entities\","
  "\"properties\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/properties\","
    "\"@type\": \"@vocab\""
  "},"
  "\"relationships\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/relationships\","
    "\"@type\": \"@vocab\""
  "},"
  "\"start\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/start\","
    "\"@type\": \"DateTime\""
  "},"
  "\"end\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/end\","
    "\"@type\": \"DateTime\""
  "},"
  "\"watchedAttributes\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/watchedAttributes\","
    "\"@type\": \"@vocab\""
  "},"
  "\"timeInterval\": \"https://uri.etsi.org/ngsi-ld/timeInterval\","
  "\"q\": \"https://uri.etsi.org/ngsi-ld/q\","
  "\"geoQ\": \"https://uri.etsi.org/ngsi-ld/geoQ\","
  "\"csf\": \"https://uri.etsi.org/ngsi-ld/csf\","
  "\"isActive\": \"https://uri.etsi.org/ngsi-ld/isActive\","
  "\"notification\": \"https://uri.etsi.org/ngsi-ld/notification\","
  "\"status\": \"https://uri.etsi.org/ngsi-ld/status\","
  "\"throttling\": \"https://uri.etsi.org/ngsi-ld/throttling\","
  "\"temporalQ\": \"https://uri.etsi.org/ngsi-ld/temporalQ\","
  "\"geometry\": \"https://uri.etsi.org/ngsi-ld/geometry\","
  "\"coordinates\": \"https://uri.etsi.org/ngsi-ld/coordinates\","
  "\"georel\": \"https://uri.etsi.org/ngsi-ld/georel\","
  "\"geoproperty\": \"https://uri.etsi.org/ngsi-ld/geoproperty\","
  "\"attributes\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/attributes\","
    "\"@type\": \"@vocab\""
  "},"
  "\"format\": \"https://uri.etsi.org/ngsi-ld/format\","
  "\"timesSent\": \"https://uri.etsi.org/ngsi-ld/timesSent\","
  "\"lastNotification\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/lastNotification\","
    "\"@type\": \"DateTime\""
  "},"
  "\"lastFailure\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/lastFailure\","
    "\"@type\": \"DateTime\""
  "},"
  "\"lastSuccess\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/lastSuccess\","
    "\"@type\": \"DateTime\""
  "},"
  "\"uri\": \"https://uri.etsi.org/ngsi-ld/uri\","
  "\"accept\": \"https://uri.etsi.org/ngsi-ld/accept\","
  "\"success\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/success\","
    "\"@type\": \"@id\""
  "},"
  "\"errors\": \"https://uri.etsi.org/ngsi-ld/errors\","
  "\"error\": \"https://uri.etsi.org/ngsi-ld/error\","
  "\"entityId\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/entityId\","
    "\"@type\": \"@id\""
  "},"
  "\"updated\": \"https://uri.etsi.org/ngsi-ld/updated\","
  "\"unchanged\": \"https://uri.etsi.org/ngsi-ld/unchanged\","
  "\"attributeName\": \"https://uri.etsi.org/ngsi-ld/attributeName\","
  "\"reason\": \"https://uri.etsi.org/ngsi-ld/reason\","
  "\"timerel\": \"https://uri.etsi.org/ngsi-ld/timerel\","
  "\"time\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/time\","
    "\"@type\": \"DateTime\""
  "},"
  "\"endTime\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/endTime\","
    "\"@type\": \"DateTime\""
  "},"
  "\"timeproperty\": \"https://uri.etsi.org/ngsi-ld/timeproperty\","
  "\"subscriptionId\": {"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/subscriptionId\","
    "\"@type\": \"@id\""
  "},"
  "\"notifiedAt\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/notifiedAt\","
    "\"@type\": \"DateTime\""
  "},"
  "\"data\": \"https://uri.etsi.org/ngsi-ld/data\","
  "\"triggerReason\": \"https://uri.etsi.org/ngsi-ld/triggerReason\","
  "\"values\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/hasValues\","
    "\"@container\": \"@list\""
  "},"
  "\"objects\":{"
    "\"@id\": \"https://uri.etsi.org/ngsi-ld/hasObjects\","
    "\"@type\": \"@id\","
    "\"@container\": \"@list\""
  "},"
  "\"@vocab\": \"https://uri.etsi.org/ngsi-ld/default-context/\""
"}}";
