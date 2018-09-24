# <a name="top"></a>Orion 実装クックブック

* [コマンドライン・パラメータの追加](#adding-a-command-line-parameter)
* [REST サービスの追加](#adding-a-rest-service)
* [機能テスト・ケースの追加](#adding-a-functional-test-case)
* [機能テスト・ケースのデバッグ](#debug-a-functional-test-case)
* ['405 Method Not Allowed' のキャッチ](#catching-a-405-method-not-allowed)
* [メモリ・リークの修正](#fixing-a-memory-leak)

<a name="adding-a-command-line-parameter"></a>
## コマンドライン・パラメータの追加
Orion に新しい CLI パラメータを追加するのは簡単です。これは、CLI パラメータのパーシングとチェックを担当するライブラリがあるためです。このライブラリ ([**parseArgs**](sourceCode.md#srclibparseargs)) は、最初のアクションの一つとして、`contextBroker.cpp` の[メイン・プログラム](sourceCode.md#srcappcontextbroker)によって呼び出されます。CLI 引数をパーシングする関数は、`parseArgs()` で、次の3つのパラメータがあります :

* `argC`, メイン・プログラムの引数の数
* `argV`, メイン・プログラムの引数ベクトル
* `paArgs`, Broker が認識する CLI パラメータを記述するベクトル

基本的に2つのことを実装する必要があります :

* 新しい CLI パラメーターの値を保持する**変数**、および
* `PaArgument` ベクトルの**新しい項目** `paArgs`

新しいCLIパラメーターが `-v` (verbose) のようなブール値の場合、`bool` 変数が必要です。`-dbHost <host name>` のようなテキスト・パラメータの場合は、char-vector が使用されるなどです。

最も簡単な方法は、同じタイプの古い CLI パラメータを単純にコピーすることです。

`PaArgument` ベクトルの項目 `paArgs` には、9つの異なる情報が含まれています :

* CLI オプションの名前
* パーシング後にその値を保持する変数へのポインタ
* 環境変数の名前 (はい、オプションには、環境変数も渡すことができます)
* CLI パラメータ変数のタイプ : 
    * `PaBool`
    * `PaString`
    * `PaInt`
    * `PaDouble`
    * ... (`src/lib/parseArgs/parseArgs.h` の`PaType` enum を参照してください)

* CLI パラメータ自体のタイプ :
    * `PaOpt`, **オプション**のパラメータ
    * `PaReq`, **必須**パラメータ
    * `PaHid`, **隠し**パラメータ (`usage()`で表示されません)
* デフォルト値 (パラメータが指定されていない場合に使用される値)
* 最小値 (最小値が必要ない場合に、`PaNL` を使用)
* 最大値 (最大値が必要ない場合に、`PaNL` を使用)
* `usage()` 関数に使用される説明の文字列

**注意** : 

* Boolean CLI パラメータは2つだけの可能な値をとることができます : true か false。コマンドラインには値は追加されず、オプション自体だけが追加されます。例えば、ちょうど `-port <port number>` とは対照的な`-fg` です
* あまり意味がありませんが、文字列には最小値と最大値はありませんので、`PaNL`は、常に文字列に使用されます
* `PaArgument` の2番目の項目はポインタでなければならないので、文字列 (文字ベクトルはポインタ) でなければ、値を保持する変数の参照 (`&x`) を渡す必要があります
* `PaArgument` (デフォルト値) の6番目の項目は整数 (`long long` ) です。したがって、デフォルト値が文字列の場合、整数に型キャストする必要があります。これには特別なマクロ(`_i`)があります

### 例 : 整数の CLI パラメータ `-xyz` を追加
現実的な例として、`-xyz` と呼ばれる整数の CLI パラメーターを追加することができます。

`src/app/contextBroker/contextBroker.cpp` を編集して、既存の整数 CLI パラメータを検索します。

1. 整数変数 `xyz` を作成します。ここで `int port` は `Option variables` コメントを検索します

2. xyz のために、`PaArgument` の行を追加します
    * `PaArgument paArgs[]` を検索してください
    * そのベクトルの中で、4番目の項目として `PaInt` を持つベクトル項目を探します : `-port` パラメータが見つかります : 

      `{ "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },`


    * その行をコピーし、コピーした行で、`port` を `xyz` に変更します。以下の通りです : 

    ```
       { "-port", &port, "PORT", PaInt, PaOpt, 1026, PaNL, PaNL, PORT_DESC },
       { "-xyz",  &xyz,  "XYZ",  PaInt, PaOpt, 1026, PaNL, PaNL, XYZ_DESC  },
    ``` 

3. `PORT_DESC` の直後に、説明文字列 `XYZ_DESC` を作成します 
4. `-xyz` が "必須オプション" である場合、`PaOpt` を `PaReq` に変更します。隠しパラメータの場合は、`PaHid` です
5. `-xyz` のデフォルト値の `1026` を変更します。たとえば、`47`
6. `-xyz` の最小値と最大値を設定します (`PaArgument` 行内の項目7と項目8)
7. Brokerをコンパイルします (`make debug install`)
8. `contextBroker -u` を実行すると、以下が表示されます (`PaHid` 使用されていない限り) :
    `[option '-xyz <description of xyz>]`
9. `contextBroker -U` を実行すると、デフォルト値、最小値、最大値など、CLI パラメータに関する詳細が表示されます
10. `-xyz` に最小/最大制限を指定した場合は、無効な値で Broker を起動して、それが不正であることを確認してください
11. `-xyz` に `PaReq` を作成した場合は 、`-xyz` を指定しないで、Broker を起動して、何が起こるかを見てください
12. `-xyz` に `PaHid` を作成した場合は、`contextBroker -u` の実行中に、それが見えないことを確認してください

オプションとしての環境変数に関する注意 :

* 組み込み環境変数には `ORION_` というプレフィックスが付きます (`paConfig("builtin prefix", ...)` へのコールを見てください) `-t`, `-logDir` などの組み込み CLI オプションは、それらの環境変数ためのプレフィックスを得ます

`PaArgument` ベクトル・項目の3番目の項目に `XYZ` という環境変数名を指定した場合、プレフィックスを尊重して、`ORION_XYZ` と呼んでください。つまり、実際にはオプションのために環境変数を使用したことはなく、`ORION_` プレフィックスは現在の実装では尊重されていません。これはちょっと残念ですが、修正するのは本当に簡単です (もちろん、下位互換性の問題は数えません;-))
ただし、環境変数と各 CLI オプション の詳細情報を表示するために `-U` CLI オプションを試してください。

環境変数を使用して CLI オプションを設定するには、これをテストとして実行します :

```
% export FOREGROUND=1
% contextBroker -U  # UPPERCASE U !
Extended Usage: contextBroker  [option '-U' (extended usage)]                       TRUE /FALSE/  (command line argument)
...
                               [option '-fg' (don't start as daemon)]  FOREGROUND   TRUE /FALSE/  (environment variable)
...
% unset FOREGROUND
```

`-fg` オプションの `(environment variable)` と書いてある右端の列に注意してください。
言って一番右の列に注意(environment variable)するために `-fg` オプションを選択します。これは、`-fg` の値が環境変数 (`FOREGROUND`) から取得され、`FOREGROUND` が存在する (設定されていない) 限り、Orion はフォアグラウンドで起動します。

[Top](#top)

<a name="adding-a-rest-service"></a>
## REST サービスの追加
Orion Context Broker がサポートする REST サービスは、[orionRestServices.cpp](sourceCode.md#srcappcontextbroker) にある 7つの `RestService` なベクトル ` restServiceV` の項目です。Orion がサポートする HTTP Method/Verb ごとに1つのサービス・ベクトルがあります。GET, PUT, POST, PATCH, DELETE, OPTIONS および 'bad verb' の特殊なベクトルです。サポートされているサービスのセットはたくさんの役割を定義し、1つの `RestService` ベクトルまたは他のもので REST インタフェースを起動することによって、broker ができることを定義します。そのすべてのサービスはこれらの7つのベクトルに含まれます。

Orion に REST サービスを追加するには、`RestService xxxServiceV[]` (サービスの動詞である `xxx` (`get`, ` put` など)) の新しい項目が必要です。CLI パラメータと同様に、最も簡単な方法は、古いサービス (`xxxServiceV` の項目) をコピーし、必要に応じてコピーを変更することです。

`RestService` ベクトル のこの新しい項目を理解するには、`src/lib/rest/RestService.h` の構造体 `RestService` を見てください :

```
typedef struct RestService  
{  
  RequestType   request;          // The type of the request  
  int           components;       // Number of components in the URL path  
  std::string   compV[10];        // Vector of URL path components. E.g. { "v2", "entities" }  
  std::string   payloadWord;      // No longer used, should be removed ... ?  
  RestTreat     treat;            // service function pointer  
} RestService;
```

だから、RestService ベクトル `putServiceV` が次のような場合、例えば `PUT /v2/entities/{EntitId}/attrs/{AttributeName}/metadata/{MetadataName}` のような、REST サービスを追加します :

```
{ Metadata,  7, { "v2", "entities", "*", "attrs", "*", "metadata", "*" }, "", putMetadata }
```

注意 :

* 項目1 : `Metadata` は、`src/lib/ngsi/Request.h` の `enum RequestType` の enum 定数として追加されなければなりません
* 項目3 : `"*"`。コンポーネント・ベクトル  `RestService::compV` のアスタリスクは ANY 文字列と一致し、エンティティ ID、属性名などを含むパスが定義されている場合は必ず、`"*"` を必ず使用する必要があります
* 項目5 : `putMetadata()` は、`PUT /v2/entities/*/attrs/*/metadata/*`のサービス・ルーチンであり、関数を実装しなければなりません。NGSIv2 サービス・ルーチン用のライブラリのディレクトリは、`src/lib/serviceRoutinesV2` です。[ライブラリの説明](sourceCode.md#srclibserviceroutinesv2)を参照してください

また、`orionRestServices.cpp`では、これらの` RestService` ベクトル行は実際には長く、スタイル・ガイドは長すぎる行に反していることに注意してください。 しかし、定義を使用して行を短くするだけでは、コードを理解しにくくなるので、必要ではありません。

> サイドノート : [スタイル・ガイド](../contribution_guidelines.md#s9-line-length)によれば、ソースコードの行は 120文字を超えるべきではありません (**shouldn't**)

サービス・ルーチン `putMetadata()` は、`src/lib/serviceRoutinesV2/putMetadata.h/cpp` に常駐し、署名は次のようにする必要があります :

```
std::string putMetadata  
(  
  ConnectionInfo*            ciP,  
  int                        components,  
  std::vector<std::string>&  compV,  
  ParseData*                 parseDataP  
)  
```

`entity id`, `attribute name` および `metadata name` (URL パスのすべての部分)、成分ベクトル `compV` から"extracted (抽出)" されなければなりません :

```
  std::string entityId      = compV[2];  
  std::string attributeName = compV[4];  
  std::string metadataName  = compV[6];  
```

エンティティ/属性/メタデータを変更/作成するすべてのサービス・ルーチンは、NGSIv1 サービス・ルーチン `postUpdateContext()` に依存しており、`putMetadata()` も例外ではありません。したがって、`putMetadata()` では、`putMetadata()` のパラメータを使って、 `UpdateContextRequest` オブジェクトを構築し、`postUpdateContext()` を呼び出す必要があります。このようなもの : 

```
  parseDataP->upcr.res.fill(entityId, attributeName, metadataName, ActionTypeAppend);
  postUpdateContext(ciP, components, compV, parseDataP, NGSIV2_FLAVOUR_ONAPPEND);    
```
 
`UpdateContextRequest` は、一連の `fill()` メソッド (2017年3月現在7つの `fill()` メソッド) を持ち、`putMetadata()` で要求に適した fill メソッドがない場合は、`UpdateContextRequest` に fill メソッドを自走する必要があります。

これは簡単で、古い、同様の fill メソッドからコピーするだけです。

CMake ファイル `src/lib/serviceRoutinesV2/CMakeLists.txt` に `putMetadata.cpp` を追加し、broker をコンパイルしてください。`putMetadata()` が正しく動作することをテストするためには、新しい機能テスト・ケースを実装する必要があります。 [次のレシピ](#adding-a-functional-test-case)でその方法を説明しています。

"POST/PATCH/XXX /v2/entities/*/attrs/*/metadata/*" をキャプチャして、`405 Method Not Allowed` でレスポンスするには、[バッド・メソッド (bad method)に関するレシピ](#catching-a-405-method-not-allowed)を見てください。

[Top](#top)

<a name="adding-a-functional-test-case"></a>
## 機能テスト・ケースの追加
Orion の機能テストは、`.test` のサフィックスを持つ、テキスト・ファイルで、`test/functionalTest/cases/{case-dir}` にあります。"case directories"は、github の issues の後に名前が付けられます。

いつものように、新しい機能テストを実装する最も簡単な方法は、古いものから "steal (盗む)" することです。

機能テストファイルには、次の6つのセクションがあります :

1. 著作権のセクション
2. NAME セクション
3. SHELL-INIT セクション
4. SHELL セクション
5. EXPECT/REGEXPECT セクション
6. TEARDOWN セクション

各セクション (ファイルの先頭から始まる著作権プリアンブルを除く) には、すべてのセクションが開始/終了する機能テスト・ハーネスを示すヘッダが必要です :

* `--NAME--`
* `--SHELL-INIT--`
* `--SHELL--`
* `--REGEXPECT--` / `--EXPECT--`
* `--TEARDOWN--`

`--REGEXPECT--` が使用されていて、 `--EXPECT--` でなければ、期待されるセクションは正規表現を許可します。これは、これら2つの間で唯一異なるものです。

### 著作権のセクション
このセクションは単に著作権のヘッダです。古いものをコピーしてください。必要に応じて、年を変更することを忘れないでください。

### NAMEセクション
このセクションにテストの名前を記入してください : 

```
--NAME--  
Example Test Case
```

### SHELL-INIT セクション
ここで初期化タスクが実行されます。次のように : 

* データベースの抹消
* Broker の起動
* コンテキスト・プロバイダの起動
* アキュムレータの起動

例 (通常の場合) :

```
--SHELL-INIT--  
dbInit CB
brokerStart CB
accumulatorStart
```

既存のテスト・ケース `test/functionalTest/cases/1016_cpr_forward_limit/fwd_query_limited.test` から "stolen (盗用した)" Broker と5つのコンテキスト・プロバイダの例 : 

```
--SHELL-INIT--  
dbInit CB  
dbInit CP1  
dbInit CP2  
dbInit CP3  
dbInit CP4  
dbInit CP5  
brokerStart CB 0 IPV4 "-cprForwardLimit 3"  
brokerStart CP1  
brokerStart CP2  
brokerStart CP3  
brokerStart CP4  
brokerStart CP5  
```

Microsoft Windows と Mac で使われる典型的な形式である、"CRLF ライン・ターミネータ付き ASCII テキスト" 形式 を使って、テスト・ファイルに問題があることを発見しました。例 : 

```
$ file path/to/sample_test.test
path/to/sample_test.test: ASCII text, with CRLF line terminators
```

したがって、ファイルが通常の ASCII 形式を使用するようにすることをお勧めします。[`dos2unix`](http://freshmeat.sourceforge.net/projects/dos2unix) のようないくつかのツールを使用して自動変換を行うことができます。最後に、次のようなものが必要です :

```
$ file path/to/sample_test.test
path/to/sample_test.test: ASCII text
```

### SHELL セクション
Broker は SHELL-INIT セクションで起動され、このセクションでは、curl コマンドおよび、その他のコマンドが実行され、リクエストを Orion に送信して機能テストを実行します。

`orionCurl` と呼ばれるシェル・セクションを読みやすく実装するために呼び出されるシェル関数が実装されています。`orionCurl` の実装、および他の多くのヘルプ機能が `test/functionalTest/harnessFunctions.sh` の中に見つかります。

シェルセクションの各ステップは、次のような短い記述的なヘッダで始まることに注意してください。

```
echo "0x. description of test step 0x"  
echo "==============================="  
```
  
これらのステップは、現在のステップを出力の次のものから分離するために、`echo` を2回呼び出すことで終了します。 これは非常に重要なことです。出力を読むのがずっと**簡単**です。それは、**EXPECT/REGEXPECT** セクションに続くセクションに一致する必要があります。

エンティティの作成などの典型的なステップは次のようになります :

```
echo "01. Create entity E1 with attribute A1"  
echo "======================================"  
payload='{  
  "id": "E1",  
  "type": "T1",  
  "A1": {  
    "value": 1,  
    "type": "Integer",  
    "metadata": {  
      "md1": {  
        "value": 14  
      }  
    }  
  }  
}'  
orionCurl --url /v2/entities --payload "$payload"  
echo  
echo
```

### EXPECT/REGEXPECT セクション
まず、テストハーネス (`test/functionalTest/testHarness.sh`) は、2つのタイプの 'expect sections' を認めます。いずれか :

```
--EXPECT--
```

または

```
--REG-EXPECT--
```

**1つを選ぶ**必要があります。ほとんど**すべて**の現在の functests が、この `--REG-EXPECT--` タイプを使用します。 --REG-EXPECT-- の利点は、正規表現を `REGEX()` 構文を使用して追加できることです。これは日付の比較や Orion によって作成され、レジストレーション id や 相関器 (correlator)、単純なタイムスタンプのような、レスポンスで返された IDs の比較にとって非常に重要です。重要な制限は、REG-EXPECT セクションには、1行に **REGEX** が1つしかないということです。

つまり、REG-EXPECT セクションでは、問題のテスト・ステップからの予想される出力を追加します。たとえば、SHELL セクションについての上記のサブ・チャプターの例 "01. Create entity E1 with attribute A1" は、
この対応する部分を --REGEXPECT-- セクションに置いてください :

```
01. Create entity E1 with attribute A1  
======================================  
HTTP/1.1 201 Created  
Content-Length: 0  
Location: /v2/entities/E1?type=T1  
Fiware-Correlator: REGEX([0-9a-f\-]{36})  
Date: REGEX(.*)  
  
  
  
```
  
2行目の最初の行の後に出てくるものは、`orionCurl` から出てくるものは、最初に HTTP ヘッダであり、その後は最終的なペイロードです。この例では、ペイロードはありません。

相関器 (correlator) と日付の `REGEX()` の二つの出現に注目してください :

* 相関器 (correlator) は36文字の文字列で、ハイフンを含む16進数です。この正規表現は、各ハイフンがどこに来なければならないかを正確に知るようになりましたが、実際には必要ではありません
* `Date` HTTP ヘッダの2番目の REGEX もより詳細に記述できます。また必要ありません

### TEARDOWN セクション
ここでプロセスが強制終了され、データベースが削除されるため、次のテスト・ケースがクリーン・スレートで開始されます。最も一般的なコマンドは次のとおりです :

```
--TEARDOWN--  
brokerStop CB  
dbDrop CB  
```
 
アキュムレータが使用される場合、またはアキュムレータが使用される場合は、アキュムレータも停止する必要があります :

```
accumulatorStop  
brokerStop CP1  
brokerStop CP2  
```
 
機能テストでは、コンテキスト・プロバイダとして機能する Orion のインスタンスを開始することに注意してください。コンテキスト・プロバイダとして機能するインスタンスのログ・ファイル・ディレクトリおよびポート番号などが変更されます。`CP1_PORT`, `CP2_PORT` などの変数については、`scripts/testEnv.sh` を参照してください。

そして、データベース (テナント) は一掃されなければなりません : 

```
dbDrop CP1  
dbDrop CP2  
```
 
テナントが Orion と一緒に使用されている場合は、コンテキスト・プロバイダとは対照的に Orion と同様に動作します :

```
orionCurl --tenant T1 --url /v2/entities --payload "$payload"  
```

tenant T1 (データベース名 ftest-T1) も抹消する必要があります :

```
dbDrop t1  
```
  
`T1` ではなく、`t1` が使われていることに注意してください。 これは、Orion がテナントをすべて小文字に変換するためです。

[Top](#top)

<a name="debug-a-functional-test-case"></a>
## 機能テスト・ケースのデバッグ

場合によっては、テスト・ケース (つまり.test ファイル) をデバッグする必要があります。たぶん、既存の .test は、コードのいくつかの変更のために失敗し始めました。あるいは、新しい .test として報告されたバグかもしれません。新しいバグを報告する好ましい方法です! :)

どんな場合でも、Orionを デバッガ の下で実行すると便利です。たとえば、[`gdb`](https://www.gnu.org/software/gdb) または、その [グラフィカル・フロントエンド](https://sourceware.org/gdb/wiki/GDB%20Front%20Ends)です。このようにして、プログラム・ロジックが実行されるときに、ブレーク・ポイントを設定したり、ステップ・バイ・ステップの実行を使用したり、関数呼び出しのスタックや変数を検査したりすることができます。

手続きは簡単です。あなたのテストが `test_to_debug.test ` だとしましょう。まず、そのファイルを編集して、 `brokerStart` と `brokerStop` 行をコメントアウトしてください。場合によっては、`dbDrop` 行をコメント・アウトすることも良い考えです。したがって、.test が終了した後で DB を見ることができます。

次に、デバッガで contextBroker プロセスを起動します。正確な手順は、使用されている特定のデバッガまたはデバッガのフロントエンドによって異なりますので、ここでは詳しく説明しません。ただし、関数テスト・フレームワークで使用されるポートとデータベースに対応するため、contextBroker には次の CLI パラメータを使用することが重要です :

```
-db ftest -port 9999
```

さらに、次の他の CLI パラメータは、デバッガでの実行に役立ちます :

```
-fg                  (to run in foreground)
-logLevel INFO       (to have useful information in /tmp/contextBroker.log file)
-httpTimeout 100000  (to avoid problems with timeout, e.g. due to you are holding in a breakpoint for a long time)
-reqTimeout 0        (also to avoid problems with timeouts)
-noCache             (in some cases, cache management adds "noise" to logs; this flag disables it)
```

最後に、`test_to_debug.test` の機能テストを実行します :

```
CB_MAX_TRIES=1 /path/to/testHarness.sh /path/to/test_to_debug.test
```

テストは、デバッガで contextBroker プロセスを使用して実行を開始します。たとえば、.test ケースが横断する場所にブレーク・ポイントを設定した場合、実行はそこで停止します。

終了したら、.test ファイルに `brokerStart`, `brokerStop`, `dbDrop`行を復元してください。

[Top](#top)

<a name="catching-a-405-method-not-allowed"></a>
## '405 Method Not Allowed' のキャッチ
Orion はリクエストをサポートしています :

* `GET /v2/entities/{EntityId}`
* `DELETE /v2/entities/{EntityId}`

しかし、`POST /v2/entities/{EntityId}` が Broker に発行された場合はどうなりますか?

まあ、`POST /v2/entities/{EntityId}`  サポートされていないので、通常は、`404 Not Found` の結果になります。しかし、Orion は、サービス・ルーチン `badVerbGetDeleteOnly()` で URL `/v2/entities/{EntityId}` の ANY メソッドをキャッチするため、Orionは、`405 Method Not Allowed` でレスポンスできます。URL は OK ですが、動詞/メソッドはサポートされていません。

`contextBroker.cpp` と入力して、このセクションを検索してください : 

```
  #define API_V2                                                                                       \  
  { "GET",    EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          entryPointsTreat         }, \  
  { "*",      EPS,          EPS_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetOnly           }, \  
                                                                                                       \  
  { "GET",    ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          getEntities              }, \  
  { "POST",   ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          postEntities             }, \  
  { "*",      ENT,          ENT_COMPS_V2,         ENT_COMPS_WORD,          badVerbGetPostOnly       }, \  
  
  { "GET",    IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         getEntity                }, \  
  { "DELETE", IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         deleteEntity             }, \  
  { "*",      IENT,         IENT_COMPS_V2,        IENT_COMPS_WORD,         badVerbGetDeleteOnly     }, \  
```

最後の3行は興味深いものです。

このセクションの前に、次の定義が行われます :

```
  #define IENT                    EntityRequest  
  #define IENT_COMPS_V2           3, { "v2", "entities", "*" }  
  #define IENT_COMPS_WORD         ""  
```
      ""  
だから、このように : 

* URL パス `/v2/entities/{EntityId}` を持つリクエストと `GET` メソッドが Brokerに入ると、サービス・ルーチン `getEntity()` がリクエストを処理します
* メソッドが代わりに "DELETE" の場合、 `deleteEntity()` がリクエストを処理します
* 他の動詞 (POST, PUTなど) の場合、`badVerbGetDeleteOnly()` は、リクエストを処理します。`badVerbGetDeleteOnly()` リクエストを処理するとき、レスポンスは `405 Method Not Allowed` となり、HTTP ヘッダ `Allow: GET, DELETE` がレスポンスに含まれます。

[Top](#top)

<a name="fixing-a-memory-leak"></a>
## メモリ・リークの修正
メモリ・リークは、[valgrind memcheck](http://valgrind.org/docs/manual/mc-manual.html) を使用して検出されます。特別なシェル・スクリプト `test/valgrind/valgrindTestSuite.sh` がこの目的のために開発され、make のステップがそれにリンクされています : `make valgrind`

`valgrindTestSuite.sh` を手動で実行する場合は、Orion を動作させるために DEBUG モードでコンパイルする必要があることに注意してください  (`make debug install`)

valgrind の出力は、テスト・ケースと同じ名前のファイルに保存されますが、サフィックス `valgrind.out` は付いています。

通常、Broker にはメモリ・リークはありませんので、**メモリ・リークを伴う**動作を行うには、一時的にメモリ・リークを追加する必要があります :

* お気に入りのエディタで `src/lib/ngsi10/UpdateContextRequest.cpp` ファイルを開きます
* メソッド `UpdateContextRequest::release()` を見つけ、`contextElementVector.release()` の呼び出しをコメントにします :

  ```
  void UpdateContextRequest::release(void)  
  {  
    // contextElementVector.release();  
  }  
  ```
  
* Broker を再コンパイルします : 

  ```
  make debug install
  ```

* リークを確認するために使用するテスト・ケース `UpdateContextRequest` に対して、valgrind テストを実行します :

  ```
  % valgrindTestSuite.sh -filter in_out_formats.test

  Test 001/1: 0000_content_related_headers/in_out_formats  ..... FAILED (lost: 2000). Check in_out_formats.valgrind.out for clues

  1 tests leaked memory:
    001: 0000_content_related_headers/in_out_formats.test (lost 2000 bytes, see in_out_formats.valgrind.out)
  ```

* ファイル `test/functionalTest/cases/0000_content_related_headers/in_out_formats.valgrind.out` を開きます
* 文字列 "definitely lost" を検索します : 

```
==19688== 2,000 (544 direct, 1,456 indirect) bytes in 4 blocks are definitely lost in loss record 313 of 318  
==19688==    at 0x4A075FC: operator new(unsigned long) (vg_replace_malloc.c:298)  
==19688==    by 0x6EABD4: contextElement(std::string const&, std::string const&, ParseData*) (jsonUpdateContextRequest.cpp:50)  
==19688==    by 0x6A7CF7: treat(ConnectionInfo*, std::string const&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:180)  
==19688==    by 0x6A9935: jsonParse(ConnectionInfo*, std::pair<std::string const, boost::property_tree::basic_ptree<std::string, std::string, std::less<std::string> > >&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:376)  
==19688==    by 0x6AA0BF: jsonParse(ConnectionInfo*, std::pair<std::string const, boost::property_tree::basic_ptree<std::string, std::string, std::less<std::string> > >&, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:416)  
==19688==    by 0x6AA94A: jsonParse(ConnectionInfo*, char const*, std::string const&, JsonNode*, ParseData*) (jsonParse.cpp:532)  
==19688==    by 0x6A3E6F: jsonTreat(char const*, ConnectionInfo*, ParseData*, RequestType, std::string const&, JsonRequest**) (jsonRequest.cpp:232)  
==19688==    by 0x688C42: payloadParse(ConnectionInfo*, ParseData*, RestService*, JsonRequest**, JsonDelayedRelease*, std::vector<std::string, std::allocator<std::string> >&) (RestService.cpp:122)  
==19688==    by 0x68B112: restService(ConnectionInfo*, RestService*) (RestService.cpp:543)  
==19688==    by 0x67E8E7: serve(ConnectionInfo*) (rest.cpp:561)  
==19688==    by 0x683E4A: connectionTreat(void*, MHD_Connection*, char const*, char const*, char const*, char const*, unsigned long*, void**) (rest.cpp:1550)  
==19688==    by 0x850B78: call_connection_handler (connection.c:1584)  
```

さて、スタックフレーム#2を見て、リークが、`jsonUpdateContextRequest.cpp` の 50行名 (正確な行番号があなたのケースで、わずかに異なる場合があります) の `contextElement()` への呼び出しから来ているようです。`UpdateContextRequest::release()` で `ContextElementVector::release()` の呼び出しをコメントしたので、私たちはこのリークをなぜ持っているのかはすでに知っていますが、割り当てが行われる場所と、 非常に異なるものは、アロケートされたオブジェクトが解放されるべき場所です。

これは、フリー/削除の呼び出しをどこで行うべきかを知り、リークを修正するのは難しい作業です。それはしばしば明らかですが、必ずしもそうではありません。まれに、リークを修正しようとするとき、割り当てられたバッファをすぐに解放する、つまり前回使用する前に解放するため、すべてのリークが修正された時点ですべての機能テストが完全に機能することを確認することが非常に重要です。`jsonUpdateContextRequest.cpp` で見つかったリークを想像してください。バッファが割り当てられた直後に解放され、そこから `ContextElementVector::release()` の間にバッファが使用されると、おそらく SIGSEGV が発生します。

[Top](#top)
