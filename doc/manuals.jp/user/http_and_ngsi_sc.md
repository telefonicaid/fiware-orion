# HTTP および NGSI のレスポンス・コード (HTTP and NGSI response codes)

このセクションで説明する HTTP と NGSI のレスポンス・コードの区別は、NGSIv1 にのみ適用されます。NGSIv2 は、HTTP レスポンス・コードだけを使用するより簡単なアプローチを採用しています。

NGSIv1 API レスポンスでは、2つの独立したレスポンス・コードが検討されています : 1つは NGSI レベルの "内部" (つまり REST HTTP レスポンス・ペイロードにエンコードされている) と HTTP レベルの他の "外部" (HTTP レスポンス・コード自体) です。このマニュアルでは、API の NGSI の側面に焦点を当てているので、このドキュメントでは、特に明記しない限り、HTTP コードは "200 OK" とみなしています。

コードとその独立性の両方の存在を説明するために、存在しないエンティティ (たとえば "foo") に対する queryContext オペレーションを考えてみましょう。HTTP レスポンス・コードとヘッダを出力するには、curl コマンドの -v フラグに注意してください :

```
# curl localhost:1026/v1/contextEntities/foo -s -S --header 'Accept: application/json' -v | python -mjson.tool
* About to connect() to localhost port 1026 (#0)
*   Trying ::1...
* connected
* Connected to localhost (::1) port 1026 (#0)
> GET /v1/contextEntities/foo HTTP/1.1
> User-Agent: curl/7.26.0
> Host: localhost:1026
> Accept: application/json
>
* additional stuff not fine transfer.c:1037: 0 0
* HTTP 1.1 or later with persistent connection, pipelining supported
< HTTP/1.1 200 OK
< Content-Length: 220
< Content-Type: application/json
< Date: Thu, 10 Sep 2015 18:40:37 GMT
<
{ [data not shown]
* Connection #0 to host localhost left intact
* Closing connection #0
{
    "contextElement": {
        "id": "foo",
        "isPattern": "false",
        "type": ""
    },
    "statusCode": {
        "code": "404",
        "details": "Entity id: /foo/",
        "reasonPhrase": "No context element found"
    }
}
```
この場合、HTTP が "200 OK" の間、NGSI レスポンス・コードは "404 No context element found" であることに注意してください。したがって、言い換えれば、HTTP レベルでの通信は NGSI レベルで発生したエラー条件(Orion Context Broker データベースに存在しないエンティティ)でも OK でした。

次の例は、許可されていない HTTP verb をクライアントが使用しようとしているために、HTTP レベルの問題が発生した場合を示しています。この場合、HTTP レスポンス・コード "405 Method Not Allowed" が生成されます。

```
curl -X PATCH localhost:1026/v1/contextEntities/foo -s -S --header 'Accept: application/json' -v
* About to connect() to localhost port 1026 (#0)
*   Trying ::1...
* connected
* Connected to localhost (::1) port 1026 (#0)
> PATCH /v1/contextEntities/foo HTTP/1.1
> User-Agent: curl/7.26.0
> Host: localhost:1026
> Accept: application/json
>
* additional stuff not fine transfer.c:1037: 0 0
* HTTP 1.1 or later with persistent connection, pipelining supported
< HTTP/1.1 405 Method Not Allowed
< Content-Length: 0
< Allow: POST, GET, PUT, DELETE
< Date: Thu, 10 Sep 2015 18:42:38 GMT
<
* Connection #0 to host localhost left intact
* Closing connection #0
```
