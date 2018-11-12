# Multi tenancy

The Orion Context Broker implements a simple multitenant/multiservice
model based and logical database separation, to ease service/tenant
based authorization policies provided by other FIWARE components or
third party software, e.g. the ones in the FIWARE security framework
(PEP proxy, IDM and Access Control). This functionality is activated
when the "-multiservice" command line option is used. When
"-multiservice" is used, Orion uses the "Fiware-Service" HTTP header in
the request to identify the service/tenant. If the header is not present
in the HTTP request, the default service/tenant is used.

Multitenant/multiservice ensures that the
entities/attributes/subscriptions of one service/tenant are "invisible"
to other services/tentants. For example, queryContext on tenantA space
will never return entities/attributes from tenantB space. This isolation
is based on database separation, which [details are described in the
Installation and Administration
manual](../admin/database_admin.md#multiservicemultitenant-database-separation).

In addition, note that when "-multiservice" is used Orion includes the
"Fiware-Service" header in the notifyContextRequest and
notifyContextAvailability request messages associated to subscriptions
in the given tenant/service (except for the default service/tenant, in
which case the header is not present), e.g.:

    POST http://127.0.0.1:9977/notify
    Content-Length: 725
    User-Agent: orion/0.13.0
    Host: 127.0.0.1:9977
    Accept: application/json
    Fiware-Service: t_02
    Content-Type: application/json

    {
    ...
    }

Regarding service/tenant name syntax, it must be a string of
alphanumeric characters (and the "\_" symbol). Maximum length is 50
characters,
which should be enough for most use cases. Orion Context Broker
interprets the tenant name in lowercase, thus, although you can use
tenants such as in updateContext "MyService" it is not advisable, as the
notifications related with that tenant will be sent with "myservice"
and, in that sense, it is not coherent the tenant you used in
updateContext compared with the one that Orion sends in
notifyContextRequest.
