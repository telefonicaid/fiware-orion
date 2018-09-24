# 0.14.1 以前のバージョンから 0.14.1 以降へアップグレード

Orion Context Broker で使用されているデータベースに以前の情報があり、そのデータベースに geo-located エンティティが含まれている場合は、次の手順を適用する必要があります。それ以外の場合は、デフォルトのアップグレード手順を使用できます。

-   contextBroker を停止します
-   contextBroker の旧バージョンを削除します

        yum remove contextBroker

-   [データベースのバックアップをとってください](database_admin.md#backup)。これは、問題が発生した場合の安全対策です。たとえば、swap_coords.js スクリプトが終了する前に中断され、データベースデータが非整合状態で終了するなどです
-   次のスクリプトをダウンロードしてください : [swap_coords.js](https://github.com/telefonicaid/fiware-orion/tree/0.14.1-FIWARE-3.5.1/scripts/managedb/swap_coords.js)
-   次のコマンドを使用して、データベースにスクリプトを適用してください ("db" はデータベース名です)。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります

        mongo <db> swap_coords.js

-   新しい contextBroker バージョンをインストールしてください。yum キャッシュのためにコマンドが失敗することがありますが、その場合は "yum clean all" を実行してやり直してください

        yum install contextBroker

-   contextBroker を開始します

rpm コマンドはスーパーユーザ権限を必要とするので、root として実行するか sudo コマンドを実行する必要があることに注意してください。
