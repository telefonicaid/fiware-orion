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
`-key` and `-cert` options, to especify the files containing the private key
and certificates for the server, respectively). Check the [command line
options section in the administration manual for
details](../admin/cli.md#command-line-options).
Note that current Orion version cannot run in both HTTP and HTTPS at the
same time, i.e. using `-https` disables HTTP.

## HTTPS notifications

Apart from using HTTPS in the API server exported by Orion, you can also use HTTPS in
notifications. In order to do so you have to use the "https" protocol schema in URL in your
subscriptions, e.g.

```
  ...
  "url": "https://mymachime.example.com:1028/notify"
  ...
```

If you use Rush relayer (see [how to run Orion using Rush](../admin/rush.md)) then Orion to Rush request
is sent in HTTP, then Rush will encrypt it using HTTPS towards the final receiver. If you don't use Rush
relayer, then Orion will send HTTPS notification natively. In that case, note that by default Orion will
reject connections to non-trusted endpoints (i.e. the ones which which certificate cannot be authenticated
with known CA certificates). If you want to avoid this behaviour you need to use the `-insecureNotif`
[CLI parameter](../admin/cli.md) but note that doing so is an insecure configuration (e.g. you could suffer
[man-in-the-middle attacks](https://en.wikipedia.org/wiki/Man-in-the-middle_attack)).
