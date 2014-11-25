#ifndef SRC_LIB_PARSEARGS_PAPRIVATE_H_
#define SRC_LIB_PARSEARGS_PAPRIVATE_H_

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
#include <string>

#include "parseArgs/baStd.h"         /* BaBoolean                            */
#include "parseArgs/parseArgs.h"     /* PaArgument                           */



/* ****************************************************************************
*
* PA_IS_XXX
*/
#define PA_IS_OPTION(argP)      ((argP->what & PawOption)    == PawOption)
#define PA_IS_PARAMETER(argP)   ((argP->what & PawParameter) == PawParameter)
#define PA_IS_VARIABLE(argP)    ((argP->what & PawVariable)  == PawVariable)



/* ****************************************************************************
*
* PaWhat - 
*/
typedef enum PaWhat
{
  PawOption    = (1 << 0),
  PawParameter = (1 << 1),
  PawVariable  = (1 << 2),
  PawBuiltin   = (1 << 3)
} PaWhat;



/* ****************************************************************************
*
* PaTypeUnion - 
*/
typedef union PaTypeUnion
{
  char                c;
  uint8_t             uc;
  int16_t             s;
  uint16_t            us;
  int32_t             i;
  uint32_t            ui;
  int64_t             l;
  uint64_t            ul;
  char*               string;
  unsigned char*      ustring;
  bool                boolean;
} PaTypeUnion;



/* ****************************************************************************
*
* paBuiltin - 
*/
extern PaiArgument*  paiList;



/* ****************************************************************************
*
* paBuiltin - 
*/
extern PaiArgument paBuiltin[];



/* ****************************************************************************
*
* stoi - convert string to integer
*
* PREFIXES
*   B:    Boolean
*   0:    Octal
*   H:    Hexadecimal
*   H':   Hexadecimal
*   0x:   Hexadecimal
*/
extern int stoi(char* string);

#endif  // SRC_LIB_PARSEARGS_PAPRIVATE_H_
