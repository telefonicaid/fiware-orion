#ifndef WS_NOTIFY_H
#define WS_NOTIFY_H

#include <map>
#include <vector>
#include <string>

struct lws;

void addSender(const std::string &subId, lws *ws);
void removeSenders(const std::vector<std::string>& subIds);
int sendNotifyContextRequestWs
(
  const std::string& subId,
  const std::map<std::string, std::string>& headers,
  const std::string& data
);

#endif
