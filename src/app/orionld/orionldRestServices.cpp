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
#include "orionld/serviceRoutines/orionldPostEntities.h"
#include "orionld/serviceRoutines/orionldPostEntity.h"
#include "orionld/serviceRoutines/orionldPostSubscriptions.h"
#include "orionld/serviceRoutines/orionldPostRegistrations.h"
#include "orionld/serviceRoutines/orionldGetEntity.h"
#include "orionld/serviceRoutines/orionldGetEntities.h"
#include "orionld/serviceRoutines/orionldGetSubscriptions.h"
#include "orionld/serviceRoutines/orionldGetSubscription.h"
#include "orionld/serviceRoutines/orionldGetRegistrations.h"
#include "orionld/serviceRoutines/orionldGetRegistration.h"
#include "orionld/serviceRoutines/orionldPatchEntity.h"
#include "orionld/serviceRoutines/orionldPatchSubscription.h"
#include "orionld/serviceRoutines/orionldPatchRegistration.h"
#include "orionld/serviceRoutines/orionldDeleteEntity.h"
#include "orionld/serviceRoutines/orionldDeleteAttribute.h"
#include "orionld/serviceRoutines/orionldDeleteSubscription.h"
#include "orionld/serviceRoutines/orionldDeleteRegistration.h"

#include "orionld/rest/OrionLdRestService.h"       // OrionLdRestServiceSimplified
#include "orionld/orionldRestServices.h"           // Own Interface



// -----------------------------------------------------------------------------
//
// getServices -
//
static OrionLdRestServiceSimplified getServices[] =
{
  { "/ngsi-ld/v1/entities/*",          orionldGetEntity          },
  { "/ngsi-ld/v1/entities",            orionldGetEntities        },
  { "/ngsi-ld/v1/subscriptions",       orionldGetSubscriptions   },
  { "/ngsi-ld/v1/subscriptions/*",     orionldGetSubscription    },
  { "/ngsi-ld/v1/registrations",       orionldGetRegistrations   },
  { "/ngsi-ld/v1/registrations/*",     orionldGetRegistration    }
};



// ----------------------------------------------------------------------------
//
// postServices -
//
static OrionLdRestServiceSimplified postServices[] =
{
  { "/ngsi-ld/v1/entities",            orionldPostEntities       },
  { "/ngsi-ld/v1/entities/*/attrs",    orionldPostEntity         },
  { "/ngsi-ld/v1/subscriptions",       orionldPostSubscriptions  },
  { "/ngsi-ld/v1/registrations",       orionldPostRegistrations  }
};



// ----------------------------------------------------------------------------
//
// patchServices -
//
static OrionLdRestServiceSimplified patchServices[] =
{
  { "/ngsi-ld/v1/entities/*/attrs",    orionldPatchEntity        },
  { "/ngsi-ld/v1/subscriptions/*",     orionldPatchSubscription  },
  { "/ngsi-ld/v1/registrations/*",     orionldPatchRegistration  }
};



// ----------------------------------------------------------------------------
//
// deleteServices -
//
static OrionLdRestServiceSimplified deleteServices[] =
{
  { "/ngsi-ld/v1/entities/*/attrs/*",  orionldDeleteAttribute    },  // Very important that orionldDeleteAttribute comes before orionldDeleteEntity
  { "/ngsi-ld/v1/entities/*",          orionldDeleteEntity       },
  { "/ngsi-ld/v1/subscriptions/*",     orionldDeleteSubscription },
  { "/ngsi-ld/v1/registrations/*",     orionldDeleteRegistration }
};



// -----------------------------------------------------------------------------
//
// restServiceVV -
//
OrionLdRestServiceSimplifiedVector restServiceVV[] =
{
  { getServices,    6 },
  { NULL,           0 },
  { postServices,   4 },
  { deleteServices, 4 },
  { patchServices,  3 },
  { NULL,           0 },
  { NULL,           0 },
  { NULL,           0 },
  { NULL,           0 }
};
