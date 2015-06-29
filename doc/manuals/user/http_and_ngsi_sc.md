# HTTP and NGSI response codes

Two independent response codes are being considered in the API
responses: one "internal" at NGSI level (i.e. encoded in the REST HTTP
response payload) and other "external" at HTTP level (the HTTP response
code itself). Note that this manual focuses on the NGSI aspects of the
API, thus we always assume in this documentation (unless otherwise
noted) that HTTP code is "200 OK".

In order to illustrate the existence of both codes and their
independence, let's consider a queryContext operation on a non-existing
entity (e.g. "foo"). Note the -v flag in the curl command, in order to
print the HTTP response codes and headers:

    # curl localhost:1026/v1/contextEntities/foo -s -S --header 'Content-Type: application/xml' -v | xmllint --format -
    * About to connect() to localhost port 1026 (#0)
    *   Trying ::1... connected
    * Connected to localhost (::1) port 1026 (#0)
    > GET /v1/contextEntities/foo HTTP/1.1
    > User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7 NSS/3.13.1.0 zlib/1.2.3 libidn/1.18 libssh2/1.2.2
    > Host: localhost:1026
    > Accept: */*
    > Content-Type: application/xml
    >
    < HTTP/1.1 200 OK
    < Content-Length: 316
    < Content-Type: application/xml
    < Date: Mon, 31 Mar 2014 10:13:45 GMT
    <
    { [data not shown]
    * Connection #0 to host localhost left intact
    * Closing connection #0
    <?xml version="1.0"?>
    <contextElementResponse>
      <contextElement>
        <entityId type="" isPattern="false">
          <id>foo</id>
        </entityId>
      </contextElement>
      <statusCode>
        <code>404</code>
        <reasonPhrase>No context element found</reasonPhrase>
        <details>Entity id: 'foo'</details>
      </statusCode>
    </contextElementResponse>

Note that in this case the NGSI response code is "404 No context element
found" while the HTTP is "200 OK". Thus, in other words, the
communication at HTTP level was ok, although an error condition (the
entity doesn't exist in Orion Context Broker database) happened at the
NGSI level.

The following example shows a case of an HTTP level problem, due to a
client attempting to get the response in a MIME type not supported by
Orion (in this case "text/plain"). In this case, an HTTP response code
"406 Not Acceptable" is generated.

    # curl localhost:1026/v1/contextEntities/foo -s -S --header 'Accept: text/plain' -v | xmllint --format -
    * About to connect() to localhost port 1026 (#0)
    *   Trying ::1... connected
    * Connected to localhost (::1) port 1026 (#0)
    > GET /v1/contextEntities/foo HTTP/1.1
    > User-Agent: curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.19.7 NSS/3.13.1.0 zlib/1.2.3 libidn/1.18 libssh2/1.2.2
    > Host: localhost:1026
    > Accept: text/plain
    >
    < HTTP/1.1 406 Not Acceptable
    < Content-Length: 196
    < Content-Type: application/xml
    < Date: Mon, 31 Mar 2014 10:16:16 GMT
    <
    { [data not shown]
    * Connection #0 to host localhost left intact
    * Closing connection #0
    <?xml version="1.0"?>
    <orionError>
      <code>406</code>
      <reasonPhrase>Not Acceptable</reasonPhrase>
      <details>acceptable types: 'application/xml' but Accept header in request was: 'text/plain'</details>
    </orionError>