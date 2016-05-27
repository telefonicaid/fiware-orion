# HTTP and NGSI response codes

The HTTP and NGSI response code distinction described in this section
only applies to NGSIv1. NGSIv2 adopts a simpler approach which only
uses the HTTP response code.

Two independent response codes are being considered in the NGSIv1 API
responses: one "internal" at NGSI level (i.e. encoded in the REST HTTP
response payload) and other "external" at HTTP level (the HTTP response
code itself). Note that this manual focuses on the NGSI aspects of the
API, thus we always assume in this documentation (unless otherwise
noted) that HTTP code is "200 OK".

In order to illustrate the existence of both codes and their
independence, let's consider a queryContext operation on a non-existing
entity (e.g. "foo"). Note the -v flag in the curl command, in order to
print the HTTP response codes and headers:

```
# curl localhost:1026/v1/contextEntities/foo -s -S --header 'Accept: application/json' -v | python -mjson.tool
* About to connect() to localhost port 1026 (#0)
*   Trying ::1...
* connected
* Connected to localhost (::1) port 1026 (#0)
> GET /v1/contextEntities/foo HTTP/1.1
> User-Agent: curl/7.26.0
> Host: localhost:1026
> Accept: application/json
>
* additional stuff not fine transfer.c:1037: 0 0
* HTTP 1.1 or later with persistent connection, pipelining supported
< HTTP/1.1 200 OK
< Content-Length: 220
< Content-Type: application/json
< Date: Thu, 10 Sep 2015 18:40:37 GMT
<
{ [data not shown]
* Connection #0 to host localhost left intact
* Closing connection #0
{
    "contextElement": {
        "id": "foo",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "404",
        "details": "Entity id: /foo/",
        "reasonPhrase": "No context element found"
    }
}
```
Note that in this case the NGSI response code is "404 No context element
found" while the HTTP is "200 OK". Thus, in other words, the
communication at HTTP level was ok, although an error condition (the
entity doesn't exist in Orion Context Broker database) happened at the
NGSI level.

The following example shows a case of an HTTP level problem, due to a
client attempting to use a HTTP verb that is not allowed. In this case,
an HTTP response code "405 Method Not Allowed" is generated.

```
curl -X PATCH localhost:1026/v1/contextEntities/foo -s -S --header 'Accept: application/json' -v
* About to connect() to localhost port 1026 (#0)
*   Trying ::1...
* connected
* Connected to localhost (::1) port 1026 (#0)
> PATCH /v1/contextEntities/foo HTTP/1.1
> User-Agent: curl/7.26.0
> Host: localhost:1026
> Accept: application/json
>
* additional stuff not fine transfer.c:1037: 0 0
* HTTP 1.1 or later with persistent connection, pipelining supported
< HTTP/1.1 405 Method Not Allowed
< Content-Length: 0
< Allow: POST, GET, PUT, DELETE
< Date: Thu, 10 Sep 2015 18:42:38 GMT
<
* Connection #0 to host localhost left intact
* Closing connection #0
```
