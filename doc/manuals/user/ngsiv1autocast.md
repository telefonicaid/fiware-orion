# NGSIv1 Autocast

By default, due to parsing technology, NGSIv1 attribute creation/update operations always store attributes as strings,
even when then input JSON uses other native type (such as Number, Boolean, etc.). In addition, the DateTime type is not
supported in NGSIv1 so attributes of this type are stored as plain strings. This is a problem when advance functionality
(such as NGSIv2 filtering) want to be used on those attribute values.

The NGSIv1 autocast feature allows to alleviate this situation. It is activated with the `-ngsiv1Autocast`
[CLI option](../admin/cli.md). When enabled, Orion Context Broker will interpret the following attribute types in
NGSIv1 create/update operations:

* "Number" (and its alias "Quantity")
* "DateTime" (and its alias "ISO8601")
* "Boolean"

The processing is as follows:

* At attribute creation time, if the attribute type _in the request_ is one of the above, a casting to the corresponding
  type is done before storing the attribute.
* At attribute modification time, involving such modification _both attribute value and type_, if the attribute
  type _in the request_ is one of the above, a casting to the corresponding type is done before storing the attribute.
* At attribute modification time, involving such modification _only attribute value_ (note this is a possibility in NGSIv1),
  if the attribute type _stored by Orion_ is one of the above, a casting to the corresponding type is done before updating the attribute value.

Some additonal remarks:

* If the conversion to the target type is not possible, such as `{ "value": "Yes", "type": "Number" }`,
  or `{ "value": 29, "type": "Boolean" }`, then the attribute is stored as a string.
* This only applies to NGSiv1 create/update operations. NGSIv2 functionality remaing unaltered, no matter if `-ngsiv1Autocast` is
  used or not.
