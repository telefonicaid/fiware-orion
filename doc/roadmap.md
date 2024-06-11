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

* This section has been last updated in August 2023. Please take into account its 
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

- Dynamic / high order attribute values (e.g. an attribute being a sum of two other attributes) based on subscriptions to do internal modifications to CB entities
([#3815](https://github.com/telefonicaid/fiware-orion/issues/3815), [#4513](https://github.com/telefonicaid/fiware-orion/issues/4513))
- MQTT notification retrial ([#4439](https://github.com/telefonicaid/fiware-orion/issues/4439))
- Allow multiple types in entity to support UNE 178503 requirements ([#3638](https://github.com/telefonicaid/fiware-orion/issues/3638))
- Pattern/filter batch updates ([#2389](https://github.com/telefonicaid/fiware-orion/issues/2389))
- Notification endpoint alias ([#3655](https://github.com/telefonicaid/fiware-orion/issues/3655))
- Aggregation operations API ([#3816](https://github.com/telefonicaid/fiware-orion/issues/3816))
- Rework commands (and deprecate registration API) ([#4397](https://github.com/telefonicaid/fiware-orion/issues/4397))


## Medium term

The following list of features are planned to be addressed in the medium term,
typically within the subsequent release(s) generated in the next **9 months**
after next planned release:

- Permissions attribute on entities ([#4514](https://github.com/telefonicaid/fiware-orion/issues/4514))
- Advanced query language ([#4395](https://github.com/telefonicaid/fiware-orion/issues/4395))
- Signed entities ([#4398](https://github.com/telefonicaid/fiware-orion/issues/4398))
- Service provisioning API (pools, etc.) ([#4442](https://github.com/telefonicaid/fiware-orion/issues/4442))
- Advanced subscription management
   - Subscription debug mode (precise statistics consolidation, keep recent history of notifications sent, etc.) ([#4399](https://github.com/telefonicaid/fiware-orion/issues/4399))
   - Custom ID subscription ([#4400](https://github.com/telefonicaid/fiware-orion/issues/4400))
   - Query Subscrition by fields (endpoint, attributes...) ([#4392](https://github.com/telefonicaid/fiware-orion/issues/4392))
   - Flag to disable metadata in subscriptions ([#4401](https://github.com/telefonicaid/fiware-orion/issues/4401))
- Remove registration API ([#4402](https://github.com/telefonicaid/fiware-orion/pull/4402))

## Long term

The following list of features are proposals regarding the longer-term evolution
of the product even though development of these features has not yet been
scheduled for a release in the near future. Please feel free to contact us if
you wish to get involved in the implementation or influence the roadmap

- Performance improvements in the notifications system ([#3461](https://github.com/telefonicaid/fiware-orion/issues/3461))
- Improve performance in update (connection-oriented, lightweight ingestion, etc.)

## Features already completed

The following list contains all features that were in the roadmap and have already been implemented.

- Expression language support (JEXL) ([#4004](https://github.com/telefonicaid/fiware-orion/issues/4004)) ([4.0.0](https://github.com/telefonicaid/fiware-orion/releases/tag/4.0.0))
- MQTT Retain flag ([#4388](https://github.com/telefonicaid/fiware-orion/issues/4388)) ([3.11.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.11.0))
- Custom notifications: simplifying sending JSON requests ([#2560](https://github.com/telefonicaid/fiware-orion/issues/2560)) ([3.8.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.8.0))
- New subscripition modes (create only, update only, delete only and combinations) ([#1494](https://github.com/telefonicaid/fiware-orion/issues/1494)) ([3.7.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.7.0))
- Per sub/reg HTTP timeout ([#3842](https://github.com/telefonicaid/fiware-orion/issues/3842)) 
([3.3.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.3.0))
- Attribute update operators (inc, push, etc.) ([#3814](https://github.com/telefonicaid/fiware-orion/issues/3814))
([3.3.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.3.0))
- MQTT notifications (community) ([#3001](https://github.com/telefonicaid/fiware-orion/issues/3001)) 
([3.2.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.2.0))
- Unrestricted text attributes ([#3550](https://github.com/telefonicaid/fiware-orion/issues/3550)) 
([3.2.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.2.0))
- Service provisioning CLI (pools, etc.) ([#3843](https://github.com/telefonicaid/fiware-orion/issues/3843))
([3.1.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.1.0))
- Migration from legacy to new MongoDB driver ([#3132](https://github.com/telefonicaid/fiware-orion/issues/3132))
([3.0.0](https://github.com/telefonicaid/fiware-orion/releases/tag/3.0.0))
- Improvements and fixed based on pentesting reports (
[#3603](https://github.com/telefonicaid/fiware-orion/issues/3603),
[#3604](https://github.com/telefonicaid/fiware-orion/issues/3604),
[#3605](https://github.com/telefonicaid/fiware-orion/issues/3605),
[#3606](https://github.com/telefonicaid/fiware-orion/issues/3606),
[#3607](https://github.com/telefonicaid/fiware-orion/issues/3607),
[#3608](https://github.com/telefonicaid/fiware-orion/issues/3608),
[#3609](https://github.com/telefonicaid/fiware-orion/issues/3609))
([2.4.0](https://github.com/telefonicaid/fiware-orion/releases/tag/2.4.0))
- Notification flow control ([#3568](https://github.com/telefonicaid/fiware-orion/issues/3568))
([2.4.0](https://github.com/telefonicaid/fiware-orion/releases/tag/2.4.0))




