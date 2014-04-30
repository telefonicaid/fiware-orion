#include "coap.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#define __USE_POSIX 1
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <string>
#include <string.h>

#include <curl/curl.h>

#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


/* ****************************************************************************
*
* writeMemoryCallback -
*/
static size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
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
* buildHttpRequest -
*/
void Coap::sendHttpRequest(CoapPDU * request, MemoryStruct* chunk)
{
  char*     url                          = NULL;
  int       recvURILen                   = 0;
  CURL*     curl                         = curl_easy_init();
  CURLcode  res;
  char      uriBuffer[URI_BUFFER_SIZE];

  if (curl)
  {
    // Allocate to hold HTTP response
    chunk = new MemoryStruct;
    chunk->memory = (char*)malloc(1); // will grow as needed
    chunk->size = 0; // no data at this point

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


    // --- Prepare URL
    request->getURI(uriBuffer, URI_BUFFER_SIZE, &recvURILen);
    url = new char[strlen(host) + recvURILen];
    strcpy(url, host);
    if (recvURILen > 0)
      strncat(url, uriBuffer, recvURILen);

    // --- Set contents
    u_int8_t* payload = request->getPayloadCopy();


    // --- Prepare CURL handle with obtained options
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_PORT, port);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Allow redirection (?)
    curl_easy_setopt(curl, CURLOPT_HEADER, 1); // Activate include the header in the body output
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); // Put headers in place
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeMemoryCallback); // Send data here
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk); // Custom data for response handling
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);


    // --- Do HTTP Request
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }


    // --- Barf out
    LM_V(("CHUNK:\n%s", chunk->memory));



    // --- Cleanup curl environment
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }
}



/* ****************************************************************************
*
* callback -
*/
void Coap::http2Coap(MemoryStruct *http, CoapPDU *coap)
{

}



/* ****************************************************************************
*
* callback -
*/
int Coap::callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom)
{
  MemoryStruct* httpResponse = NULL; // Will contain HTTP Response

  socklen_t addrLen = sizeof(struct sockaddr_in);
  if (recvFrom->ss_family == AF_INET6) {
    addrLen = sizeof(struct sockaddr_in6);
  }

  // Prepare for and send request to MHD in HTTP
  sendHttpRequest(request, httpResponse);
  if (httpResponse == NULL)
  {
    // Could not get an answer
  }

  CoapPDU *coapResponse = new CoapPDU();
  // Parse header
  http2Coap(httpResponse, coapResponse);

  // Prepare appropriate response in CoAP
  coapResponse->setVersion(1);
  coapResponse->setMessageID(request->getMessageID());
  coapResponse->setToken(request->getTokenPointer(),request->getTokenLength());
  //response->setToken((uint8_t*)"\1\16",2);
  char *payload = (char*)"This is a mundanely worded test payload.";

  // respond differently, depending on method code
  switch(request->getCode()) {
    case CoapPDU::COAP_EMPTY:
      // makes no sense, send RST
    break;
    case CoapPDU::COAP_GET:
      coapResponse->setCode(CoapPDU::COAP_CONTENT);
      coapResponse->setContentFormat(CoapPDU::COAP_CONTENT_FORMAT_TEXT_PLAIN);
      coapResponse->setPayload((uint8_t*)payload,strlen(payload));
    break;
    case CoapPDU::COAP_POST:
      coapResponse->setCode(CoapPDU::COAP_CREATED);
    break;
    case CoapPDU::COAP_PUT:
      coapResponse->setCode(CoapPDU::COAP_CHANGED);
    break;
    case CoapPDU::COAP_DELETE:
      coapResponse->setCode(CoapPDU::COAP_DELETED);
      coapResponse->setPayload((uint8_t*)"DELETE OK",9);
    break;
    default:
    break;
  }

  // type
  switch(request->getType()) {
    case CoapPDU::COAP_CONFIRMABLE:
      coapResponse->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_NON_CONFIRMABLE:
      coapResponse->setType(CoapPDU::COAP_ACKNOWLEDGEMENT);
    break;
    case CoapPDU::COAP_ACKNOWLEDGEMENT:
    break;
    case CoapPDU::COAP_RESET:
    break;
    default:
      return 1;
    break;
  };

  // send the packet
  ssize_t sent = sendto(sockfd, coapResponse->getPDUPointer(), coapResponse->getPDULength(), 0, (sockaddr*)recvFrom, addrLen);
  if (sent < 0) {
    LM_V(("Error sending packet: %ld.",sent));
    perror(NULL);
    return 1;
  } else {
    LM_V(("Sent: %ld",sent));
  }

  return 0;
}



/* ****************************************************************************
*
* serve -
*/
void Coap::serve()
{
  // Buffers for UDP and URIs
  char buffer[BUFFER_SIZE];
  char uriBuffer[URI_BUFFER_SIZE];
  int  recvURILen = 0;
  int  ret = 0;

  // storage for handling receive address
  struct sockaddr_storage recvAddr;
  socklen_t               recvAddrLen = sizeof(struct sockaddr_storage);
  //struct sockaddr_in      *v4Addr;
  //struct sockaddr_in6     *v6Addr;
  //char                    straddr[INET6_ADDRSTRLEN];

  // Prepare binding address
  struct addrinfo *bindAddr = NULL;
  struct addrinfo hints;

  LM_V(("Setting up bind address"));
  memset(&hints, 0x00, sizeof(struct addrinfo));
  hints.ai_flags      = 0;
  hints.ai_socktype   = SOCK_DGRAM;
  hints.ai_flags     |= AI_NUMERICSERV;
  hints.ai_family     = PF_INET; // ipv4, PF_INET6 for ipv6 or PF_UNSPEC to let OS decide

  int error = getaddrinfo(host, portString, &hints, &bindAddr);
  if (error) {
    LM_V(("Could not start CoAP server: Error getting address info: %s.", gai_strerror(error)));
    return;
  }

  // Setting up the UDP socket
  int sd = socket(bindAddr->ai_family,bindAddr->ai_socktype,bindAddr->ai_protocol);

  // Binding socket
  if (bind(sd,bindAddr->ai_addr,bindAddr->ai_addrlen)!=0) {
    LM_V(("Could not start CoAP server: Error binding socket"));
    perror(NULL);
    return;
  }

  // reuse the same PDU
  CoapPDU *recvPDU = new CoapPDU((uint8_t*)buffer, BUFFER_SIZE, BUFFER_SIZE);

  LM_V(("Listening for packets..."));
  while (1) {
    // receive packet
    ret = recvfrom(sd, &buffer, BUFFER_SIZE, 0, (sockaddr*)&recvAddr, &recvAddrLen);
    if (ret == -1) {
      LM_V(("Error receiving data"));
      return;
    }

    // print src address
//    switch (recvAddr.ss_family) {
//      case AF_INET:
//        v4Addr = (struct sockaddr_in*)&recvAddr;
//        LM_V(("Got packet from %s:%d", inet_ntoa(v4Addr->sin_addr), ntohs(v4Addr->sin_port)));
//      break;

//      case AF_INET6:
//        v6Addr = (struct sockaddr_in6*)&recvAddr;
//        LM_V(("Got packet from %s:%d",inet_ntop(AF_INET6, &v6Addr->sin6_addr, straddr, sizeof(straddr)), ntohs(v6Addr->sin6_port)));
//      break;
//    }

    // validate packet
    if (ret > BUFFER_SIZE) {
      LM_V(("PDU too large to fit in pre-allocated buffer"));
      continue;
    }
    recvPDU->setPDULength(ret);
    if (recvPDU->validate() != 1) {
      LM_V(("Malformed CoAP packet"));
      continue;
    }
    LM_V(("Valid CoAP PDU received"));
    //recvPDU->printHuman();

    // Treat URI
    if (recvPDU->getURI(uriBuffer, URI_BUFFER_SIZE, &recvURILen) != 0) {
      LM_V(("Error retrieving URI"));
      continue;
    }

    if (recvURILen == 0) {
      LM_V(("There is no URI associated with this Coap PDU"));
    } else {
      // Invoke a callback thread
      boost::thread *workerThread = new boost::thread(boost::bind(&Coap::callback, this, recvPDU, sd, &recvAddr));
      workerThread->get_id();

      // Wait for thread to finnish (like using no threads at all) for now
      workerThread->join();

      // Old call
      //callback(recvPDU, sd, &recvAddr);
      continue;
    }

    // no URI, handle cases

    // code == 0, no payload, this is a ping request, send RST
    if ((recvPDU->getPDULength() == 0) && (recvPDU->getCode() == 0)) {
      LM_V(("CoAP ping request"));
    }

  }
}



/* ****************************************************************************
*
* run -
*/
int Coap::run(const char *_host, unsigned short _port)
{
  char* portString = new char[6];
  snprintf(portString, 6, "%hd", _port);

  this->port = _port;
  this->host = new char[strlen(_host)];
  strcpy(this->host, _host);
  this->portString = new char[strlen(portString)];
  strcpy(this->portString, portString);

  boost::thread *coapServerThread = new boost::thread(boost::bind(&Coap::serve, this));

  coapServerThread->get_id();

  // Main thread waits for this coap thread? NO
  //coapServerThread->join();
  return 0;
}

void Coap::printAddressStructures(struct addrinfo *addr) {
  int count = 0;
  while (addr) {
    printf("Address %d:",count++);
    printf("   ");
    switch(addr->ai_family) {
      case AF_INET:
        printf("IPv4");
      break;
      case AF_INET6:
        printf("IPv6");
      break;
      default:
        printf("Unknown address family");
      break;
    }
    switch (addr->ai_socktype) {
      case SOCK_DGRAM:
        printf(", UDP");
      break;

      case SOCK_STREAM:
        printf(", TCP");
      break;

      case SOCK_RAW:
        printf(", RAW");
      break;

      default:
        printf(", Unknown socket type.");
      break;
    }

    // print out address host and port
    struct sockaddr_in *v4Addr;
    struct sockaddr_in6 *v6Addr;
    char straddr[INET6_ADDRSTRLEN];
    switch (addr->ai_family) {
      case AF_INET:
        v4Addr = (struct sockaddr_in*)addr->ai_addr;
        printf(", %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
      break;

      case AF_INET6:
        v6Addr = (struct sockaddr_in6*)addr->ai_addr;
        printf(", %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
      break;
    }
    printf(" ");
      addr = addr->ai_next;
   }
}

void Coap::printAddress(struct addrinfo *addr) {
  // print out bound address
  if (addr->ai_family == AF_INET) {
    struct sockaddr_in *v4Addr = (struct sockaddr_in*)addr->ai_addr;
    printf("UDP socket: %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
  } else if (addr->ai_family == AF_INET6) {
    char straddr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *v6Addr = (struct sockaddr_in6*)addr->ai_addr;
    printf("UDP socket: %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
  }
}
