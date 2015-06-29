### Forbidden characters

<span style="color:red">**Since 0.17.0**</span>

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