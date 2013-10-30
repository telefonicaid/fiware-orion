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
#include "rest/RestService.h"
#include "rest/rest.h"
#include "rest/restReply.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>



/* ****************************************************************************
*
* PAYLOAD_SIZE - 
*/
#define PAYLOAD_SIZE      (64 * 1024 * 1024)
#define PAYLOAD_MAX_SIZE  (1 * 1024 * 1024)



/* ****************************************************************************
*
* Globals
*/
static unsigned short            port          = 0;
static char                      bindIp[15]    = "0.0.0.0";
static RestService*              restServiceV  = NULL;
static MHD_Daemon*               mhdDaemon     = NULL;
static struct sockaddr_in        sad;



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
  else if (strcasecmp(key.c_str(), "content-type") == 0)    headerP->contentType    = value;
  else if (strcasecmp(key.c_str(), "content-length") == 0)  headerP->contentLength  = atoi(value);
  else
    LM_W(("'unsupported' HTTP header: '%s', value '%s'", ckey, value));


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



extern char savedResponse[2 * 1024 * 1024];
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
  ConnectionInfo* ciP = (ConnectionInfo*) *con_cls;

  if (ciP == NULL)
    return;

  LM_T(LmtHttpRequest, ("Request Completed. Read %d bytes of %d", ciP->payloadSize, ciP->httpHeaders.contentLength));

  if (savedResponse[0] != 0)
  {
     // LM_T(LmtSavedResponse, ("Saved response found - responding ..."));
     LM_M(("Saved response found - responding ..."));
     restReply(ciP, savedResponse);
     savedResponse[0] = 0;
  }

  if (ciP->payload)
     free(ciP->payload);
  delete(ciP);
  *con_cls = NULL;
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
* connectionTreat - 
*
* This is the MHD_AccessHandlerCallback function for MHD_start_daemon
* This function returns:
* o MHD_YES  if the connection was handled successfully
* o MHD_NO   if the socket must be closed due to a serious error
*
* This function is called once when the headers are read.
* Then it is called again.
* And in the case of a message with payload, it is called a third time.
*
* Call 1: *con_cls == NULL
* Call 2: *con_cls != NULL  AND  *upload_data_size != 0
* Call 3: *con_cls != NULL  AND  *upload_data_size == 0
*
* upload_data_size has to do with payload and only valid for such requests.
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
  static int      requests = 0;

  if (ciP == NULL)
    ++requests;

  if (ciP)
  {
     ciP->callNo += 1;
     LM_T(LmtMhd, ("Request %d, callNo == %d: %d bytes of payload (of %d)", requests, ciP->callNo, *upload_data_size, ciP->httpHeaders.contentLength));
  }

  if ((ciP != NULL) && (ciP->httpHeaders.contentLength > PAYLOAD_MAX_SIZE))
  {
    LM_W(("Content-Length: %d (0x%x) too large. Max size is %d (0x%x)", ciP->httpHeaders.contentLength, ciP->httpHeaders.contentLength, PAYLOAD_MAX_SIZE, PAYLOAD_MAX_SIZE));
    ciP->requestEntityTooLarge = true;
  }

  if (((ciP != NULL) && (ciP->httpHeaders.contentLength == 0)) && (ciP->method == "POST" || ciP->method == "PUT"))
  {
    LM_W(("Zero/No Content-Length in PUT/POST request"));
    std::string errorMsg = restErrorReplyGet(ciP, ciP->outFormat, "", url, SccLengthRequired, "bad request", "Zero/No Content-Length in PUT/POST request");
    ciP->httpStatusCode = SccLengthRequired;
    restReply(ciP, errorMsg);
    LM_RE(MHD_YES, ("Zero/No Content-Length in PUT/POST request"));
  }

  // In the first call to this callback, only the headers are complete
  // So, prepare for the data and return
  if (ciP == NULL)
  {
    LM_T(LmtHttpDaemon, ("HTTP method: %s, URL: %s, version: %s", method, url, version));

    // POST 1
    ciP = new ConnectionInfo(url, method, version);
    if (ciP == NULL)
      LM_RE(MHD_NO, ("Error allocating ConnectionInfo"));
    
    // Saving the pointer to the newly allocated ConnectionInfo
    *con_cls         = (void*) ciP;

    ciP->callNo                    = 1;
    ciP->httpHeaders.contentLength = 0;
    ciP->requestEntityTooLarge     = false;

    MHD_get_connection_values(connection, MHD_HEADER_KIND, httpHeaderGet, &ciP->httpHeaders);

    ciP->connection = connection;
    ciP->outFormat  = wantedOutputSupported(ciP->httpHeaders.accept, &ciP->charset);
    if (ciP->outFormat == NOFORMAT)
    {
       /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
       ciP->outFormat = XML; // We use XML as default format
       ciP->httpStatusCode = SccNotAcceptable;
       char detail[256];
       snprintf(detail, sizeof(detail), "aceptable types: application/xml but Accept header in request was: '%s'", ciP->httpHeaders.accept.c_str());
       restReply(ciP, "Not Acceptable", detail);
       return MHD_YES;
    }

    /* Note that the Content-Type check is done only in cases where there is an actual conent, e.g. a GET request
     * without content doesn't need to check Content-Type */
    if (ciP->httpHeaders.contentLength != 0)
    {
      /* The ciP->httpHeaders.contentType constructor inits with an empty string, so if the lengh() is 0 at this point
       * that means that the header was not used in the HTTP request */
      if (ciP->httpHeaders.contentType.length() == 0)
      {
          /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
          ciP->httpStatusCode = SccUnsupportedMediaType;
          restReply(ciP, "Unsupported Media Type", "Content-Type header not used, default application/octet-stream is not supported");
          return MHD_YES;
      }

      if ((ciP->httpHeaders.contentType != "application/xml") && (ciP->httpHeaders.contentType != "application/json"))
      {
          /* This is actually an error in the HTTP layer (not exclusively NGSI) so we don't want to use the default 200 */
          ciP->httpStatusCode = SccUnsupportedMediaType;
          restReply(ciP, "Unsupported Media Type", "not supported content type: " + ciP->httpHeaders.contentType);
          return MHD_YES;
      }
    }

    if (((ciP->method == "POST") || (ciP->method == "PUT")) && (ciP->httpHeaders.contentLength != 0))
    {
      ciP->verb = (ciP->method == "POST")? POST : PUT;
    }
    else if (((ciP->method == "POST") || (ciP->method == "PUT")) && (ciP->httpHeaders.contentLength == 0))
    {
       LM_W(("PUT/POST and contentLength == 0"));
    }

    ciP->payload     = (char*) calloc(1, PAYLOAD_SIZE);
    ciP->payloadSize = 0;

    return MHD_YES;
  }

  if (*upload_data_size != 0)
     ciP->payloadSize = *upload_data_size;

  LM_D(("Accept: %s", ciP->httpHeaders.accept.c_str()));
  ciP->inFormat  = formatParse(ciP->httpHeaders.contentType, NULL);
  ciP->outFormat = formatParse(ciP->httpHeaders.accept, NULL);

  LM_T(LmtRest, ("method: %s", ciP->method.c_str()));
  if (ciP->method == "POST")
    ciP->verb = POST;
  else if (ciP->method == "GET")
    ciP->verb = GET;
  else if (ciP->method == "PUT")
    ciP->verb = PUT;
  else if (ciP->method == "DELETE")
    ciP->verb = DELETE;


  if (*upload_data_size == ciP->httpHeaders.contentLength)
  {
    ciP->payloadSize = *upload_data_size;

    if (*upload_data_size != 0)
    {
      // Adding string zero termination
       char* payload = (char*) upload_data;
       payload[*upload_data_size] = 0;

       strcpy(ciP->payload, upload_data);
       ciP->payloadSize = *upload_data_size;
    }

    LM_T(LmtRest, ("Not post-processing %d bytes of data - setting upload_data_size to ZERO?", ciP->payloadSize));
    *upload_data_size = 0; // FIXME 8 Check this '*upload_data_size = 0'

    if (ciP->httpHeaders.contentLength == 0)
    {
      restService(ciP, restServiceV);
      return MHD_YES;
    }
  }
  else if ((*upload_data_size < ciP->httpHeaders.contentLength) && (*upload_data_size != 0))
    LM_W(("Got INCOMPLETE POST payload (%d accumulated bytes): '%s'", *upload_data_size, upload_data));
  else if (*upload_data_size == 0)
  {
    if (ciP->requestEntityTooLarge == true)
    {
      char detail[256];

      snprintf(detail, sizeof(detail), "payload size: %d", ciP->httpHeaders.contentLength);
      restErrorReply(ciP, ciP->outFormat, "", ciP->url, 413, "Payload Too Large", detail);
    }
    else
    {
      LM_T(LmtService, ("Calling restService '%s' with payload: '%s'", ciP->url.c_str(), ciP->payload));
      restService(ciP, restServiceV);
    }
  }

  return MHD_YES;
}



/* ****************************************************************************
*
* restInit - 
*/
void restInit(char* _bind, unsigned short _port, RestService* _restServiceV)
{
   strcpy(bindIp, _bind);

   port          = _port;
   restServiceV  = _restServiceV;

   savedResponse[0] = 0;
}



/* ****************************************************************************
*
* restStop - 
*/
void restStop(void)
{
   if (mhdDaemon == NULL)
      LM_E(("MHD not started"));
   else
      MHD_stop_daemon(mhdDaemon);
}



/* ****************************************************************************
*
* restStart - 
*/
int restStart(void)
{
  if (port == 0)
     LM_RE(1, ("Please call restInit before starting the REST service"));

  int ret = inet_pton(AF_INET, bindIp, &(sad.sin_addr.s_addr));
  if (ret != 1) {
    LM_RE(1, ("could not parse bind IP address %s", bindIp));
  }

  sad.sin_family = AF_INET;
  sad.sin_port = htons(port);

  LM_T(LmtHttpDaemon, ("Starting http daemon on IP %s port %d", bindIp, port));
  mhdDaemon = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, // MHD_USE_SELECT_INTERNALLY
                               htons(port),
                               NULL,
                               NULL,
                               &connectionTreat,
                               NULL,
                               MHD_OPTION_NOTIFY_COMPLETED,
                               requestCompleted,
                               NULL,
                               MHD_OPTION_CONNECTION_MEMORY_LIMIT,
                               2 * PAYLOAD_SIZE,
                               MHD_OPTION_SOCK_ADDR, (struct sockaddr*) &sad,
                               MHD_OPTION_END);
  
  if (mhdDaemon == NULL)
     LM_RE(1, ("MHD_start_daemon failed"));

  return 0;
}
