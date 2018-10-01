# 0.19.0 以前のバージョンから 0.19.0 以降へアップグレード

このプロシージャは、データベースに少なくとも1つのエンティティ・ドキュメントが含まれている限り、常に実行する必要があります。

-   contextBroker を停止します
-   contextBroker の旧バージョンを削除します

        yum remove contextBroker

-   [データベースのバックアップをとってください](database_admin.md#backup)。これは、問題が発生した場合の安全対策です。たとえば、fix_default_sp.py スクリプトが終了する前に中断され、データベースデータが非整合状態で終了するなどです
-   次のスクリプトをダウンロードしてください : [fix_default_sp.py](https://github.com/telefonicaid/fiware-orion/blob/0.19.0-FIWARE-4.2.2/scripts/managedb/fix_default_sp.py)
-   pymongo を以前にインストールしたことがない場合はインストールしてください。スクリプトの依存関係のためです

        pip-python install pymongo

-   次のコマンドを使用して、データベースにスクリプトを適用します。"db" はデータベース名です。[マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)を使用する場合は、各テナント/サービスのデータベースに手順を適用する必要があります。このスクリプトはしばらく時間がかかります。たとえば、約 50,000 エンティティと、データベースでは30分程度です。mongo シェルで次のコマンドを使用して進行状況を監視することができ、スクリプトが動作している間はカウントが減少します : db.entities.count({"_id.servicePath": null})

        python fix_default_sp.py orion

-   以下のメッセージが表示された場合は、次の手順に進む前に解決する必要のある問題があります。以下のトラブル・シューティングを参照してください

        WARNING: some problem was found during the process.

-   新しい contextBroker バージョンをインストールしてください。yum キャッシュのためにコマンドが失敗することがありますが、その場合は "yum clean all" を実行してやり直してください

        yum?install?contextBroker

-   contextBroker を開始します

rpm コマンドはスーパーユーザ権限を必要とするので、root として実行するか sudo コマンドを実行する必要があることに注意してください。

## トラブル・シューティング

2つの異なる種類の問題が発生する可能性があります :

-    重複した" エンティティ。つまり、データベース内では同じ ID と型を持ち、そのうちの1つは servicePath "/" と、もう1つは servicePath がない場合です。これは、前述のプロセスを 0.19.0 またはそれ以降にアップグレードする前に実行していても、最初にアップグレードした後に fix_default_sp.py スクリプトを実行した場合に発生する可能性があります。スクリプトのアップグレードと実行の間に、いくつかの更新エンティティ・オペレーションが Orion API を使用して行われた場合です。この状況は問題です。たとえば、[ここ](http://stackoverflow.com/questions/28498460/orion-cb-does-not-change-the-value-of-an-attribute)で説明しているようにエンティティの更新で問題が発生する可能性があります。できるだけ早く修正する必要があります。この問題の症状は、fix_default_sp.py に出力に0より大きい結果値を持つ次のカウンタがあります :

        duplicated cases of entities with "/" and null "/" service path (NEED FIX)

    解決方法は、[merge_default_sp_dups.py](https://github.com/telefonicaid/fiware-orion/blob/0.19.0-FIWARE-4.2.2/scripts/managedb/merge_default_sp_dups.py) スクリプトを実行して 、複製されたドキュメントの各ペアを単一のエンティティドキュメントに統合することです。スクリプトは、修正するデータベースをパラメータとして使用します。fix_default_sp.py で使用するものと同じでなければなりません。

        python merge_default_sp_dups.py orion

    merge_default_sp_dups.py を実行した後、fix_default_sp.py を再度実行して NEED FIX カウンタが0であり、"警告 : プロセス中に何らかの問題が見つかりました" というメッセージが表示されないことを確認するために高度に再計算されます。

    merge_default_sp_dups.py ではすべての状況を解決することができないことに注意してください。非常にまれなケースでは、いくつかのペアを統合することはできません。この場合、スクリプトの実行終了時に出力されたサマリーに0より大きい値が表示されます。

        ----- processed in total:  540 (total time: 0:00:44.143435)
        ----- dup processed:       481
        ----- skipped:             7

    merge_default_sp_dups.py は、次の状況でペアの連結をスキップします :

    -   ペア内のいくつかのドキュメントに同じ名前の属性がいくつかあります。これは、0.17.0 以前の Orion で作成されたエンティティで、属性を識別するために名前と型の組み合わせを使用していたエンティティに対応している可能性があります。これはもはや 0.19.0 で使用されないので、重複した属性を削除する必要があります。重複した属性を削除する手順は、[StackOverflow のこの記事](http://stackoverflow.com/questions/30242731/fix-duplicate-name-situation-due-to-entities-created-before-orion-0-17-0/30242791#30242791)に記載されています
    -   null servicePath に対応するペアの要素の変更時刻は、servicePath"/" に対応するペアの要素の変更時刻よりも大きいことです。これは非常に異常な状況です。これは、servicePath "/" ドキュメントのデータが null servicePath ドキュメントのデータよりも新鮮でなければならないためです。もし起これば、これについて教えてください

    最後に、"ERROR pushing attributes at DB" または "ERROR removing dup at DB" などの DB エラーが発生する可能性があります。以下に説明する fix_default_sp.py の DB エラーで説明したのと同じ処理に従ってください。

-   DB の更新中に予期しない問題が発生したためです。この問題の症状は、次のような fix_default_sp.py 出力にエラーが発生しています :

        * Error editing entity at DB:

    この問題の一般的な解決策はありません。ケース・バイ・ケースで分析する必要があります。これが起こった場合、問題が解決したかどうかを確認するために、[StackOverflow の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)を参照してください。そうでない場合は、新たな質問を作成してくださいい。"fiware-orion" タグと、あなたのケースでの正確な "Error editing entity at DB" を含めることを忘れないでください。
