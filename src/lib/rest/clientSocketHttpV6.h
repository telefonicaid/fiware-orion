// RBL nuevo

#ifndef CLIENT_SOCKET_HTTP_V6_H
#define CLIENT_SOCKET_HTTP_V6_H

/* ****************************************************************************
*
* FILE                    clientSocketHttpV6.h
*
* CREATION DATE           Dic 2013
*
* AUTHOR                  developer 
*
*
*/
#include <string>
#include <vector>

#include "ConnectionInfo.h" 

#define  TAM_BUF        8192


/***************************************************************************
*
* socketHttpConnect - 
*/
extern int socketHttpConnectV6(std::string host, unsigned short port);



/* ****************************************************************************
*
* sendHttpSocket - 
*/
extern std::string sendHttpSocketV6( std::string ip,
                                   unsigned short port, 
                                   std::string verb,
                                   std::string resource, 
                                   std::string content_type, 
                                   std::string content,
                                   bool waitForResponse = true
                                   );

#endif
