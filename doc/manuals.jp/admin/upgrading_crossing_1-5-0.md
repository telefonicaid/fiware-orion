# 1.5.0 以前のバージョンから 1.5.0 以降へアップグレード

この手順は、DB に [ID メタデータ](../user/metadata.md#id)を持つ属性が含まれている場合にのみ実行する必要があります。

-   contextBroker を停止します
-   contextBroker の旧バージョンを削除します

        yum remove contextBroker

-   [DB のバックアップをとってください](database_admin.md#metadata-id-for-attributes)。これは、問題が発生した場合の安全対策です。たとえば、終了する前に一部のスクリプトが中断され、データベースデータが非整合状態で終了するなどです。
-   次のスクリプトをダウンロードします :
    -   [change_attr_id_separator.py](https://github.com/telefonicaid/fiware-orion/blob/1.5.0/scripts/managedb/change_attr_id_separator.py)
-   pymongo を以前にインストールしたことがない場合はインストールしてください。スクリプトの依存関係のためです

        pip-python install pymongo

-   次のコマンドを使用して、change_attr_id_separator.py スクリプトを DB に適用します。db はデータベース名です。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。スクリプトにはしばらく時間がかかりますので、インタラクティブな進行カウンタが表示されます

        python change_attr_id_separator.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。以下の "mdsvector2mdsobject のトラブル・シューティング" を参照してください

        WARNING: some problem was found during the process.
        ERROR: document ... change attempt failed

-   新しい contextBroker バージョンをインストールしてください。yum キャッシュのためにコマンドが失敗することがありますが、その場合は "yum clean all" を実行してやり直してください

        yum install contextBroker

-   contextBroker を開始します

rpm コマンドはスーパーユーザ権限を必要とするので、root として実行するか sudo コマンドを使用して実行する必要があることに注意してください。

## トラブル・シューティング

3つの異なる種類の問題が発生する可能性があります :

-   次のエラーメッセージが表示されます :

        OperationFailed: Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.

    mongo シェルで次のコマンドを使用して適切なインデックスを作成してから、スクリプトをもう一度実行してください。

        db.entities.createIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

    移行が終了したら、次のコマンドを使用して索引を削除できます :

        db.entities.dropIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

-   DB の更新中に予期しない問題が発生したためです。この問題の症状は次のようなエラーになります :

        - <n>: ERROR: document <...> change attempt failed!

    この問題の一般的な解決策はありません。ケース・バイ・ケースで分析する必要があります。これが起こった場合は、 問題が解決したかどうかを確認するために、[StackOverflow for fiware-orion の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)をご覧ください。そうでない場合は、独自の質問を作成してください  ("fiware-orion" タグと正確なエラーメッセージをあなたのケースに含めることを忘れないでください)。

