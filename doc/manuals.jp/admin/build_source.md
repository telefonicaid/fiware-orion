# ソースからのビルド

Orion Context Broker のリファレンス配布は CentOS 8.x です。これは、broker を他のディストリビューションに組み込むことができないことを意味しません (実際には可能です)。このセクションでは、他のディストリビューションをビルドする方法についても説明しています。CentOS を使用していない人に役立つかもしれません。ただし、"公式にサポートされている" 唯一の手順は CentOS 8.x 用の手順です。他のものは "現状のまま" 提供され、随時時代遅れになる可能性があります。

## CentOS 8.x (正式サポート)

Orion Context Broker は、以下のライブラリをビルドの依存関係として使用します :

* boost: 1.66
* libmicrohttpd: 0.9.70 (ソースから)
* libcurl: 7.61.1
* openssl: 1.1.1g
* libuuid: 2.32.1
* Mongo C driver: 1.17.4 (ソースから)
* rapidjson: 1.1.0 (ソースから)
* gtest (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)
* gmock (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)

基本的な手順は次のとおりです (root 権限でコマンドを実行しないと仮定し、root 権限が必要なコマンドに sudo を使用します) :

* 必要なビルドツール (コンパイラなど) をインストールします

        sudo yum install make cmake gcc-c++

* 必要なライブラリをインストールします (次の手順で説明する、ソースから取得する必要があるものを除きます)

        sudo yum install boost-devel libcurl-devel gnutls-devel libgcrypt-devel openssl-devel libuuid-devel cyrus-sasl-devel

* ソースから Mongo Driver をインストールします

        wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.4/mongo-c-driver-1.17.4.tar.gz
        tar xfvz mongo-c-driver-1.17.4.tar.gz
        cd mongo-c-driver-1.17.4
        mkdir cmake-build
        cd cmake-build
        cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
        make
        sudo make install

* ソースから rapidjson をインストールする :

        wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
        tar xfvz v1.1.0.tar.gz
        sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include

* ソースから libmicrohttpd をインストールします (`./configure` 下のコマンドはライブラリの最小限のフットプリントを得るための推奨ビルド設定を示していますが、上級ユーザの方は好きなように設定できます)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz
        tar xvf libmicrohttpd-0.9.70.tar.gz
        cd libmicrohttpd-0.9.70
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* コードを取得します (または、圧縮されたバージョンや別の URL パターンを使用してダウンロードできます。例えば、`git clone git@github.com:telefonicaid/fiware-orion.git`) :

        sudo yum install git
        git clone https://github.com/telefonicaid/fiware-orion

* ソースをビルドします :

        cd fiware-orion
        make

* (オプションですが強く推奨されます) 単体テストと機能テストを実行します。 これについては、[以下の特定のセクション](#testing-coverage-and-rpm)で詳しく説明します。

* バイナリをインストールします。INSTALL_DIR を使用して、インストール・プレフィックス・パス (デフォルトは /usr) を設定することができます。したがって、broker は `$INSTALL_DIR/bin` ディレクトリにインストールされます

        sudo make install INSTALL_DIR=/usr

* broker のバージョン・メッセージを呼び出す、すべてが正常であることを確認してください :

        contextBroker --version

### テスト, カバレッジと RPM

Orion Context Broker には、次の手順に従って実行できる一連のユニット・テスト, valgrind, およびエンドツーエンドのテストが付属しています (オプションですが、強くお勧めします):

* ソースから GoogleTest/Mock をインストールします (これには RPM パッケージがありますが、現在の CMakeLists.txt 構成では機能しません)。以前の URL は http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 でしたが、Google は2016年8月下旬にそのパッケージを削除し、機能しなくなりました。

        sudo yum install python2
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sed -i 's/env python/env python2/' gtest/scripts/fuse_gtest_files.py  # little hack to make installation to work on CentOS 8
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

aarch64 アーキテクチャの場合、yum を使用して perl-Digest-MD5 と libxslt をインストールし、`--build=arm-linux` オプションを指定して `/configure` を実行します。

* MongoDB をインストールします (テストはローカルホストで実行されている mongod に依存します)。詳細については、[MongoDB の公式ドキュメント](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/)を確認してください。推奨バージョンは 4.4 です (以前のバージョンで動作する可能性がありますが、お勧めしません)。

* ユニット・テストを実行します

        make unit_test

* 機能テストと valgrind テストに必要な追加のツールをインストールします:

        sudo yum install curl nc valgrind bc
        sudo pip2 install virtualenv

aarch64 アーキテクチャの場合、さらに yum で、python-devel と libffi-devel をインストールします。これは、pyOpenSSL をビルドするときに必要です。


* テスト・ハーネスのための環境を準備します。基本的には、`accumulator-server.py` スクリプトをコントロールの下にあるパスにインストールしなければならず、`~/bin` が推奨です。また、`/usr/bin` のようなシステム・ディレクトリにインストールすることもできますが、RPM インストールと衝突する可能性がありますので、お勧めしません。さらに、ハーネス・スクリプト (`scripts/testEnv.sh` ファイル参照) で使用されるいくつかの環境変数を設定し、必要な Python パッケージを使用して virtualenv 環境を作成します。

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        virtualenv /opt/ft_env --python=/usr/bin/python2
        . /opt/ft_env/bin/activate
        pip install Flask==1.0.2 pyOpenSSL==19.0.0

* この環境でテスト・ハーネスを実行してください (時間がかかりますので、気をつけてください)

        make functional_test INSTALL_DIR=~

* すべての機能テストに合格したら、valgrind テストを実行できます (これは機能テストよりも時間がかかります) :

        make valgrind INSTALL_DIR=~

次の手順を使用して、Orion Context Broker のカバレッジレポートを生成できます (オプション) :

* lcov ツールをインストールします

        # Download .rpm file from http://downloads.sourceforge.net/ltp/lcov-1.14-1.noarch.rpm
        sudo yum install lcov-1.14-1.noarch.rpm

* まず、unit_test と functional_test の成功パスを実行して、すべてが正常であることを確認します (上記参照)

* カバレッジを実行します

        make coverage INSTALL_DIR=~

ソースコードの RPM を生成することができます (オプション) :

* 必要なツールをインストールします

        sudo yum install rpm-build

* RPM を生成します

        make rpm

* 生成された RPM は `~/rpmbuild/RPMS/x86_64` ディレクトリに置かれます

## Ubuntu 20.04 LTS

この手順は、Ubuntu 20.04 LTS 上で x86_64 および aarch64 アーキテクチャ用の Orion Context Broker をすることです。
また、Orion が依存する MongoDB 4.4 をビルドするための手順が含まれています。Orion Context Brokerは、ビルドの依存関係と
して次のライブラリを使用します :

* boost: 1.71.0
* libmicrohttpd: 0.9.70 (from source)
* libcurl: 7.68.0
* openssl: 1.1.1f
* libuuid: 2.34-0.1
* Mongo C driver: 1.17.4 (from source)
* rapidjson: 1.1.0 (from source)
* gtest (only for `make unit_test` building target): 1.5 (from sources)
* gmock (only for `make unit_test` building target): 1.5 (from sources)

基本的な手順は次のとおりです (root 権限でコマンドを実行しないと仮定し、root 権限が必要なコマンドに sudo を使用します) :

* 必要なビルドツール (コンパイラなど) をインストールします

        sudo apt install build-essential cmake

* 必要なライブラリをインストールします (次の手順で説明する、ソースから取得する必要があるものを除きます)

        sudo apt install libboost-dev libboost-regex-dev libboost-thread-dev libboost-filesystem-dev \
                         libcurl4-gnutls-dev gnutls-dev libgcrypt-dev libssl-dev uuid-dev libsasl2-dev

* ソースから Mongo Driver をインストールします

        wget https://github.com/mongodb/mongo-c-driver/releases/download/1.17.4/mongo-c-driver-1.17.4.tar.gz
        tar xfvz mongo-c-driver-1.17.4.tar.gz
        cd mongo-c-driver-1.17.4
        mkdir cmake-build
        cd cmake-build
        cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
        make
        sudo make install

* ソースから rapidjson をインストールします :

        wget https://github.com/miloyip/rapidjson/archive/v1.1.0.tar.gz
        tar xfvz v1.1.0.tar.gz
        sudo mv rapidjson-1.1.0/include/rapidjson/ /usr/local/include

* ソースから libmicrohttpd をインストールします (`./configure` 下のコマンドはライブラリの最小限のフットプリントを
得るための推奨ビルド設定を示していますが、上級ユーザの方は好きなように設定できます)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.70.tar.gz
        tar xvf libmicrohttpd-0.9.70.tar.gz
        cd libmicrohttpd-0.9.70
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* コードを取得します (または、圧縮されたバージョンや別の URL パターンを使用してダウンロードできます。例えば、
`git clone git@github.com:telefonicaid/fiware-orion.git`) :

        sudo apt install git
        git clone https://github.com/telefonicaid/fiware-orion

* ソースをビルドします :

        cd fiware-orion
        make

* (オプションですが、強くお勧めします) 単体テスト (unit test) と機能テスト (functional tests) を実行します。
詳細については、[以下のセクション](#testing-and-coverage)をご覧ください。

* バイナリをインストールします。INSTALL_DIR を使用して、インストール・プレフィックス・パス (デフォルトは /usr) を設定する
ことができます。したがって、broker は `$INSTALL_DIR/bin` ディレクトリにインストールされます

        sudo make install INSTALL_DIR=/usr

* broker のバージョン・メッセージを呼び出し、すべてが正常であることを確認してください :

        contextBroker --version

<a name="testing-and-coverage"></a>
### テストとカバレッジ

Orion Context Broker には、次の手順 (オプション) に従って実行することができる、valgrind およびエンド・ツー・エンドのテストの
機能的なスイートが付属しています :

* ソースから Google Test/Mock をインストールします (このための RPM パッケージがありますが、現在の CMakeLists.txt
の設定では動作しません)。以前は URL は http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 でしたが、
Google では2016年8月下旬にそのパッケージを削除したため、動作しなくなりました

        sudo apt install python-is-python2 xsltproc
        wget https://nexus.lab.fiware.org/repository/raw/public/storage/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

aarch64 アーキテクチャの場合、`.-configure` を `--build=arm-linux` オプションとともに実行します。

* MongoDB をインストールします (テストはローカルホストで実行されている mongod に依存します)。詳細については、
  [MongoDB の公式ドキュメント](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-ubuntu/)を確認してください。
  推奨バージョンは 4.4 です (以前のバージョンで動作する可能性がありますが、お勧めしません)。

* 単体テストを実行します :

        make unit_test

* 機能テストと valgrind テストに必要な追加のツールをインストールします :

        curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py 
        sudo python get-pip.py 
        sudo apt install netcat valgrind bc 
        sudo pip install --upgrade pip 
        pip install virtualenv

aarch64 アーキテクチャの場合、さらに apt で、`python2-dev` と `libffi-dev` をインストールします。これは、pyOpenSSL をビルドするときに必要です。

* テスト・ハーネスのための環境を準備します。基本的には、`accumulator-server.py` スクリプトをコントロールの下にあるパスに
インストールしなければならず、`~/bin` が推奨です。また、`/usr/bin` のようなシステム・ディレクトリにインストールすることも
できますが、RPM インストールと衝突する可能性がありますので、お勧めしません。さらに、ハーネス・スクリプト (`scripts/testEnv.sh`
ファイル参照) で使用されるいくつかの環境変数を設定し、Ubuntu のデフォルトの Flask の代わりに Flask version 1.0.2 を使用する
ために、virtualenv 環境を作成する必要があります。この環境でテスト・ハーネスを実行します。

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        virtualenv /opt/ft_env
        . /opt/ft_env/bin/activate
        pip install Flask==1.0.2 pyOpenSSL==19.0.0

* テスト・ハーネスを実行してください (時間がかかりますので、気をつけてください) make コマンドでテストを開始する前に、テストが失敗しないように次のパッチを適用してください。

        sed -i -e "s/Peer certificate cannot be authenticated[^\"]*/SSL peer certificate or SSH remote key was not OK/" /opt/fiware-orion/test/functionalTest/cases/0706_direct_https_notifications/direct_https_notifications_no_accept_selfsigned.test
        make functional_test INSTALL_DIR=~

* すべての機能テストに合格したら、valgrind テストを実行できます (これは機能テストよりも時間がかかります) :

        make valgrind

次の手順を使用して、Orion Context Broker のカバレッジレポートを生成できます (オプション) :

* lcov ツールをインストールします

        sudo apt install lcov

* まず、unit_test と functional_test の成功パスを実行して、すべてが正常であることを確認します (上記参照)

* カバレッジを実行します

        make coverage INSTALL_DIR=~

* テストの実行後にシステムサービスとして Orion を実行するためのセットアップを行います。`curl localhost:1026/version` コマンドを実行して、
セットアップが成功したことを確認します :

        sudo mkdir /etc/sysconfig
        sudo cp /opt/fiware-orion/etc/config/contextBroker /etc/sysconfig/
        sudo touch /var/log/contextBroker/contextBroker.log
        sudo chown orion /var/log/contextBroker/contextBroker.log
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/logrotate.d/logrotate-contextBroker-daily /etc/logrotate.d/
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/sysconfig/logrotate-contextBroker-size /etc/sysconfig/
        sudo cp /opt/fiware-orion/rpm/SOURCES/etc/cron.d/cron-logrotate-contextBroker-size /etc/cron.d/
        sudo systemctl daemon-reload
        sudo systemctl start contextBroker.service 
