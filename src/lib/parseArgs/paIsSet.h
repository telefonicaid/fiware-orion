#ifndef SRC_LIB_PARSEARGS_PAISSET_H_
#define SRC_LIB_PARSEARGS_PAISSET_H_

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


/* ****************************************************************************
*
* PaArgument - defined in parseArgs.h and needed in paIsSet
*/
struct PaArgument;



/* ****************************************************************************
*
* paIsSet - is an argument existing in the parse list
*/
extern bool paIsSet(int argC, char* argV[], PaArgument* paArgs, const char* option);



/* ****************************************************************************
*
* paIsSetSoGet - return value of option 'option'
*/
extern const char* paIsSetSoGet(int argC, char* argV[], const char* option);

#endif  // SRC_LIB_PARSEARGS_PAISSET_H_
