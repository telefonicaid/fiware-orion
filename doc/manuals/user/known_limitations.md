# Known limitations

## Request maximum size

Orion Context Broker has a default maximum request size of 1 MB. If you don't take
this limitation into account, you will get messages such the following ones:

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

If this 1 MB limit doesn't work for you, you can change it using the [CLI option](../admin/cli.md) `-inReqPayloadMaxSize`.
However, before doing this please have a look at [performance considerations](../admin/perf_tuning.md#payload-and-message-size-and-performance).

## Notification maximum size

Notification maximum size (including HTTP request line, headers and payload) is set to 8MB by default.
Larger notifications will not be sent by the context broker and you will get the following trace in the log file:

    HTTP request to send is too large: N bytes

where N is the number of bytes of the too large notification.

You can change this limit by starting the broker with the [CLI option](../admin/cli.md) `-outReqMsgMaxSize`.
However, before doing this please have a look at [performance considerations](../admin/perf_tuning.md#payload-and-message-size-and-performance).

## Content-Length header is required

Orion Context Broker expects always a Content-Length header in all
client requests, otherwise the client will receive a "411 Length
Required" response. This is due to the way the underlying HTTP library
(microhttpd) works, see details in [this email thread in the microhttpd
mailing
list](http://lists.gnu.org/archive/html/libmicrohttpd/2014-01/msg00063.html).

## Subscription cache limitation

Orion Context Broker uses a subscription cache (actually, a bad name, given it is more a mem map than a cache ;) to speed up
subscription triggering. That cache consumes RAM space and if you are using an abnormally high number of subscriptions, Orion
may crash due to memory outage. It would be extremely rare to have that situation in a real usage case (we have been able to
reproduce the situation only in a laboratory setup) but, if happens, then disable cache usage with the `-noCache` CLI switch.

As a reference, in our lab tests in a machine with Orion 1.13.0 running with 4 GB RAM, Orion crashed when the number 
of subscriptions got higher than 211.000 subscriptions.

There is [an issue in the repository](https://github.com/telefonicaid/fiware-orion/issues/2780) about improvements related with this.
