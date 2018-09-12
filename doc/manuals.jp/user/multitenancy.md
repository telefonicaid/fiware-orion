# マルチテナンシー (Multi tenancy)

Orion Context Broker は、FIWARE セキュリティフレームワーク (PEP proxy, IDM, Access Control) のような他の FIWARE コンポーネントまたはサードパーティ製ソフトウェアによって提供されるサービス/テナントベースの認可ポリシーを容易にするために、単純なマルチテナント/マルチサービスモデルベースの論理データベース分離を実装します。この機能は、"-multiservice" コマンドライン・オプションを使用すると有効になります。"-multiservice" が使用されている場合、Orion はリクエスト内の "Fiware-Service" HTTP ヘッダを使用して、サービス/テナントを識別します。ヘッダが HTTP リクエストに存在しない場合は、デフォルトのサービス/テナントが使用されます。

マルチテナント/マルチサービスは、あるサービス/テナントのエンティティ/属性/サブスクリプションが他のサービス/テントに対して "不可視" であることを保証します。たとえば、tenantA スペースの queryContext は、テナント B スペースからエンティティ/属性を返すことはありません。この隔離は、データベースの分離に基づいています。詳細については、[インストールおよび管理のマニュアル](../admin/database_admin.md#multiservicemultitenant-database-separation)で説明しています。

さらに、"-multiservice" が使用されている場合、Orion は、指定されたテナント/サービスのサブスクリプションに関連する notifyContextRequest および notifyContextAvailability リクエストメッセージに "Fiware-Service" ヘッダを含めます (ただし、デフォルトのサービス/テナントは例外です。ヘッダは存在しません)。例 :

    POST http://127.0.0.1:9977/notify
    Content-Length: 725
    User-Agent: orion/0.13.0
    Host: 127.0.0.1:9977
    Accept: application/json
    Fiware-Service: t_02
    Content-Type: application/json

    {
    ...
    }

サービス/テナント名の構文に関しては、英数字の文字列 (および "\_" 記号) でなければなりません。最大長は50文字です。ほとんどの使用例で十分です。Orion Context Broker は小文字でテナント名を解釈します。そのため、updateContext で "MyService" などのテナントを使用することはできますが、そのテナントに関連する通知は "myservice" で送信され、その意味では Orion が notifyContextRequest で送信するものと比較して、updateContext で使用したテナントが一貫していません。
