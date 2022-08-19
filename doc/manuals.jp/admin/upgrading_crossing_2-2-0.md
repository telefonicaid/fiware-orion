# pre-2.2.0 バージョンから 2.2.0 以降へアップグレード

Orion 2.2.0 ではメタデータ ID 機能が削除されており、これには DB モデルの変更が含まれます。
この手順では、データにそのようなメタデータ ID があるかどうかを確認し、
その場合はその状況に対処する方法を説明します。

-   contextBroker を停止します
-   以前のcontextBrokerバージョンを削除します

        yum remove contextBroker

-   [DB のバックアップを取ります](database_admin.md#backup)
    (これは問題が発生した場合の安全対策です。例えば、スクリプトが終了する前に
    中断され、データベースのデータが一貫性のない状態で終了する場合などです)
-   次のスクリプトをダウンロードしてください :
    -   [check_metadata_id.py](https://github.com/telefonicaid/fiware-orion/blob/2.2.0/scripts/managedb/upgrade-2.2.0/check_metadata_id.py)
-   以前に pymongo をインストールしていない場合は、pymongo をインストールします
    (これはスクリプト依存です)

        pip-python install pymongo

-   次のコマンドを使用して、`check_metadata_id.py` スクリプトを DB に適用します
    ('db' はデータベース名です)。
    [マルチテナント/マルチサービス](database_admin.md#multiservicemultitenant-database-separation)
    を使用している場合は、各テナント/サービスのデータベースごとに手順を
    適用する必要があります。スクリプトにはしばらく時間
    がかかることがあるため、対話型の進行状況カウンターが表示されます。

        python check_metadata_id.py orion

-   このようなメッセージが表示された場合は、"メタデータ ID の処理" を参照してください

        entity {u'type': u'...', u'id': u'...', u'servicePath': u'...'}: found attr <...>, metadata id <...>, value <...>

-   上記のメッセージが表示されない場合は、データに ID はありません。
    次の手順に進むことができます

-   次のメッセージが表示された場合は、次のステップに進む前に解決する必要が
    ある問題がいくつかあります。以下の "トラブルシューティング"
    のセクションを確認してください。

        WARNING: some problem was found during the process.
        ERROR: document ... change attempt failed

-   新しい contextBroker バージョンをインストールします (yum
    キャッシュが原因でコマンドが失敗することがあります。その場合は、
    "yum clean all" を実行してやり直します）

        yum install contextBroker

-   contextBroker を起動します

yum コマンドはスーパーユーザ特権を必要とするため、root として実行するか、
sudo コマンドを使用する必要があります。

## メタデータ ID への対応

データベースにメタデータ ID がある場合は、それらを削除する必要があります。
その方法は2つあります :

-   使用していた古いバージョンの Orion で Orion API を使用する (推奨)
    `DELETE /v1/contextEntities/{entityId}/attributes/{attrName}/{attrId}`
    または
    `DELETE /v1/contextEntities/type/{entityType}/id/{entityId}/attributes/{attrName}/{attrId}`
    オペレーションを使うことができます

-   自動修正モード (autofix mode) で、`check_metadata_id.py` スクリプトを
    使用します。そうするためには、スクリプトを編集して、`autofix = None`
    の行を `autofix = 'as_new_attrs'` または `autofix = 'as_metadata'`
    のいずれかに変更してください
    - `as_new_attrs` モードでは、ID を持つ各属性は `<attrName>:<id>` の形式で
      新しい属性に変換されます (例えば、メタデータ ID `id1` を持つ `temperature`
      属性がある場合、それは  `temperature:id1` に変換されます)
      まれに、destination 属性が既にエンティティに存在している場合は
      `- ERROR: attribute <...> already exist in entity ... Cannot be automatically fixed`
      メッセージが表示されますので手動で対処する必要があります。
    - `as_metadata` モードでは、属性は名前を変えず、ID は通常のメタデータに
      変換されます。したがって、外部クライアント
      (つまり、`GET /v2/entities` を実行するシステム) の観点からは、
      何の違いもありません。しかしながら、これは、１つより多い ID
      を有する属性が存在しない (すなわち、スクリプトの最初のパスにおいて
      `少なくとも１つの属性を有するエンティティ` がゼロである)
      と仮定し、そうでなければ失敗します。

## トラブルシューティング

2種類の問題が発生する可能性があります :

-   次のエラーメッセージが表示されます :

        OperationFailed: Sort operation used more than the maximum 33554432 bytes of RAM. Add an index, or specify a smaller limit.

    mongo シェルで次のコマンドで適切なインデックスを作成してから、
    スクリプトをもう一度実行してください。

        db.entities.createIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

    移行が終了したら、次のコマンドでインデックスを削除できます :

        db.entities.dropIndex({"_id.id": 1, "_id.type": -1, "_id.servicePath": 1})

-   DB の更新中に予期しない問題が発生したため。この問題の症状は、
    次のようなエラーが発生していることです :

        - <n>: ERROR: document <...> change attempt failed!

    この問題に対する一般的な解決策はありません。ケース・バイ・ケースで分析する
    必要があります。このような問題が発生した場合、問題がで解決されるかどうか
    チェックするために、
    [fiware-orion に関する StackOverflow の既存の質問](http://stackoverflow.com/questions/tagged/fiware-orion)
    を見てください。そうでなければ、あなた自身の質問を作成してください。
    その場合は "fiware-orion" タグと正確なエラー・メッセージを含めるのを
    忘れないでください。
