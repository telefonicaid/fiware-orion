#ifndef COAP_H
#define COAP_H

#include <vector>
#include "cantcoap.h"
#include "uthash.h"
// for mbed compatibility
//#define failGracefully exit

class Coap
{
  private:

    typedef int (*ResourceCallback)(CoapPDU *pdu, int sockfd, struct sockaddr_storage *recvFrom);

    // resource URIs here
    //const char *gURIA = "/test";

//    const char *gURIList[] = {
//      "/test"
//    };

    // URIs mapped to callback functions here
//    ResourceCallback gCallbacks[] = {
//      gTestCallback
//    };

//    std::vector<ResourceCallback> gCallbacks;

    static const int gNumResources = 1;



    // using uthash for the URI hash table. Each entry contains a callback handler.
    struct URIHashEntry {
        const char *uri;
        ResourceCallback callback;
        int id;
        UT_hash_handle hh;
    };

    int setupAddress(const char *host, const char *port, struct addrinfo **output, int socktype, int protocolFamily);
    void printAddressStructures(struct addrinfo *addr);
    void printAddress(struct addrinfo *addr);
    int gTestCallback(CoapPDU *request, int sockfd, struct sockaddr_storage *recvFrom);

  public:
    Coap()
    {
//      gCallbacks.push_back(gTestCallback);
    }

    // callback functions defined here

    int run(int argc, char **argv);
};

#endif // COAP_H

