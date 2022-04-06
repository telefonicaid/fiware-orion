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
extern "C"
{
#include "kbase/kMacros.h"                                     // K_VEC_SIZE
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/types/OrionldResponseErrorType.h"            // Own interface



// -----------------------------------------------------------------------------
//
// errorTypeStringV -
//
static const char* errorTypeStringV[] =
{
  "https://uri.etsi.org/ngsi-ld/errors/OK",
  "https://uri.etsi.org/ngsi-ld/errors/InvalidRequest",
  "https://uri.etsi.org/ngsi-ld/errors/BadRequestData",
  "https://uri.etsi.org/ngsi-ld/errors/AlreadyExists",
  "https://uri.etsi.org/ngsi-ld/errors/OperationNotSupported",
  "https://uri.etsi.org/ngsi-ld/errors/ResourceNotFound",
  "https://uri.etsi.org/ngsi-ld/errors/InternalError",
  "https://uri.etsi.org/ngsi-ld/errors/TooComplexQuery",
  "https://uri.etsi.org/ngsi-ld/errors/TooManyResults",
  "https://uri.etsi.org/ngsi-ld/errors/LdContextNotAvailable",
  "https://uri.etsi.org/ngsi-ld/errors/NoMultiTenantSupport",
  "https://uri.etsi.org/ngsi-ld/errors/NonExistingTenant"
};



// ----------------------------------------------------------------------------
//
// orionldResponseErrorType -
//
const char* orionldResponseErrorType(OrionldResponseErrorType type)
{
  if (type < K_VEC_SIZE(errorTypeStringV))
    return errorTypeStringV[type];
  return "UnknownError";
}
