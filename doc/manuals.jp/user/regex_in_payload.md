# ペイロードでの正規表現の使用 (Using regular expressions in payloads)

いくつかのオペレーション (NGSIv1 と NGSIv2 の両方) では、次のように正規表現を使用できます。サポートされている正規表現のセットは、POSIX 拡張です (詳細は [こちら](https://stackoverflow.com/questions/46888312/regular-expressions-in-orion-context-broker)を参照してください)。

NGSIv1 の例 :

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

NGSIv2 の例 :

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

正規表現言語では、バックスラッシュ (`\`) を使用できます。たとえば、`SENSOR\d{2}` は、SENSOR と2桁の数字が続く文字列と一致します。ただし、`\` は JSON の特殊文字です。[JSON 仕様](http://www.json.org)によれば、それは `\\` でエンコードする必要があります。

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

NGSIv2 の例 :

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

シェルを使用してリクエストを送信すると、問題がさらに楽になる可能性があります。たとえば、複数行の curl を使用する場合、シェルレベルで `\` をエスケープする必要があります。例えば :

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

つまり、シェルレベルの `SENSOR\\\\d{2}` 文字列は、Orion の JSON 解析段階の後の正規表現 `SENSOR\d{2}` に順番に対応するペイロードレベル (つまり、ワイヤを介して送信される実際の HTTP リクエスト) で `SENSOR\\d{2}` に変換されます。ただし、ペイロードにファイルを使用する場合、シェルのエスケープは不要です。つまり、次のようになります :

```
curl localhost:1026/v2/op/query -s -S --header 'Content-Type: application/json' --header 'Accept: application/json' -d @payload.json
```


次の payload.json ファイルを使用します :

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

これに関する詳細は [github の次の issue コメント](https://github.com/telefonicaid/fiware-orion/issues/2142#issuecomment-228062834)に記載されています。
