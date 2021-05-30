/*
*
* Copyright 2021 FIWARE Foundation e.V.
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
#include "orionld/troe/PgConnectionPool.h"                     // PgConnectionPool
#include "orionld/troe/pgConnectionPools.h"                    // pgPoolMaster
#include "orionld/troe/pgConnectionPoolInsert.h"               // Own interface



// -----------------------------------------------------------------------------
//
// pgConnectionPoolInsert -
//
// The linked list of connection pools will always have the NULL database as its first pool
// So, insertions are done on pgPoolMaster->next.
// New pools are inserted in the beginning of the list, so we don't need to maintain a pointer
// to the last item in the linked list.
//
void pgConnectionPoolInsert(PgConnectionPool* poolP)
{
  poolP->next        = pgPoolMaster->next;
  pgPoolMaster->next = poolP;
}
