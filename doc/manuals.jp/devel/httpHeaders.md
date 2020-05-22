# <a name="top"></a>Orion HTTP ヘッダ
このドキュメントでは、**src/lib/rest/HttpHeaders.h** で定義されている Orion で使用されるすべてのヘッダについて説明します。

1)  [Accept](#1-accept)        
2)  [Content-Length](#2-content-length)         
3)  [Content-Type](#3-content-type)                
4)  [Expect](#4-expect)                                                                                
5)  [Host](#5-host)                        	                                                      
6)  [Origin](#6-origin)                                                       
7)  [User-Agent](#7-user-agent)    
8)  [X-Forwarded-For 及び X-Real-IP](#8-x-forwarded-for-and-x-real-ip)

9) [Access-Control-Allow-Origin](#9-access-control-allow-origin)
10) [Access-Control-Allow-Headers](#10-access-control-allow-headers)                                                                   
11) [Access-Control-Allow-Methods](#11-access-control-allow-methods)
12) [Access-Control-Max-Age](#12-access-control-max-age)                                 
13) [Access-Control-Expose-Headers](#13-access-control-expose-headers)
14) [Allow](#14-allow)                                                                                    
15) [Location](#15-location)                                                 
16) [Ngsiv2-AttrsFormat](#16-ngsiv2-attrsformat)                                    
17) [Fiware-Service](#17-fiware-service)
18) [Fiware-Servicepath](#18-fiware-servicepath)
19) [Fiware-Total-Count](#19-fiware-total-count)
20) [Fiware-Correlator](#20-fiware-correlator)                                     

21) [X-Auth-Token](#21-x-auth-token)



# Orion で使用される HTTP ヘッダとその説明



## 1. Accept

Accept HTTP ヘッダは、Orion が受信する着信 HTTP リクエストで使用されます。リクエストで Accept HTTP ヘッダを使用して、
クライアントが受け入れる MIME タイプを指定します。通常、その値は `application/json` (JSON MIME タイプの場合) ですが、
NGSIv2 API の一部の操作では `text/plain` も許可されます。たとえば、リクエスト URL で使用する場合 :
	
    curl ... -H 'Accept: application/json'
		 
このリクエストでは、明確に定義された JSON 形式でレスポンス・データを取得します。

この仕様の API レスポンス・ペイロードは、`application/json` および (属性値タイプの操作用) `text/plain` MIME タイプに
基づいています。Accept タイプと異なる HTTP リクエストを発行するクライアントは、`406 Not Acceptable` エラーを受け取ります。

[トップ](#top)

## 2. Content-Length

このヘッダは、リクエストとレスポンスの両方で使用されます。Content-Length HTTP ヘッダは、Orion のレスポンスの必須ヘッダ
です。リクエストとレスポンスのボディの長さをバイト単位で定義します。Orion Context Broker は、すべてのクライアント・
リクエストで常に Content-Length ヘッダを期待します。そうでない場合、クライアントは `411 Length Required` レスポンスを
受信します。これは、ベースとなる HTTP ライブラリ (libmicrohttpd) の動作方法によるものです。
	
    Content-Length: 34	

[トップ](#top)
 
## 3. Content-Type 

リクエストまたはレスポンスの MIME タイプを指定するために使用されます。通常、その値は `application/json` (JSON MIME
タイプの場合) ですが、NGSIv2 API の一部の操作では `text/plain` も許可されます。

    curl ... -H 'Content-Type: application/json'

このリクエストは、明確に定義された JSON 形式でデータを送信します。
    	
[トップ](#top)	
  
## 4. Expect

Orion は、発信リクエスト (通知およびフォワードされたクエリ/更新) で空の `Expect:` ヘッダを送信します。詳細については、
[flaviocopes.com](https://flaviocopes.com/http-request-headers/),
[ietf.org](https://tools.ietf.org/html/rfc7231#section-5.1.1),
[wikipedia.org](https://en.wikipedia.org/wiki/List_of_HTTP_header_fields)
の Expect HTTP ヘッダの一般的なドキュメントに従ってください。

    Expect:
	
[トップ](#top)	
	
## 5. Host

Orion は、このヘッダを発信リクエスト (通知およびフォワードされた更新/クエリ) に含めます。このヘッダは、通知のレシーバー
のポート番号を提供します。 	
	
    Host: localhost:1028
		
[トップ](#top)

## 6. Origin

Origin は、Orion が受信する着信 HTTP リクエストで使用されます。この操作は CORS に関連しています。
[ユーザ・マニュアルの CORS について](../user/cors.md)を参照してください。
	
    Origin: http://www.example-social-network.com
		
[トップ](#top)	
  
## 7. User-Agent

User-Agent HTTP ヘッダは、通知およびフォワードされたリクエストで使用されます。使用する Orion のバージョンとその転送
ライブラリについて説明します。ユーザ・エージェントを識別する文字列を提供します。

    User-Agent: orion/2.2.0-next libcurl/7.29.0     (通知およびフォワードされたリクエスト内)

[トップ](#top)		
		
<a name="8-x-forwarded-for-and-x-real-ip"></a>
## 8. X-Forwarded-For 及び X-Real-IP

両方のヘッダは、Orion が受信する着信 HTTP リクエストで使用されます。X-Forwarded-For ヘッダは、トランザクションのソース
として HTTP リクエストの元の IP をオーバーライドします。X-Real-IP および X-Forwarded-For (Orion 上で潜在的なプロキシに
よって使用される) は IP をオーバーライドします。両方がされた場合、X-Real-IP は、X-Forwarded-For よりも優先されます。
	
    X-Forwarded-For: 129.78.138.66
	
[トップ](#top)

## 9. Access-Control-Allow-Origin

これは、Orion によって送信される発信 HTTP レスポンスで使用されるオプションのヘッダです。この動作は CORS に関連しています。
[ユーザ・マニュアルの CORS について](../user/cors.md#access-control-allow-origin)を参照してください。
    
    Access-Control-Allow-Origin: *
		
[トップ](#top)


## 10. Access-Control-Allow-Headers

Orion によって送信される発信 HTTP レスポンスで使用されます。
この動作は CORS に関連しています。[ユーザ・マニュアルの CORS について](../user/cors.md#access-control-allow-headers)
を参照してください。

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token	
									               								   
[トップ](#top)		
		
		
## 11. Access-Control-Allow-Methods

Orion によって送信される発信 HTTP レスポンスで使用されます。この動作は CORS に関連しています。
[ユーザ・マニュアルの CORS について](../user/cors.md#access-control-allow-methods)を参照してください。

[トップ](#top)

									  
## 12. Access-Control-Max-Age

Orion によって送信される発信 HTTP レスポンスで使用されます。この動作は CORS に関連しています。
[ユーザ・マニュアルの CORS について](../user/cors.md#access-control-allow-methods)を参照してください。
		
    Access-Control-Max-Age: 86400

[トップ](#top)


## 13. Access-Control-Expose-Headers

Orion によって送信される発信 HTTP レスポンスで使用されます。この動作は CORS に関連しています。
[ユーザ・マニュアルの CORS について](../user/cors.md#access-control-allow-methods)を参照してください。
	
    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location.

[トップ](#top)


## 14. Allow

Orion によって送信される発信 HTTP レスポンスで使用されます。クライアントが特定の URL リソースで誤った HTTP メソッドを使用
した場合、このヘッダは、どのメソッドが許可されるかをクライアントに通知するために使用されます。

    Allow: GET

[トップ](#top)
		
    
## 15. Location

これは、サブスクリプション、エンティティの作成およびレジストレーション時に Orion によって送信される発信 HTTP レスポンスで
使用されます。レスポンスには、サブスクリプション ID, エンティティ ID, またはレジストレーション ID を保持する Location
ヘッダが含まれます。サブスクリプション、エンティティ、またはレジストレーションの更新と削除に使用される24桁の16進数です。
リクエストで `-v` を使用して、レスポンスの Location ヘッダを取得します。
	
    curl -v localhost:1026/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
    {
     //payload
    }
    EOF
	
取得するレスポンス:
	
    Location: /v2/subscriptions/57458eb60962ef754e7c0998
	
[トップ](#top)


## 16. Ngsiv2-AttrsFormat

Orion によって送信される発信 HTTP 通知で使用されます。通知には、関連するサブスクリプションの形式の値を持つ HTTP ヘッダ
`Ngsiv2-AttrsFormat` を含める必要があります。これにより、通知の受信者は、通知ペイロードから形式を推測する必要なく形式を
認識できます。通知にカスタム・ペイロードが使用された場合、通知の `Ngsiv2-AttrsFormat` ヘッダに `custom` の値が使用される
ことに注意してください。Ngsiv2-AttrsFormat は、カスタム通知での変更不可能なヘッダです。たとえば、
`"httpCustom": { ... "headers": {"Ngsiv2-Attrsformat": "something"} ...}` としても、無視されます。
	
    Ngsiv2-Attrsformat: normalized

[トップ](#top)	


## 17. Fiware-Service

Fiware-Service は、Orion で管理されるあらゆる種類の HTTP トランザクション (着信/発信リクエストおよび発信レスポンス) で使用
されます。`-multiservice` が使用された場合、Orion は、特定のテナント/サービスのサブスクリプションに関連付けられた通知
リクエストに `Fiware-Service` ヘッダを含めます (デフォルトのサービス/テナントを除き、ヘッダは存在しません)。
[ユーザ・マニュアルのマルチテナンシーについて](../user/multitenancy.md)を参照してください。

    POST http://127.0.0.1:9977/notify    
    Content-Length: 725    
    User-Agent: orion/2.3.0    
    Host: 127.0.0.1:9977      
    Accept: application/json    
    Fiware-Service: t_02    
    Content-Type: application/json    
    {    
	   ...    
    }
	
[トップ](#top)


## 18. Fiware-Servicepath

Fiware-ServicePath は、Orion で管理されるあらゆる種類の HTTP トランザクション (着信/発信リクエストおよび発信レスポンス)
で使用されるオプションのヘッダです。[ユーザ・マニュアルの service_path について](../user/service_path.md)を参照してください。

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte/Parterre1		

[トップ](#top)
 
 
## 19. Fiware-Total-Count

これは、Orion によって送信される発信 HTTP レスポンスで使用されるオプションのヘッダです。この操作はページネーションに関連
しています。[ユーザ・マニュアルのページネーションについて](../user/pagination.md#pagination) を参照してください。
	    
[トップ](#top)


## 20. Fiware-Correlator

Fiware-Correlator は、発信レスポンスで使用されます。また、通知およびフォワードされたクエリ/更新で送信または伝播されます。
Fiware-Correlator は、カスタム通知での変更不可能なヘッダです。たとえば、
`"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` としても、無視されます。
[管理者マニュアルのログについて](../admin/logs.md)を参照してください。
    
    Fiware-Correlator: 600119ce-eeaa-11e9-9e0c-080027a71049	

[トップ](#top)
	

## 21. X-Auth-Token

X-Auth-Token はオプションの HTTP ヘッダで、Orion はリクエストで受信し、元のリクエストに関連付けられた他のリクエスト
(通知およびフォワードされたクエリ/更新済み) に透過的に伝播します。
[PEP Steelskin など](https://github.com/telefonicaid/fiware-pep-steelskin)の Orion と統合されたセキュリティ・エンフォース・
プロキシによって使用されます。
	
    "X-Auth-Token": "fff0f4af447f4b589c835f805fe4be29"

[トップ](#top)
