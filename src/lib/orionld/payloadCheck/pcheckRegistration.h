#ifndef SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECKREGISTRATION_H_
#define SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECKREGISTRATION_H_

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
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}



// -----------------------------------------------------------------------------
//
// pcheckRegistration -
//
// This function makes sure that the incoming payload is OK
// - all types are as they should
// - the values with special needs are correct (if DataTikme for example)
// - no field is duplicated
// - etc
//
// However, there is ONE thing that cannot be checked here, as we still haven't queries the database
// for the original registration - the one to be patched.
// The problem is with registration properties.
// A registration can have any property added to it, and with any name.
// In this function we can check that the property isn't duplicated but, as this is a PATCH, the
// property to be patched MUST ALREADY EXIST in the original registration, and this cannot be checked until
// AFTER we extract the the original registration from the database.
//
// SO, what we do here is to separate the regiustration properties from the rest of the fields and keep thjat
// as a KjNode tree, until we are able to perform this check.
// The output parameter 'propertyTreeP' is used for this purpose.
//
extern bool pcheckRegistration(KjNode* registrationP, bool idCanBePresent, KjNode**  propertyTreeP);

#endif  // SRC_LIB_ORIONLD_PAYLOADCHECK_PCHECKREGISTRATION_H_
