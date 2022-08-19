# 0.21.0 以前のバージョンから 0.21.0 以降へのアップグレード

このプロシージャは、データベースに少なくとも1つのエンティティ・ドキュメントが含まれている限り、常に実行する必要があります。

-   contextBroker を停止します
-   contextBroker の旧バージョンを削除します

        yum remove contextBroker   

-   [DB のバックアップをとってください](database_admin.md#backup)。これは、問題が発生した場合の安全対策です。たとえば、終了する前に一部のスクリプトが中断され、データベースデータが非整合状態で終了するなどです
-   次のスクリプトをダウンロードします :
    -   [fix_location_gjson.py](https://github.com/telefonicaid/fiware-orion/blob/0.21.0/scripts/managedb/fix_location_gjson.py)
    -   [attrsvector2attrsobject.py](https://github.com/telefonicaid/fiware-orion/blob/0.21.0/scripts/managedb/attrsvector2attrsobject.py)
-   pymongo を以前にインストールしたことがない場合はインストールしてください。スクリプトの依存関係のためです

        pip-python install pymongo

-   fix_location_gjson.py を DB に適用するには、次のようにします(db はデータベース名)。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。スクリプトにはしばらく時間がかかり、インタラクティブな進行カウンタが表示されます

        python fix_location_gjson.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。以下の "トラブル・シューティング" を参照してください

        WARNING: some problem was found during the process.

-   attrsvector2attrsobject.py をデータベースに適用するに、次のようにします (db はデータベース名)。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。スクリプトにはしばらく時間がかかり、インタラクティブな進行カウンタが表示されます

        python attrsvector2attrsobject.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。以下の "トラブルシューティング" を参照してください

        WARNING: some problem was found during the process.

-   新しい contextBroker バージョンをインストールしてください。yum キャッシュのためにコマンドが失敗することがありますが、その場合は "yum clean all" を実行してやり直してください

        yum install contextBroker

-   contextBroker を開始します

rpm コマンドはスーパーユーザ権限を必要とするので、root として実行するか sudo コマンドを実行する必要があることに注意してください。

## トラブル・シューティング

2つの異なる種類の問題が発生する可能性があります :

-   attrsvector2attrsobject.py の実行中に、指定されたエンティティの属性名が重複しているために発生します。この問題の症状は次のようなエラーになります :

        - <n>: dupplicate attribute detected in entity {"type": "...", "id": "...", "servicePath": "..."}: <AttrName>. Skipping

    これは、Orion 0.17.0 以前で作成されたエンティティに対応していて、属性を識別するために名前型の組み合わせを使用していました。重複する属性を削除し、attrsvector2attrsobject.py を再度実行して解決します。重複した属性を削除する手順は、[StackOverflow のこの記事](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791)に記載されています。

-   DB の更新中に予期しない問題が発生したためです。この問題の症状は次のようなエラーになります :

        - <n>: ERROR: document <...> change attempt failed!

    この問題の一般的な解決策はありません。ケース・バイ・ケースで分析する必要があります。これが起こった 場合、問題が解決したかどうかを確認するために、[StackOverflow の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)を参照してください。そうでない場合は、独自の質問を作成してください。"fiware-orion" タグとあなたのケースでの正確なエラーメッセージを含めることを忘れないでくださいです。
