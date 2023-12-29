#ifndef SRC_LIB_ORIONLD_Q_QVARIABLEFIX_H_
#define SRC_LIB_ORIONLD_Q_QVARIABLEFIX_H_

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



// ----------------------------------------------------------------------------
//
// qVariableFix -
//
// - If simple attribute name - all OK
// - If attr.b.c, then 'attr' must be extracted, expanded and then '.md.b.c' appended
// - If attr[b.c], then 'attr' must be extracted, expanded and then '.b.c' appended
//
// After implementing expansion in metadata names, for attr.b.c, 'b' needs expansion also
//
extern char* qVariableFix(char* varPathIn, bool forDb, bool* isMdP, char** detailsP);

#endif  // SRC_LIB_ORIONLD_Q_QVARIABLEFIX_H_
