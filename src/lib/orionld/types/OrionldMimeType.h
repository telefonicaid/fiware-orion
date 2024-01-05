#ifndef SRC_LIB_ORIONLD_TYPES_ORIONLDMIMETYPE_H_
#define SRC_LIB_ORIONLD_TYPES_ORIONLDMIMETYPE_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
// Mimetype -
//
typedef enum MimeType
{
  MT_NONE           = 0,
  MT_NOTGIVEN       = 1,
  MT_JSON           = 2,
  MT_TEXT           = 3,
  MT_HTML           = 4,
  MT_JSONLD         = 5,
  MT_GEOJSON        = 6,
  MT_MERGEPATCHJSON = 7
} MimeType;

#define MT_DEFAULT         MT_JSON
#define MT_DEFAULT_STRING  "JSON"



extern const char* mimeType(MimeType mimeType);
extern MimeType    mimeTypeFromString(const char* mimeType, char** charsetP, bool wildcardsAccepted, bool textOk, uint32_t* acceptMaskP);

#endif  // SRC_LIB_ORIONLD_TYPES_ORIONLDMIMETYPE_H_
