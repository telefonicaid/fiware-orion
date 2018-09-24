# <a name="top"></a>メトリックAPI (Metrics API)

* [イントロダクション](#introduction)
* [オペレーション](#operations)
    * [メトリックを取得](#get-metrics)
    * [メトリックをリセット](#reset-metrics)
    * [取得とリセット](#get-and-reset)
* [メトリック](#metrics)

<a name="introduction"></a>
## イントロダクション

Orion は関連するオペレーション・メトリックを取得するために使用できる REST ベースの API を実装しています。この API は、低レベルでデバッグを目的とした [統計情報 API](statistics.md)を補完するものです。

この API は、`-disableMetrics` [CLI パラメータ](cli.md)を使用してオーバーヘッド (システムコールとセマフォが関係するため、メトリックの収集には多少のコストがかかります) を避けるためにオフにすることができます。

[トップ](#top)

<a name="operations"></a>
## オペレーション

<a name="get-metrics"></a>
#### メトリックを取得

```
GET /admin/metrics
```

レスポンス・ペイロードは、構造化された方法で情報を格納するマルチレベルの JSON ツリーです。これは、[サービス](../user/multitenancy.md)と[サブサービス](../user/service_path.md) ("サービスパス" とも呼ばれます) に基づいています。ツリーの任意のポイントで、キーの値は、そのキーに関連付けられた実際の情報がないことを意味する `{}` とすることができます。

最初のレベルには、**services** と **sum** という 2つのキーがあります。シーケンスでは、**services** 値は、そのキーが service 名であり、その値が対応する情報を持つオブジェクトです。**sum** 値は、すべてのサービスの集約情報のための情報を持つオブジェクトです。

```
{
  "services": {
    "service1": <service 1 info>,
    "service2": <service 2 info>,
    ...
    "serviceN": <service N info>
  }
  "sum": <aggregated info for all services>
}
```

サービス情報オブジェクトに関しては、**subservs** と **sum** の 2つのキーを使用します。シーケンスでは、**subservs** 値は、そのキーが Subservice 名であるオブジェクトで、その値は、対応する Subservice に関する情報を持つオブジェクトです。**sum** 値は、特定のサービス内のすべての Subservice の集約情報のための情報を持つオブジェクトです。

```
{
  "subservs": {
    "subservice1": <subservice 1 info>,
    "subservice2": <subservice 2 info>,
    ...
    "subserviceN": <subservice N info>
  }
  "sum": <aggregated info for all subservice in the given service>
}
```

上記の構造の Subservice 名は、最初のスラッシュなしで表示されます。たとえば、Subservice 名が (`Fiware-ServicePath` ヘッダで使用されている) `/gardens` 場合は、それに使用されるキーは、`gardens` (`/` なしの) になります。他のものは最初のものとは異なり、削除されません。例えば `/gardens/north` はキー `gardens/north` を使用します。

subservice 情報オブジェクトに関して、キーは異なるメトリックの名前です。

```
{
  "metric1": <metric 1>,
  "metric2": <metric 2>,
  ...
  "metricN": <metric N>
}
```

メトリックのリストは、[メトリックのセクション](#metrics)で提供されています。

いくつかの追加意見 :

* 無効なサービスやサブサービスに対応した要求は (つまり、[ここ](../user/multitenancy.md)と[ここ](../user/service_path.md) で説明されている構文規則に従わないもの) ペイロードに含まれていません。つまり、それに関連するメトリックは、単に無視されます。
* デフォルトのサービスは、サービスキーとして **default-service** を使用します。`-` キャラクタは通常のサービスでは許可されていないため、通常のサービスと衝突することはありません。
* ルートサブサービス (`/`) は、サブサービスキーとして **root-subserv** を使用します。`-` キャラクタは通常のサブサービスでは許可されていないため、通常のサブサービスと衝突することはありません。
* サブサービスの列挙を使用する要求 (例えば `Fiware-ServicePath: /A, /B`) は、リストの最初の要素に関連付けられます。つまり `A` です。
* "recursive subservice" (例えば、`Fiware-ServicePath: /A/#`) を使用する要求は、再帰性を考慮せずにサブサービスに関連付けられます。つまり `A` です。

[トップ](#top)

<a name="reset-metrics"></a>
### メトリックをリセット

```
DELETE /admin/metrics
```

このオペレーションは、Orion が開始されたばかりのように、すべてのメトリックをリセットします。

[トップ](#top)

<a name="get-and-reset"></a>
### 取得とリセット

```
GET /admin/metrics?reset=true
```

このオペレーション (実際には [メトリックの取得](#get-metrics)の変形) では結果が得られ、同時にアトミックな方法でメトリックがリセットされます。

[トップ](#top)

<a name="metrics"></a>
## メトリック

* **incomingTransactions** : Orion によって消費されたリクエストの数です。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **incomingTransactionRequestSize** : 着信トランザクションに関連するリクエストの合計サイズ (バイト) です (Orion の観点からは "in")。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **incomingTransactionResponseSize** : 着信トランザクションに関連するレスポンスの合計サイズ (バイト) です (Orion の観点からは "out")。このメトリックでは、すべての種類のトランザクション(正常トランザクションかエラートランザクションかにかかわらず)がカウントされます
* **incomingTransactionErrors** : エラーとなった着信トランザクションの数です
* **serviceTime** : トランザクションを処理する平均時間です。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **outgoingTransactions** : Orion によって送信された要求の数 (通知と CPrs への転送リクエストの両方) です。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **outgoingTransactionRequestSize** : 発信トランザクションに関連する要求の合計サイズ (バイト) です (Orion の観点からは "out")。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **outgoingTransactionResponseSize** : 発信トランザクションに関連する応答の合計サイズ (バイト) です (Orion の観点からの "in")。このメトリックでは、すべての種類のトランザクション (正常トランザクションかエラートランザクションかにかかわらず) がカウントされます
* **outgoingTransactionErrors** : エラーになった発信トランザクションの数です

[トップ](#top)
