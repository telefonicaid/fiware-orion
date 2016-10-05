#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

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
#include <vector>

#include "common/MimeType.h"



// -----------------------------------------------------------------------------
//
// HttpAcceptHeader -
//
// The incoming data is a comma-separated list of media-range, accept-params, and accept-extension.
// The three components are separated by semicolon, and so are multiple accept-extensions.
// There is only one media-range and only one instance of accept-params (or zero)
//
// See https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
//
// Accept = "Accept" ":" <media-range> [;<accept-params>[;accept-extension]*]
//
//   media-range:      < */* | type/* | type/subtype >
//   accept-params:    [ q=<qvalue> ]
//   accept-extension: [ token=<token|quoted-string> ]*
//
// The qvalue is a parameter for indicating a relative quality factor. It goes from 1 to 0, 1 being max-priority.
// The default value of q is 1.
//
typedef struct HttpAcceptHeader
{
  std::string               mediaRange;
  double                    qvalue;
  std::vector<std::string>  acceptExtensions;
} HttpAcceptHeader;



/* ****************************************************************************
*
* HttpHeaders - 
*/
typedef struct HttpHeaders
{
  HttpHeaders();

  void      release(void);
  bool      accepted(const std::string& mime);
  MimeType  outformatSelect(void);

  std::vector<HttpAcceptHeader*> acceptHeaderV;

  bool          gotHeaders;
  std::string   userAgent;
  std::string   host;
  std::string   accept;
  std::string   expect;
  std::string   contentType;
  std::string   origin;
  std::string   tenant;
  std::string   servicePath;
  std::string   xauthToken;
  std::string   xrealIp;
  std::string   xforwardedFor;
  std::string   correlator;

  bool          servicePathReceived;

  unsigned int  contentLength;
  std::string   connection;
} HttpHeaders;

#endif
