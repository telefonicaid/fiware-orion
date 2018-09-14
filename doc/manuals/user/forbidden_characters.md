# Forbidden characters

## General restrictions

In order to avoid script injections attack in some circustances (e.g.
cross domain to co-located web servers in the same hot that CB) the
following characters are forbidden in any request:

-   &lt;
-   &gt;
-   "
-   '
-   =
-   ;
-   (
-   )

Any attempt of using them will result in a NGSI 400 Bad Request response
like this:

    {
        "orionError": {
            "code": "400",
            "details": "Illegal value for JSON field",
            "reasonPhrase": "Bad Request"
        }
    }

If your aplication needs to use these characteres, you should encode it
using a scheme not including forbidden characters before sending the
request to Orion (e.g. [URL
encoding](http://www.degraeve.com/reference/urlencoding.php)).

There is another set of characters that requires special care from the 
user perspective. Namely, the ones in the following list:

-   #
-   ?
-   /
-   %
-   &

These characters have special meaning in the URL interpretation, and, 
considering there are convenience operations that use entity, type and
attribute identifiers as part of the URL, their use should be avoided. 
The use of these characters is perfectly safe when only standard operations
are involved, anyway. 

### Exceptions

There are some exception cases in which the above restrictions do not apply. In particular, in the following fields:

* URL parameter `q` and the value of "FIWARE::StringQuery" scope allow the special characters needed by the Simple Query Language
* URL parameter `mq` and the value of "FIWARE::StringQuery::Metadata" scope allow the special characters needed by the Simple Query Language
* URL parameter `georel` and `coords` and the corresponding fields in the "FIWARE::Location::NGSIv2" scope allow `;`

## Specific restrictions for ID fields

NGSIv2 introduces syntax restrictions for ID fields (such as entity id/type, attribute name/type
or metadata name/type) which are described in the "Field syntax restrictions" section in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable).

### Custom payload special treatment

NGSIv2 provides a templating mechanism for subscriptions which allows to generate custom notifications
(see "Custom notifications" section in
the [NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/stable)). Forbidden
characters restrictions apply to the `httpCustom.payload` field in NGSIv2 API operations, such as
POST /v2/subscription or GET /v2/subscriptions.

However, at notification time, any URL encoded characters in `httpCustom.payload` are decoded.

Example:

Let's consider the following `notification.httpCustom` object in a given subscription.

```
"httpCustom": {
  "url": "http://foo.com/entity/${id}",
  "headers": {
    "Content-Type": "application/json"
  },
  "method": "PUT",
  "qs": {
    "type": "${type}"
  },
  "payload": "{ %22temperature%22: ${temperature}, %22asString%22: %22${temperature}%22 }"
}
```

Note that the above payload value is the URL encoded version of this string:
`{ "temperature": ${temperature}, "asString": "${temperature}" }`.

Now, let's consider that NGSIv2 implementation triggers a notification associated to this subscription.
Notification data is for entity with id `DC_S1-D41` and type `Room`, including an attribute named
`temperature` with value 23.4. The resulting notification after applying the template would be:

```
PUT http://foo.com/entity/DC_S1-D41?type=Room
Content-Type: application/json 
Content-Length: 43

{ "temperature": 23.4, "asString": "23.4" }
```
