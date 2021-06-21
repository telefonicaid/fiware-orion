# Using regular expressions in payloads

In some operations regular expressions can be used, as shown below. The
set of supported regular expressions is POSIX Extended (see details [here](https://stackoverflow.com/questions/46888312/regular-expressions-in-orion-context-broker)).

Example:

```
POST /v2/op/query

{
  "entities": [
    {
      "idPattern": ".*",
      "type": "Sensor"
    }
  ]
}
```

The regular expression language allows the usage of backslash (`\`). For example, `SENSOR\d{2}` matches
strings containing SENSOR followed by two digits. However, note that `\` is a special character in JSON,
which, according to [the JSON specification](http://www.json.org), needs to be encoded as `\\`.

Example:

```
POST /v2/op/query

{
  "entities": [
    {
      "idPattern": "SENSOR\\d{2}",
      "type": "Sensor"
    }
  ]
}
```

The issue could be even funnier if you use the shell to send the request. For example, if you use multi-line curl you need
to escape `\` at shell level. Eg:

```
curl localhost:1026/v2/op/query -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
  "entities": [
    {
      "idPattern": "SENSOR\\\\d{2}",
      "type": "Sensor"
    }
  ]
}
EOF
```

In other words, the `SENSOR\\\\d{2}` string at shell level is transformed to `SENSOR\\d{2}` at payload level (i.e. the actual HTTP request
that is sent over the wire) which, in sequence, corresponds to the `SENSOR\d{2}` regular expression after JSON parsing stage at Orion. However, note that
if you use a file for your payload, the shell escaping isn't needed, i.e.:

```
curl localhost:1026/v2/op/query -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @payload.json
```


would use the following payload.json file:

```
{
  "entities": [
    {
      "idPattern": "SENSOR\\d{2}",
      "type": "Sensor"
    }
  ]
}
```

More details on this can be found in [the following issue comment at github](https://github.com/telefonicaid/fiware-orion/issues/2142#issuecomment-228062834).
