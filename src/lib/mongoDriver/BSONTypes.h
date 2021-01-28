#ifndef SRC_LIB_MONGODRIVER_BSONTYPES_H_
#define SRC_LIB_MONGODRIVER_BSONTYPES_H_

/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/


namespace orion
{

enum BSONType {
    MinKey = -1,
    EOO = 0,
    NumberDouble = 1,
    String = 2,
    Object = 3,
    Array = 4,
    BinData = 5,
    Undefined = 6,
    jstOID = 7,
    Bool = 8,
    Date = 9,
    jstNULL = 10,
    RegEx = 11,
    DBRef = 12,
    Code = 13,
    Symbol = 14,
    CodeWScope = 15,
    NumberInt = 16,
    Timestamp = 17,
    NumberLong = 18,
    JSTypeMax = 19,
    BigDecimal = 20,
    MaxKey = 127
};
}


#endif // SRC_LIB_MONGODRIVER_BSONTYPES_H_
