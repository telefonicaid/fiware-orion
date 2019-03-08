# 既知の制限事項 (Known limitations)

## リクエストの最大サイズ

Orion Context Broker の現在の最大リクエストサイズは1 MB です。この制限は、ほとんどのユース・ケースで十分であり、同時に、リクエストが大きすぎるためにサービス拒否を回避します。この制限を考慮しないと、次のようなメッセージが表示されます :

```
{
  "errorCode" : {
    "code" : "413",
    "reasonPhrase" : "Request Entity Too Large",
    "details" : "payload size: 1500000, max size supported: 1048576"
  }
}
```

または、巨大なリクエストを送信する場合は、次のようにします :

    <html>
      <head><title>Internal server error</title></head>
      <body>Some programmer needs to study the manual more carefully.</body>
    </html>

("Some programmer needs to study the manual more carefully" というテキストを無視してください。Orion Context Broker の基盤となる HTTP ライブラリーの開発者は面白い人です (funny guys) :)

この 1MB の制限があまりにも粗すぎると判明した場合は、今後のリリースでのご意見をお待ちしておりますので、E メールをお送りください。


## 通知の最大サイズ

通知の最大サイズは 8MB に設定されています。Context Broker によってより大きな通知が送信されず、ログ・ファイルに次のトレースが記録されます :

    HTTP request to send is too large: N bytes

N は大きすぎる通知のバイト数です。

## Content-Length ヘッダが必要です

Orion Context Broker は、すべてのクライアントリクエストで常に Content-Length ヘッダをリクエストします。そうでない場合、クライアントは "411 Length Required" レスポンスを受信します。これは、ベースとなる HTTP ライブラリ (microhttpd) が動作する方法によるものです。詳細については、[microhttpd メーリングリストのこの email スレッド](http://lists.gnu.org/archive/html/libmicrohttpd/2014-01/msg00063.html)を参照してください。

## エンティティ・フィールドの長さ制限

MongoDB レイヤの制限により、エンティティ ID、型およびサービスパスの長さは、次の規則に従わなければなりません。

    length (id) + length (type) + length (servicePath) + 10 < 1024

それ以外の場合は、エンティティ作成時にエラーが発生します。

## エンティティ・フィールドのサイズ制限

基礎となる DB の制限 (詳細は [こちら](https://github.com/telefonicaid/fiware-orion/issues/1289)) により、エンティティ ID、エンティティ型、エンティティサービスパスの合計サイズ (合計) は1014文字を超えることはできません (これは typo ではなく、1024 - 10 に相当します。;)。上限を超えようとすると、"Too long entity id/type/servicePath combination" という 400 Bad Request エラーが発生します。

## サブスクリプション・キャッシュの制限

Orion Context Broker は、サブスクリプション・トリガーを高速化するためにサブスクリプション・キャッシュを使用します。キャッシュよりも mem マップであるため、実際は悪名です。そのキャッシュは RAM 領域を消費し、異常に高い数のサブスクリプションを使用している場合、メモリの枯渇により、Orion がクラッシュする可能性があります。 実際の使用例ではそのような状況になることは非常にまれです (実験室の設定でしか状況を再現できませんでした) が、その場合は、 `-noCache` CLI スイッチでキャッシュ使用を無効にしてください。

参考までに、Orion 1.13.0 が稼働する、4 GB RAM を搭載したマシンのラボ・テストでは、サブスクリプション数が 211.000 を上回ると、Orion がクラッシュしました。

これに関連する改善について、[リポジトリに issue](https://github.com/telefonicaid/fiware-orion/issues/2780) があります。
