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
* Author: developer
*/

#include <stdio.h>              /* sscanf                                    */
#include <stdlib.h>             /* atoi                                      */
#include <string.h>             /* strtoul, strncmp                          */
#include <string>               /* std::string                               */

#include "parseArgs/baStd.h"    /* Own interface                             */



/* ****************************************************************************
*
* IntType -
*/
typedef enum IntType
{
  Bin,
  Oct,
  Dec,
  Hex
} IntType;



/* ****************************************************************************
*
* baStoi - string to integer
*/
int64_t baStoi(char* string, int* baseP, char* errorText, int errorTextLen)
{
  char     last;
  int64_t  multiplicator = 1;
  int      sign          = 1;
  int64_t  value;
  int      base;
  char*    validchars = (char*) "Q";

  if ((string == NULL) || (string[0] == 0))
  {
    return 0;
  }

  if (*string == '-')
  {
    ++string;
    sign = -1;
  }

  last = string[strlen(string) - 1];
  if (last == 'k')
  {
    multiplicator = 1024;
  }
  else if (last == 'M')
  {
    multiplicator = 1024 * 1024;
  }
  else if (last == 'G')
  {
    multiplicator = 1024 * 1024 * 1024;
  }

#ifdef __LP64__
  else if (last == 'T')
  {
    multiplicator = (int64_t) 1024 * 1024 * 1024 * 1024;
  }
  else if (last == 'P')
  {
    multiplicator = (int64_t) 1024 * 1024 * 1024 * 1024 * 1024;
  }
#endif

  if (multiplicator != 1)
  {
    string[strlen(string) - 1] = 0;
  }

  if (strncmp(string, "0x", 2) == 0)
  {
    base        = 16;
    string      = &string[2];
    validchars  = (char*) "0123456789abcdfeABCDEF";
  }
  else if (strncmp(string, "H'", 2) == 0)
  {
    base        = 16;
    string      = &string[2];
    validchars  = (char*) "0123456789abcdfeABCDEF";
  }
  else if (strncmp(string, "H", 1) == 0)
  {
    base        = 16;
    string      = &string[1];
    validchars  = (char*) "0123456789abcdfeABCDEF";
  }
  else if (strncmp(string, "0", 1) == 0)
  {
    base        = 8;
    string      = &string[1];
    validchars  = (char*) "01234567";
  }
  else if (strncmp(string, "B", 1) == 0)
  {
    base        = 2;
    string      = &string[1];
    validchars  = (char*) "01";
  }
  else
  {
    base        = 10;
    validchars  = (char*) "0123456789";
  }

  if (baseP)
  {
    *baseP = base;
  }

  if (strspn(string, validchars) != strlen(string))
  {
    if (errorText)
    {
      snprintf(errorText, errorTextLen, "bad string in integer conversion: '%s'", string);
    }

    return -1;
  }

  value = strtoull(string, NULL, base);

  return sign * multiplicator * value;
}



/* ****************************************************************************
*
* baStof - string float to binary float
*/
float baStof(char* string)
{
  float f;

  f = strtod(string, NULL);

  return f;
}



/* ****************************************************************************
*
* baStod - string double to binary double
*/
double baStod(char* string)
{
  double f;

  f = strtod(string, NULL);

  return f;
}



/* ****************************************************************************
*
* baWs - is the character 'c' a whitespace (space, tab or '\n')
*/
bool baWs(char c)
{
  switch (c)
  {
  case ' ':
  case '\t':
  case '\n':
    return true;
    break;

  default:
    return false;
    break;
  }
}



/* ****************************************************************************
*
* baWsNoOf - number of whitespaces in the string 'string'
*/
int baWsNoOf(char* string)
{
  int no = 0;

  while (*string != 0)
  {
    if (baWs(*string) == true)
      ++no;

    ++string;
  }

  return no;
}



/* ****************************************************************************
*
* baWsStrip -
*/
char* baWsStrip(char* s)
{
  char* str;
  char* tmP;
  char* toFree;

  if ((s == NULL) || (s[0] == 0))
  {
    return s;
  }

  str = strdup(s);
  if (str == NULL)
  {
    s[0] = 0;
    return s;
  }

  toFree = str;
  while ((*str == ' ') || (*str == '\t'))
  {
    ++str;
  }

  tmP = &str[strlen(str) - 1];

  while ((tmP > str) && ((*tmP == ' ') || (*tmP == '\t')))
  {
    --tmP;
  }
  ++tmP;
  *tmP = 0;

  if (str[0] != 0)
  {
    // strlen(str) is OK as 'str' is a copy of 's' and 'str' doesn't grow
    snprintf(s, strlen(str), "%s", str);
  }
  else
  {
    s[0] = 0;
  }

  free(toFree);

  return s;
}



/* ****************************************************************************
*
* baWsOnly -
*/
bool baWsOnly(char* str)
{
  while (*str != 0)
  {
    if (baWs(*str) == false)
    {
      return false;
    }

    ++str;
  }

  return true;
}



/* ****************************************************************************
*
* baCharCount -
*/
int baCharCount(char* line, char c)
{
  int noOf = 0;

  while (*line != 0)
  {
    if (*line == c)
    {
      ++noOf;
    }

    ++line;
  }

  return noOf;
}
