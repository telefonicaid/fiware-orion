# Pagination

NGSIv2 implements a pagination mechanism in order to help clients to retrieve
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
total of 322 results. The client could do the following (only the URL is
included, for the sake of completeness).

    GET <orion_host>:1026/v2/entities?limit=100&options=count
    ...
    (The first 100 elements are returned, along with the `Fiware-Total-Count: 322`
    header, which makes the client aware of how many entities there are in total and,
    therefore, the number of subsequent queries to be done)

    GET <orion_host>:1026/v2/entities?offset=100&limit=100
    ...
    (Entities from 101 to 200)

    GET <orion_host>:1026/v2/entities?offset=200&limit=100
    ...
    (Entities from 201 to 300)

    GET <orion_host>:1026/v2/entities?offset=300&limit=100
    ...
    (Entities from 301 to 222)

Note that if the request uses an offset beyond the total number of results, an
empty list is returned, as shown below:

```
GET <orion_host>:1026/v2/entities?offset=1000&limit=100
...
[]
```
