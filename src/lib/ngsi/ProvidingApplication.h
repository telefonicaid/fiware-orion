#ifndef SRC_LIB_NGSI_PROVIDINGAPPLICATION_H_
#define SRC_LIB_NGSI_PROVIDINGAPPLICATION_H_

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

#include "common/MimeType.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* ProviderFormat -
*/
typedef enum ProviderFormat
{
  PfNone,
  PfJson,
  PfV2
} ProviderFormat;



/* ****************************************************************************
*
* ProvidingApplication -
*/
typedef struct ProvidingApplication
{
  std::string     string;
  MimeType        mimeType;        // Not part of NGSI itself, used by the CB to specify the preferred Mime-Type for CPr interaction
                                   // FIXME: Remove mimeType when NGSIv1 forwarding is removed.
  ProviderFormat  providerFormat;  // PfJson ("JSON" in mongo): NGSIv1, PfV2: NGSIv2

  ProvidingApplication();
  void          set(const std::string& value);
  void          setMimeType(const MimeType _mimeType);
  void          setProviderFormat(const ProviderFormat _providerFormat);
  std::string   get(void);
  MimeType      getMimeType(void);
  bool          isEmpty(void);
  std::string   toJsonV1(bool comma);
  const char*   c_str(void);
  void          release(void);

  std::string   check(void);
} ProvidingApplication;

#endif  // SRC_LIB_NGSI_PROVIDINGAPPLICATION_H_
