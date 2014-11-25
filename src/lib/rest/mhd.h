#ifndef MHD_H
#define MHD_H

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <stdint.h>
#include <stdarg.h>      // microhttpd.h needs va_alist
#include <sys/types.h>   // microhttpd.h needs fd_set
#include <microhttpd.h>



/* ****************************************************************************
*
* POSTBUFFERSIZE - 
*/
#define POSTBUFFERSIZE (16 * 1024)



/* ****************************************************************************
*
* Shortening names for MHD types ...
*/
typedef struct MHD_PostProcessor           MHD_PostProcessor;
typedef struct MHD_Connection              MHD_Connection;
typedef struct MHD_Response                MHD_Response;
typedef struct MHD_Daemon                  MHD_Daemon;
typedef enum   MHD_RequestTerminationCode  MHD_RequestTerminationCode;
typedef enum   MHD_ValueKind               MHD_ValueKind;

#endif
