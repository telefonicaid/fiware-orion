#include "HttpProxy.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#define __USE_POSIX 1
//#include <netdb.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
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
size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  MemoryStruct *mem = (MemoryStruct *)userp;

  mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    LM_V(("Not enough memory (realloc returned NULL)\n"));
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
std::string sendHttpRequest(char *host, unsigned short port, CoapPDU *request)
{
  char*         url                      = NULL;
  int           recvURILen               = 0;
  CURL*         curl                     = curl_easy_init();
  MemoryStruct* httpResponse             = NULL;
  CURLcode      res;
  char          uriBuffer[COAP_URI_BUFFER_SIZE];

  if (curl)
  {
    // Allocate to hold HTTP response
    httpResponse = new MemoryStruct;
    httpResponse->memory = (char*)malloc(1); // will grow as needed
    httpResponse->size = 0; // no data at this point

    // --- Set HTTP verb
    std::string httpVerb = "";

    switch(request->getCode()) {
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
    LM_V(("Got an HTTP %s", httpVerb.c_str()));


    // --- Prepare headers
    struct curl_slist *headers = NULL;
    CoapPDU::CoapOption* options = request->getOptions();
    int numOptions = request->getNumOptions();
    for (int i = 0; i < numOptions ; i++)
    {
      switch (options[i].optionNumber)
      {
        case CoapPDU::COAP_OPTION_URI_PATH:
        {
          char buffer[options[i].optionValueLength + 1];
          memcpy(&buffer, options[i].optionValuePointer, options[i].optionValueLength);
          buffer[options[i].optionValueLength] = '\0';
          LM_V(("Got URI_PATH option: '%s'", buffer));
          break;
        }
        case CoapPDU::COAP_OPTION_CONTENT_FORMAT:
        {
          std::string string = "Content-type: ";
          char buffer[options[i].optionValueLength + 1];
          memcpy(&buffer, options[i].optionValuePointer, options[i].optionValueLength);
          buffer[options[i].optionValueLength] = '\0';
          string += buffer;
          headers = curl_slist_append(headers, string.c_str());
          LM_V(("Got CONTENT-FORMAT option: '%s'", string.c_str()));
          break;
        }
        case CoapPDU::COAP_OPTION_ACCEPT:
        {
          std::string string = "Accept: ";
          char buffer[options[i].optionValueLength + 1];
          memcpy(&buffer, options[i].optionValuePointer, options[i].optionValueLength);
          buffer[options[i].optionValueLength] = '\0';
          string += buffer;
          headers = curl_slist_append(headers, string.c_str());
          LM_V(("Got ACCEPT option: '%s'", string.c_str()));
          break;
        }
        default:
        {
          char buffer[options[i].optionValueLength + 1];
          memcpy(&buffer, options[i].optionValuePointer, options[i].optionValueLength);
          buffer[options[i].optionValueLength] = '\0';
          LM_V(("Got unknown option: '%s'", buffer));
          break;
        }
      }
    }


    // Set Content-length
    std::stringstream contentLengthStringStream;
    contentLengthStringStream << request->getPayloadLength();
    std::string finalString = "Content-length: " + contentLengthStringStream.str();
    headers = curl_slist_append(headers, finalString.c_str());
    LM_V(("Got: '%s'", finalString.c_str()));

    // Set Expect
    headers = curl_slist_append(headers, "Expect: ");

    // --- Prepare URL
    request->getURI(uriBuffer, COAP_URI_BUFFER_SIZE, &recvURILen);
    url = new char[strlen(host) + recvURILen];
    strcpy(url, host);
    if (recvURILen > 0)
      strncat(url, uriBuffer, recvURILen);

    // --- Set contents
    //u_int8_t* payload = new u_int8_t[request->getPayloadLength()];

//    u_int8_t* payload = request->getPayloadCopy();
    char* payload = (char*)request->getPayloadCopy();


    // --- Prepare CURL handle with obtained options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirection (?)
    curl_easy_setopt(curl, CURLOPT_HEADER, 1); // Activate include the header in the body output
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Put headers in place
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeMemoryCallback); // Send data here
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)httpResponse); // Custom data for response handling
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (u_int8_t*) payload);


    // --- Do HTTP Request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    // --- Cleanup curl environment
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  std::string ret;
  ret.assign(httpResponse->memory, httpResponse->size);
//  return httpResponse;
  return ret;
}



/* ****************************************************************************
*
* http2Coap -
*/
void http2Coap(MemoryStruct *http, CoapPDU *coap)
{

}
