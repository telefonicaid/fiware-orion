#ifndef SRC_LIB_ORIONLD_COMMON_PQHEADER_H_
#define SRC_LIB_ORIONLD_COMMON_PQHEADER_H_

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



// -----------------------------------------------------------------------------
//
// RedHat UBI distro gets (for some reason I cannot understand) the postgres driver
// installed directly under /usr.
// Can't match that with other distros, not without passing the complete path to the include directory
// to the preprocessor (/usr/pgsql-12/include/, or /usr/include/postgres)
//
#ifdef REDHAT_UBI
#include "/usr/pgsql-12/include/libpq-fe.h"
#else
#include <postgresql/libpq-fe.h>
#endif

#endif  // SRC_LIB_ORIONLD_COMMON_PQHEADER_H_
