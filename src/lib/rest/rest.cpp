/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/wsStrip.h"
#include "common/globals.h"
#include "rest/RestService.h"
#include "rest/rest.h"
#include "rest/restReply.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netdb.h>



/* ****************************************************************************
*
* IP - 
*/
#define  LOCAL_IP_V6  "::"
#define  LOCAL_IP_V4  "0.0.0.0"



/* ****************************************************************************
*
* PAYLOAD_SIZE - 
*/
#define PAYLOAD_SIZE       (64 * 1024 * 1024)
#define PAYLOAD_MAX_SIZE   (1 * 1024 * 1024)
#define STATIC_BUFFER_SIZE (32 * 1024)



/* ****************************************************************************
*
* Globals
*/
static RestService*              restServiceV          = NULL;
static unsigned short            port                  = 0;
static RestServeFunction         serveFunction         = NULL;
static bool                      acceptTextXml         = false;
static char                      bindIp[MAX_LEN_IP]    = "0.0.0.0";
static char                      bindIPv6[MAX_LEN_IP]  = "::";
IpVersion                        ipVersionUsed         = IPDUAL;

static MHD_Daemon*               mhdDaemon             = NULL;
static MHD_Daemon*               mhdDaemon_v6          = NULL;
static struct sockaddr_in        sad;
static struct sockaddr_in6       sad_v6;
__thread char                    static_buffer[STATIC_BUFFER_SIZE];



/* ****************************************************************************
*
* httpHeaderGet - 
*/
static int httpHeaderGet(void* cbDataP, MHD_ValueKind kind, const char* ckey, const char* value)
{
  HttpHeaders*  headerP = (HttpHeaders*) cbDataP;
  std::string   key     = ckey;

  LM_T(LmtHttpHeaders, ("HTTP Header:   %s: %s", key.c_str(), value));

  if      (strcasecmp(key.c_str(), "user-agent") == 0)      headerP->userAgent      = value;
  else if (strcasecmp(key.c_str(), "host") == 0)            headerP->host           = value;
  else if (strcasecmp(key.c_str(), "accept") == 0)          headerP->accept         = value;
  else if (strcasecmp(key.c_str(), "expect") == 0)          headerP->expect         = value;
  else if (strcasecmp(key.c_str(), "connection") == 0)      headerP->connection     = value;
  else if (strcasecmp(key.c_str(), "content-type") == 0)    headerP->contentType    = value;
  else if (strcasecmp(key.c_str(), "content-length") == 0)  headerP->contentLength  = atoi(value);
  else
    LM_T(LmtHttpUnsupportedHeader, ("'unsupported' HTTP header: '%s', value '%s'", ckey, value));


  if ((strcasecmp(key.c_str(), "connection") == 0) && (headerP->connection != "") && (headerP->connection != "close"))
     LM_W(("connection '%s' - currently not supported, sorry ...", headerP->connection.c_str()));

  /* Note that the strategy to "fix" the Content-Type is to replace the ";" with 0
   * to "deactivate" this part of the string in the checking done at connectionTreat() */
  char* cP = (char*) headerP->contentType.c_str();
  char* match;
  if ((match = strstr(cP, ";")) != NULL)
  {
     *match = 0;
     headerP->contentType = cP;
  }

  headerP->gotHeaders = true;

  return MHD_YES;
}



/* ****************************************************************************
*
* wantedOutputSupported - 
*/
static Format wantedOutputSupported(std::string acceptList, std::string* charsetP)
{
  std::vector<std::string>  vec;
  char* copy;
  if (acceptList.length() == 0) 
  {
    /* HTTP RFC states that a missing Accept header must be interpreted as if the client is
     * accepting any type */
    copy = strdup("*/*");
  }
  else 
  {
    copy = strdup((char*) acceptList.c_str());
  }
  char*                     cP   = copy;

  do
  {
     char* comma;

     comma = strstr(cP, ",");
     if (comma != NULL)
     {
        *comma = 0;
        
        cP = wsStrip(cP);
        vec.push_back(cP);
        cP = comma;
        ++cP;
     }
     else
     {
        cP = wsStrip(cP);
        if (*cP != 0)
        {
           vec.push_back(cP);
        }
        *cP = 0;
     }

  } while (*cP != 0);

  free(copy);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
     char* s;

     //
     // charset embedded in 'Accept' header?
     //
     if ((s = strstr((char*) vec[ix].c_str(), ";")) != NULL)
     {
        *s = 0;
        ++s;
        s = wsStrip(s);
        if (strncmp(s, "charset=", 8) == 0)
        {
           s = &s[8];
           s = wsStrip(s);

           if (charsetP != NULL)
              *charsetP = s;
        }
     }

     std::string format = vec[ix].c_str();
     if (format == "*/*")              return XML;
     if (format == "*/xml")            return XML;
     if (format == "application/*")    return XML;
     if (format == "application/xml")  return XML;
     if (format == "application/json") return JSON;
     if (format == "*/json")           return JSON;
     
     if ((acceptTextXml == true) && (format == "text/xml"))  return XML;

     // Here we put in cases for JSON, TEXT etc ...


     //
     // Resetting charset
     //
     if (charsetP != NULL)
        *charsetP = "";
  }

  LM_RE(NOFORMAT, ("No valid 'Accept-format' found"));
}



/* ****************************************************************************
*
* serve - 
*/
static void serve(ConnectionInfo* ciP)
{
  if (ciP->payload == NULL)
    LM_T(LmtService, ("Serving request %s %s without payload", ciP->method.c_str(), ciP->url.c_str()));
  else
    LM_T(LmtService, ("Serving request %s %s with %lu bytes of payload", ciP->method.c_str(), ciP->url.c_str(), ciP->httpHeaders.contentLength));

  restService(ciP, restServiceV);
}



/* ****************************************************************************
*
* requestCompleted - 
*/
static void requestCompleted
(
  void*                       cls,
  MHD_Connection*             connection,
  void**                      con_cls,
  MHD_RequestTerminationCode  toe
)
{
  ConnectionInfo* ciP      = (ConnectionInfo*) *con_cls;

  if ((ciP->payload != NULL) && (ciP->payload != static_buffer))
    free(ciP->payload);

  delete(ciP);
  *con_cls = NULL;
}



/* ****************************************************************************
*
* formatCheck - 
*/
static int formatCheck(ConnectionInfo* ciP)
{
  ciP->inFormat   = formatParse(ciP->httpHeaders.contentType, NULL);
  ciP->outFormat  = wantedOutputSupported(ciP->httpHeaders.accept, &ciP->charset);

  if (ciP->outFormat == NOFORMAT)
  {
    /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
    ciP->answer         = restErrorReplyGet(ciP, XML, "", "OrionError", SccNotAcceptable,
                                            std::string("acceptable types: 'application/xml' but Accept header in request was: '") + ciP->httpHeaders.accept + "'");
    ciP->httpStatusCode = SccNotAcceptable;

    ciP->outFormat      = XML; // We use XML as default format

    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* contentTypeCheck -
*
* NOTE
*   Any failure about Content-Type is an error in the HTTP layer (not exclusively NGSI)
*   so we don't want to use the default 200
*/
static int contentTypeCheck(ConnectionInfo* ciP)
{
  std::string details = "";

  //
  // Three cases:
  //   1. If there is no payload, the Content-Type is not interesting
  //   2. Payload present but no Content-Type 
  //   3. Content-Type present but not supported

  if (ciP->httpHeaders.contentLength == 0)
    details = "";
  else if (ciP->httpHeaders.contentType == "")
    details = "Content-Type header not used, default application/octet-stream is not supported";
  else if ((acceptTextXml == true) && (ciP->httpHeaders.contentType == "text/xml"))
    details = "";
  else if ((ciP->httpHeaders.contentType != "application/xml") && (ciP->httpHeaders.contentType != "application/json"))
     details = std::string("not supported content type: ") + ciP->httpHeaders.contentType;

  if (details != "")
  {
    ciP->answer         = restErrorReplyGet(ciP, ciP->outFormat, "", "OrionError", SccUnsupportedMediaType, details);
    ciP->httpStatusCode = SccUnsupportedMediaType;

    return 1;
  }

  return 0;
}



/* ****************************************************************************
*
* connectionTreat - 
*
* This is the MHD_AccessHandlerCallback function for MHD_start_daemon
* This function returns:
* o MHD_YES  if the connection was handled successfully
* o MHD_NO   if the socket must be closed due to a serious error
*
* - This function is called once when the headers are read and the ciP is created.
* - Then it is called for data payload and once all the payload an acknowledgement
*   must be done, setting *upload_data_size to ZERO.
* - The last call is made with *upload_data_size == 0 and now is when the connection
*   is open to send responses.
*
* Call 1: *con_cls == NULL
* Call 2: *con_cls != NULL  AND  *upload_data_size != 0
* Call 3: *con_cls != NULL  AND  *upload_data_size == 0
*/
static int connectionTreat
(
   void*            cls,
   MHD_Connection*  connection,
   const char*      url,
   const char*      method,
   const char*      version,
   const char*      upload_data,
   size_t*          upload_data_size,
   void**           con_cls
)
{
  ConnectionInfo* ciP      = (ConnectionInfo*) *con_cls;
  size_t          dataLen  = *upload_data_size;

  // 1. First call - setup ConnectionInfo and get/check HTTP headers
  if (ciP == NULL)
  {
    if ((ciP = new ConnectionInfo(url, method, version, connection)) == NULL)
      LM_RE(MHD_NO, ("Error allocating ConnectionInfo"));
        
    *con_cls = (void*) ciP; // Pointer to ConnectionInfo for subsequent calls

    MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, &ciP->httpHeaders);

    if ((formatCheck(ciP) != 0) || (contentTypeCheck(ciP) != 0))
      LM_W(("Error in out-format or content-type"));

    return MHD_YES;
  }


  //
  // 2. Data gathering calls
  //
  // 2-1. Data gathering calls, just wait
  // 2-2. Last data gathering call, acknowledge the receipt of data
  //
  if (dataLen != 0)
  {
    if (dataLen == ciP->httpHeaders.contentLength)
    {
      if (ciP->httpHeaders.contentLength <= PAYLOAD_MAX_SIZE)
      {
        if (ciP->httpHeaders.contentLength > STATIC_BUFFER_SIZE)
          ciP->payload = (char*) malloc(ciP->httpHeaders.contentLength);
        else
          ciP->payload = static_buffer;

        ciP->payloadSize = dataLen;
        memcpy(ciP->payload, upload_data, dataLen);
        ciP->payload[dataLen] = 0;
      }
      else
      {
        char details[256];
        snprintf(details, sizeof(details), "payload size: %d, max size supported: %d", ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE);

        ciP->answer         = restErrorReplyGet(ciP, ciP->outFormat, "", ciP->url, SccRequestEntityTooLarge, details);
        ciP->httpStatusCode = SccRequestEntityTooLarge;
      }

      // All payload received, acknowledge!
      *upload_data_size = 0;
    }
    else
      LM_T(LmtPartialPayload, ("Got %d of payload of %d bytes", dataLen, ciP->httpHeaders.contentLength));

    return MHD_YES;
  }


  // 3. Finally, serve the request (unless an error has occurred)
  if (((ciP->verb == POST) || (ciP->verb == PUT)) && (ciP->httpHeaders.contentLength == 0))
  {
    std::string errorMsg = restErrorReplyGet(ciP, ciP->outFormat, "", url, SccLengthRequired, "Zero/No Content-Length in PUT/POST request");
    ciP->httpStatusCode = SccLengthRequired;
    restReply(ciP, errorMsg);
  }
  else if (ciP->answer != "")
    restReply(ciP, ciP->answer);
  else
    serveFunction(ciP);

  return MHD_YES;
}



/* ****************************************************************************
*
* restStart - 
*/
static int restStart(IpVersion ipVersion)
{
  int ret;

  if (port == 0)
     LM_RE(1, ("Please call restInit before starting the REST service"));

  if ((ipVersion == IPV4) || (ipVersion == IPDUAL)) 
  { 
    // Code for IPv4 stack
    ret = inet_pton(AF_INET, bindIp, &(sad.sin_addr.s_addr));
    if (ret != 1) {
      LM_RE(2, ("V4 inet_pton fail for %s", bindIp));
    }

    LM_V(("IPv4 port: %d", port));
    sad.sin_family = AF_INET;
    sad.sin_port   = htons(port);

    LM_V(("Starting http daemon on IPv4 %s port %d", bindIp, port));
    mhdDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, // MHD_USE_SELECT_INTERNALLY
                               htons(port),
                               NULL,
                               NULL,
                               connectionTreat,
                               NULL,
                               MHD_OPTION_NOTIFY_COMPLETED,
                               requestCompleted,
                               NULL,
                               MHD_OPTION_CONNECTION_MEMORY_LIMIT,
                               2 * PAYLOAD_SIZE,
                               MHD_OPTION_SOCK_ADDR, (struct sockaddr*) &sad,
                               MHD_OPTION_END);
  
    if (mhdDaemon == NULL)
       LM_RE(3, ("MHD_start_daemon failed"));

  }  

  if ((ipVersion == IPV6) || (ipVersion == IPDUAL))
  { 
    // Code for IPv6 stack
    ret = inet_pton(AF_INET6, bindIPv6, &(sad_v6.sin6_addr.s6_addr));
    if (ret != 1) {
      LM_RE(1, ("V6 inet_pton fail for %s", bindIPv6));
    }

    sad_v6.sin6_family = AF_INET6;
    sad_v6.sin6_port = htons(port);

    LM_V(("Starting http daemon on IPv6 %s port %d", bindIPv6, port));

    mhdDaemon_v6 = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION | MHD_USE_IPv6,
                               htons(port),
                               NULL,
                               NULL,
                               connectionTreat,
                               NULL,
                               MHD_OPTION_NOTIFY_COMPLETED,
                               requestCompleted,
                               NULL,
                               MHD_OPTION_CONNECTION_MEMORY_LIMIT,
                               2 * PAYLOAD_SIZE,
                               MHD_OPTION_SOCK_ADDR, (struct sockaddr*) &sad_v6,
                               MHD_OPTION_END);

    if (mhdDaemon_v6 == NULL)
       LM_RE(1, ("MHD_start_daemon_v6 failed"));

  }

  return 0;
}



/* ****************************************************************************
*
* restInit - 
*
* FIXME P5: add vector of the accepted content-types, instead of the bool
*           argument _acceptTextXml that was added for iotAgent only.
*           See Issue #256
*/
void restInit(RestService* _restServiceV, IpVersion _ipVersion, const char* _bindAddress, unsigned short _port, RestServeFunction _serveFunction, bool _acceptTextXml)
{
  port          = _port;
  restServiceV  = _restServiceV;
  ipVersionUsed = _ipVersion;
  serveFunction = (_serveFunction != NULL)? _serveFunction : serve;
  acceptTextXml = _acceptTextXml;

  strncpy(bindIp, LOCAL_IP_V4, MAX_LEN_IP - 1);
  strncpy(bindIPv6, LOCAL_IP_V6, MAX_LEN_IP - 1);

  if (isIPv6(std::string(_bindAddress)))
    strncpy(bindIPv6, _bindAddress, MAX_LEN_IP - 1);
  else
    strncpy(bindIp, _bindAddress, MAX_LEN_IP - 1);

  if ((_ipVersion == IPV4) || (_ipVersion == IPDUAL))
     strncpy(bindIp, bindIp, MAX_LEN_IP - 1);

  if ((_ipVersion == IPV6) || (_ipVersion == IPDUAL))
     strncpy(bindIPv6, bindIPv6, MAX_LEN_IP - 1);


  // Starting REST interface
  int r;
  if ((r = restStart(_ipVersion)) != 0)
  {
    fprintf(stderr, "restStart: error %d\n", r);
    orionExitFunction(1, "restStart: error");
  }
}
