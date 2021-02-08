# Orion Context Broker コントリビューション ガイド

このドキュメントでは、Orion Context Broker にコントリビューションするためのガイドラインについて説明します。コードにコントリビューションする予定がある場合は、このドキュメントを読んでその内容に精通してください。

## 基本ルールと期待

始める前に、ここに私たちがあなたに期待する (そしてあなたが他の人に期待すべき) いくつかのことがあります：

* このプロジェクトに関する会話には、親切で思慮深くしてください。私たちは皆、さまざまな背景やプロジェクトから来ています。つまり、"オープンソースがどのように行われるか" についてさまざまな見方をしている可能性があります。あなたのやり方が正しいことを彼らに納得させるのではなく、他の人の話を聞くようにしてください
* あなたの貢献がすべてのテストに合格することを確認してください。テストが失敗した場合は、貢献をマージする前にそれらに対処する必要があります
* コンテンツを追加するときは、それが広く価値があるかどうかを考慮してください。あなたやあなたの雇用主が作成したものへの参照やリンクを追加しないでください。他の人がそれを評価した場合にそうします
* ソフトウェアの脆弱性を報告する場合は、Orion リポジトリのメンテナに連絡して、非公開で話し合ってください

## 一般原則

* Orion Context Broker のプログラミング言語は、C と C++ です。ただし、スクリプト・ツールに関するいくつかのサイド・ピースは bash と Python で書かれています
* 非効率的なコードでは、効率的なコード、すなわち、より良いパフォーマンスを達成するコードが好まれます。複雑なコードでは、シンプルなコード、すなわち、クリーンで短いコードが優先されます。シンプルさにおいて、小さなペナルティを伴う効率の大幅な節約が可能です。効率において、小さなペナルティを伴うシンプルさの大幅な節約も可能です
* Orion にコントリビューションされた新しいファイルは、ファイルシステムの[レイアウト・ガイドライン](#filesystem-layout-guidelines)に従わなければなりません
* Orion Context Broker にコントリビューションしたコードは、コードを扱うすべての開発者に共通のプログラミング・スタイルを設定するために、[コード・スタイルのガイドライン](#code-style-guidelines)に従わなければなりません
* Orion Context Broker のコードで許可されているものと許可されていないものに関する[プログラミング・パターン](#programming-patterns)に関するセクションも参照してください

プル・リクエストなどのコントリビューションのワークフロー自体は別のドキュメントに記載されています。[FIWARE開発ガイドライン](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Developer_Guidelines)を参照してください。

## 貢献する方法

貢献したい場合は、まず [issues](https://github.com/telefonicaid/fiware-orion/issues) を検索し、他の誰かが、[プル・リクエスト](https://github.com/telefonicaid/fiware-orion/pulls)を出して、同様のアイデアや質問を提起したかどうかを確認します。

アイデアがリストに表示されておらず、このガイドの目標に適合していると思われる場合は、次のいずれかを実行してください:

* タイプミスの修正など、貢献が軽微な場合は、プルリクエストを開きます
* 新しいガイドなど、貢献が重要な場合は、最初に問題を開くことから始めます。そうすれば、あなたが仕事をする前に、他の人が議論に参加することができます

## プル・リクエストの手順

[FIWARE開発ガイドライン](https://forge.fiware.org/plugins/mediawiki/wiki/fiware/index.php/Developer_Guidelines)で説明したように、コントリビューションはプル・リクエスト (PR) を使用して行われます。このような PR で使用される詳細な "手順"は、以下で説明されます。

* マスター・ブランチへの直接コミットは、単一行の変更を含み、許可されていません。あらゆる変更は PR として送付しなければなりません
* PR が番号付きの issue を実装/修正している場合は、作成時に PR の本文で発行番号を参照する必要があります
* 誰でも PR にコメントを付けることができます。Github が提供するレビュー機能を使って直接コメントすることもできます
* トレーサビリティ上の理由から、一般的なコメントの代わりにコード行コメントを使用します。以下のコメントのライフサイクルを参照してください
* コメントのライフサイクル
  * コメントが作成され、"コメント・スレッド"が開始されます
  * 新しいコメントを元のコメントに追加して、ディスカッションを開始することができます
  * ディスカッションの後、コメント・スレッドは次のいずれかの方法で終了します :
    * `Fixed in <commit hash>` ディスカッションに PR ブランチ内の修正が含まれる場合、コミット・ハッシュ (commit hash) が参照として含まれます
    * `NTC` 最後に何もする必要がない場合 (NTC = Nothing To Change)
  * PR は、以下の条件が満たされたときにマージできます :
    * すべてのコメント・スレッドは閉じている
    * ディスカッションの参加者全員が、一般的なコメント `LGTM` を出した (LGTM = Looks good to me)
  * セルフ・マージは許可されていません。まれに正当な場合を除きます

新しい PRs にコントリビューションする際に考慮すべきいくつかの補足 :

* PR には、コードのコントリビューションだけでなく、対応するドキュメント (新しいもの、既存のものへの変更)  とテスト が含まれます
* PR の変更は、新しい機能のために追加された新しいテストに加えて、既存のテスト (ユニット、機能、メモリ、e2e) に基づいて完全な回帰 (full regression) をパスする必要があります
* PR は、レビューが可能な適切なサイズでなければなりません。PR が大き過ぎると、これ以上のディスカッションをすることなく、"より小さなサイズで作業をやり直してください"とクローズことができます

## コミュニティ

オープンソース・ガイドに関する議論は、このリポジトリの [Issues](https://github.com/telefonicaid/fiware-orion/issues) と[プル・リクエスト](https://github.com/telefonicaid/fiware-orion/pulls)のセクションで行われます。どなたでもこれらの会話に参加できます。

可能な限り、メンテナに直接連絡するなど、これらの会話をプライベート・チャネルに持ち込まないでください。コミュニケーションを公開し続けることは、誰もが会話から利益を得て学ぶことができることを意味します。

## ファイルシステムのレイアウト・ガイドライン

### ディレクトリのレイアウト

ディレクトリ構造の詳細については、[開発マニュアルのこのセクション](devel/directoryStructure.md)を参照してください。

### ソースコード・ファイルのファイル・レイアウト

ソースコード・ファイルは `src/` ディレクトリの下にあります。

ソース・ファイルには、サフィックス '.cpp' を使用されなければならず、ヘッダ・ファイルには '.h'を使用しなければなりません。

原則として、C/C++ ソースコードでは、すべてのコンセプトに独自のモジュールを用意するべきです (SHOULD)。モジュールでは、ヘッダ・ファイル (`.h`) とそれに対応するソース・ファイル (`.cpp`) が参照されます。場合によっては、対応する `.cpp` ソース・ファイルがなく、ヘッダ・ファイルだけが必要です。定数、マクロ、インライン関数のみを含むヘッダ・ファイルです。

クラスはヘッダ・ファイル内のクラス定義とソース・ファイル内のクラスの実装とで、独自のモジュールに存在すべきです (SHOULD)。このモジュールの名前はクラスの名前です。例えば、クラスのサブスクリプションはモジュール { `Subscription.h` / `Subscription.cpp` } にあります。

あるクラスが関連するクラス、例えばサブクラスや組み込みクラ、を必要とする場合、それらは同じモジュール内に存在するはずですが、他の場所では使用されない場合に限ります。それ以外の場所で使用されている場合は、影響を受けるクラスに対して新しいモジュールを作成する必要があります。

##  コード・スタイルのガイドライン

現在、Orion の既存のコードベースのすべてがこれらのルールに準拠しているわけではありません。ガイドラインが作成される前に書かれたコードの一部があります。しかし、新しいコードのコントリビューションはすべてこれらの規則に従わなければならず、最終的に古いコードはガイドラインに準拠するように変更されます。

### '従わなければならない' ルール ('MUST follow' rules)

#### M1 (ヘッダ・ファイルインクルージョン) :

*ルール* : すべてのヘッダ・ファイルまたはソース・ファイルには、必要なすべてのヘッダ・ファイルとその他のヘッダ・ファイルが含まれていなければなりません (MUST)。他のヘッダ・ファイルのインクルージョンに依存してはいけません。また、すべてのヘッダ・ファイルとソース・ファイルには、それ自体は必要ないヘッダ・ファイルを含めてはいけません (MUST NOT)

*理由* : 各ファイルは、他のファイルに含まれている/含まれていないファイルに依存してはいけません。また、ヘッダ・ファイルに必要以上のファイルが含まれている場合、'クライアント' にはそれらの '余分な' ファイルも含めて、他の選択肢はありません。これは時に競合を招き、避けなければなりません。また、コンパイル時間も長くなります

*チェック方法* : 手動

#### M2 (著作権ヘッダ)

*ルール* : すべてのファイル、ソースコード、またはそうでないもに、著作権ヘッダが必要です (MUST) :

C++ ファイルの場合 :

```
/*
*
* Copyright 20XX Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: <the author>
*/
```

Python、bash スクリプト、CMakeLists.txt など :

```
# Copyright 20XX Telefonica Investigacion y Desarrollo, S.A.U
#
# This file is part of Orion Context Broker.
#
# Orion Context Broker is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Orion Context Broker is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
# General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
#
# For those usages not covered by this license please contact with
# iot_support at tid dot es
#
# Author: <the author>
```

*理由* : すべてのファイルに対して同質の著作権ヘッダーを持つことです

*チェック方法* : 内部スクリプトの ```scripts/check_files_compliance.py``` または ```scripts/style_check.sh``` を使用してチェックします

#### M3 (関数ヘッダ)

*ルール* : すべての関数はヘッダを持っていなければなりません (MUST)。関数の機能、パラメータの記述リスト、およびその戻り値の簡単な説明があるべきです (SHOULD)

例 :

```
/* ****************************************************************************
*
* parseUrl - parse a URL and return its pieces
*
*  [ Short description if necessary ]
*
* PARAMETERS
*   - url         The URL to be examined
*   - host        To output the HOST of the URL 
*   - port        To output the PORT of the URL
*   - path        To output the PATH of the URL
*   - protocol    To output the PROTOCOL of the URL
*
* RETURN VALUE
*   parseUrl returns TRUE on successful operation, FALSE otherwise
*
* NOTE
*   About the components in a URL: according to 
*   https://tools.ietf.org/html/rfc3986#section-3,
*   the scheme component is mandatory, i.e. the 'http://' or 'https://' must 
*   be present, otherwise the URL is invalid.
*/
```

*理由* : このように準備した場合、コードは読みやすくなります

*チェック方法* : 手動


#### M4 (インデント)

*ルール* : 空白 (たとえば、タブなし) のみを使用し、一度に2つのスペースをインデントします

*理由* : 2つの空白で十分です。それはラインをあまり長くしません

*チェック方法* : 内部スクリプト ``scripts/style_check.sh``` を使用してチェックします

#### M5 (変数宣言) :

*ルール* : 宣言された各変数は別々の行になければなりません :

```
int a;
int b;
```

以下の使用法は避けなければなりません :

```
int a, b;
```

*理由* : 読みやすいためです

*チェック方法* : 手動

#### M6 (命名規則) :

*ルール* : 次の命名規則が適用されます :

* クラス/構造体名の場合は、キャメル・ケース (CamelCase)。たとえば、`SubscriptionResponse`
* マクロ名と `#define` 定数の場合は、UPPER_LETTER。たとえば、`LM_E(...)`, `EARTH_RADIUS`
* 他のすべての場合は、キャメル・ケース (CamelCase)。たとえば、`connectionMemory`

*理由* : このルールは、マクロや定数を扱っているのか、変数を扱っているのかを簡単に理解できるようにします。関数対マクロと同じです

*チェック方法* : 手動

#### M7 (複数行マクロ) :

*ルール* : 複数の行にまたがるマクロは、バックスラッシュを水平に整列させなければなりません (MUST)

```
#define LONG_MACRO(a, b)    \
do                          \
{                           \
  action();                 \
} while 0;
```

*理由* : 見た目にわかりやすいためです

*チェック方法* : 手動

#### M8 (コードブロック) :

*ルール* : 開閉ブラケット `{` および `}` は、別の行にする必要があります (MUST)

```
if ()
{
  xxx();  // 
}
else
{
}
```

また、開閉ブラケットは1ライナーにも使用されます。

これは、コードを本当に読みやすくするために、このルールの例外があります。つまり、if 文の後の本当に短い文のセットです。例を挙げて説明するのが簡単です : 

```
// Stupid ctoi (char to integer) implementation:

int ctoi(char c)
{
  if (c == '0')    return 0;
  if (c == '1')    return 1;
  if (c == '2')    return 2;
  if (c == '3')    return 3;
  ...
}
```

同じことが、'switch/case' にも当てはまります :

```
int ctoi(char c)
{
  switch (c)
  {
  case '0':   return 0;
  case '1':   return 1;
  case '2':   return 2;
  case '3':   return 3;
  ...
  }
}
```

*理由* : これは、ブラケットが水平に整列されているため、ブロックがどこで終わるかを理解しやすくなります。代替スタイルの `if () {` では、1行を節約するために、紙に印刷する本を編集するときに多くの意味を持ちますが、エディタのみを参照しているソースコード・ファイルにその行を節約することはあまり意味がありません

*チェック方法* : 手動

#### M9 (switch文) :

*ルール* : Switch ステートメントは、別々の行の `{` 関する他のステートメントと同じ規則に従います。`switch` ステートメントと異なる点は、ラベルが含まれていることです。switch ステートメントでは、`case` は switch に属しているため、`switch と同じインデントを持ちます

非常に短い 'case-statements' は、前の例で見たように、'case-label'と同じ行にすることができます。また、return が使用されていて、break が必要な場合でも、break は実際には必要ありません :

```
switch (c)
{
case '0':   i = 0; break;
case '1':   i = 1; break;
case '2':   i = 2; break;
case '3':   i = 3; break;
...
}
```

より長いステートメントが使用される場合、'case-label' はそれ自身の行を持たなければならず （MUST)、ステートメントは通常どおり1つの行をそれぞれ持たなければなりません (MUST)。また、次のように 'case-block' が終了して別のものの開始を明確に示すために、'break' と 'case'の間に空行がなけれなりません (MUST) :

```
switch (c)
{
case '0':
  strncpy(out, "This would be case 0", sizeof(out));
  break;

case '1'
  ...
}
```

*理由* : 一般的に、別々の行にあるそれぞれのステートメントは、コードを読みやすくします。しかし、これらの典型的な非常に短い switch は、ルールを少し緩めれば、はるかに読みやすくなります

*チェック方法* : 手動

#### M10 (コマンドと演算子の分離) :

*ルール* : 演算子 (=, ==, など)は、1つのスペースが先行します。カンマの後には1つのスペースが続きます

```
myFunction(x, y, z);
if (a == b)
{
  c = d;
}
```

スペースがない

```
myFunction(x,y,z);
if (a==b)
{
  c=d;
}
```

*理由* : 見た目にわかりやすいためです

*チェック方法* : 内部スクリプト ```scripts/style_check.sh``` を使用してチェックします

#### M11 (スペース)

*ルール* :  '(' が続くキーワードには、キーワードと括弧の間にスペースが必要です (MUST) : 

```
if () ...
while () ...
do () ...
switch () ...
etc
```

一方、関数呼び出しまたは関数宣言は、関数名と括弧の間にスペースを入れてはなりません (MUST NOT) :

```
funcCall();
```

* また、`(` の後と `)` の前に、スペースが存在してはなりません (MUST NOT)
* 配列 (`[]`) に対しても同じことです
* すべてのバイナリ演算子は、スペース (`=`, `==`, `>=`, `<`, など) で囲まれなければならなりません (MUST)
* 単項演算子は、それ自体とオペランド (`!`, `++`, など) の間に空白あってはなりません (MUST NOT)
* `if`, `for`, `while` などの条件は、以下のルールに従う必要があります (MSUT)
   * 論理演算子 (`&&`, `||`) は、スペースで先行しなければなりません : ` && `, ` || `
   * すべての演算子 (`==`, `>` ...) はスペースで先行しなければなりません : `if (a == 7) ...`
   * `(` の後、または `)` の前に、スペースは不要です
* コンマの後に、通常のテキストの場合と同じように1つのスペースがあります。例外は、カンマの後に複数のスペースが続く場合は、水平方向の配置が読みやすくなります :

```
mongoInit(user, pwd, host);
xyzInit(user,   pwd  host);
``` 

*理由* : 見た目にわかりやすいためです

*チェック方法* : 内部スクリプト ```scripts/style_check.sh``` を使用してチェックします

#### M12 (ヘッダファイル内での `using namespace`)

*ルール* : ```using namespace XXX``` ディレクティブは、ヘッダー・ファイルでは使用しないでください

*理由* : ヘッダー・ファイルは、ヘッダー・ファイルを含むソース・ファイルを決定するべきではありません。ヘッダー・ファイルにこのディレクティブが含まれている場合、ヘッダー・ファイルを含むすべてのソースファイルに、直接的または間接的にそのディレクティブも強制されます

*チェック方法* : インテリジェントな ```grep``` を使用します

#### M12bis (ソースファイル内での `using namespace`)

*ルール* : `using namespace XXX` ディレクティブは、ソースファイル (たとえば、`.cpp`) で使用しないでください。`using XXX::YYY` ディレクティブは、 ソースファイルで使用することができます

*理由* : `using namespace XXX`が使われていないときの functions/variables/types がより明確になります

*チェック方法* : 内部スクリプト `scripts/style_check.sh` を使用してチェックします

### '従うべき' ルール ('SHOULD follow' rules) :

#### S1 (ステートメント)

*ルール* : 各ステートメントはそれ自身の行を持つべきです (SHOULD)

*理由* : コードを読みやすくなります

*チェック方法* : 手動

#### S2 (オブジェクト・アクションの命名規則) :

*ルール* : "objectAction" を使用してください (SHOULD)。例 :

```
listInit()
listReset()
listFind()
```

の代わりに

```
initList()
resetList()
findList()
```

このルールは、変数名、関数名、さらにはファイル名にも適用されます。外部関数が1つしかないため、ファイルの名前は関数の名前です

*理由* : 一連の関数は、プレフィックス "object" (例 : "list")のおかげで、グループ化されています

*チェック方法* : 手動

#### S3 (部分条件における括弧)

*ルール* : `if`, `for`, `while` などの条件は、括弧内に部分条件 (part-conditions) を持っている必要があります

```
if ((a == 2) || (a == 7)) ...
```

*理由* : 読みやすく、優先順位のために望ましくない動作をすることは不可能です

*チェック方法* : 手動

#### S4 (機能分離)

*ルール* : 関数ヘッダの前に3つの空行を追加して、1つの関数がどこで終了し、別の関数が開始されるかを明確に確認します

理由 : 1つの機能が終了し、別の機能が開始されたことを明確に確認するとよいでしょう。なぜ3行ですか？まあ、1行では不十分で、3行が良いのです

*チェック方法* : 手動

#### S5 (前提条件)

*ルール* : 処理を開始する前に、関数がパラメータを評価し、必要であればエラーを返すことを強くお勧めします

例 :

```
// Bad implementation:
bool stringCheck(char* s1, char* s2, char* s3, char* s4)
{
  bool ret = false;

  if (s1 != NULL)
  {
    if (s2 != NULL)
    {
      if (s3 != NULL)
      {
        if (s4 != NULL)
        {
          printf("All four strings non-NULL");
          ret = true;
        }
      }
    }
  }

  return ret;
}

// Better implementation
bool stringCheck(char* s1, char* s2, char* s3, char* s4)
{
  if ((s1 == NULL) || (s2 == NULL) || (s3 == NULL) || (s4 == NULL))
  {
    return false;
  }

  printf("All four strings non-NULL");
  return true;
}
```

*理由* : 長いネストされた 'if-else' は、関数を理解しにくくするだけです

*チェック方法* : 手動

#### S6 (可変整列)

*ルール* : 変数は水平方向に整列されなければならず (SHOULD)、また割当てが行われた場合、 `=` 演算子も整列されるべきです (SHOULD) :

```
int             x  = 12;
float           f  = 3.14;
struct timeval  tm;
```

型名と変数名の間には、2つのスペース区切りがあり、型と変数名が明確に分かれています。時には型が複数の単語で構成されていることもあります (例 : `struct timeval`)

*理由* : コードが読みやすくなります

*チェック方法* : 手動

#### S7 (ソース/ヘッダ・ファイルの順序)

*ルール* : 次の順序を使用する必要があります :

* `#ifndef/#define` (ヘッダ・ファイルの場合のみ、明らかに)
* 著作権のヘッダ
* インクルード (Includes)
* 外部宣言 (該当する場合はソースファイルのみ)
* 定義 (マクロを含む)
* typedefs
* 変数 (ヘッダ・ファイルの場合は外部宣言)
* 関数 (ヘッダ・ファイルの場合は外部宣言)
* #endif (ヘッダ・ファイルの場合のみ、明らかに)

*理由* : 順序、順序、さらに順序

*チェック方法* : 手動

#### S8 (Includes)

*ルール* : 

* インクルードファイルのリストは、 `<stdio.h>` または `<string.h>` のような、標準 C ライブラリからファイルを開始しなければなりません (MUST)
* その後、`<string>` または `<vector>` のよな STL の C++ ファイル
* `mongo/xxx.h` のようなサードパーティのヘッダ・ファイル
* この実行可能ファイルの他のライブラリからのファイル
* 同じライブラリのファイル、または実行可能ファイルの場合は同じディレクトリ
* 最後に、対応するヘッダ・ファイル

おそらく、実行可能ファイルの `main()` を含むファイルを除き、すべての C/C++ ファイルには、対応するヘッダ・ファイルが必要です。例 : `list.cpp` は関数を実装し、一方、`list.h` は、他のファイルが含める外部部分です。

*理由* : ソースファイルがそれに対応するヘッダファイルを含むことが非常に重要である理由を説明するします : `list.cpp` は `list.h` を含んでいないと仮定し、ヘッダファイルに listInit(void) を見つけますが、ソースファイル `list.cpp` の `listInit(int)` はうまくコンパイルしますが、 使用されると、ユーザは `list.h` をインクルードします。これには `listInit` にパラメータがないことを示します。結果？ 災害。しばしば SIGSEGV。したがって、これを解決するには、すべての C/C++ ファイルに対応するヘッダ・ファイルを含めなければなりません。この問題はコンパイル時に常に検出されます

*チェック方法* : 最初の std C ファイル、次に std C++ ファイルなどのインクルード・ファイルのの順序を除き、手動でチェックします。これは内部スクリプト ```scripts/style_check.sh``` でチェックします

#### S9(行長) :

*ルール* : 行は120文字を超えるべきではありません (SHOULD)。しかし、行を分割することによって可読性が悪化した場合は、行を必要な長さのままにします。行を変数の助けを借りて短縮できるかどうかを検討してください。例 :

```
//
// Example of a long line
//
if ((contextAttributeP->metadataVector[0]->name == "") && (contextAttributeP->metadataVector[0]->type == "") && (contextAttributeP->metadataVector[0]->xyz == NULL))
{
}
            
//
// Same line made shorter using a help variable
//
Metadata* mdP = contextAttributeP->metadataVector[0];

if ((mdP->name == "") && (mdP->type == "") && (mdP->xyz == NULL))
{
}
```

*理由* : 120は、現在の視覚化技術である、高解像度モニタ、サイズ変更可能なウィンドウおよびスクロール・バーを考慮に入れた妥当な限界です

*チェック方法* : 内部スクリプト ```scripts/style_check.sh``` でチェックします

#### S10 (三項演算子)

*ルール* : 三項演算子は歓迎すべきものですが、ネストしてはいけません :

```
char* boolValue = (b == true)? "true" : "false";
```

さて、もしそれを入れ子にするのであれば、(`True`, `False`, `Perhaps` という値を持つ列挙型としてふるまいましょう) :

```
char* xboolValue = (b == True)? "True" : (b == False)? "False" : "Perhaps";
```

まだ非常にコンパクトですが、もはや非常にエレガントでなく、間違いなく分かりにくいものです。この例が非常に簡単であることを覚えておいてください。ネストされた三項演算子は使うべきではありません (SHOULD NOT)。 この特定のケースでは、`switch` が良い選択肢かもしれません :

```
char* boolValue;

switch (b)
{
case True:     boolValue = "True";    break;
case False:    boolValue = "False";   break;
case Perhaps:  boolValue = "Perhaps"; break;
}
```

*理由* : 読みやすさ

*チェック方法* : 手動

#### S11(空行)

*ルール* : 関数の論理部分の間に空行を使用するのは良い方法です。1行で十分で、誇張する必要はありません。また、関数の先頭の変数宣言の後に、コードが始まる前に空行を追加してください

ブロック `{}` の後で、空の行が良い考えです

`return` の前に、空の行はほとんど常に良いアイデアです。`return` は重要な指示であり、完全に見えるべきです

前述したように、switch 内の case の間では、`case` とさまざまなコマンドが行を共有する、短い switchを除いて、空の行が必須です (MUST)

空行は、 `{`  の後、または `}` の前に現れてはいけません (MUST NOT)

*理由* : これは実際にソースコードの読みやすさを助けることができます

*チェック方法* : 手動

#### S12 (C ++ の初期化リスト)

*ルール* : C ++ の初期化リストを使用する必要があります。また、その行が長すぎる場合を除き、コンストラクタと同じ行になければなりません。行が長すぎる場合は、初期化された変数はすべてその行に置かれ、2つのスペースでインデントされます。例 :

```
X:X(int _i, float _f):  i(_i), f(_f)
{
}

Y:Y(const std::string& _fooName, const std::string& _myLongFooName):
  fooName(_fooName),
  myLongFooName(_myLongFooName)
{
}
```

*理由* : この方法は、実行時、オブジェクト構築時に値がメモリに割り当てられるので、コンストラクタ本体に値を割り当てるよりも効率的です

*チェック方法* : コンパイラの警告と手動との組み合わせ

#### S13 (ポインタ変数の命名)

*ルール* : ポインタ変数名はサフィックスとして `P` (大文字の P　) を使用する必要があります

```
Entity* eP;
```

*理由* : ポインタ変数は、glanze で簡単に識別することができるため、コードが明確になります

*チェック方法* : 手動

#### S14 (関数呼び出しの参照変数)

*ルール* :  "C++参照" によって渡されるパラメータは、`const` かどうかにかかわらず、計算結果ではなく変数でなければなりません。例を参照してください :

```
  extern void f(const BSONObj& bobj);

  // NOT like this:
  f(BSON("a" << 12));

  // BUT, like this:
  BSONObj bo = BSON("a" << 12);
  f(bo);

```

*理由* : 参照されるパラメータに値を渡すのは単に奇妙なことです。もし `const` 使用されていなければ、コンパイル・エラーが発生します。それは多くの意味があります。変数がないときに、関数は変数をどのように変更できますか？ "C" では、より簡単です。`&` 演算子を使用して変数への参照を送信するには、変数が必要です。C++ではちょっと変わってしまいました。ヘルパー変数を追加することで回避できます。上記の例では `bo`。"C ポインタ" と "C++ 参照" の違いは最小限ですが、実際にはコンパイラの実装に依存します。これに関する議論のために、この[質問](https://stackoverflow.com/questions/44239212/how-do-c-compilers-actually-pass-literal-constant-in-reference-parameters)と stackoverflow の[これ](https://stackoverflow.com/questions/2936805/how-do-c-compilers-actually-pass-reference-parameters)を見てください

*チェック方法* : 手動

#### S15 (最後に、エラー出力パラメータ)

*ルール* : 関数が出力パラメータを使用して呼び出し側にエラー出力を提供する場合、そのパラメータは、パラメータ・リストの最後に宣言されるべきです (SHOULD)。たとえば :

```
+void mongoRegistrationGet
(
  ngsiv2::Registration*  regP,
  const std::string&     regId,
  const std::string&     tenant,
  const std::string&     servicePath,
  OrionError*            oeP
);

void myFunction(const std::string s, std::string* err);
```

*理由* : コードはこのように多くの順序になります

*チェック方法* : 手動

#### S16 (empty()を使用して長さがゼロの文字列をチェックする)

*ルール*: すべての文字列の空かどうかのチェックに empty() を使用し、`""` または `length() == 0` との比較を回避します。

*根拠*: empty() は、[長さゼロの文字列をチェックする最良の方法](http://stackoverflow.com/questions/41425569/checking-for-empty-string-in-c-alternatives) です。

*チェック方法* : 手動

## プログラミングパターン

いくつかのパターンは、強い理由がある場合を除いて、Orion Context Broker のコードでは許可されていません。それらの一部を使用する予定の場合は、コア開発者と事前に相談してください。

* 多重継承。それはコードの複雑さをもたらします。それは一般的に "問題のある" パターンであると認められています
* C++ の例外
  * C++ でチェックされていない例外は、実行の流れに従うのをより困難にします。関数が例外をスローしてリファクタリングとデバッグを複雑にするかどうかは難しいです
  * 例外セーフな機能では、関数では、リソースの漏洩を避けるために RAII を超えてトランザクションの意味を保持する必要があり、微妙なやりとりによってコードがエラーを起こしやすくなります。マルチスレッド環境では悪化します
  * このプロジェクトのフレーバーは C に明確に偏っています。多くのプレーンな C ライブラリが使用されており、混合例外や従来の if-checking メソッドはできるだけ避けるべきです
* オブジェクトの受け渡し。関数の呼び出し時に非効率なオブジェクトのコピーを避けるために、次の基準が適用されます
  * 関数がオブジェクトを変更することができないようにすべきであるならば (たとえば、"読み取り専用")、const 参照が使用されるべきです。例 : `const BigFish& bf`
  * 関数がオブジェクトを変更できなければならない場合 (たとえば : "読み取り/書き込み" または "書き込み")、ポインタ型を使用する必要があります。例 : `BigFish* bf`。この場合、C++ 参照も同様に使用できますが、関数によってオブジェクトが変更される可能性があることを明確に示すためにポインタを使用することをお勧めします

```
void myFunction(const BigFish& in, BigFish* out)
```

* 文字列が返されます。文字列を返すときに非効率なオブジェクトのコピーを避けるために、`const std::string&`  戻り値の型が優先されます。このパターンを使用できない場合 (関数の呼び出しで "black", "red" などのリテラル文字列が使用されている場合など)、`const char*` を戻り値の型として使用する必要があります

```
const std::string& myFunction(...)
const char* myOtherFunction(...)
```
