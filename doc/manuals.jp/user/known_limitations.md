# 既知の制限事項 (Known limitations)

## リクエストの最大サイズ

Orion Context Broker のデフォルトの最大リクエストサイズは1MBです。
この制限を考慮に入れないと、次のようなメッセージが表示されます :

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

この1MBの制限がうまくいかない場合は、[CLI オプション](../admin/cli.md)
`-inReqPayloadMaxSize` を使って変更できます。 ただし、これを行う前に、
[パフォーマンスの考慮事項](../admin/perf_tuning.md#payload-and-message-size-and-performance)
を確認してください。


## 通知の最大サイズ

通知の最大サイズ (HTTP リクエスト・ライン, ヘッダ, ペイロードを含む) はデフォルトで8MBに設定されています。
それより大きい通知は Context Broker によって送信されず、ログ・:ファイルに次のトレースが表示されます :

    HTTP request to send is too large: N bytes

N は大きすぎる通知のバイト数です。

この制限を変更するには、[CLI オプション](../admin/cli.md) `-outReqMsgMaxSize`
を使って Context Broker を起動します。ただし、これを行う前に、
[パフォーマンスの考慮事項](../admin/perf_tuning.md#payload-and-message-size-and-performance)
を確認してください。

## Content-Length ヘッダが必要です

Orion Context Broker は、すべてのクライアントリクエストで常に Content-Length ヘッダをリクエストします。そうでない場合、クライアントは "411 Length Required" レスポンスを受信します。これは、ベースとなる HTTP ライブラリ (microhttpd) が動作する方法によるものです。詳細については、[microhttpd メーリングリストのこの email スレッド](http://lists.gnu.org/archive/html/libmicrohttpd/2014-01/msg00063.html)を参照してください。

## サブスクリプション・キャッシュの制限

Orion Context Broker は、サブスクリプション・トリガーを高速化するためにサブスクリプション・キャッシュを使用します。キャッシュよりも mem マップであるため、実際は悪名です。そのキャッシュは RAM 領域を消費し、異常に高い数のサブスクリプションを使用している場合、メモリの枯渇により、Orion がクラッシュする可能性があります。 実際の使用例ではそのような状況になることは非常にまれです (実験室の設定でしか状況を再現できませんでした) が、その場合は、 `-noCache` CLI スイッチでキャッシュ使用を無効にしてください。

参考までに、Orion 1.13.0 が稼働する、4 GB RAM を搭載したマシンのラボ・テストでは、サブスクリプション数が 211.000 を上回ると、Orion がクラッシュしました。

これに関連する改善について、[リポジトリに issue](https://github.com/telefonicaid/fiware-orion/issues/2780) があります。
