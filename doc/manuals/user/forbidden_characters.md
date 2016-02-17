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

Any attemp of using them will result in a NGSI 400 Bad Request response
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

There are some exception cases in which the above restrictions are not checked. In particuler, in the following fields:

* URL parameter `q` and the value of "FIWARE::StringQuery" scope allow the special characters needed by the Simple Query Language
* URL parameter `georel` and `coords` and the corresponding fields in the "FIWARE::Location::NGSIv2" scope allow `;`

## Specific restrictions for ID fields

NGSIv2 introduces syntax restrictions for ID fields (such as entity id/type, attribute name/type
or metadata name/type) which are described in the "Field syntax restrictions" section in the
[NGSIv2 specification](http://telefonicaid.github.io/fiware-orion/api/v2/). You can also
enable them for NGSIv1, as described in [this section of the documentation](v1_v2_coexistence.md#checking-id-fields).
