# 属性の構造化された値 (Structured values for attributes)

22.5 や "yellow" などの単純な値以外にも、複雑な構造を属性値として使用できます。特に、属性は、作成/更新時にベクトルまたはキー値マップ (通常は "object" と呼ばれます) に設定できます。これらの値は、クエリおよび通知時に取得されます。

ベクトルまたはキーマップの値は、それぞれ JSON ベクターおよび JSON オブジェクトに直接対応します。したがって、次の create entity リクエストは、属性 "A" の値をベクトルに設定し、属性 "B" の値をキーマップ・オブジェクトに設定します エンティティ作成オペレーションでは表示しますが、属性更新に対しても同じことが適用されます。

NGSIv2 の場合 :

```
curl localhost:1026/v2/entities -s -S --header 'Content-Type: application/json' \
   -d @- <<EOF
{
  "id": "E1",
  "type": "T",
  "A": {
    "type": "T",
    "value": [
      "22",
      {
          "x": [
              "x1",
              "x2"
          ],
          "y": "3"
      },
      [
          "z1",
          "z2"
      ]
    ]
  },
  "B": {
    "type": "T",
    "value": {
      "x": {
          "x1": "a",
          "x2": "b"
      },
      "y": [
          "y1",
          "y2"
      ]
    }
  }
}
EOF
```

NGSIv1 の場合 :

``` 
(curl localhost:1026/v1/updateContext -s -S --header 'Content-Type: application/json' \
    --header 'Accept: application/json' -d @- | python -mjson.tool) <<EOF
{
    "contextElements": [
        {
            "type": "T1",
            "isPattern": "false",
            "id": "E1",
            "attributes": [
                {
                    "name": "A",
                    "type": "T",
                    "value": [
                        "22",
                        {
                            "x": [
                                "x1",
                                "x2"
                            ],
                            "y": "3"
                        },
                        [
                            "z1",
                            "z2"
                        ]
                    ]
                },
                {
                    "name": "B",
                    "type": "T",
                    "value": {
                        "x": {
                            "x1": "a",
                            "x2": "b"
                        },
                        "y": [
                            "y1",
                            "y2"
                        ]
                    }
                }
            ]
        }
    ],
    "updateAction": "UPDATE"
}
EOF
``` 

属性の値は、Orion Context Broker によってフォーマットに依存しない表現で内部的に格納されます。

JSON 表現を揃えるために NGSIv1 の場合、すべてのベクトルとキーマップを走査した後の属性の構造化された値の最後の "leaf" 要素は、常に文字列と見なされることに注意してください。したがって、たとえば、`{"x": 3.2 }` などの更新でフィールド値として JSON 番号を使用することはできますが、クエリや通知で文字列を取得します。例 : `{"x":"3.2"}`

NGSIv2 では、すべてのベクトルとキーマップを走査した後の属性の構造化された値の最後の "leaf" 要素は、正しいネイティブ JSON 型(数値、ブール値など)を使用します。したがって、たとえば、`{"x": 3.2 }` のような更新でフィールド値として JSON 番号を使用すると、クエリと通知で数字を取得します。例 : `{"x": 3.2 }`
