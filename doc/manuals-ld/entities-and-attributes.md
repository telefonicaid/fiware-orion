# Introduction to NGSI-LD Entities and Attributes

An entity in NGSI-LD can be anything, it depends completely on the data model of the user.
A bus may be an entity, its driver another.
A refrigerator could be an entity, the four milk bottles it contains may be another four entities.

Entities have:
* an *id* (mandatory) that is a string (that is a URI), with which it can be recognized,
* a *type* (mandatory), that is a string, with which is can be categorized,
* an optional *location*, a "GeoProperty" that describes where the entity is located,
* an optional *observationSpace*, that ...
* an optional *operationSpace*, that ...
* any number of *attributes* that describe the entity


## Attributes
Attributes describe the entity they belong to.
There is no limit as to how many attributes an entity can have.
Attributes can contain sub-attributes, and the sub-attributes follow the same rules that the "toplevel" attributes.
There is also no limit as to how many sub-attributes an attribute can have.
Sub-attributes can also have sub-attributes, of course, and like that to infinity.

NGSI-LD defines four different types of attributes:
* Property
* Relationship
* GeoProperty
* TemporalProperty

The term *attribute* refers to _properties_, in all its forms, and _relationships_, and it will be used throughout this document.


### Property
Possibly the most common type of attributes, *Property*, must contain a **type** telling the broker it is a "Property".
The **value** is *mandatory* inside a property and it can be of any JSON type except null (Number, String, Object, Boolean, Array).

This is how a very simple Property _P1_ is expressed in JSON (P1 inside the entity _urn:entities:E1_):
```json
{
  "id: "urn:entities:E1",
  "type": "T",
  "P1": {
    "type": "Property",
    "value": 23
  }
}
```

This payload data could be used *as is* in a request to create the entity _urn:entities:E1_:
```bash
$ payloadData='{
  "id": "urn:entities:E1",
  "type": "T",
  "P1": {
    "type": "Property",
    "value": 23
  }
}'
$ curl localhost:1026//ngsi-ld/v1/entities -d "$payloadData"
```

Here a somewhat more complex example - an attribute *P1 with a _compound value_ and one sub-attribute *P11* (a Property):
```json
{
  "id: "urn:entities:E1",
  "type": "T",
  "P1": {
    "type": "Property",
    "value": {
      "city": "Torrevieja",
      "street": "Parodi Hermanos",
      "streetNumber": 123
    }
    "P11": {
      "type": "Property",
      "value": 27
    }
  }
}
```

Apart from the mandatory *name*, *type*, *value*, and *optional sub-attributes*, properties can contain three "special sub-properties":
* observedAt (a string containing an ISO8601 timestamp)
* unitCode  (a string identfying the unit code of the value of the property)
* datasetId (a string that is a URI - allowing identification of a set or group of target relationship objects)
 

### Relationship
The second type of attributes, _relationships_, contain information on how an entity (or an attribute) relate to other entities.

An example entity with a *Relationship*: 
```json
{
  "id: "urn:entities:anura:E1",
  "type": "T",
  "Cousin": {
    "type": "Relationship",
    "object": "http://a.b.c/species/RanaPipiens.jsonld"
  }
}
```
The **type** with value "Relationship" defines the attribute "Cousin" to be a *Relationship*.
A relationship MUST have an "object", and the object must be a string that is a URI.
Note that properties have a **value** while relationships have an **object**.

Just like properties, relationships can have sub-attributes, and just like properties, relationships have a few "special sub-properties":
* observedAt (a string containing an ISO8601 timestamp)
* datasetId (a string that is a URI - allowing identification of a set or group of target relationship objects)

Note that "unitCode" is missing.
It's not needed, as a relationship has no "value". It's value is its "object", and that is always a string that is a URI.


### GeoProperty
Geospatial Properties are represented using [GeoJSON Geometries](https://tools.ietf.org/html/rfc7946) in NGSI-LD.
The "type" of this kind of properties must be "GeoProperty".
A geo-property, apart from t he "type" set to "GeoProperty", have a mandatory value that must be a JSON object with "type" and "coordinates":
```
  "geoProp1": {
    "type": "GeoProperty",
    "value": {
      "type": "Point",
      "coordinates": []
    }
  }
```
The supported values of value::type are:
* Point
* MultiPoint
* LineString
* MultiLineString
* Polygon
* MultiPolygon
 
In other words, all the types defined by GeoJSON except *GeometryCollection*.
The second field of the value (_coordinates_) is always an array, but what's inside the array depends on the first field (_type_).
For example, a "Point" is an array of two floats - longitude and latitude - **in that order**.
I said two floats, but there can an optional third float in the array - the Z-value (altitude or elevation).

The values of *coordinates* for each of these GeoJSON geometry types are:
* Point: an array of a single position. E.g. "coordinates": [ 1.01, 4.003 ]
* MultiPoint: an array of points. E.g.: "coordinates": [ [ 1.01, 4.003 ], [ 2.01, 4.003 ] ]
* LineString: an array of two or more points (just like MultiPoint ...)
* MultiLineString: an array of LineString arrays. E.g.: "coordinates": [ [ [ 1.01, 4.003 ], [ 2.01, 4.003 ] ], [ [ 3.01, 4.003 ], [ 4.01, 4.003 ] ] ]
* Polygon: in short, an array of arrays of points, with the last point coinciding with the first point and in counterclockwise order.
           E.g.: "coordinates": [[ [0,0], [4,0], [4,-2], [0,-2], [0,0] ]]
* 

Polygons can have holes as well, that's the reason for the "extra array layer" (to create a Donut! :)) and in such case,
the second array of points would be the hole. The hole is expressed clockwise, opposite to the way the "outer ring" is expressed.

If you are curious as to what the difference is between a MultiPoint and a LineString, or need more info on Polygons,
or anything else concerning GeoJSON, please refer to the [RFC](https://tools.ietf.org/html/rfc7946)

### TemporalProperty
_TemporalProperty_ is for internal use and it is not allowed for users to create arbitrary attributes of that type.
NGSI-LD defines the following Properties of type _TemporalProperty_:
* observedAt
* createdAt
* modifiedAt

The first temporal property, _observedAt_ can be set by requests, while the second and third are system attributes and cannot be set to specific values by requests.
As the names imply, _createdAt_ is the data and time the entity was created, while _modifiedAt_ is the date and time that the entity was last modified.
