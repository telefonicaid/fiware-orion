#ifndef SRC_LIB_ORIONLD_COMMON_SCOMPARE_H_
#define SRC_LIB_ORIONLD_COMMON_SCOMPARE_H_

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



#define SCOMPARE2(s, c0, c1)                    \
  ((s[0] == c0) && (s[1] == c1))
#define SCOMPARE3(s, c0, c1, c2)                                         \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2))
#define SCOMPARE4(s, c0, c1, c2, c3)                                     \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3))
#define SCOMPARE5(s, c0, c1, c2, c3, c4)                                 \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4))
#define SCOMPARE6(s, c0, c1, c2, c3, c4, c5)                             \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5))
#define SCOMPARE7(s, c0, c1, c2, c3, c4, c5, c6)                         \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6))
#define SCOMPARE8(s, c0, c1, c2, c3, c4, c5, c6, c7)                     \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6) && (s[7] == c7))
#define SCOMPARE9(s, c0, c1, c2, c3, c4, c5, c6, c7, c8)                 \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6) && (s[7] == c7) && (s[8] == c8))
#define SCOMPARE10(s, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9)            \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6) && (s[7] == c7) && (s[8] == c8) && (s[9] == c9))
#define SCOMPARE11(s, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10)       \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6) && (s[7] == c7) && (s[8] == c8) && (s[9] == c9) && (s[10] == c10))
#define SCOMPARE12(s, c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11)  \
  ((s[0] == c0) && (s[1] == c1) && (s[2] == c2) && (s[3] == c3) && (s[4] == c4) && (s[5] == c5) && (s[6] == c6) && (s[7] == c7) && (s[8] == c8) && (s[9] == c9) && (s[10] == c10) && (s[11] == c11))

#endif  // SRC_LIB_ORIONLD_COMMON_SCOMPARE_H_
