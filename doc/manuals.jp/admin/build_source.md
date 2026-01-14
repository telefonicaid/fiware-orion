# ソースからのビルド

Orion Context Broker のリファレンス配布は Debian 13 です。これは、broker を他のディストリビューションに組み込むことができないことを意味しません (実際には可能です)。このセクションには、Debian を使用していない人に役立つ可能性があるため、他のディストリビューションに組み込む方法に関する指示が含まれる場合があります。ただし、"公式にサポートされている" 唯一の手順は Debian 13 用の手順です。

公式以外のディストリビューションで Docker コンテナ・イメージをビルドする方法は、Docker ドキュメントの [3.1 非公式ディストリビューションでのビルド](../../../docker/README.jp.md#31-building-in-not-official-distributions)・セクションで確認できます。

*注:* このドキュメントで説明されているビルド プロセスには cjexl ライブラリは含まれていません。これは、基本的なビルド プロセスの観点からはオプションであると見なされているためです。

## Debian 13 (正式サポート)

Orion Context Broker は、以下のライブラリをビルドの依存関係として使用します :

* boost: 1.83
* libmicrohttpd: 1.0.2 (ソースから)
* libcurl: 8.14.1
* openssl: 3.5.4
* libuuid: 2.41
* zlib1g 1:1.3.dfsg+really1.3.1-1+b1
* librdkafka: 2.8.0
* libmosquitto: 2.0.22 (ソースから)
* Mongo C driver: 2.2.1 (ソースから)
* rapidjson: 1.1.0 (ソースから)
* gtest (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)
* gmock (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)

基本的な手順は次のとおりです (root 権限でコマンドを実行しないと仮定し、root 権限が必要なコマンドに sudo を使用します) :

* 必要なビルドツール (コンパイラなど) をインストールします

        sudo apt-get install make cmake g++

* 必要なライブラリをインストールします (次の手順で説明する、ソースから取得する必要があるものを除きます)

        sudo apt-get install libssl-dev libcurl4-openssl-dev libboost-dev libboost-regex-dev libboost-filesystem-dev libboost-thread-dev uuid-dev libgnutls28-dev libsasl2-dev libgcrypt-dev librdkafka-dev zlib1g-dev libzstd-dev liblz4-dev

* ソースから Mongo Driver をインストールします

        wget https://github.com/mongodb/mongo-c-driver/releases/download/2.2.1/mongo-c-driver-2.2.1.tar.gz
        tar xfvz mongo-c-driver-2.2.1.tar.gz
        cd mongo-c-driver-2.2.1
        mkdir cmake-build
        cd cmake-build
        cmake ..
        make
        sudo make install

* ソースから rapidjson をインストールする :

        wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
        tar xfvz v1.1.0.tar.gz
        sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include
        # Next line is to comment a problematic line in document.h when GCC14+ is used
        sudo sed -i 's/^.*GenericStringRef& operator=(const GenericStringRef& rhs).*$/\/\/&/' /usr/local/include/rapidjson/document.h

* ソースから libmicrohttpd をインストールします (`./configure` 下のコマンドはライブラリの最小限のフットプリントを得るための推奨ビルド設定を示していますが、上級ユーザの方は好きなように設定できます)

        wget https://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-1.0.2.tar.gz
        tar xvf libmicrohttpd-1.0.2.tar.gz
        cd libmicrohttpd-1.0.2
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* ソースから mosquitto をインストールします (WITH_CJSON, WITH_STATIC_LIBRARIES, WITH_SHARED_LIBRARIES の設定を変更することで、mosquitto-2.0.22/ の下の config.mk ファイルを変更してビルドを微調整できます)

        wget https://mosquitto.org/files/source/mosquitto-2.0.22.tar.gz
        tar xvf mosquitto-2.0.22.tar.gz
        cd mosquitto-2.0.22
        sed -i 's/WITH_CJSON:=yes/WITH_CJSON:=no/g' config.mk
        sed -i 's/WITH_STATIC_LIBRARIES:=no/WITH_STATIC_LIBRARIES:=yes/g' config.mk
        sed -i 's/WITH_SHARED_LIBRARIES:=yes/WITH_SHARED_LIBRARIES:=no/g' config.mk
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # Update /etc/ld.so.cache with the new library files in /usr/local/lib

* コードを取得します (または、圧縮されたバージョンや別の URL パターンを使用してダウンロードできます。例えば、`git clone git@github.com:telefonicaid/fiware-orion.git`) :

        sudo apt-get install git
        git clone https://github.com/telefonicaid/fiware-orion

* ソースをビルドします :

        cd fiware-orion
        make

* (オプションですが強く推奨されます) 単体テストと機能テストを実行します。 これについては、[以下の特定のセクション](#testing-and-coverage)で詳しく説明します。

* バイナリをインストールします。INSTALL_DIR を使用して、インストール・プレフィックス・パス (デフォルトは /usr) を設定することができます。したがって、broker は `$INSTALL_DIR/bin` ディレクトリにインストールされます

        sudo make install INSTALL_DIR=/usr

* broker のバージョン・メッセージを呼び出す、すべてが正常であることを確認してください :

        contextBroker --version

<a name="testing-and-coverage"></a>

### テストとカバレッジ

Orion Context Broker には、次の手順に従って実行できる一連のユニット・テスト, valgrind, およびエンドツーエンドのテストが付属しています (オプションですが、強くお勧めします):

* ソースから GoogleTest/Mock をインストールします。以前の URL は http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 でしたが、Google は2016年8月下旬にそのパッケージを削除し、機能しなくなりました。

        wget https://src.fedoraproject.org/repo/pkgs/gmock/gmock-1.5.0.tar.bz2/d738cfee341ad10ce0d7a0cc4209dd5e/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        # システム内の fiware-orion リポジトリのローカル・コピーの場所に応じて、次の行の /path/to/fiware-orion を調整します。
        patch -p1 gtest/scripts/fuse_gtest_files.py < /path/to/fiware-orion/test/unittests/fuse_gtest_files.py.patch
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

aarch64 アーキテクチャの場合、apt-get を使用して libxslt をインストールし、`--build=arm-linux` オプションを指定して `/configure` を実行します。

* MongoDB をインストールします (テストはローカルホストで実行されている mongod に依存します)。 詳細については、[MongoDB の公式ドキュメント](https://www.mongodb.com/docs/manual/tutorial/install-mongodb-on-debian/)を確認してください。推奨バージョンは 8.0 です (以前のバージョンでも動作する可能性がありますが、お勧めしません)。

* ユニット・テストを実行します

        make unit_test

* 機能テストと valgrind テストに必要な追加のツールをインストールします:

        sudo apt-get install curl netcat-traditional valgrind bc python3 python3-pip mosquitto

* テスト・ハーネスのための環境を準備します。基本的には、`accumulator-server.py` スクリプトをコントロールの下にあるパスにインストールしなければならず、`~/bin` が推奨です。また、`/usr/bin` のようなシステム・ディレクトリにインストールすることもできますが、他のプログラムと衝突する可能性がありますので、お勧めしません。さらに、ハーネス・スクリプト (`scripts/testEnv.sh` ファイル参照) で使用されるいくつかの環境変数を設定し、必要な Python パッケージを使用して virtualenv 環境を作成します。

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        python3 -m venv /opt/ft_env   # or 'virtualenv /opt/ft_env --python=/usr/bin/python3' in some systems
        . /opt/ft_env/bin/activate
        pip install Flask==2.0.2 Werkzeug==2.0.2 paho-mqtt==1.6.1 amqtt==0.11.0b1 setuptools==80.9.0 confluent-kafka==2.11.0

* Kafkaクライアントをインストール (CLI).

      sudo apt-get install openjdk-21-jre-headless
      wget https://downloads.apache.org/kafka/3.9.1/kafka_2.12-3.9.1.tgz
      tar xvf kafka_2.12-3.9.1.tgz
      # Add kafka_2.12-3.9.1/bin to your PATH (e.g. editing ~/.bashrc)

次の手順では、[このドキュメント](../../../test/functionalTest/README.md#start-needed-services)で説明されているように、テストに必要なサービスをセットアップする必要があります。

* この環境でテスト・ハーネスを実行してください (時間がかかりますので、気をつけてください)

        make functional_test INSTALL_DIR=~

* すべての機能テストに合格したら、valgrind テストを実行できます (これは機能テストよりも時間がかかります) :

        make valgrind INSTALL_DIR=~

次の手順を使用して、Orion Context Broker のカバレッジレポートを生成できます (オプション) :

* lcov ツールをインストールします

        sudo apt-get install lcov xsltproc

* まず、unit_test と functional_test の成功パスを実行して、すべてが正常であることを確認します (上記参照)

* カバレッジを実行します

        make coverage INSTALL_DIR=~

*注意*: デバッグトレースに依存する機能テストは、カバレッジ実行時に失敗すると予想されます（例：notification_different_sizes や not_posix_regex_idpattern.test）。これは、デバッグトレースで使用される LM_T マクロが、条件カバレッジに「ノイズ」を追加するため、カバレッジコードビルドで無効化されているためです。これにより、カバレッジレポートの有用性が向上します。
