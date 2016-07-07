# Using regular expressions in payloads

In some operations (both in NGSIv1 and NGSIv2) regular expressions can be used, as shown below.

NGSIv1 example:

```
POST /v1/queryContext

{
  "entities": [
    {
      "type": "Sensor",
      "isPattern": "true",
      "id": ".*"
    }
  ]
}
```

NGSIv2 example:

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

The regular expressions language allows the usage of backlash (`\`). For example, `SENSOR\d{2}` matches
strings containing SENSOR followed by two numbers. However, note that `\` is an special character in JSON,
which, according to [the JSON specifciation](http://www.json.org), needs to be encoded as `\\`.

```
POST /v1/queryContext

{
  "entities": [
    {
      "type": "Sensor",
      "isPattern": "true",
      "id": "SENSOR\\d{2}"
    }
  ]
}
```

NGSIv2 example:

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
that is send on the wire) which, in sequence, corresponds to the `SENSOR\d{2}` regular expression after JSON parsing stage at Orion. However, note that
if you use a file for your payload the shell escaping hasn't to be used, that is:

```
curl localhost:1026/v2/op/query -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @payload.json
```


will use the following payload.json file:

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

More detail on this can be found in [the following issue coment at github](https://github.com/telefonicaid/fiware-orion/issues/2142#issuecomment-228062834).
