# Service paths

## Entity service paths

Orion Context Broker supports hierarchical scopes, so entities can be
assigned to a scope at creation time with
[updateContext](walkthrough_apiv1.md#update-context-elements) (or [related
convenience operation](walkthrough_apiv1.md#convenience-update-context)). Then,
[queryContext](walkthrough_apiv1.md#query-context-operation) and [subscribeContext](walkthrough_apiv1.md#context-subscriptions) (and related
convenience operations) can be also scoped to locate entities in the corresponding scopes.

For example, consider an Orion-based application using the following
scopes (shown in the figure):

-   Madrid, as first level scope
-   Gardens and Districts, as second-level scope (children of Madrid)
-   ParqueNorte, ParqueOeste and ParqueSur (children of Gardens) and
    Fuencarral and Latina (children of Districts)
-   Parterre1 and Parterre2 (children of ParqueNorte)

![](ServicePathExample.png "ServicePathExample.png")

The scope to use is specified using the "Fiware-ServicePath" HTTP in
update/query request. For example, to create the entity "Tree1" of type
"Tree" in "Parterre1" the following Fiware-ServicePath will be used:

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte/Parterre1

In order to search for "Tree1" in that scope, the same
Fiware-ServicePath will be used.

Scopes are hierarchical and hierarchical search can be done. In order to
do that the '\#' special keyword is used. Thus, a queryContext with
pattern entity id ".\*" of type "Tree" in `/Madrid/Gardens/ParqueNorte/#`
will return all the trees in ParqueNorte, Parterre1 and Parterre2.

Finally, you can query for disjoint scopes, using a comma-separated list
in the Fiware-ServicePath header. For example, to get all trees in both
ParqueNorte and ParqueOeste (but not ParqueSur) the following
Fiware-ServicePath would be used in queryContext request:

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte, /Madrid/Gardens/ParqueOeste

Some additional remarks:

-   Limitations:
    -   Scope must start with "/" (only 'absolute' scopes are allowed)
    -   10 maximum scope levels in a path
    -   50 maximum characters in each level (1 char is minimum),
        only alphanum and underscore allowed
    -   10 maximum disjoint scope paths in a comma-separated list in
        query Fiware-ServicePath header (no more than 1 scope path in
        update Fiware-ServicePath header)
    -   Trailing slashes are discarded

-   Fiware-ServicePath is an optional header. It is assumed that all the
    entities created without Fiware-ServicePath (or that don't include
    service path information in the database) belongs to a root scope
    "/" implicitely. All the queries without using Fiware-ServicePath
    (including subscriptions) are on "/\#" implicitly. This behavior
    ensures backward compatibility to pre-0.14.0 versions.

-   It is possible to have an entity with the same ID and type in
    different Scopes. E.g. we can create entity ID "Tree1" of type
    "Tree" in /Madrid/Gardens/ParqueNorte/Parterre1 and another entity
    with ID "Tree1" of type "Tree" in Madrid/Gardens/ParqueOeste without
    getting any error. However, queryContext can be weird in this
    scenario (e.g. a queryContext in Fiware-ServicePath /Madrid/Gardens
    will returns two entities with the same ID and type in the
    queryContextResponse, making hard to distinguish to which scope
    belongs each one)

-   Entities belongs to one (and only one) scope.

-   Fiware-ServicePath
    header is included in NGSI10 notifyContext requests sent by Orion.

-   The scopes entities can be combined orthogonally with the
    [multi-service/multi-tenant
    functionality](multitenancy.md#multi-service-tenancy). In that case,
    each "scope tree" lives in a different service/tenant and they can
    use even the same names with complete database-based isolation. See
    figure below.

![](ServicePathWithMultiservice.png "ServicePathWithMultiservice.png")

-   Current version doesn’t allow to change the scope to which an entity
    belongs through the API (a workaround is to modify the
    \_id.servicePath field in the entities collection directly).

## Service paths in subscriptions and registrations

While entities belong to services *and* servicepaths, subscriptions and registrations
belong *only* to the service. The servicepath in subscriptions and registrations
doesn't denote sense of belonging, but is the expression of the query associated
to the subscription or registration.

Taking this into consideration, the following rules apply:

* Fiware-ServicePath header is ignored in `GET /v2/subscriptions/{id}` and
  `GET /v2/registrations/{id}` operations, as the id fully qualifies the subscription or registration
  to retrieve.
* Fiware-ServicePath header is taken into account in `GET /v2/subscriptions` and `GET /v2/registrations`
  in order to narrow down the results to subscriptions/registrations that use *exactly*
  that service path as query.

