# 正規表現の使用 (Using regular expressions)

## 正規表現フォーマット

Orion でサポートされている正規表現のセットは POSIX Extended です (詳細は[こちら](https://stackoverflow.com/questions/46888312/regular-expressions-in-orion-context-broker)をご覧ください)

POSIX はルック・アラウンド (lookarounds) をサポートしていないため、このような式は使用できないことに注意してください:

```
^(?!WeatherObserved).*
```

ただし、POSIX と同等のものを見つけることができます。たとえば、上記の式はこれと同等です
(詳細は[この投稿](https://stackoverflow.com/a/37988661/1485926)を参照):

```
^(([^\nW].{14}|.[^\ne].{13}|.{2}[^\na].{12}|.{3}[^\nt].{11}|.{4}[^\nh].{10}|.{5}[^\ne].{9}|.{6}[^\nr].{8}|.{7}[^\nO].{7}|.{8}[^\nb].{6}|.{9}[^\ns].{5}|.{10}[^\ne].{4}|.{11}[^\nr].{3}|.{12}[^\ntv].{2}|.{13}[^\ne].|.{14}[^\nd]).*|.{0,14})$
```

それらは長くて醜いかもしれませんが、機能的には同等です。

## ペイロードの正規表現

一部の操作では、以下に示すように、ペイロードで正規表現を使用できます。

例 :

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

例 :

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
