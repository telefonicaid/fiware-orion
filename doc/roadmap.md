# Orion Context Broker Roadmap

This product is a FIWARE Generic Enabler. If
you would like to learn about the overall Roadmap of FIWARE, please check
section "Roadmap" on the FIWARE Catalogue.

## Introduction

This section elaborates on proposed new features or tasks which are expected to
be added to the product in the foreseeable future. There should be no assumption
of a commitment to deliver these features on specific dates or in the order
given. The development team will be doing their best to follow the proposed
dates and priorities, but please bear in mind that plans to work on a given
feature or task may be revised. All information is provided as a general
guidelines only, and this section may be revised to provide newer information at
any time.

Disclaimer:

* This section has been last updated in March 2020. Please take into account its 
  content could be obsolete.
* Note we develop this software in Agile way, so development plan is continuously 
  under review. Thus, this roadmap has to be understood as rough plan of features 
  to be done along time which is fully valid only at the time of writing it. This
  roadmap has not be understood as a commitment on features and/or dates.
* Some of the roadmap items may be implemented by external community developers, 
  out of the scope of GE owners. Thus, the moment in which these features will be
  finalized cannot be assured.

## Short term

The following list of features are planned to be addressed in the short term,
and incorporated into the coming release(s) of the product:

- Unrestricted text attributes ([#3550](https://github.com/telefonicaid/fiware-orion/issues/3550))
- Notification flow control ([#3568](https://github.com/telefonicaid/fiware-orion/issues/3568))
- Improvements and fixed based on pentesting reports

## Medium term

The following list of features are planned to be addressed in the medium term,
typically within the subsequent release(s) generated in the next **9 months**
after next planned release:

- Allow multiple types in entity to support UNE 178503 requirements ([#3638](https://github.com/telefonicaid/fiware-orion/issues/3638))
- Attribute update operators (inc, push, etc.)
- MQTT notifications (community)
- Custom notifications: simplifying sending JSON requests ([#2560](https://github.com/telefonicaid/fiware-orion/issues/2560))
- Advanced query language
- Notification endpoint alias ([#3655](https://github.com/telefonicaid/fiware-orion/issues/3655))

## Long term

The following list of features are proposals regarding the longer-term evolution
of the product even though development of these features has not yet been
scheduled for a release in the near future. Please feel free to contact us if
you wish to get involved in the implementation or influence the roadmap

- Migration from legacy to new MongoDB driver
- Dynamic / high order attribute values (e.g. an attribute being a sum of two other attributes)
- Performance improvements in the notifications system ([#3461](https://github.com/telefonicaid/fiware-orion/issues/3461))
- Improve performance in update/notifications (connection-oriented, lightweight ingestion, etc.)
