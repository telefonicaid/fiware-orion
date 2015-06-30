# Deprecated functionality

Deprecated features are features that Orion stills support but that are
not mantained or evolved any longer. In particular:

-   Bugs or issues related with deprecated features and not affecting
    any other feature are not addressed (they are closed in github.com
    as soon as they are spotted).
-   Documentation on deprecated features is removed from the repository documentation. Documentation is still 	available in the documentation set associated to older versions (either in the respository release branches or   	the pre-0.23.0 documentation in the FIWARE wiki). 
-   Deprecated functionality is eventually removed from Orion. Thus you
    are strongly encouraged to change your implementations using Orion
    in order not rely on deprecated functionality.

A list of deprecated features and the version in which they were deprecated follows:

* XML is deprecated since Orion 0.23.0.
* Deprecated command line arguments in Orion 0.21.0:
	* **-ngsi9**. The broker runs only NGSI9 (NGSI10 is not used).
	* **-fwdHost <host>**. Forwarding host for NGIS9 registerContext when
    the broker runs in "ConfMan mode".
	* **-fwdPort <port>**. Forwarding port for NGIS9 registerContext when
    the broker runs in "ConfMan mode".
* Configuration Manager role (deprecated in 0.21.0)
* Associations (deprecated in 0.21.0).