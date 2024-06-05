# Deprecated functionality

Deprecated features are features that Orion stills support but that are
not maintained or evolved any longer. In particular:

-   Bugs or issues related with deprecated features and not affecting
    any other feature are not addressed (they are closed in github.com
    as soon as they are spotted).
-   Documentation on deprecated features is removed from the repository documentation.
    Documentation is still available in the documentation set associated to older versions
    (either in the repository release branches or the pre-0.23.0 documentation in the FIWARE wiki).
-   Deprecated functionality is eventually removed from Orion. Thus you
    are strongly encouraged to change your implementations using Orion
    in order not rely on deprecated functionality.

A list of deprecated features and the version in which they were deprecated follows:

* CLI parameters (and associated env vars): `-dbhost`, `-rplSet`, `-dbTimeout`, `-dbuser`,
  `-dbAuthMech`, `-dbAuthDb`, `-dbSSL` and `-dbDisableRetryWrites` in Orion 3.12.0. Use `dbURI` instead,
  checking [this section](#mapping-to-mongouri-from-old-cli-parameters) if you need to know hot to build the MongoDB URI (removed in Orion 4.0.0).
* `geo:point`, `geo:line`, `geo:box` and `geo:polygon` attribute types in Orion 3.10.0. Use `geo:json` instead.
* `GET /v2` operation in Orion 3.8.0. This operation is pretty useless and not actually used.
* Initial notification in subscriptions (along with `skipInitialNotification` option) in Orion 3.1.0.
  (removed in Orion 3.2.0). The results covered by initial notification can be very large and we cannot apply pagination here
  (as it is done in the synchronous retrieval of entities using `GET /v2/entities`). In fact, only
  the first 20 entities are returned, which makes this funcionality very limited. As alternative,
  if you need to know the status of your system at subscription time, then use `GET /v2/entities`
  with proper pagination.
* Rush support (along with the related CLI parameter: `-rush`) in Orion 2.1.0 (removed in Orion 2.3.0).
* NGSIv1 API (along with related CLI parameters: `-strictNgsiv1Ids` and `-ngsiv1Autocast`) in Orion 2.0.0.
  Use NGSIv2 API instead.
    * Context availability subscriptions (AKA NGSI9 subscriptions), as part of NGSIv1, in Orion 2.0.0
      (removed in Orion 2.6.0)
    * All the other NGSIv1 operations removed in Orion 3.10.0, except the following ones:
        * `PUT /v1/contextEntities/{id}`
        * `DELETE /v1/contextEntities/{id}`
        * `GET /v1/contextEntities/{id}/attributes/{name}`
        * `POST /v1/updateContext`
        * `POST /NGSI10/updateContext`
        * `POST /v1/queryContext`
        * `POST /NGSI10/queryContext`
    * NGSIv1 format in subscription notifications (`notification.atttrsFormat` set to `legacy`) removed in Orion 4.0.0
    * Finally, the last remaining NGSIv1 operations where removed in Orion 4.0.0
* `attributes` field in `POST /v2/op/query` is in Orion 1.15.0. It is a combination of `attrs` (to select
  which attributes to include in the response to the query) and unary attribute filter in `q` within
  `expression` (to return only entities which have these attributes). Use them instead.
* Usage of that is `APPEND`, `APPEND_STRICT`, `UPDATE`, `DELETE` and `REPLACE` in `POST /v2/op/update` is
  deprecated in Orion 1.14.0. Use `append`, `appendStrict`, `update`, `delete` and `replace` counterparts.
* Metadata ID is deprecated in Orion 1.13.0 (removed in Orion 2.2.0). On the one hand, this functionality is not compatible with the
  NGSIv2 JSON representation format (attribute names are used as keys in a JSON object, so names cannot be
  duplicated). On the other hand, IDs can easily be implemented using prefixes/suffixes in attribute names,
  e.g. `temperature:ground` and `temperature:ceiling`. As a consecuence of this deprecation, the following
  operations are also deprecated:
	* `GET /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
	* `GET /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
	* `POST /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
	* `PUT /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
	* `PUT /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
	* `DELETE /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
	* `DELETE /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
* The usage of `options` URL parameter in order to include `dateCreated` and/or `dateModified`
  attributes in NGSIv2 is deprecated in Orion 1.5.0. Please use `attrs` URI parameter instead.
* `/ngsi10` and `/ngsi9` as URL path prefixes are deprecated in Orion 1.2.0. Please,
  use `/v1` and `/v1/registry` instead.
        * `/ngsi9` URL paths removed in Orion 3.8.0
* `location` metadata to specify entity location is deprecated in Orion 1.1.0 (removed in Orion 3.11.0). The new way
  of specifying entity location is to use `geo:json` type for the attribute (see details in
  [the corresponding section of the Orion API specification](orion-api.md#geospatial-properties-of-entities).
* Deprecated command line argument in Orion 0.26.1 (removed in Orion 1.0.0).
  * **--silent**. Suppress all log output except errors (Please use *-logLevel ERROR* instead)
* ONTIMEINTERVAL subscriptions are deprecated since Orion 0.26.0 (removed in Orion 1.0.0).
  ONTIMEINTERVAL subscriptions have several problems (introduce state in CB, thus making horizontal
  scaling configuration much harder, and makes it difficult to introduce pagination/filtering).
  Actually, they aren't really needed, as any use case based on ONTIMEINTERVAL notification can
  be converted to an equivalent use case in which the receptor runs queryContext at the same
  frequency (and taking advantage of the features of queryContext, such as pagination or filtering).
* XML is deprecated since Orion 0.23.0 (removed in Orion 1.0.0).
* Deprecated command line arguments in Orion 0.21.0 (removed in 0.25.0):
	* **-ngsi9**. The broker runs only NGSI9 (NGSI10 is not used).
	* **-fwdHost <host>**. Forwarding host for NGIS9 registerContext when the broker runs in "ConfMan mode".
	* **-fwdPort <port>**. Forwarding port for NGIS9 registerContext when the broker runs in "ConfMan mode".
* Configuration Manager role (deprecated in 0.21.0, removed in 0.25.0)
* Associations (deprecated in 0.21.0, removed in 0.25.0).

### Mapping to MongoURI from old CLI parameters

Considering we have the following CLI parameters:

* `-dbhost HOST`
* `-rplSet RPLSET`
* `-dbTimeout TIMEOUT`
* `-dbuser USER`
* `-dbpass PASS`
* `-dbAuthMech AUTHMECH`
* `-dbAuthDb AUTHDB`
* `-dbSSL`
* `-dbDisableRetryWrites`

The resulting MongoURI (i.e. the value for `-dbURI`) should be:

> mongodb://[USER:PASS@]HOST/[AUTHDB][?replicaSet=RPLSET[&authMechanism=AUTHMECH][&tls=true&tlsAllowInvalidCertificates=true][&retryWrites=false][&connectTimeoutMS=TIMEOUT]

Notes:

* The `&tls=true&tlsAllowInvalidCertificates=true` token is added if `-dbSSL` is used
* The `&retryWrites=false` token is added if `-dbDisableRetryWrites` is used
* Other `[...]` mean optional tokens, depending on if the corresponding parameter is used or not.

## Log deprecation warnings

Some (not all) usages of deprecated features can be logged using the `-logDeprecate` [CLI flag](admin/cli.md)
(or `deprecate` parameter in the [log admin REST API](admin/management_api.md#log-configs-and-trace-levels))
in the WARN log level).

Have a look to [this section of the documentation](admin/logs.md#log-deprecated-usages) for more detail about this.

## Using old Orion versions

Although you are encouraged to use always the newest Orion version, take into account the following
information in the case you want to use old versions:

* Code corresponding to old releases (since Orion 0.8.1, the first one available as open source) is
  available at the [Orion github repository](http://github.com/telefonicaid/fiware-orion). Each release number
  (e.g. 0.23.0) has associated the following:
	* A tag, e.g. `0.23.0`. It points to the base version.
	* A release branch, `release/0.23.0`. The HEAD of this branch usually matches the aforementioned tag. However, if some
    hotfixes were developed on the base version, this branch contains such hotfixes.
* Documentation corresponding to old versions can be found:
	* For 0.23.0 and before: documentation is available at FIWARE public wiki ([user manual](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_User_and_Programmers_Guide)
    and [admin manual](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Publish/Subscribe_Broker_-_Orion_Context_Broker_-_Installation_and_Administration_Guide)).
	* For 0.24.0 or newer: documentation is available at [readthedocs.io](https://fiware-orion.readthedocs.io).
    Use the panel in the left bottom corner to navigate to the right version.
* Docker images corresponding to Orion 0.24.0 and newer can be found at [Dockerhub](https://hub.docker.com/r/fiware/orion/tags/).

The following table provides information about the last Orion version supporting currently removed features:

| **Removed feature**                                                        | **Last Orion version supporting feature** | **That version release date**   |
|----------------------------------------------------------------------------|-------------------------------------------|---------------------------------|
| `attributes` field in `POST /v2/entities` operation                        | Not yet defined                           | Not yet defined                 |
| `APPEND`, `UPDATE`, etc. action types in `POST /v2/op/update`              | Not yet defined                           | Not yet defined                 |
| `dateCreated` and `dateModified` in `options` URI parameter                | Not yet defined                           | Not yet defined                 |
| `GET /v2` operation                                                        | Not yet defined                           | Not yet defined                 |
| `geo:point`, `geo:line`, `geo:box` and `geo:polygon` attribute types       | Not yet defined                           | Not yet defined                 |
| CLI `-dbhost`, `-rplSet`, `-dbTimeout`, `-dbuser`, `-dbAuthMech`, `-dbAuthDb`, `-dbSSL` and `-dbDisableRetryWrites` (and associated env vars)                        | 3.12.0                           | February 29th, 2024                 |
| `location` metadata to specify entity location                             | 3.10.1                                    | June 12th, 2023                 |
| NGSIv1 API (along with CLI: `-strictNgsiv1Ids` and `-ngsiv1Autocast`)      | 3.9.0 (*)                                 | June 2nd, 2023                  |
| `/ngsi10` and `/ngsi9` URL prefixes                                        | 3.7.0 (*)                                 | May 26th, 2022                  |
| Initial notification upon subscription creation or update                  | 3.1.0                                     | June 9th, 2021                  |
| NGSIv1 Context availability subscriptions (NGSI9 suscriptions)             | 2.5.2                                     | December 17th, 2020             |
| Rush (along with CLI: `-rush`)                                             | 2.2.0                                     | February 21st, 2019             |
| `id` metadata (and associated NGSIv1 operations)                           | 2.1.0                                     | December 19th, 2018             |
| XML API                                                                    | 0.28.0                                    | February 29th, 2016             |
| ONTIMEINTERVAL subscription                                                | 0.28.0                                    | February 29th, 2016             |
| CLI `--silent`                                                             | 0.28.0                                    | February 29th, 2016             |
| Configuration Manager role (including `-ngsi9`, `-fwdHost` and `-fwdPort`) | 0.24.0                                    | September 14th, 2015            |
| Associations                                                               | 0.24.0                                    | September 14th, 2015            |

(*) The removal was not fully done in a single version, but this is last one in which the functionality was still complete
