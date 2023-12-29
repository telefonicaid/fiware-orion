#ifndef SRC_LIB_ORIONLD_Q_QBUILD_H_
#define SRC_LIB_ORIONLD_Q_QBUILD_H_

/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
#include "orionld/types/QNode.h"                               // QNode



// -----------------------------------------------------------------------------
//
// qBuild - build QNode tree
//
// DESCRIPTION
//   'q' filters are used for querying entities (GET /entities, POST /entityOperations/query) and
//   for subscriptions/notifications.
//
//   For entity queries, the filter is parsed, used and thrown away - kalloc is used.
//
//   However, for subscriptions:
//   - the filter needs to be expanded and saved in the database
//   - the filter needs to ba allocated and stored in the subscription cache (for GET /subscriptions ops)
//   - the resulting QNode tree needs to be allocated and stored in the subscription cache
//
//   At startup, when the subscription cache is populated from the database content, the same applies (for subscriptions).
//   As well as for regular sub-cache refresh operations.
//
//   We assume for now that "GET /subscriptions" operations always use the sub-cache.
//
// PARAMETERS
//   q          the string as it figures in uri param or payload body
//   qRenderP   output parameter for the prepared filter (expansion, dotForEq, '.value')
//   v2ValidP   output parameter indicating whether the q string id valid for NGSIv2
//   isMdP      output parameter indicating whether the q para meter corresponds to 'mq' in NGSIv2
//
extern QNode* qBuild(const char* q, char** qRenderP, bool* v2ValidP, bool* isMdP, bool qToDbModel, bool pernot);

#endif  // SRC_LIB_ORIONLD_Q_QBUILD_H_
