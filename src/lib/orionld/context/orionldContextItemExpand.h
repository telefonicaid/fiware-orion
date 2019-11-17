#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTITEMEXPAND_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTITEMEXPAND_H_

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
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/OrionldContextItem.h"                  // OrionldContextItem



// -----------------------------------------------------------------------------
//
// orionldContextItemExpand -
//
// PARAMETERS
//   contextP                the context
//   shortName               the name to expand
//   valueMayBeExpandedP     pointer to a bool that is set to true if @type == @vocab
//   contextItemPP           to give the caller the complete result of the lookup
//
// RETURN VALUE
//   orionldContextItemExpand returns a pointer to the expanded value of 'shortName'
//
// NOTE
//   If no expansion is found, and the default URL has been used, then room is allocated using
//   kaAlloc, allocating on orionldState.kalloc, the connection buffer that lives only during
//   the current request. It is liberated "automatically" when the thread exits.
//
//   If the expansion IS found, then a pointer to the longname (that is part of the context where it was found)
//   is returned and we save some time by not copying anything.
//
extern char* orionldContextItemExpand
(
  OrionldContext*       contextP,
  const char*           shortName,
  bool*                 valueMayBeExpandedP,
  bool                  useDefaultUrlIfNotFound,
  OrionldContextItem**  contextItemPP
);

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTITEMEXPAND_H_
