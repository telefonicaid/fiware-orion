# <a name="top"></a>開発マニュアル

*注 : このドキュメントでは、リリース 2.1.x の Orion Context Broker について説明しています。*

## 対象読者

このマニュアルの対象読者は、Orion Context Broker の機能を変更または追加するため、Orion Context Broker の内部を理解する必要がある開発者です。また、Orion Context Broker の実装方法に興味を持っている読者です。

Orion は C/C++ で書かれており、これらのプログラミング言語に関するこれまでの知識は、このドキュメントを理解するのに役立ちます。

Orion が依存している外部ライブラリに関するこれまでの知識も、理解に役立ちます。すなわち : 

* Microhttpd
* Libcurl
* Rapidjson (NGSIv2 JSON パーシング)
* MongoDB C++ ドライバ
* Boost property tree (NGSIv1 JSON パーシング)

MongoDB の場合は、ドライバの知識だけでなく、一般的な MongoDB 技術も推奨されます。

また、NGSI がリクエストのペイロードに使用されるため、NGSI の事前の知識が役立ちます。

## コンテンツ

* [ハイレベルの内部アーキテクチャ](architecture.md) : Orion Context Broker 内部アーキテクチャの概要。このドキュメントは他のドキュメントよりも先に読むことをお勧めします
* [ディレクトリ構造](directoryStructure.md) : Orion Context Broker リポジトリで使用されるディレクトリ構造の説明
* [ソースコード](sourceCode.md) : メインプログラムとライブラリ。ソースコードが構成されているさまざまなライブラリおよびメインプログラムの説明
* [フロー・インデックス](flowsIndex.md) : 開発ドキュメントに記載されているすべてのフロー図のインデックス。手元にある非常に便利な "マップ"
* [セマフォ](semaphores.md) : このドキュメントでは、Orion が内部同期に使用するさまざまなセマフォについて詳しく説明します
* [クックブック](cookbook.md) : このドキュメントでは、開発関連の便利なレシピについて説明します
