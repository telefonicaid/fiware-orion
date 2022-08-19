/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: TID Developer
*/

#include "proxyCoap/HttpProxy.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <sstream>
#include <string.h>

#include <curl/curl.h>


/* ****************************************************************************
*
* writeMemoryCallback -
*/
size_t writeMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  size_t realsize = size * nmemb;
  MemoryStruct* mem = (MemoryStruct *) userp;

  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
  {
    LM_W(("Not enough memory (realloc returned NULL)\n"));
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}


/* ****************************************************************************
*
* sendHttpRequest -
*/
std::string sendHttpRequest(const char* host, unsigned short port, CoapPDU* request)
{
  int           recvURILen               = 0;
  CURL*         curl                     = curl_easy_init();
  MemoryStruct  httpResponse;
  CURLcode      res;
  char          uriBuffer[COAP_URI_BUFFER_SIZE];

  if (curl)
  {
    // Allocate to hold HTTP response
    httpResponse.memory = (char*) malloc(1); // will grow as needed
    httpResponse.size = 0; // no data at this point

    // --- Set HTTP verb
    std::string httpVerb = "";

    switch(request->getCode())
    {
      case CoapPDU::COAP_POST:
        httpVerb = "POST";
        break;

      case CoapPDU::COAP_PUT:
        httpVerb = "PUT";
        break;

      case CoapPDU::COAP_DELETE:
        httpVerb = "DELETE";
        break;

      case CoapPDU::COAP_EMPTY:
      case CoapPDU::COAP_GET:
      default:
        httpVerb = "GET";
        break;
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, httpVerb.c_str());
    LM_T(LmtCoap, ("Got an HTTP %s", httpVerb.c_str()));


    // --- Prepare headers
    struct curl_slist*   headers    = NULL;
    CoapPDU::CoapOption* options    = request->getOptions();
    int                  numOptions = request->getNumOptions();

    for (int i = 0; i < numOptions ; i++)
    {
      u_int16_t     opt     = options[i].optionNumber;
      std::string   string  = "";
      u_int8_t      buffer[options[i].optionValueLength + 1];

      memcpy(buffer, options[i].optionValuePointer, options[i].optionValueLength);

      switch (opt)
      {
      case CoapPDU::COAP_OPTION_URI_PATH:
        buffer[options[i].optionValueLength] = '\0';
        LM_T(LmtCoap, ("Got URI_PATH option: '%s'", buffer));
        break;

      case CoapPDU::COAP_OPTION_CONTENT_FORMAT:
        switch (buffer[0])
        {
        case CoapPDU::COAP_CONTENT_FORMAT_APP_JSON:
          string = "Content-type: application/json";
          break;

        case CoapPDU::COAP_CONTENT_FORMAT_APP_XML:
          string = "Content-type: application/xml";
          break;

        default:
          string = "Content-type: application/json";
          break;
        }
        headers = curl_slist_append(headers, string.c_str());
        LM_T(LmtCoap, ("Got CONTENT-FORMAT option: '%s'", string.c_str()));
        break;

      case CoapPDU::COAP_OPTION_ACCEPT:
        switch (buffer[0])
        {
        case CoapPDU::COAP_CONTENT_FORMAT_APP_JSON:
          string = "Accept: application/json";
          break;

        case CoapPDU::COAP_CONTENT_FORMAT_APP_XML:
          string = "Accept: application/xml";
          break;

        default:
          string = "Accept: application/json";
          break;
        }
        headers = curl_slist_append(headers, string.c_str());
        LM_T(LmtCoap, ("Got ACCEPT option: '%s'", string.c_str()));
        break;

      default:
        LM_T(LmtCoap, ("Got unknown option"));
        break;
      }
    }


    // Set Content-length
    if (request->getPayloadLength() > 0)
    {
      std::stringstream contentLengthStringStream;
      contentLengthStringStream << request->getPayloadLength();
      std::string finalString = "Content-length: " + contentLengthStringStream.str();
      headers = curl_slist_append(headers, finalString.c_str());
      LM_T(LmtCoap, ("Got: '%s'", finalString.c_str()));

      // --- Set contents
      char* payload = (char*) request->getPayloadCopy();
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (u_int8_t*) payload);
    }

    // Set Expect
    headers = curl_slist_append(headers, "Expect: ");

    // --- Prepare URL
    request->getURI(uriBuffer, COAP_URI_BUFFER_SIZE, &recvURILen);
    char url[strlen(host) + recvURILen + 1];
    strncpy(url, host, strlen(host));
    if (recvURILen > 0)
      strncat(url, uriBuffer, recvURILen);
    url[strlen(host) + recvURILen] = '\0';
    LM_T(LmtCoap, ("URL: '%s'", url));

    // --- Prepare CURL handle with obtained options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirection (?)
    curl_easy_setopt(curl, CURLOPT_HEADER, 1); // Activate include the header in the body output
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Put headers in place
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeMemoryCallback); // Send data here
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &httpResponse); // Custom data for response handling


    // --- Do HTTP Request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      LM_W(("curl_easy_perform() failed: %s\n", curl_easy_strerror(res)));

      // --- Cleanup curl environment
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);

      return "";
    }

    // --- Cleanup curl environment
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  std::string ret;
  ret.assign(httpResponse.memory, httpResponse.size);
  free(httpResponse.memory);
  return ret;
}

