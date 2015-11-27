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

* ONTIMEINTERVAL subscriptions are deprecated since Orion 0.25.0. OTIMEINTERVAL subscriptions have
  several problems (introduce state in CB, thus making much harder horizontal scaling configuration,
  and make difficult to introduce pagination/filtering). Actually, thery aren't actually needed,
  as any use case based on ONTIMEINTERVAL notification can be converted to an equivalent use case
  in which the receptor runs queryContext at the same frequency (and taking advantage of the
  functionalitiese of queryContext, such as pagination or filtering).
* XML is deprecated since Orion 0.23.0.
* Deprecated command line arguments in Orion 0.21.0 (removed in 0.25.0):
	* **-ngsi9**. The broker runs only NGSI9 (NGSI10 is not used).
	* **-fwdHost <host>**. Forwarding host for NGIS9 registerContext when
    the broker runs in "ConfMan mode".
	* **-fwdPort <port>**. Forwarding port for NGIS9 registerContext when
    the broker runs in "ConfMan mode".
* Configuration Manager role (deprecated in 0.21.0, removed in 0.25.0)
* Associations (deprecated in 0.21.0, removed in 0.25.0).
