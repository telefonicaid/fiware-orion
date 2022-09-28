# Orion-LD Context Broker Roadmap

This product is a FIWARE Incubated Generic Enabler.
If you want to learn about the overall Roadmap of FIWARE, please have a look at the section "Roadmap" in the FIWARE Catalogue.

## Short/Medium Term Roadmap
* Implement the new registrations, as of NGSI-LD API v1.6.1
* Registration-Cache for better performance
* Implement forwarding using the new registrations, tentative order of first requests:
  - GET /entities
  - PATCH /entities/{entityId}
* Memory management for containers - for a better experience with Orion-LD inside containers

For Medium/Long term planning, please refer to the [Issue #280](https://github.com/FIWARE/context.Orion-LD/issues/280)
