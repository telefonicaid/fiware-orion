#ifndef COAP_H
#define COAP_H

#include <sys/socket.h>
#include "cantcoap.h"

class Coap
{
  private:
    static const int BUFFER_SIZE     = 500;
    static const int URI_BUFFER_SIZE = 32;

    int  gTestCallback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);
    void worker();
    void serve(const char *host, const char *port);

    void printAddressStructures(struct addrinfo *addr);
    void printAddress(struct addrinfo *addr);

  public:


    int  run(const char *host, unsigned short port);
};

#endif // COAP_H

