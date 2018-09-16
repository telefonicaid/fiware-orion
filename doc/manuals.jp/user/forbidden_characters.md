# 禁止されている文字 (Forbidden characters)

## 禁止されている文字

いくつかの状況でスクリプト注入攻撃を回避するために、次の文字はすべてのリクエストで禁止されています (例えば Context Broker と同じホスト内の同じ場所にある Web サーバとのクロスドメイン) :

-   &lt;
-   &gt;
-   "
-   '
-   =
-   ;
-   (
-   )

これらを使用しようとすると、次のような NGSI 400 Bad Request レスポンスが発生します :

    {
        "orionError": {
            "code": "400",
            "details": "Illegal value for JSON field",
            "reasonPhrase": "Bad Request"
        }
    }

アプリケーションがこれらの文字を使用する必要がある場合は、Orion にリクエストを送信する前に、禁止文字を含まないスキーム (例えば [URL エンコーディング](http://www.degraeve.com/reference/urlencoding.php)) を使用してエンコードする必要があります。

ユーザの観点から特別な注意が必要な別の文字セットがあります。つまり、次のリストにあるもの :

-   \#
-   ?
-   /
-   %
-   &

これらの文字は URL 解釈では特別な意味を持ち、エンティティ、型、属性識別子を URL の一部として使用するコンビニエンス・オペレーションがあることを考慮すると、その使用は避けるべきです。これらの文字の使用は、とにかく標準的なオペレーションだけが含まれている場合は完全に安全です。

### 例外

上記の制限が適用されない例外がいくつかあります。特に、次のフィールドでは :

* URL パラメータ `q` と "FIWARE::StringQuery" スコープの値は、シンプルなクエリ言語に必要な特殊文字を許可します
* URL パラメータ `mq` と "FIWARE::StringQuery::Metadata" スコープの値は、シンプルなクエリ言語に必要な特殊文字を許可します
* URL パラメータの `georel` と `coords` と、それに対応する "FIWARE::Location::NGSIv2" スコープのフィールドは `;` を許可します

## ID フィールドの特定の制限事項

NGSIv2 では、ID フィールド (エンティティ ID/型, 属性名/型, メタデータ名/型など) の構文制限が導入されています。これについては、[NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Field syntax restrictions" のセクションで説明しています。

### カスタムペイロード特別扱い

NGSIv2 はカスタム通知を生成するサブスクリプションのためのテンプレート・メカニズムを提供します ([NGSI v2 仕様](http://telefonicaid.github.io/fiware-orion/api/v2/stable/)の "Custom notifications" を参照)。禁止された文字の制限は、POST/v2/subscription または GET/v2/subscriptions のような、NGSIv2API オペレーションの `httpCustom.payload` フィールドに適用されます。

ただし、通知時に、`httpCustom.payload` の中の URL でエンコードされた文字はすべてデコードされます。

例 :

特定のサブスクリプション中の次のような `notification.httpCustom` オブジェクトを考えてみましょう。

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

上記ペイロード値は、この文字列の URL エンコードされたバージョンであることに注意してください :
`{ "temperature": ${temperature}, "asString": "${temperature}" }`.

さて、NGSIv2 実装がこのサブスクリプションに関連する通知をトリガすることを考えてみましょう。通知データは、id が `DC_S1-D41`、型が `Room` で、値が 23.4 の temperature という名前の属性を含むエンティティ用です。テンプレートを適用した結果の通知は次のようになります :

```
PUT http://foo.com/entity/DC_S1-D41?type=Room
Content-Type: application/json 
Content-Length: 43

{ "temperature": 23.4, "asString": "23.4" }
```
