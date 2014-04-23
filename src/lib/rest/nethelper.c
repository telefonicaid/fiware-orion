#include <sys/types.h>
#include <sys/socket.h>
#define __USE_POSIX 1
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <math.h>
#include "nethelper.h"


#define INFO(...) printf(__VA_ARGS__); printf("\r\n")
#define ERR(...) printf(__VA_ARGS__); printf("\r\n")

#define DEBUG 1

#ifdef DEBUG
	#define DBG(...) fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\r\n")
	#define DBGX(...) fprintf(stderr,__VA_ARGS__);
#else
	#define DBG(...) {};
	#define DBGX(...) {};
#endif

int setupAddress(
	char *host,
	char *port,
	struct addrinfo **output,
	int socktype,
	int protocolFamily
	) {

	// check parameters
	if(host==NULL||port==NULL||output==NULL) {
		return -1;
	}

	// create hints for address lookup
	struct addrinfo hints;
	memset(&hints,0x00,sizeof(struct addrinfo));
	hints.ai_flags = 0;

	// check if host is numeric
	int hostIsNumeric = 1;
	for(unsigned int i=0; i<strlen(host); i++) {
		if(host[i]>'9'||host[i]<'.') {
			hostIsNumeric = 0;
			break;
		}
	}

	// check if port is numeric
	int portIsNumeric = 1;
	for(unsigned int i=0; i<strlen(port); i++) {
		if(port[i]>'9'||port[i]<'.') {
			portIsNumeric = 0;
			break;
		}
	}

	// deal with wildcard binding
	if(!hostIsNumeric) {
		if(strcmp(host,"*")==0||strcmp(host,"all")==0) {
			hints.ai_flags |= AI_PASSIVE;
		}
	}

	// set socket type
	if((socktype==SOCK_DGRAM)||(socktype==SOCK_STREAM)||(socktype==SOCK_RAW)) {
		hints.ai_socktype = socktype;
	} else {
		DBG("Unsupported socket type");
		return -1;
	}

	DBG("Host/Port numeric? %d %d",hostIsNumeric,portIsNumeric);

	// avoid doing respective lookups when either host and port or numeric
	if(hostIsNumeric) {
		hints.ai_flags |= AI_NUMERICHOST;
	}
	if(portIsNumeric) {
		hints.ai_flags |= AI_NUMERICSERV;
	}

	// use IPv4 or IPv6 according to instruction
	// try to ensure IP version works on operating system
	hints.ai_flags |= AI_ADDRCONFIG;
	if(protocolFamily==PF_INET||protocolFamily==PF_INET6)  {
		hints.ai_family = protocolFamily;
	} else {
		// let OS decide
		hints.ai_family = PF_UNSPEC;
	}
	int error = getaddrinfo(host,port,&hints,output);
	if(error) {
		DBG("Error getting address info: %s.",gai_strerror(error));
		return error;
	}

	return 0;
}

void printAddressStructures(struct addrinfo *addr) {
	int count = 0;
	while(addr) {
		DBG("Address %d:",count++);
		DBGX("   ");
		switch(addr->ai_family) {
			case AF_INET:
				DBGX("IPv4");
			break;

			case AF_INET6:
				DBGX("IPv6");
			break;

			default:
				DBGX("Unknown address family");
			break;
		}

		switch(addr->ai_socktype) {
			case SOCK_DGRAM:
				DBGX(", UDP");
			break;

			case SOCK_STREAM:
				DBGX(", TCP");
			break;

			case SOCK_RAW:
				DBGX(", RAW");
			break;

			default:
				DBGX(", Unknown socket type.");
			break;
		}

		// print out address host and port
		struct sockaddr_in *v4Addr;
		struct sockaddr_in6 *v6Addr;
		char straddr[INET6_ADDRSTRLEN];
		switch(addr->ai_family) {
			case AF_INET:
				v4Addr = (struct sockaddr_in*)addr->ai_addr;
				DBGX(", %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
			break;

			case AF_INET6:
				v6Addr = (struct sockaddr_in6*)addr->ai_addr;
				DBGX(", %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
			break;
		}
		DBG(" ");

      addr = addr->ai_next;
   }
}

void printAddress(struct addrinfo *addr) {
	// print out bound address
	if(addr->ai_family==AF_INET) {
		struct sockaddr_in *v4Addr = (struct sockaddr_in*)addr->ai_addr;
		INFO("UDP socket: %s:%d",inet_ntoa(v4Addr->sin_addr),ntohs(v4Addr->sin_port));
	} else if(addr->ai_family==AF_INET6) {
		char straddr[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *v6Addr = (struct sockaddr_in6*)addr->ai_addr;
		INFO("UDP socket: %s:%d",inet_ntop(AF_INET6,&v6Addr->sin6_addr,straddr,sizeof(straddr)),ntohs(v6Addr->sin6_port));
	}
}
