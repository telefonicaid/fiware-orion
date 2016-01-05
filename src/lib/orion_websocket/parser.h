#ifndef WS_PARSER_H
#define WS_PARSER_H

#include <vector>
#include <string>

void ws_parser_parse(std::string &url, std::string &verb, std::string &payload, const char *msg);

#endif
