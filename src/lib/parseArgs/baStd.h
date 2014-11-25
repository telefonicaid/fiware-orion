#ifndef SRC_LIB_PARSEARGS_BASTD_H_
#define SRC_LIB_PARSEARGS_BASTD_H_

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
#include <string>



/* ****************************************************************************
*
* BA_VEC_SIZE - size of vector
*/
#define BA_VEC_SIZE(v)   (sizeof(v) / sizeof(v[0]))



/* ****************************************************************************
*
* BA_FT - get boolean string
*/
#define BA_FT(b) \
  (((b) == true)? "TRUE" : (((b) == false)? "FALSE" : "undef"))



/* ****************************************************************************
*
* BA_ONOFF - return ON or OFF 
*/
#define BA_ONOFF(b) \
  (((b) == TRUE)? "ON" : (((b) == FALSE)? "OFF" : "undef"))



/* ****************************************************************************
*
* baStoi - string to integer
*/
extern int64_t baStoi(char* string, int* baseP = (int*) 0, char* errorText = (char*) 0, int errorTextLen = 0);



/* ****************************************************************************
*
* baStof - string float to binary float
*/
extern float baStof(char* string);



/* ****************************************************************************
*
* baStod - string double to binary double
*/
extern double baStod(char* string);



/* ****************************************************************************
*
* baWs - is the character 'c' a whitespace (space, tab or '\n')
*/
extern bool baWs(char c);



/* ****************************************************************************
*
* baWsNoOf - number of whitespaces in the string 'string'
*/
extern int baWsNoOf(char* string);


/* ****************************************************************************
*
* baWsStrip - 
*/
char* baWsStrip(char* string);



/* ****************************************************************************
*
* baWsOnly - 
*/
extern bool baWsOnly(char* str);



/* ****************************************************************************
*
* baCharCount - 
*/
extern int baCharCount(char* line, char c);

#endif  // SRC_LIB_PARSEARGS_BASTD_H_
