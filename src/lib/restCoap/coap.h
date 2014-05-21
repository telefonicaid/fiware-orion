#ifndef COAP_H
#define COAP_H

#include <sys/socket.h>
#include "cantcoap.h"

class Coap
{
  private:

    char             *host;
    char             *portString;
    unsigned short    port;

    int   callback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);
    void  serve();

  public:
    int   run(const char *_host, unsigned short _port);


};

#endif // COAP_H

