A quick recipe on how to use apiary client to render HTML from .apib in your local system:

* Install apiary client (see https://github.com/apiaryio/apiary-client). Hint: if you have problems installing it,
  review if you have installed Ruby and libssl devel packages.
* Use it in the following way:

```
apiary preview --path=doc/apiary/v2/fiware-ngsiv2-reference.apib --output=/tmp/render.html
```

See also the scripts in the scripts/apib_renders/ directory.
