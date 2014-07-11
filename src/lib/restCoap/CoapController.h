#ifndef COAPCONTROLLER_H
#define COAPCONTROLLER_H

#include <netinet/in.h>
#include <boost/thread.hpp>
#include <string>

#include "cantcoap.h"

class CoapController
{

  std::string     host;
  std::string     coapPortStr;
//  char*           host;
//  char*           coapPortStr;
  unsigned short  httpPort;
  unsigned short  coapPort;

  int sendDatagram(int sockfd, boost::scoped_ptr<CoapPDU>& res, sockaddr* recvFrom);
  int sendError(int sockfd, CoapPDU* req, sockaddr* recvFrom, CoapPDU::Code code);

  int callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);

public:
  CoapController(const char* _host, unsigned short _httpPort, unsigned short _coapPort);
  void serve();
};

#endif // COAPCONTROLLER_H
