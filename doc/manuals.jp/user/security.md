# セキュリティの考慮事項 (Security considerations)

## 認証と認可

Orion は、"ネイティブ" 認証やアクセス制御を実施する権限メカニズムを提供していません。しかし、認証/認可は、[FIWARE GEs が提供するアクセス制御フレームワーク](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE.ArchitectureDescription.Security.Access_Control_Generic_Enabler)を達成することができます。

具体的には、Orion は [FIWARE PEP Proxy GE](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/FIWARE.OpenSpecification.Security.PEP_Proxy_Generic_Enabler)を使用してこのフレームワークに統合されています。現時点では、Orion Context Broker と連携できる2つの GE インプリメンテーション (GEis) が存在します :

-   [Wilma](http://catalogue.fiware.org/enablers/pep-proxy-wilma) (PEP Proxy 
    GE reference implementation)
-   [Steelskin](https://github.com/telefonicaid/fiware-pep-steelskin)

上記のリンクには、両方の GEis の使用方法に関するドキュメントがあります。

## HTTPS API

Orion ContextBroker は `-https` をオプションを使用して、HTTPS をサポートしています (さらに、-key オプションと -cert オプションが必要です。このオプションは、サーバの秘密鍵と証明書を含むファイルをそれぞれ指定します)。詳細については、[管理マニュアルのコマンドライン・オプションのセクション](../admin/cli.md#command-line-options)を確認してください。現在の Orion のバージョンは、HTTP と HTTPS の両方で同時に実行することはできません。つまり、`-https` を使用は HTTP を無効にします。

## HTTPS 通知

Orion によってエクスポートされた API サーバで HTTPS を使用する以外に、通知で HTTPS を使用することもできます。これを行うには、サブスクリプションで URL の中に "https" プロトコル・スキーマを使用する必要があります。

```
  ...
  "url": "https://mymachime.example.com:1028/notify"
  ...
```

Rush relayer ([Rush を使用して Orion を実行する方法](../admin/rush.md)を参照) を使用し、Orion から Rush へのリクエストが HTTP で送信される場合、Rush は HTTPS を使用して最終的な受信者に向けて暗号化します。Rush relayer を使用しない場合、Orion はネイティブに HTTPS 通知を送信します。その場合、デフォルトでは、Orion は信頼できないエンドポイント (つまり、既知の CA 証明書で認証できない証明書) への接続を拒否します。この動作を避けたい場合は、`-insecureNotif` [CLI パラメータ](../admin/cli.md)を使用する必要がありますが、そうすることは安全ではない設定です (例 : [man-in-the-middle 攻撃](https://en.wikipedia.org/wiki/Man-in-the-middle_attack)に苦しむ可能性があります)。
