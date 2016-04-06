#include "wsNotify.h"
#include "constants.h"
#include "parser.h"

#include <libwebsockets.h>
#include <cstring>


std::map<std::string, lws*>client;

void addSender(const std::string &subId, lws *ws)
{
  client[subId] = ws;
}

void removeSenders(const std::vector<std::string>& subIds)
{
  for (unsigned i = 0; i < subIds.size(); ++i)
  {
    client.erase(subIds[i]);
  }
}


/* ****************************************************************************
*
* sendNotifyContextRequestWs - 
*
* Send notification throught websocket.
* On success return 0, on error return 1
*/


int sendNotifyContextRequestWs
(
  const std::string& subId,
  const std::map<std::string, std::string>& headers,
  const std::string& data
)
{
  const char *msg = ws_parser_notify(subId, headers, data);

  std::map<std::string, lws*>::iterator it = client.find(subId);

  if (it == client.end())
    return 1;

  int msg_size = strlen(msg);
  size_t size = msg_size +
                LWS_SEND_BUFFER_PRE_PADDING +
                LWS_SEND_BUFFER_POST_PADDING;

  unsigned char *buff = (unsigned char *) malloc(size);
  unsigned char *p = &buff[LWS_SEND_BUFFER_PRE_PADDING];
  sprintf((char *)p, "%s", msg);

  int written = 0;

  while (written < msg_size)
  {
    written += lws_write(it->second, (p + written), strlen(msg), LWS_WRITE_TEXT);
    if (written == -1)
        return 1;
  }

  free((char *)msg);
  free(buff);

  return 0;
}
