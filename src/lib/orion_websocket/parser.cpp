#include "parser.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "logMsg/logMsg.h"

void ws_parser_parse(std::string &url, std::string &verb, std::string &payload, const char *msg)
{
  rapidjson::Document doc;
  doc.Parse(msg);

  if (!doc.IsObject())
  {
    url.clear();
    verb.clear();
    payload.clear();
    return;
  }

  url = doc["url"].GetString();
  verb = doc["verb"].GetString();

  if (doc.HasMember("payload") && doc["payload"].IsObject())
  {
    rapidjson::StringBuffer buff;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
    doc["payload"].Accept(writer);
    payload = buff.GetString();
  }
  else
    payload.clear();
}
