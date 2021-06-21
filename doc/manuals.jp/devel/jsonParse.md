# <a name="top"></a>JSON Parse NGSIv1

* [イントロダクション](#introduction)
* [パーシング・プロセス](#parsing-process)
	* [実装の詳細](#implementation-details)
* [トップ・レベル `jsonParse()`](#top-level-jsonparse)
* [ロー・レベル `jsonParse()`](#low-level-jsonparse)

<a name="introduction"></a>
## イントロダクション

Orion Context Broker には JSON パーシングのためのライブラリが1つではなく2つあります。これは、もともと NGSIv1 JSON のパーシング用に選択された外部ライブラリは、String, Number, Boolean, Null などの JSON 値の型を区別できず、すべての値を文字列として扱うためです。これは NGSIv2 では受け入れられず、別の外部 JSON ライブラリ [rapidjson](http://rapidjson.org/) が選ばれました。Orion の2つの JSON ライブラリは、Orion が使用できるように外部ライブラリを必要に応じて実装しています。

このドキュメントでは、NGSIv1 のパーシングの詳細について説明します (NGSIv1 は廃止されていることに注意してください)。NGSIv2 のパーシングの詳細は[別のドキュメント](jsonParseV2.md) に記述されています。

一般に、NGSIv1 パーシング・ロジック は NGSIv2 ロジックより複雑です。NGSIv1 のパーシングで何かを変更する必要はないでしょう。これは Orion API の古いバージョンであり、NGSIv1 ではなく NGSIv2 の開発に集中する必要があるためです。

パーシング・ステップの目的は、この場合のテキスト・バッファの JSON を C++ のクラス/構造体のインスタンスに変換することです。外部ライブラリは JSON 文字列をパーシングし、Orion ライブラリは情報を抽出し、C++ のクラス/構造体のインスタンスを生成します。

非常にシンプルですが説明的な例です : 

`POST /v1/queryContext` の次のペイロード... : 

```
{
  "entities": [
    {
      "type": "Room",
      "id": "ConferenceRoom"
    }
  ],
  "attributes": [ "temperature" ]
}
```

... このような `QueryContextRequest` クラスの C++ インスタンスに変換されます :

```
QueryContextRequest* qprP = new QueryContextRequest();
EntityId*            eP   = new EntityId();

eP->id   = "ConferenceRoom";
eP->type = "Room";

qprP->entityIdVector.push_back(eP);
qprP->attributeList.push_back("temperature");
```

`QueryContextRequest` のこのインスタンスは **jsonParse** ライブラリによって作成され、サービス・ルーチン `postQueryContext()` はそれを [**mongoBackend**](sourceCode.md#srclibmongobackend) 関数 `mongoQueryContext()`に渡します。

[Top](#top)

<a name="parsing-process"></a>
## パーシング・プロセス

**jsonParse** ライブラリには、`jsonParse()` という名前を持つ2つのオーバーロードされた関数が含まれています。

* 最初の `jsonParse()` は、リクエストごとに1回だけ呼び出されるトップ・レベル関数です。[トップ・レベルの jsonParse() の専用セクション](#top-level-jsonparse)を参照してください。
* 2番目の`jsonParse()` は、[boost property_tree](https://theboostcpplibraries.com/boost.propertytree) から出力される、パースされたツリー内のノードごとに、**トップ・レベルの** `jsonParse()` によって呼び出されます。そして、それは出力ツリーの後に*再帰的に呼び出されます*。この2番目のロー・レベルの `jsonParse()` をトップ・レベルの `jsonParse()` 関数と区別するために `jsonParse()*` を使用します。 [ロー・レベルの jsonParse()の専用セクション](#low-level-jsonparse) に詳しい説明があります。

次の図でで使用される具体例は、`POST /v1/updateContextRequest` のペイロードのパーシングです。

<a name="flow-pp-01"></a>
![Parsing an NGSIv1 payload](../../manuals/devel/images/Flow-PP-01.png)

_PP-01: NGSIv1 ペイロードのパーシング_

* `payloadParse()` は、呼び出し可能な3つのパーシング関数の1つである JSON ペイロードのための NGSIv1 パーシング関数を呼び出します : NGSIv1 JSON, NGSIv2 JSON およびテキストのパーシング (ステップ1)
* `jsonTreat()`は、`jsonRequestGet()`を呼び出してリクエストのタイプを調べ、ペイロードのパースに必要な  `JsonRequest` 構造体へのポインタを返します (ステップ2)
        * ペイロードの種類ごとに、共通のパース・ルーチンとは異なる入力が必要です。`JsonRequest` 構造体のベクトルはこの情報を含み、`jsonRequestGet()` はベクトルの対応する `JsonRequest` 構造体をルックアップして返します。後の  `JsonRequest` 構造体の詳細を参照して下さい
* リクエスト・タイプの特定の情報を知っている `jsonTreat()` はトップ・レベルの `jsonParse()` を呼び出します。その責任はペイロードのパーシングを開始することです (ステップ3)。`jsonParse()` はペイロードを `stringstream` に読み込み、ペイロードのパーシングを処理する `read_json()` 関数を呼び出し、ペイロードをプロパティ・ツリーに変換します
* その後、ブースト・プロパティ・ツリーを Orion 構造体に変換するために、結果のツリーに `jsonParse()*` (ロー・レベル) が呼び出されます(ステップ4)。実際、`jsonParse()*` は、この時点で JSON のトップ・レベル・キーが処理される回数だけ呼び出されます
	* 例 : `{ "a": ..., "b": ..., "c:"... }`, `jsonParse()*` は、3回呼び出されます。`"a"`, `"b"` および `"c"` のために一度ずつです
* `jsonParse()*` は各ノードで `treat()` 関数を呼び出し (ステップ5)、ノードがリーフでない場合、ノードの各子ノードに対して自己への再帰呼び出しを行います
* `treat()` 関数は、ペイロード内の禁止文字をチェックし、問題のノードの特定の Parse-Function を呼び出します (ステップ6)。この特定の Parse-Function へのポインタは、構造体がどのように見つけられるかということであり、各ノードへのパスと同様に、`JsonRequest` 構造体 にあります
* Parse-Function はツリー・ノードから情報を抽出し、パース結果全体の結果である Orion 構造体に追加します。ツリーの各ノードには独自の Parse-Function があり、この図では選択された Parse-Functions のほんの一部が示されていることに注意してください。実際、この `UpdateContextRequest` ペイロードをパースするために、19個以上の Parse-Functionsがあります。`jsonParse/jsonUpdateContextRequest.cpp` を参照してください。

[Top](#top)

<a name="implementation-details"></a>
### 実装の詳細
前述のように、`src/lib/jsonParse/jsonRequest.cpp` の `jsonTreat()` は `src/lib/rest/RestService.cpp` の `payloadParse()` によって呼び出されます。`jsonTreat()` を見る前に、関数で非常に重要な役割を果たす `JsonRequest` 構造体を見てみましょう : 

```
typedef struct JsonRequest
{
  RequestType      type;          // Type of request (URI PATH translated to enum)
  std::string      method;        // HTTP Method (POST/PUT/PATCH ...)
  std::string      keyword;       // Old reminiscent from XML parsing
  JsonNode*        parseVector;   // Path and pointer to entry function for parsing
  RequestInit      init;          // pointer to function for parse initialization
  RequestCheck     check;         // pointer to	function for checking of the parse result
  RequestPresent   present;       // pointer to	function for presenting the parse result in log file
  RequestRelease   release;       // pointer to	function that frees up memory after the parse result has been used
} JsonRequest;
```

ペイロードでサポートされるリクエストの完全なリストについては、`src/lib/jsonParse/jsonRequest.cpp` に `JsonRequest` のベクトルである `jsonRequest` 変数も参照してください。

`jsonTreat()` が行う最初のことは、`jsonRequestGet()` を呼び出して、`jsonRequest` ベクトル内の項目を検索することです。ベクトル項目を検索する検索条件は、リクエストの URL パスと使用される、**HTTP Method** に依存する、**RequestType** です。

URL パスと HTTP Method の組み合わせが `JsonRequest` ベクトル (`jsonRequest`) に見つからない場合、リクエストは有効ではなく、エラーが返されます。 見つかった場合、ベクトル項目は、ペイロードをパースし、対応する未処理構造体を構築するために必要なすべての情報を含んでいます。

#### ペイロードで新しいリクエストを追加

パーシングするペイロードを持つ NGSIv1 でリクエストを追加するには、項目を `jsonRequest` ベクトルに**追加する必要があります**。

次に、リクエストのベクトル項目を見つけたら、`jsonTreat()` は、以下を実行します :

* `init()`
* `parse()`
* `check()`

`release()` は、パース結果が使われるまで呼び出すことができません。 すなわち、エラーが検出されなかった場合です。 エラーの場合、`release()` 関数は payloadParse の呼び出しの直後に呼び出されます。その結果はガベージであり、使用できません。通常、パースは正常に動作し、パース・ステップの結果のインスタンスは処理のために mongoBackend に渡され、mongoBackend の処理が完了するまで release 関数を呼び出すことはできません。対応するサービス・ルーチンは mongoBackend を呼び出し、`restService()` はサービス・ルーチンを呼び出します :

```
std::string response = serviceV[ix].treat(ciP, components, compV, &parseData);
```

サービス・ルーチンから復帰した後、パーシングの結果は危険なく解放されます。

[Top](#top)

<a name="top-level-jsonparse"></a>
## トップ・レベル `jsonParse()`
前述のように、`src/lib/jsonParse/jsonParse.cpp` に `jsonParse()` という2つの異なる関数があります。1つのトップ・レベルと1つのよりロー・レベルです。トップ・レベルの `jsonParse()` はエントリ関数であり `src/lib/jsonParse/jsonParse.cpp` の外部から見ることができます :

```
std::string jsonParse
(
  ConnectionInfo*     ciP,          // Connection Info valid for the life span of the request
  const char*         content,      // Payload as a string
  const std::string&  requestType,  // The type of request (URL パス)
  JsonNode*           parseVector,  // Function pointers etc for treatment of the nodes
  ParseData*          parseDataP    // Output pointer to C++ classes for the result of the the parse
)
```

この関数は、`src/lib/jsonParse/jsonRequest.cpp` の `payloadParse()` のよって呼び出されます。これは、`src/lib/rest/RestService.cpp` の `payloadParse()` よって、順番に呼び出されます。

この関数の目的は、`boost::property_tree::ptree` の助けを借りて、コンテンツ (パラメータ内の JSON 文字列) のパーシング を開始することです。

* リクエストされた場合、タイミング統計の開始時間を取得します
* *エスケープされた文字*を修正します。すなわち、スラッシュの前のバックスラッシュを削除します: `"\/"` => `"/"` 
* `ptree` 変数 `tree` に `content` を読み込みます
* ツリーの各第1レベル・ノードに対してロー・レベルの `jsonParse()` を呼び出します。ロー・レベルの `jsonTreat()` はより深く潜ります
* ロー・レベルの `jsonTreat()` が失敗した場合、**Error** を返します
* リクエストされた場合、タイミング統計情報の終了時刻を取得し、後で使用するために差分時間を保存します

[Top](#top)

<a name="low-level-jsonparse"></a>
## ロー・レベル jsonParse()
ロー・レベル  `jsonParse` は `src/lib/jsonParse/jsonParse.cpp` の中で、静的であり、自身が行う再帰呼び出しを除いて、高水準の `jsonParse()` によってただ呼び出されるのみです。

その署名 : 

```
static std::string jsonParse
(
  ConnectionInfo*                           ciP,          // "Global" info about the current request
  boost::property_tree::ptree::value_type&  v,            // The node-in-the-tree
  const std::string&                        _path,        // The path to the node-in-the-tree 
  JsonNode*                                 parseVector,  // Function pointers etc for treatment of the nodes
  ParseData*                                parseDataP    // Output pointer to C++ classes for the result of the the parse
)
```

さまざまなパラメータを1つずつ説明しましょう。

### `ConnectionInfo* ciP`

この `ConnectionInfo` へのポインタは、MHD (libmicrohttpd) が、`src/lib/rest/rest.cpp` の中で `connectionTreat` リクエストを読み込む際にコールバックに使用する関数によって生成されます。`ciP` は以下のようなリクエストに関する情報を含んでいます :

* HTTP メソッド/動詞
* HTTP ヘッダ
* URI パラメータ (例 : `?a=1&b=2`)
* URI パス (例 : `/v1/queryRequest`) ...
* ... そして、他にも。`src/lib/rest/ConnectionInfo.h` を参照してください

`ConnectionInfo` へのポインタは、**jsonParse**, **jsonParseV2**, **rest**, **serviceRoutines** および **serviceRoutinesV2** のライブラリの多くの関数に渡されます。

### `boost::property_tree::ptree::value_type& v`

これはツリー内で現在処理されているノードへの参照です。それについてはあまり説明したくありません。詳細については、[boost_type_treeのドキュメント](https://theboostcpplibraries.com/boost.propertytree)を参照してください。

### `const std::string& _path`

`jsonParse()` は、ノードへのパスを文字列として保持し、ツリー内のどのノードが処理されているかを正確に把握します。たとえば :

```
{
  "entities": [
    {
      "type": "Room",
      "id": "ConferenceRoom"
    }
  ],
  "attributes": [ "temperature" ]
}
```

`type`ノードには、`/entities/entity/type` パスがあります。中間名の `entity` は、`entities` はベクトルだからです。これについては後で詳しく説明します。

### `JsonNode* parseVector`

`JsonNode` は、`src/lib/jsonParse/JsonNode.h` で、次のように定義された構造体です。

```
typedef std::string (*JsonNodeTreat)(const std::string& path, const std::string& value, ParseData* reqDataP);

typedef struct JsonNode
{
  std::string    path;
  JsonNodeTreat  treat;
} JsonNode;
```

`JsonNode` インスタンスは、ノードのパスを含んでいます。たとえば、`/entities/entity/type` と、そのまさにそのパスを持つノードの対応する treat-function への参照です。これは `jsonTreat` がツリー内の各ノードに対してどの treat-function を呼び出すかを知る方法です。例として、`src/lib/jsonParse/jsonQueryContextRequest.cpp` の `jsonQcrParseVector` 変数を参照してください：

```
JsonNode jsonQcrParseVector[] =
{
  { "/entities",                                                           jsonNullTreat           },
  { "/entities/entity",                                                    entityId                },
  { "/entities/entity/id",                                                 entityIdId              },
  { "/entities/entity/type",                                               entityIdType            },
  { "/entities/entity/isPattern",                                          entityIdIsPattern       },
  ...
```

説明したように、これは **path-in-the-tree** と対応する **treat-function** のベクトルであり、これはロー・レベルの `jsonParse()` が、ツリー内の各ノードに対して、どの treat-function をそれぞれ呼び出すかを知る方法です 。

このベクトルと他のベクトル (ペイロードのタイプごとに1つ) は、`JsonRequest` タイプの `src/lib/jsonParse/jsonRequest.cpp` の `jsonRequest` 変数によって使用されます。

```
typedef struct JsonRequest
{
  RequestType      type;
  std::string      method;
  std::string      keyword;
  JsonNode*        parseVector;
  RequestInit      init;
  RequestCheck     check;
  RequestPresent   present;
  RequestRelease   release;
} JsonRequest;
```

`jsonRequest` は `JsonRequest` のベクトルであり、`jsonParse` によってパースされるすべてのペイロードを定義します。` jsonRequest` の詳細については、[実装の詳細](#implementation-details)を参照してください。

`JsonRequest` ベクトルは `src/lib/jsonParse/jsonRequest.cpp` で宣言されています :

```
static JsonRequest jsonRequest[] =
{
  // NGSI9
  { RegisterContext,                       "POST", "registerContextRequest",                        FUNCS(Rcr)   },
  { DiscoverContextAvailability,           "POST", "discoverContextAvailabilityRequest",            FUNCS(Dcar)  },

  // NGSI10
  { QueryContext,                          "POST", "queryContextRequest",                           FUNCS(Qcr)   },
  { UpdateContext,                         "POST", "updateContextRequest",                          FUNCS(Upcr)  },
  ...
};
```

マクロ `FUNCS()` は、行を少し短くすることで、次のようになります :

```
#define FUNCS(prefix) json##prefix##ParseVector, json##prefix##Init,    \
                      json##prefix##Check,       json##prefix##Present, \
                      json##prefix##Release
```

つまり、ある種類のペイロードをパーシングするためのすべての "メソッド" です。これらの関数は、ペイロードのタイプごとに1つのモジュールに存在します :

* `src/lib/jsonParse/jsonQueryContextRequest.cpp`
* `src/lib/jsonParse/jsonUpdateContextRequest.cpp`
* など

"メソッド" は以下のものです :

* `init()`,
* `check()`,
* `present()`, および
* `release()`

ペイロードの種類ごとに1組の "メソッド" があります。

だから、`jsonXxxRequest` または `jsonXxxResponse` (`QueryContent` や `UpdateContent` などのペイロードの名前である  `Xxx`) と呼ばれるモジュールはすべて以下を含みます :

* このメソッドのセット : `init()`, `check()`, `present()`, `release()`
* 各ノードのパスと処理メソッドを含む *Parse-Vector*
* すべてのノードのメソッドを扱います

これは、`jsonParse()` が JSON 入力ペイロード文字列を C++ クラスのインスタンスに変換するために必要な関数と変数のセット全体です。

ここで、treat-methods のいくつかの例は、とても単純です : 

```
/* ****************************************************************************
*
* entityId -
*/
static std::string entityId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP = new EntityId();

  reqDataP->qcr.entityIdP->id        = "";
  reqDataP->qcr.entityIdP->type      = "";
  reqDataP->qcr.entityIdP->isPattern = "false";

  reqDataP->qcr.res.entityIdVector.push_back(reqDataP->qcr.entityIdP);

  return "OK";
}



/* ****************************************************************************
*
* entityIdId -
*/
static std::string entityIdId(const std::string& path, const std::string& value, ParseData* reqDataP)
{
  reqDataP->qcr.entityIdP->id = value;

  return "OK";
}
```

また、これら2つの treat-function が存在する Parse-Vector を見てみましょう :

```
/* ****************************************************************************
*
* qcrParseVector -
*/
JsonNode jsonQcrParseVector[] =
{
  { "/entities",                                                           jsonNullTreat           },
  { "/entities/entity",                                                    entityId                },
  { "/entities/entity/id",                                                 entityIdId              },
  ...
```
treat-function `entityId` は、`/entities/entity` ノードが見つかると呼び出されます。`entities` は、`QueryContextRequest ` のペイロード内のベクトルです :

```
{
  "entities": [
    {
      "id": "",
      "type": "",
      ...
    }
}
```

ベクトル項目には JSON ではキー名がないため、常に複数形のベクトルの名前の単数形を使用することにしました 。つまり、**entities** のインスタンスは **entity** と呼ばれます。したがって、`/entities/entity` ノードがエンティティ・ベクトルの項目に見つかると、treat-function `entityId()` が呼び出され、`EntityId` (`class EntityId` は  `src/lib/ngsi/EntityId.h/cpp` モジュール にあります) を呼び出し、`EntityId` ポインタを `reqDataP->qcr.res.entityIdVector` にプッシュします。

treat-function `entityId()` は、また、`reqDataP->qcr.entityIdP` を設定して、最新の `EntityId` インスタンスを参照して、後続の treat-functions がそれに到達できるようにします。 例えば、`entityIdId()` はエンティティの `id` フィールドを設定するために必要です。すべて `entityIdId()` が行います。このタイプのポインタは、パース/抽出中に必要であり、これらのポインタが ParseData 構造体の理由です。

### `ParseData* parseDataP`

NGSIv1 ペイロードのパーシングは強く中央集権化され、関数ポインタが必要であるため、**すべての**タイプのペイロードに固有のタイプが必要です。 パース結果を格納する型 (C++ クラスのインスタンスは各ペイロードのタイプごとに異なります) はすべて、すべてのタイプのペイロードを含む大きな構造体にまとめられます。 次に、各 treat-function は、どのフィールドを操作するかを選択します。

この構造体へのポインタは、ロー・レベルの `jsonParse()` にパラメータとして渡されます。

`ParseData`  は、`src/lib/ngsi/ParseData.h` にあります : 

```
typedef struct ParseData
{
  std::string                                 errorString;
  ContextAttribute*                           lastContextAttribute;

  RegisterContextData                         rcr;
  DiscoverContextAvailabilityData             dcar;
  ...
} ParseData;
```

たとえば、NGSI10 クエリのパーシングは、 `QueryContextData` タイプの `ParseData::qcr` で動作します。 `QueryContextData` には、パースの結果が格納される `QueryContextRequest` のインスタンスが含まれています。しかし、パース中にヘルプ変数が必要になると、これらの "XxxData 構造体"が使用され、それらは出力インスタンス (`QueryContextData` の場合は `QueryContextRequest`) **と**  `QueryContextRequest`のパーシングに必要なヘルプ変数を含みます :

```
struct QueryContextData
{
  QueryContextRequest  res;           // Output/Result of the parse
  EntityId*            entityIdP;     // Pointer to the current EntityId
  Scope*               scopeP;        // Pointer to the	current Scope
  orion::Point*        vertexP;       // Pointer to the current Point
  int                  pointNo;       // Index of the current Point
  int                  coords;        // Number of coordinates
};
```

各 XxxData 構造体には異なるヘルプ変数のセットがあります。

[Top](#top)
