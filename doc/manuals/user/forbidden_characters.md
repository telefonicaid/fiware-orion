### Forbidden characters

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
