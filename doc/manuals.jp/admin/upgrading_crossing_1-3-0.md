# 1.3.0 以前のバージョンから 1.3.0 以降へアップグレード

このプロシージャは、データベースに少なくとも1つのエンティティ・ドキュメント **または** 少なくとも1つのコンテキスト・サブスクリプション・ドキュメントが含まれている限り、常に実行する必要があります。

-   contextBroker を停止します
-   contextBroker の旧バージョンを削除します

        yum remove contextBroker

-   [DB のバックアップをとってください](database_admin.md#backup)。これは、問題が発生した場合の安全対策です。たとえば、終了する前に一部のスクリプトが中断され、データベースデータが非整合状態で終了するなどです
-   次のスクリプトをダウンロードします :
    -   [mdsvector2mdsobject.py](https://github.com/telefonicaid/fiware-orion/blob/1.3.0/scripts/managedb/mdsvector2mdsobject.py)
    -   [csub_merge_condvalues.py](https://github.com/telefonicaid/fiware-orion/blob/1.3.0/scripts/managedb/csub_merge_condvalues.py)
-   pymongo を以前にインストールしたことがない場合はインストールしてください。スクリプトの依存関係のためです

        pip-python install pymongo

-   次のコマンドを使用して、データベースに mdsvector2mdsobject.py スクリプトを適用します。"db" はデータベース名です。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。スクリプトにはしばらく時間がかかりますので、インタラクティブな進行カウンタが表示されます

        python mdsvector2mdsobject.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。以下の "mdsvector2mdsobject のトラブル・シューティング" を参照してください。

        WARNING: some problem was found during the process.
        ERROR: document ... change attempt failed

-   次のコマンドを使用して、csub_merge_condvalues.py スクリプトを DB に適用します。db はデータベース名です。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。スクリプトにはしばらく時間がかかりますので、インタラクティブな進行カウンタが表示されます

        python csub_merge_condvalues.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。下の "csub_merge_condvalues のトラブル・シューティング" を参照してください

        WARNING: some csub were skipped
        ERROR: document ... change attempt failed

-   新しい contextBroker バージョンをインストールしてください。yum キャッシュのためにコマンドが失敗することがありますが、その場合は "yum clean all" を実行してやり直してください

        yum install contextBroker

-   contextBroker を開始します

rpm コマンドはスーパーユーザ権限を必要とするので、root として実行するか sudo コマンドを使用して実行する必要があることに注意してください。

## mdsvector2mdsobject のトラブル・シューティング

3つの異なる種類の問題が発生する可能性があります :

-   次のエラーメッセージが表示されます :

        OperationFailed: Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.

    mongo シェルで次のコマンドを使用して適切なインデックスを作成し、mdsvector2mdsobject.py をもう一度実行してください。

        db.entities.createIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

    移行が終了したら、次のコマンドを使用してインデックスを削除できます :

        db.entities.dropIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

-   mdsvector2mdsobject.py の実行中に特定のエンティティにメタデータ名が重複しているため、この問題の症状は次のようなエラーになります :

        - <n>: dupplicate metadata detected in entity {"type": "...", "id": "...", "servicePath": "..."} (attribute: <attrName>): <MetadataName>. Skipping

    これは古い Orion バージョンで作成されたエンティティに対応していて、名前型の組み合わせを使用してメタデータを識別していました。解決策は、重複したメタデータを削除し、mdsvector2mdsobject.py を再実行することです。重複したメタデータを削除する手順は、[StackOverflow のこの記事](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791)の重複した属性について記載されている手順と似ています。不確かな場合は、StackOverflow で質問を送信してください。質問に "fiware-orion" タグを付けてください。

-   DB の更新中に予期しない問題が発生したためです。この問題の症状は次のようなエラーになります :

        - <n>: ERROR: document <...> change attempt failed!

    この問題の一般的な解決策はありません。ケース・バイ・ケースで分析する必要があります。これが起こった場合は、 問題が解決したかどうかを確認するために、[StackOverflow for fiware-orion の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)をご覧ください。そうでない場合は、独自の質問を作成してください ("fiware-orion" タグと正確なエラーメッセージをあなたのケースに含めることを忘れないでください)。

## csub_merge_condvalues のトラブルシューティング

2つの異なる種類の問題が発生する可能性があります :

-   次の警告メッセージが表示されます :

        WARNING: some csub were skipped

    一部のサブスクリプション・ドキュメントがスキップされ、実際に移行されなかったことを意味します。サブスクリプション ID はスクリプト出力の一部として提供されるので、ドキュメントを DB に配置して分析することができます。一部のスキップ原因は問題なく、実際の問題を意味するものではありません。
特に、"empty conditions" は、任意の属性の変更によってトリガーされるサブスクリプションに対応するため、正常です。他のすべての原因により、潜在的な問題が隠される可能性があります。`db.csubs.remove({_id: ObjectId("<subscription ID>")>})` を使って、問題のあるドキュメントを削除するか、DB で修正することです。[csubs コレクションの DB モデル](database_model#csubs-collection)をチェックしてください。疑問がある場合は、[StackOverflow の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)をご覧ください。 あなたの問題がそこで解決されているかどうかをチェックする。そうでない場合は、新たな質問を作成してください。"fiware-orion" タグとあなたのケースでの正確なエラーメッセージを含めることを忘れないでください。

-   DB の更新中に予期しない問題が発生したためです。この問題の症状は次のようなエラーになります :

        - <n>: ERROR: document <...> change attempt failed!

    この問題の一般的な解決策はありません。ケース・バイ・ケースで分析する必要があります。これが起こった 場合、問題が解決したかどうかを確認するために、[StackOverflow の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)を参照してください。そうでない場合は、独自の質問を作成してください ("fiware-orion" タグを含むことを忘れないでくださいあなたのケースでの正確なエラーメッセージ)。
