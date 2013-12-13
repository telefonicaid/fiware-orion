#ifndef TAG_H
#define TAG_H

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

#include "common/Format.h"



/* ****************************************************************************
*
* startTag -  
*/
extern std::string startTag(std::string indent, std::string tagName, Format format, bool showTag = true);
extern std::string startTag(std::string indent, std::string xmlTag, std::string jsonTag, Format format, bool isVector = false, bool showTag = true);



/* ****************************************************************************
*
* endTag -  
*/
extern std::string endTag(std::string indent, std::string tagName, Format format, bool comma = false, bool isVector = false);



/* ****************************************************************************
*
* valueTag -  
*/
extern std::string valueTag(std::string indent, std::string tagName, std::string value, Format format, bool showComma = false, bool isAssociation = false);
extern std::string valueTag(std::string indent, std::string tagName, int value,         Format format, bool showComma = false, bool isAssociation = false);
extern std::string valueTag(std::string indent, std::string xmlTag, std::string jsonTag, std::string value, Format format, bool showComma = false, bool isAssociation = false);



/* ****************************************************************************
*
* startArray -
*/
std::string startArray(std::string indent, std::string tagName, Format format, bool showTag = true);



/* ****************************************************************************
*
* endArray -
*/
std::string endArray(std::string indent, std::string tagName, Format format);

#endif
