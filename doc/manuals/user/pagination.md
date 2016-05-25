# Pagination (using NGSIv2)

NGSIv2 implements a pagination mechanism (which by the way is very similar
to the one implemented in NGSIv1) in order to help clients to retrieve
large sets of resources. This mechanism works for all listing operations
in the API (e.g. `GET /v2/entities`, `GET /v2/subscriptions`,
`POST /v2/op/query`, etc.).

The mechanism is based on three URI parameters:

-   **limit**, in order to specify the maximum number of elements (default
    is 20, maximun allowed is 1000).

-   **offset**, in order to skip a given number of elements at the
    beginning (default is 0)

-   **count** (as `option`), if activated then a `Fiware-Total-Count`
    header is added to the response, with a count of total elements.

By default, results are returned ordered by increasing creation
time. In the case of entities query, this can be changed with the
[`orderBy` URL parameter](#ordering-results).

Let's illustrate with an example: a given client cannot process more
than 100 results in a single response and the query includes a
total of 322 results. The client could do the following (only URL is
included, for the sake of completeness).

    GET <orion_host>:1026/v2/entities?limit=100&options=count
    ...
    (The first 100 elements are returned, along with the `Fiware-Total-Count: 322`
    header, which allows the client to know how many entities are in sum and,
    therefore, the number of subsequence queries to do)

    GET <orion_host>:1026/v2/entities?offset=100&limit=100
    ...
    (Entities from 101 to 200)

    GET <orion_host>:1026/v2/entities?offset=200&limit=100
    ...
    (Entities from 201 to 300)

    GET <orion_host>:1026/v2/entities?offset=300&limit=100
    ...
    (Entities from 301 to 222)

Note that if the request uses an "out of bound" offset you will get an
empty list, as shown below:

```
GET <orion_host>:1026/v2/entities?offset=1000&limit=100
...
[]
```

# Pagination (using NGSIv1)

In order to help clients organize query and discovery requests with a
large number of responses (for example, think of how costly could be
returning a query matching 1,000,000 results in a single HTTP response
to a queryContext request),
[queryContext](walkthrough_apiv1.md#query-context-operation) (and [related
convenience operations](walkthrough_apiv1.md#Convenience_Query_Context)) and
[discoverContextAvailability](walkthrough_apiv1.md#discover-context-availability-operation)
(and [related convenience operations](walkthrough_apiv1.md#convenience-discover-context-availability))
allow pagination. The mechanism is based on three URI parameters:

-   **limit**, in order to specify the maximum number of entities or
    context registrations (for queryContext and
    discoverContextAvailability respectively) (default is 20, maximun
    allowed is 1000).

-   **offset**, in order to skip a given number of elements at the
    beginning (default is 0)

-   **details** (allowed values are “on” and “off”, default is “off”),
    in order to get a global errorCode for the response including a
    count of total elements (in the case of using “on”). Note that using
    details set to “on” slightly breaks NGSI standard, which states that
    global errorCode must be used only in the case of general error with
    the request. However, we think it is very useful for a client to
    know in advance how many results in total the query has (and if you
    want to keep strict with NGSI, you can simply ignore the details
    parameter :)

By default, results are returned ordered by increasing entity/registration creation
time. This is to ensure that if a new entity/registration is created
while the client is going through all the results the new results are
added at the end (thus avoiding duplication results). In the case of
entities query, this can be changed with the [`orderBy` URL parameter](#ordering-results).

Let’s illustrate with an example: a given client cannot process more
than 100 results in a single response and the queryContext includes a
total of 322 results. The client could do the following (only URL is
included, for the sake of completeness).

    POST <orion_host>:1026/v1/queryContext?limit=100&details=on
    ...
    (The first 100 elements are returned, along with the following errorCode in the response, 
    which allows the client to know how many entities are in sum and, therefore, the number of 
    subsequence queries to do)

      "errorCode": {
        "code": "200",
        "reasonPhrase": "OK",
        "details": "Count: 322"
      }

    POST <orion_host>:1026/v1/queryContext?offset=100&limit=100
    ...
    (Entities from 101 to 200)

    POST <orion_host>:1026/v1/queryContext?offset=200&limit=100
    ...
    (Entities from 201 to 300)

    POST <orion_host>:1026/v1/queryContext?offset=300&limit=100
    ...
    (Entities from 301 to 222)

Note that if the request uses an “out of bound” offset you will get a
404 NGSI error, as shown below:

```
POST <orion_host>:1026/v1/queryContext?offset=1000&limit=100
...
{
    "errorCode": {
        "code": "404",
        "reasonPhrase": "No context element found",
        "details": "Number of matching entities: 5. Offset is 1000"
    }
}
```

## Ordering results

In the case of entities query, the `orderBy` URL parameter can be use to
specify an order different from creation time. In particular, the value of
`orderBy` is a comma-separated list of attributes and the results are ordered
by the first attribute. On ties, the results are ordered by the second attribute and so on.
A "!" before the attribute name means that the order is reversed. For example (NGSIv2):

    GET <orion_host>:1026/v2/entities?orderBy=temperature,!humidity

or (NGSIv1):

    POST <orion_host>:1026/v1/queryContext?orderBy=temperature,!humidity

orders first by temperature in ascending order, then by humidity in decreasing order
in the case of temperature ties.

Note that the special keywords `dateCreated` and `dateModified` can be used as
elements in the `orderBy` comma-separated list (including the `!` syntax) to mean
entity creation time and entity modification time respectively.
