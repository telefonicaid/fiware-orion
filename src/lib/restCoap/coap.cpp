#include "coap.h"

#include <stdio.h>
#include <boost/thread.hpp>

#include "CoapController.h"



/* ****************************************************************************
*
* run -
*
* Prepares a new thread to serve CoAP requests
*
*/
int Coap::run(const char *_host, unsigned short _httpPort, unsigned short _coapPort)
{
  boost::thread *coapServerThread = new boost::thread(boost::bind(&CoapController::serve, new CoapController(_host, _httpPort, _coapPort)));
  coapServerThread->get_id(); // to prevent 'warning: unused'
  return 0;
}

