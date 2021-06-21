# Orion Context Broker クイックスタートガイド

Orion Context Broker クイックスタートガイド
Orion Context Broker へようこそ！この簡単なガイドでは、簡単な方法で [FIWARE Lab](https://lab.fiware.org) (FIWARE Foundation が所有し、管理) の Orion Context Broker グローバルインスタンスで作業するためのいくつかの初期ステップについて説明します。

Orion Context Broker は、[FIWARE NGSI バージョン2 API](http://fiware-ges.github.io/orion/api/v2/stable/) を実装しています。そのような API の良い学習リソースは、[NGSI version 2 Cookbook](http://fiware-ges.github.io/orion/api/v2/stable/cookbook/) です。

まず、FIWARE Lab にアカウントが必要です。もし、アカウントがなければ、[次のリンク](https://account.lab.fiware.org/sign_up)で登録してください。無料ですが、有効なメールアドレスが必要です。このアカウントを使用すると、Orion への REST API コールで使用する有効な認証トークンを取得できます。そのトークンを取得するには、`token_script.sh` スクリプトを取得し、パラメータとして `orion-gi` を使用して実行します (`orion-gi` は FIWARE インフラストラクチャの Orion グローバル・インスタンスを意味します)。スクリプトで要求されたら、FIWARE Lab のユーザとパスワードを入力してください。**電子メールのドメインを含む完全なユーザ名を使用する必要があります**。例えば、電子メールが "foo@gmail.com" の場合は "foo" だけでなく、"foo@gmail.com" です :

    # wget --no-check-certificate https://raw.githubusercontent.com/FIWARE-Ops/Tools/master/GetToken/get_token.sh
    # bash get_token.sh orion-gi
    Username: your_email@example.com
    Password:
    Token: <this is the token you need>

この方法で生成されたトークンは、[1時間後に期限切れ](https://stackoverflow.com/questions/39835218/orion-context-broker-global-instance-token)になります
が、このガイドの残りの手順を完了するのに十分な時間です。

取得した認証トークンが AUTH_TOKEN シェル変数にあると仮定します。次に、サンタンデールの都市センサ (特に騒音計) からリアルタイム情報を検索してみましょう :

```
curl orion.lab.fiware.org:1026/v2/entities/urn:smartsantander:testbed:357 \
   -X GET -s -S --header 'Accept: application/json' \
   --header  "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
```

最後の測定時間 (TimeInstant), サウンドレベル (sound), センサバッテリ充電 (batteryCharge), センサ位置 (Latitud と Longitud ... これらの最後のものはスペイン語で申し訳ありません)の JSON ドキュメントを取得します。センサは "urn:smartsantander:testbed:357" で識別されます。

道路交通に関連するもう1つのセンサをクエリしましょう :

```
curl orion.lab.fiware.org:1026/v2/entities/urn:smartsantander:testbed:3332 \
   -X GET -s -S  --header 'Accept: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
```

"urn:smartsantander:testbed:3332" センサに関する、返された JSON のデータは次のとおりです :

* 測定時間 (TimeInstant)、
* "交通量" (1時間あたりの車両数)、
* 占有率 (パーセンテージ)、
* センサ位置 (緯度と経度)

Orion Context Broker のグローバルインスタンスは、新しいエンティティの作成にも使用できます。まず、一意のエンティティ ID を選択します。一意性が重要です。Orion Context Broker インスタンスが共有されており、他のユーザのエンティティを変更できます :

    # RANDOM_NUMBER=$(cat /dev/urandom | tr -dc '0-9' | fold -w 10 | head -n 1)
    # ID=MyEntity-$RANDOM_NUMBER
    # echo $ID

次のコマンドは、Orion Context Broker に "city_location" 属性と "temperature" 属性を持つエンティティを作成します :

```
curl orion.lab.fiware.org:1026/v2/entities -X POST -s -S \
   --header 'Content-Type: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d @- <<EOF
{
  "id": "$ID",
  "type": "User",
  "city_location": {
    "value": "Madrid",
    "type": "City"
  },
  "temperature": {
    "value": 23.8,
    "type": "Number"
  }
}
EOF
```

エンティティが存在することを確認するには、パブリック・センサをクエリするのと同じ方法でクエリできます :

```
curl orion.lab.fiware.org:1026/v2/entities/$ID -X GET -s -S \
    --header 'Accept: application/json'\
    --header "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
```
もちろん、温度の変更など、属性の値を変更することもできます :

```
curl orion.lab.fiware.org:1026/v2/entities/$ID/attrs/temperature \
   -X PUT -s -S --header  'Content-Type: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d @- <<EOF
{
  "value": 18.4,
  "type": "Number"
}
EOF
```

または (よりコンパクトに) :

```
curl orion.lab.fiware.org:1026/v2/entities/$ID/attrs/temperature/value \
   -X PUT -s -S --header  'Content-Type: text/plain' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d 18.4
```

上記のクエリ・コマンドを再実行すると、温度が 18.4℃ に変わったことがわかります。

これで、Orion Context Broker のごく小さな紹介が終わりました。この FIWARE enabler (API の詳細、独自のプライベート・インスタンスのデプロイ方法、通知の受信/受信方法、ジオ・ローカリゼーションのクエリなど) について詳しく知りたい場合は、[ドキュメントのホーム](https://github.com/telefonicaid/fiware-orion)にアクセスしてください。
