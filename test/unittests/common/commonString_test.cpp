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
* iot_support at tid dot es
*
* Author: Fermin Galan
*/
#include "gtest/gtest.h"

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"
#include "common/string.h"
#include "common/wsStrip.h"

/* ****************************************************************************
*
* stringSplit -
*/
TEST(commonString, stringSplit)
{
   std::string               s1 = "";
   std::string               s2 = ":::::1:2::4";
   int                       components;
   std::vector<std::string>  out;

   components = stringSplit(s1, ':', out);
   EXPECT_EQ(0, components);
   EXPECT_EQ(0, out.size());

   components = stringSplit(s2, ':', out);
   EXPECT_EQ(4, components);
   EXPECT_EQ(4, out.size());
}



/* ****************************************************************************
*
* parseUrl -
*/
TEST(commonString, parseUrl)
{
   bool         r;
   std::string  url1   = "";
   std::string  url2   = "http:/Bad";
   std::string  url3   = "http:/XXX";
   std::string  url4   = "http://";
   std::string  url5   = "http://XXX:12:34";
   std::string  url51  = "http://http://XXX:1234/path";              // Per bug #1652
   std::string  url52  = "http://XXXX:/path";                        // Per bug #1652
   std::string  url6   = "http://XXX:1234/path";
   std::string  url7   = "http://XXX/path";
   std::string  urlv61 = "http://20:12345:20:80";
   std::string  urlv62 = "http://20:20:20:20:20:20:20:20:20:20";
   std::string  urlv63 = "http://2001:DB8:2de::e13:80/path";
   std::string  urlv64 = "http://:::80/path";

   std::string  host;
   int          port;
   std::string  protocol;
   std::string  path;

   r = parseUrl(url1, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url2, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url3, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url4, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url5, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url51, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(url52, host, port, path, protocol);
   EXPECT_FALSE(r);

   r = parseUrl(url6, host, port, path, protocol);
   EXPECT_TRUE(r);
   EXPECT_STREQ("XXX", host.c_str());
   EXPECT_EQ(1234, port);
   EXPECT_STREQ("/path", path.c_str());

   r = parseUrl(url7, host, port, path, protocol);
   EXPECT_TRUE(r);
   EXPECT_STREQ("XXX", host.c_str());
   EXPECT_EQ(80, port); // DEFAULT_HTTP_PORT == 80
   EXPECT_STREQ("/path", path.c_str());

   r = parseUrl(urlv61, host, port, path, protocol);
   EXPECT_FALSE(r);
   r = parseUrl(urlv62, host, port, path, protocol);
   EXPECT_FALSE(r);

   r = parseUrl(urlv63, host, port, path, protocol);
   EXPECT_TRUE(r);
   EXPECT_STREQ("2001:DB8:2de::e13", host.c_str());
   EXPECT_EQ(80, port);
   EXPECT_STREQ("/path", path.c_str());

   r = parseUrl(urlv64, host, port, path, protocol);
   EXPECT_TRUE(r);
   EXPECT_STREQ("::", host.c_str());
   EXPECT_EQ(80, port);
   EXPECT_STREQ("/path", path.c_str());
}



/* ****************************************************************************
*
* parseFullUrl -
*/
TEST(string, parseFullUrl)
{
    std::string url = "http://host:8080/my/path";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(8080, port) << "wrong port";
    EXPECT_EQ("/my/path", path) << "wrong path";

}

/* ****************************************************************************
*
* parseUrlShortPath -
*/
TEST(string, parseUrlShortPath)
{
    std::string url = "http://host:8080/my";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(8080, port) << "wrong port";
    EXPECT_EQ("/my", path) << "wrong path";
}

/* ****************************************************************************
*
* parseUrlWithSlashPath -
*/
TEST(string, parseUrlWithSlashPath)
{
    std::string url = "http://host:8080/";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(8080, port) << "wrong port";
    EXPECT_EQ("/", path) << "wrong path";
}


/* ****************************************************************************
*
* parseUrlWithout -
*/
TEST(string, parseUrlWithoutPath)
{
    std::string url = "http://host:8080";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(8080, port) << "wrong port";
    EXPECT_EQ("/", path) << "wrong path";
}

/* ****************************************************************************
*
* parseUrlWithoutPort -
*/
TEST(string, parseUrlWithoutPort)
{
    std::string url = "http://host/my/path";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(80, port) << "wrong port";
    EXPECT_EQ("/my/path", path) << "wrong path";
}

/* ****************************************************************************
*
* parseUrlWithoutPortAndPath -
*/
TEST(string, parseUrlWithoutPortAndPath)
{
    std::string url = "http://host";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_TRUE(result) << "wrong result (shall be true)";
    EXPECT_EQ("host", host) << "wrong host";
    EXPECT_EQ(80, port) << "wrong port";
    EXPECT_EQ("/", path) << "wrong path";
}

/* ****************************************************************************
*
* parseMalformedUrl1 -
*/
TEST(string, parseMalformedUrl1)
{
    std::string url = "http://";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_FALSE(result) << "wrong result (shall be false)";

}

/* ****************************************************************************
*
* parseMalformedUrl2 -
*/
TEST(string, parseMalformedUrl2)
{
    std::string url = "http://20:host:8080/my/path";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_FALSE(result) << "wrong result (shall be false)";
}

/* ****************************************************************************
*
* parseEmptyUrl -
*/
TEST(string, parseEmptyUrl)
{
    std::string url = "";

    std::string host;
    int         port;
    std::string path;
    std::string protocol;

    bool result = parseUrl(url, host, port, path, protocol);

    EXPECT_FALSE(result) << "wrong result (shall be false)";
}

/* ****************************************************************************
*
* i2s -
*/
TEST(commonString, i2s)
{
  char  ph[32];
  char* p;

  p = i2s(19, ph, sizeof(ph));
  EXPECT_STREQ("19", p);
}

/* ****************************************************************************
*
* parsedUptime -
*/
TEST(string, parsedUptime)
{
  std::string uptime;

  // 3 days, 4 hours, 5 min and 6 seconds
  uptime = parsedUptime(3 * (24 * 3600) + 4 * 3600 + 5 * 60 + 6);
  EXPECT_EQ("3 d, 4 h, 5 m, 6 s", uptime);
}

/* ****************************************************************************
*
* string2coords -
*/
TEST(string, string2coords)
{
  bool   r;
  double latitude  = 0;
  double longitude = 0;

  r = string2coords("2 4", latitude, longitude);
  EXPECT_FALSE(r);
  EXPECT_EQ(0, latitude);
  EXPECT_EQ(0, longitude);

  r = string2coords("2, 4", latitude, longitude);
  EXPECT_TRUE(r);
  EXPECT_EQ(2, latitude);
  EXPECT_EQ(4, longitude);

  r = string2coords("                        2                , 4                 ", latitude, longitude);
  EXPECT_TRUE(r);
  EXPECT_EQ(2, latitude);
  EXPECT_EQ(4, longitude);

  r = string2coords("2.123, 4.12345", latitude, longitude);
  EXPECT_TRUE(r);
  EXPECT_EQ(2.123, latitude);
  EXPECT_EQ(4.12345, longitude);
}

/* ****************************************************************************
*
* atoF -
*/
TEST(string, atoF)
{
  std::string  e;
  double       d;

  d = atoF("", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("empty string", e);

  d = atoF(" ", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);

  d = atoF("34", &e);
  EXPECT_EQ(34, d);
  EXPECT_EQ("", e);

  d = atoF("-34.0", &e);
  EXPECT_EQ(-34.0, d);
  EXPECT_EQ("", e);

  d = atoF("+34.1", &e);
  EXPECT_EQ(34.1, d);
  EXPECT_EQ("", e);

  d = atoF(".34", &e);
  EXPECT_EQ(.34, d);
  EXPECT_EQ("", e);

  d = atoF("--4", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("non-digit after unary minus/plus", e);

  d = atoF("+-4", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("non-digit after unary minus/plus", e);

  d = atoF("-.4", &e);
  EXPECT_EQ(-0.4, d);
  EXPECT_EQ("", e);

  d = atoF(".34.0", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("more than one dot", e);

  d = atoF("34.", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("last character in a double cannot be a dot", e);

  d = atoF("x34", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);

  d = atoF("--224", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("non-digit after unary minus/plus", e);

  d = atoF("-.224", &e);
  EXPECT_EQ(-0.224, d);
  EXPECT_EQ("", e);

  d = atoF("2-24", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);

  d = atoF("224-", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);

  d = atoF("224.-", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);

  d = atoF("224.", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("last character in a double cannot be a dot", e);

  d = atoF("2.2.4", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("more than one dot", e);

  d = atoF("2 24", &e);
  EXPECT_EQ(0.0, d);
  EXPECT_EQ("invalid characters in string to convert", e);
}



/* ****************************************************************************
*
* str2double -
*/
TEST(string, str2double)
{
  bool    b;
  double  d;

  b = str2double("a", &d);
  EXPECT_FALSE(b);

  b = str2double("99e99999999999999999999999999", &d);
  EXPECT_FALSE(b);

  b = str2double("12.0a", &d);
  EXPECT_FALSE(b);

  //
  // Different ways of ZERO
  //
  d = 14;
  b = str2double("0.0000", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(0.0, d);

  d = 14;
  b = str2double("0", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(0.0, d);

  d = 14;
  b = str2double("0e23", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(0.0, d);

  // OK to have spaces after number
  d = 14;
  b = str2double("12 ", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(12, d);

  // But NOT OK to have spaces AND a number after the number
  b = str2double("12 1", &d);
  EXPECT_FALSE(b);

  // Normal float
  d = 14;
  b = str2double("1.23", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(1.23, d);

  // Normal integer
  d = 14;
  b = str2double("23", &d);
  EXPECT_TRUE(b);
  EXPECT_EQ(23, d);
}
