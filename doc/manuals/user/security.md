# Security considerations

## Authentication and Authorization 

Orion doesn't provide "native" authentication nor any authorization mechanisms to enforce access control. However, authentication/authorization can be achieved [the access control framework provided by FIWARE GEs](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE.ArchitectureDescription.Security.Access_Control_Generic_Enabler).

More specifically, Orion is integrated in this framework using the [FIWARE PEP Proxy
GE](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE.OpenSpecification.Security.PEP_Proxy_Generic_Enabler).
At the present moment, there are two GE implementantions (GEis) that can
work with Orion Context Broker:

-   [Wilma](http://catalogue.fiware.org/enablers/pep-proxy-wilma) (the
    GE reference implementation)
-   [Steelskin](https://github.com/telefonicaid/fiware-pep-steelskin)

In the above links you will find the documentation about how to use both
GEis. 

## HTTPS API

Orion Context Broker supports HTTPS, using the `-https` options (which in addition needs the
-key and -cert options, to especify the files containing the private key
and certificates for the server, respectively). Check the [command line
options section in the administration manual for
details](../admin/cli.md#command-line-options).
Note that current Orion version cannot run in both HTTP and HTTPS at the
same time, i.e. using -https disables HTTP.

## HTTPS notifications

Apart from using HTTPS in the API server exported by Orion, you can also use HTTPS in
notifications. In order to do so:

-   You have to use the "https" protocol schema in URL in you reference
    element in subscribeContext or subscribeContextAvailability
    subscriptions, e.g.

```
  ...
  "reference": "https://mymachime.example.com:1028/notify"
  ...
```

-   You have to use Rush as relayer (as the HTTPS encoding is
    implemented in Rush). See [how to run Orion using
    Rush](../admin/rush.md)
    for additional information on this topic.
