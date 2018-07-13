# Deprecated functionality

Deprecated features are features that Orion stills support but that are
not mantained or evolved any longer. In particular:

-   Bugs or issues related with deprecated features and not affecting
    any other feature are not addressed (they are closed in github.com
    as soon as they are spotted).
-   Documentation on deprecated features is removed from the repository documentation.
    Documentation is still available in the documentation set associated to older versions
    (either in the respository release branches or the pre-0.23.0 documentation in the FIWARE wiki).
-   Deprecated functionality is eventually removed from Orion. Thus you
    are strongly encouraged to change your implementations using Orion
    in order not rely on deprecated functionality.

A list of deprecated features and the version in which they were deprecated follows:

* `attributes` field in `POST /v2/op/query` is in Orion 1.15.0. It is a combination of `attrs` (to select
  which attributes to include in the response to the query) and unary attribute filter in `q` within
  `expression` (to return only entities which have these attributes). Use them instead.
* Usage of that is `APPEND`, `APPEND_STRICT`, `UPDATE`, `DELETE` and `REPLACE` in `POST /v2/op/update` is
  deprecated in Orion 1.14.0. Use `append`, `appendStrict`, `update`, `delete` and `replace` counterparts.
* Metadata ID is deprecated in Orion 1.13.0. On the one hand, this functionality is not compatible with the
  NGSIv2 JSON representation format (attribute names are used as keys in a JSON object, so names cannot be
  duplicated). On the other hand, IDs can easily be implemented using prefixes/suffixes in attribute names,
  e.g. `temperature:ground` and `temperature:ceiling`. As a consecuence of this deprecation, the following
  operations are also deprecated:
        * `GET /v1/contextEntities/Room1/attributes/{attrName}/{id}`
        * `PUT /v1/contextEntities/Room1/attributes/{attrName}/{id}`
        * `DELETE /v1/contextEntities/Room1/attributes/{attrName}/{id}`
* The usage of `options` URL parameter in order to include `dateCreated` and/or `dateModified`
  attributes in NGSIv2 is deprecated in Orion 1.5.0. Please use `attrs` URI parameter instead.
* `/ngsi10` and `/ngsi9` as URL path prefixes are deprecated in Orion 1.2.0. Please,
  use `/v1` and `/v1/registry` instead.
* `location` metadata to specify entity location is deprecated in Orion 1.1.0. The new way
  of specifying entity location is to use `geo:point` type for the attribute (see details in
  [the corresponding section of the user manual](user/geolocation.md).
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

## Using old Orion versions

Although you are encouraged to use always the newest Orion version, take into account the following
information in the case you want to use old versions:

* Old RPMs (since Orion 0.1.1) are available at the [FIWARE yum repository](http://repositories.lab.fiware.org/repo/rpm/6/x86_64).
* Code correponding to old releases (since Orion 0.8.1, the first one available as open source) is
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
| `/ngsi10` and `/ngsi9` URL prefixes                                        | Not yet defined                           | Not yet defined                 |
| `location` metadata to specify entity location                             | Not yet defined                           | Not yet defined                 |
| `id` metadata (and associated NGSIv1 operations)                           | Not yet defined                           | Not yet defined                 |
| XML API                                                                    | 0.28.0                                    | February 29th, 2016             |
| ONTIMEINTERVAL subscription                                                | 0.28.0                                    | February 29th, 2016             |
| CLI `--silent`                                                             | 0.28.0                                    | February 29th, 2016             |
| Configuration Manager role (including `-ngsi9`, `-fwdHost` and `-fwdPort`) | 0.24.0                                    | September 14th, 2015            |
| Associations                                                               | 0.24.0                                    | September 14th, 2015            |
