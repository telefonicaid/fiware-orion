#ifndef REST_H
#define REST_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "rest/RestService.h"



/* ****************************************************************************
*
* restInit - 
*/
extern void restInit(char* bindIp, unsigned short port, RestService* restServiceV);

/* ****************************************************************************
*
* restInit_v6 -
*/
extern void restInit_v6(char* bindIpV6, unsigned short port, RestService* restServiceV);

/* ****************************************************************************
*
* restStart - 
*/
extern int restStart(void);

/* ****************************************************************************
*
* restStart_v6 -
*/
extern int restStart_v6(void);


#endif
