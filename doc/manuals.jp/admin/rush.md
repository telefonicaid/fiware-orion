# ラッシュ・リレー (Rush relayer)

Orion Context Broker を "スタンドアロン" モードで実行することとは別に、通知リレーとして [Rush](https://github.com/telefonicaid/Rush) のアドバンテージを利用することもできます。したがって、通知がレスポンスを受信している間に HTTP タイムアウトを待つことを含め、通知自体を管理する代わりに、Orion は通知を Rush に渡します。したがって、Orion は、通知送信のための "火災と忘却 fire and forget)" ポリシーを実装することができます。このタスクに特化したソフトウェアが Rush です。

さらに、Rush を使用して HTTPS を使用して通知を送信することもできます。[ユーザ＆プログラマ・マニュアル](../user/security.md)のセキュリティのセクションを参照してください 。

ラッシュを使用するには :

-   Orion からネットワークで到達できる実行中の Rush インスタンスがあることです (同じホスト内であれば、"localhost" を使用して到達可能)。Rush のインストールはこのマニュアルの範囲外です。[Rush のドキュメント](https://github.com/telefonicaid/Rush/wiki)を参照してください
-   -rush コマンドラインインターフェースを使用して Orion を実行します。この値は、Rush ホストとポートをしてしなければなりません。`-rush localhost:1234` は Rush が localhost のポート1234をリッスンしていることを意味します

HTTP Rush Relayer は、通知にのみ使用されます。Orion が HTTP クライアントとして機能する他のケース (たとえば、コンテキストプロバイダへのクエリの転送/更新) では、Rush を使用せず、常に Orion は HTTP 要求自体を送信します。
