# クロス・オリジン・リソース共有 (CORS)

起動時に `-corsOrigin` スイッチを使用して Orion に [CORS](https://developer.mozilla.org/en-US/docs/Web/HTTP/CORS) (Cross Origin Resource Sharing) を有効にすることができます。`-corsOrigin` は、CORS リクエストを行うことを許可される単一の起点の文字列値か、または任意の起点が Context Broker への CORS リクエストを行うことを許可する `__ALL` かのいずれかをとります。

Orion の CORS モードの構成可能な唯一の側面は、クライアントがプリフライト・リクエストをキャッシュできる最大時間で、これは `-corsMaxAge` スイッチによって処理されます。秒単位で最大キャッシュ時間が使用され、使用されない場合はデフォルトで `86400` (24時間) になります。

Orion CLI スイッチの詳細については 、[管理マニュアル](../admin/cli.md)を参照してください。

例えば : 

- 以下のコマンドは、10分のプリフライト・キャッシュの最大時間で任意の起点に対して CORS を有効にして Orion を開始します

        contextBroker -corsOrigin __ALL -corsMaxAge 600

- CORS を有効にして Orion を起動し、デフォルトの最大プリフライト・キャッシュ時間を持つ特定のオリジンのみを有効にする

        contextBroker -corsOrigin specificdomain.com 

CORS はすべての `/v2` リソースで使用できます。

## アクセス制御の許可オリジン

CORS モードが有効な場合、Origin ヘッダはリクエストに存在し、その値は Orion の許可された Origin に一致します。このヘッダは常にレスポンスに追加されます。

上記の条件が満たされず、Access-Control-Allow-Origin ヘッダがレスポンスに追加されない場合、すべての CORS プロセスが停止され、他の CORS ヘッダはレスポンスに追加されません。

もし `-corsOrigin` が特定の値に設定されていれば、この場合は `specificdomain.com` です : 

    Access-Control-Allow-Origin: specifidomain.com 

もし `-corsOrigin` が `__ALL` に設定されている場合 : 

    Access-Control-Allow-Origin: * 


## アクセス制御の許可メソッド

このヘッダは、`/v2` リソースに対するすべての `OPTIONS` リクエストに対する Orion のレスポンスに存在する必要があります。各リソースには独自の許可されたメソッドがあり、ヘッダ値は、[lib/serviceRoutinesV2](https://github.com/telefonicaid/fiware-orion/tree/master/src/lib/serviceRoutinesV2)の `options*Only` サービス・ルーチンによって設定されます

## アクセス制御の許可ヘッダ

このヘッダは、`/v2` リソースに対するすべての `OPTIONS` リクエストに対する Orion のレスポンスに存在する必要があります。Orion は、CORS リクエスト内の特定のヘッダ・セットを許可します。これらは [lib/rest/HttpHeaders.h](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/rest/HttpHeaders.h) で定義されています。

有効な `OPTIONS` リクエストに対する Orion のレスポンスには、以下のヘッダと値が含まれます : 

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token 

## アクセス制御の最大寿命

このヘッダは、/v2 リソースに対するすべての `OPTIONS` リクエストに対する Orion のレスポンスに存在する必要があります。ユーザは、クライアントが Orion に対して行われたプリフライト・リクエストをキャッシュできる最大時間 (秒) の値を自由に設定できます。

`-corsMaxAge` が特定の値 (このケースでは `600`) に設定されている場合、有効な `OPTIONS` リクエストに対する Orion のレスポンスには、以下のヘッダと値が含まれます : 

    Access-Control-Max-Age: 600

`-corsMaxAge` が起動時に設定されていない場合は、デフォルトで '86400' (24時間) になり、有効な `OPTIONS` リクエストに対する Orion のレスポンスには、以下のヘッダと値が含まれます : 

    Access-Control-Max-Age: 86400

## アクセス制御の公開ヘッダ

このヘッダは、有効な Origin 値で行われたすべてのリクエストに対して、Orion のレスポンスに存在する必要があります。Orion は、CORS リクエストでユーザ・エージェント (ブラウザ) が特定のレスポンス・ヘッダ・セットにアクセスすることを許可します。これらは [lib/rest/HttpHeaders.h](https://github.com/telefonicaid/fiware-orion/blob/master/src/lib/rest/HttpHeaders.h) で定義されています。

有効な CORS リクエストに対する Orion のレスポンスには、以下のヘッダと値が含まれます : 

    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location 
