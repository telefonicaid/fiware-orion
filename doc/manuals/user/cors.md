# Cross Origin Resource Sharing (CORS)

[CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS) can be enabled
for Orion using the `-corsOrigin` switch during startup. `-corsOrigin` takes either
a string value of a single origin that would be allowed to make CORS requests or
 `__ALL` for allowing any origin to make CORS requests to the Context Broker.

The only configurable aspect of Orion's CORS mode besides the allowed origin is
the maximum time a preflight request can be cached by the client, which is
handled by the `-corsMaxAge` switch. It takes the maximum amount of cache time in
seconds and defaults to `86400` (24 hours) if not used.

More information about Orion CLI switches cat be found in 
[the administration manual](../admin/cli.md).

For example:

- Below command starts Orion with CORS enabled for any origin with a 10 minute
maximum preflight cache time

        contextBroker -corsOrigin __ALL -corsMaxAge 600

- Start Orion with CORS enabled for only a specific origin with the default
maximum preflight cache time

        contextBroker -corsOrigin specificdomain.com

CORS is available for all `/v2` resources.

## Access-Control-Allow-Origin

If the CORS mode is enabled, Origin header is present in the request and its
value matches Orion's allowed origin, this header will always be added to the
response.

Please note that if the above condition is not true and
Access-Control-Allow-Origin header is not added to the response, all the CORS
processes are stopped and no other CORS headers are added to the response.

If `-corsOrigin` is set to a specific value, `specificdomain.com` in this case:

    Access-Control-Allow-Origin: specifidomain.com

If `-corsOrigin` is set to `__ALL`:

    Access-Control-Allow-Origin: *


## Access-Control-Allow-Methods

This header should be present in Orion's response for every `OPTIONS` request
made to `/v2` resources. Each resource has its own set of allowed methods and
the header value is set by the `options*Only` service routines in
[lib/serviceRoutinesV2](https://github.com/telefonicaid/fiware-orion/tree/master/src/lib/serviceRoutinesV2)

## Access-Control-Allow-Headers

This header should be present in Orion's response for every `OPTIONS` request
made to `/v2` resources. Orion allows a specific set of headers in CORS requests
and these are defined in [lib/rest/HttpHeaders.h](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/rest/HttpHeaders.h)

Orion's response to a valid `OPTIONS` request would include the header and value
below:

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token

## Access-Control-Max-Age

This header should be present in Orion's response for every `OPTIONS` request
made to `/v2` resources. The user is free to set a value for the maximum time 
(in seconds) a client is allowed to cache a preflight request made to Orion.

If `-corsMaxAge` is set to a specific value, `600` in this case, Orion's response
to a valid `OPTIONS` request would include the header and value below:

    Access-Control-Max-Age: 600

If `-corsMaxAge` is not set on startup, it will default to '86400' (24 hours) and
Orion's response to a valid `OPTIONS` request would include the header and value
below:

    Access-Control-Max-Age: 86400

## Access-Control-Expose-Headers

This header should be present in Orion's response for every request made with a
valid Origin value. Orion allows a specific set of response headers to be
accessed by the user agent (i.e. browser) in CORS requests and these are defined
in [lib/rest/HttpHeaders.h](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/rest/HttpHeaders.h)

Orion's response to a valid CORS request would include the header and value
below:

    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location
