# JSON パース NGSIv2
NGSIv2 ペイロードは、NGSIv1 ペイロードとは非常に異なる方法でパーシングされます。このドキュメントでは、NGSIv2 のパーシングの詳細について説明します。NGSIv1 のパーシングの詳細は、[別のドキュメント](jsonParse.md)に記載されています。

NGSIv1 パーシングの集中化されたアプローチの代わりに、個々のアプローチが使用されます。このアプローチの利点は、コードが理解しやすく、再利用することがはるかに簡単であることです。 サポートされていないフィールドのチェックは多くの異なる機能に広がっており、このようにしてこれらのチェックのいくつかは忘れやすいです。
開発チームはこの第2のアプローチを好みます。

## パーシングプロセス
NGSIv2 のパーシングの流れを記述するために、コードは一般的ではなく、リクエストの種類ごとに個別のペイロードが必要です。

リクエスト `POST /v2/entities` は次のフロー例で使用されます :

<a name="flow-pp-03"></a>
![Parsing an NGSIv2 payload](../../manuals/devel/images/Flow-PP-03.png)

_PP-03: NGSIv2ペイロードのパーシング_

* `payloadParse()` は、JSON ペイロード `jsonRequestTreat()` の NGSIv2 パーシング関数を呼び出します (ステップ1)
* `jsonRequestTreat()` にはリクエスト・タイプのスイッチが含まれ、リクエストのタイプの初期パーシング関数が呼び出されます。この例では、リクエスト・タイプは `EntitiesRequest` で、最初のパーシング関数は `parseEntity()` です (ステップ2)
* [rapidjson](http://rapidjson.org) が呼び出され、ペイロード全体をパースし、現在 Orion が理解できる NGSI 構造に変換されるノードのツリーを返します。パーシングに使用するメソッドは  `rapidjson::Document::Parse()` です (ステップ3)
* `parseEntity()`は、`Entity::Id`, `Entity::Type` などを抽出し、各属性の基底関数 `parseContextAttribute()` を呼び出します (ステップ4)
* `parseContextAttribute()`は、`Attribute::name`、タイプなどを抽出し、メタデータが存在する場合には `parseMetadataVector()` 関数を呼び出します (ステップ5)
* `parseMetadataVector()` は、ベクトル内の各メタデータに対して `parseMetadata()` を1回呼び出します (ステップ6)
* パースが完了したら、オブジェクトの `check()` メソッドを呼び出すことで、NGSI オブジェクトが正しいことが確認されます (ステップ7)

重要な関数である `parseEntity()` のソースコードを参照してください。この例を短くするために、ここでは仮想関数/マクロ `ERROR` を使用していますが、完全な関数は `src/lib/jsonParseV2/parseEntity.cpp` で見ることができます。

関数の各ステップを説明するコメントが挿入されています。

```
std::string parseEntity(ConnectionInfo* ciP, Entity* eP, bool eidInURL)
{
  // 1. Parse the incoming payload to convert the textual payload into a tree in RAM - this is **rapidjson**
  document.Parse(ciP->payload);

  // 2. Error checks
  if (document.HasParseError())                             ERROR("JSON Parse Error");
  if (!document.IsObject())                                 ERROR("Entity must be a JSON object");
  if ((eidInURL == false) && (!document.HasMember("id")))   ERROR("Entity ID not present");
  if ((eidInURL == true)  && (document.HasMember("id")))    ERROR("Entity ID both as URI parameter and in payload");
  if ((eidInURL == true)  && (document.HasMember("type")))  ERROR("Entity Type in Payload when Entity ID as URI parameter"));
  if (document.ObjectEmpty())                               ERROR("Empty entity");

  // 3. loop over the first level members of the payload (id, type, and attributes)
  for (Value::ConstMemberIterator iter = document.MemberBegin(); iter != document.MemberEnd(); ++iter)
  {
    std::string name = iter->name.GetString();
    std::string type = jsonParseTypeNames[iter->value.GetType()];

    // 4. Entity::id present?
    if (name == "id")
    {
      if (eidInURL == false)
      {
        if (type != "String") ERROR("Entity ID must be a string");

        eP->id = iter->value.GetString();

        if (forbiddenIdChars(ciP->apiVersion, eP->id.c_str(), "")) ERROR("Forbidden Characters in Entity ID");
      }
    }
    // 5. Entity::type present?
    else if (name == "type")
    {
      if (type != "String")  ERROR("Entity Type must be a string");

      eP->type      = iter->value.GetString();
      eP->typeGiven = true;

      if (eP->type.empty())  ERROR("Entity Type is present but empty");

      if (forbiddenIdChars(ciP->apiVersion, eP->type.c_str(), "")) ERROR("Forbidden Characters in Entity Type");
    }
    // 6. Not 'id' nor 'type' - must be an Attribute
    else
    {
      ContextAttribute* caP = new ContextAttribute();
      
      eP->attributeVector.push_back(caP);

      // 7. Extract the attribute from the parsed tree by calling lowlevel parse function 'parseContextAttribute()'
      if (parseContextAttribute(ciP, iter, caP) != "OK")
        ERROR("Error parsing attribute");
    }
  }

  // 8. More checks: Entity::id present but empty
  if ((eidInURL == false) && (eP->id == ""))    ERROR("Empty Entity ID");

  // 9. Set default value for entity type ("Thing")
  if (!eP->typeGiven)
    eP->type = DEFAULT_ENTITY_TYPE;

  return "OK";
}
```

`parseEntity()` は、トップ・レベルのパーシング関数なので、**rapidjson** を呼び出して RAM にツリーを作成する必要があります。`parseContextAttribute()`, `parseMetadataVector()`などの低レベルのパーシング関数は、トップ・レベルのパーシング関数で行われるように、実際には何もパーシングしません。代わりに、低レベル関数はツリーのその部分を調べるだけで、パラメータとして関数に渡されます。ツリーの一部を参照するパラメータは通常は `rapidjson::Value` タイプですが、それに対する、`rapidjson::Value::ConstMemberIterator` または `rapidjson::Value::ConstValueIterator` タイプのパラメータとして送られることがあります。

`src/lib/jsonParseV2` の下には、文字列のベクトルをパースする `parseStringList.h/cpp` のようにリクエスト全体をパースするためのモジュールがいくつかあります。このドキュメントを書いている時点では16です。

## jsonRequestTreat()
`jsonRequestTreat()` 関数は、NGSIv 2のパーシングのエントリ・ポイントであり、この関数は、ペイロードのタイプを調べることで、トップレベルのパーシング関数を呼び出します。

パーシング後、結果のインスタンスの `check()` メソッドが呼び出されます。`jsonRequestTreat()` によって呼び出されるすべてのパーシング関数は、もちろん最上位のパーシング関数です :

* `parseEntity()`
* `parseContextAttribute()`
* `parseAttributeValue()`
* `parseSubscription()`
* `parseBatchQuery()`
* `parseBatchUpdate()`

これらの関数のいずれかによって呼び出されるすべてのパーシング関数は、もちろん低レベルのパーシング関数です。
