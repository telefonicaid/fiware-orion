# Known limitations

## Request maximum size

The current maximum request size in Orion Context Broker is 1 MB. This
limit should suffice the most of the use cases and, at the same time,
avoids denial of service due to too large requests. If you don't take
this limitation into account, you will get messages such the following
ones:

```
{
  "errorCode" : {
    "code" : "413",
    "reasonPhrase" : "Request Entity Too Large",
    "details" : "payload size: 1500000, max size supported: 1048576"
  }
}
```

Or, if you are sending a huge request, this one:

    <html>
      <head><title>Internal server error</title></head>
      <body>Some programmer needs to study the manual more carefully.</body>
    </html>

(Please ignore the "Some programmer needs to study the manual more
carefully" text. Developers of the HTTP library in which Orion Context
Broker is based seem to be funny guys :) :)

If you find this 1MB limit too coarse, send us an email so we can
consider your feedback in future releases.

## Notification maximum size

Notification maximum size is set to 8MB. Larger notifications will not be sent by context broker and you
will get the following trace in the log file:

    HTTP request to send is too large: N bytes

where N is the number of bytes of the too large notification.

## Content-Length header is required

Orion Context Broker expects always a Content-Length header in all
client requests, otherwise the client will receive a "411 Length
Required" response. This is due to the way the underlying HTTP library
(microhttpd) works, see details in [this email thread in the microhttpd
mailing
list](http://lists.gnu.org/archive/html/libmicrohttpd/2014-01/msg00063.html).

## Entity fields length limitation

Due to limitations at MongoDB layer, the length of entity ID, type and servicePath has to follow the following rule.

    length(id) + length(type) + length(servicePath) + 10 < 1024

Otherwise, we will get an error at entity creation time.

## Entity fields size limitation

Due to underlying DB limitations (see details [here](https://github.com/telefonicaid/fiware-orion/issues/1289)),
the combined size (sum) of entity id, entity type and entity service path cannot exceed 1014 characters (this is not a typo, 
it corresponds to 1024 minus 10, details in the aforementioned link ;). If you attempt to overpass the limit you will get
a 400 BadRequest "Too long entity id/type/servicePath combination" error.
