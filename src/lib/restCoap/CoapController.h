#ifndef COAPCONTROLLER_H
#define COAPCONTROLLER_H

#include "cantcoap.h"

class CoapController
{
  char*           host;
  char*           coapPortStr;
  unsigned short  httpPort;
  unsigned short  coapPort;

  int callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);

public:
  CoapController(const char* _host, unsigned short _httpPort, unsigned short _coapPort);
  void serve();
};

#endif // COAPCONTROLLER_H
