#ifndef COAP_H
#define COAP_H

#include <sys/socket.h>
#include "cantcoap.h"

class Coap
{
  private:
    static const int BUFFER_SIZE     = 500;
    static const int URI_BUFFER_SIZE = 32;

    char           *host;
    char           *portString;
    unsigned short  port;

    void buildHttpRequest(CoapPDU *request);
    int  callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);
    void serve();

    void printAddressStructures(struct addrinfo *addr);
    void printAddress(struct addrinfo *addr);

  public:
    int  run(const char *_host, unsigned short _port);
};

#endif // COAP_H

