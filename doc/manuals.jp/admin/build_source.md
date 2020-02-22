# ソースからのビルド

Orion Context Broker のリファレンス配布は CentOS 7.x です。これは、broker を他のディストリビューションに組み込むことができないことを意味しません (実際には可能です)。このセクションでは、他のディストリビューションをビルドする方法についても説明しています。CentOS を使用していない人に役立つかもしれません。ただし、"公式にサポートされている" 唯一の手順は CentOS 7.x 用の手順です。他のものは "現状のまま" 提供され、随時時代遅れになる可能性があります。

## CentOS 7.x (正式サポート)

Orion Context Broker は、以下のライブラリをビルドの依存関係として使用します :

* boost: 1.53
* libmicrohttpd: 0.9.48 (ソースから)
* libcurl: 7.29.0
* openssl: 1.0.2k
* libuuid: 2.23.2
* Mongo Driver: legacy-1.1.2 (ソースから)
* rapidjson: 1.0.2 (ソースから)
* gtest (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)
* gmock (`make unit_test` ビルディング・ターゲットのみ) : 1.5 (ソースから)

基本的な手順は次のとおりです (root 権限でコマンドを実行しないと仮定し、root 権限が必要なコマンドに sudo を使用します) :

* 必要なビルドツール (コンパイラなど) をインストールします

        sudo yum install make cmake gcc-c++ scons

* 必要なライブラリをインストールします (次の手順で説明する、ソースから取得する必要があるものを除きます)

        sudo yum install boost-devel libcurl-devel gnutls-devel libgcrypt-devel openssl-devel libuuid-devel cyrus-sasl-devel

* ソースから Mongo Driver をインストールします

        wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
        tar xfvz legacy-1.1.2.tar.gz
        cd mongo-cxx-driver-legacy-1.1.2
        scons  --use-sasl-client --ssl                                        # The build/linux2/normal/libmongoclient.a library is generated as outcome
        sudo scons install --prefix=/usr/local --use-sasl-client --ssl        # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib

* ソースから rapidjson をインストールする :

        wget https://github.com/miloyip/rapidjson/archive/v1.0.2.tar.gz
        tar xfvz v1.0.2.tar.gz
        sudo mv rapidjson-1.0.2/include/rapidjson/ /usr/local/include

* ソースから libmicrohttpd をインストールします (`./configure` 下のコマンドはライブラリの最小限のフットプリントを得るための推奨ビルド設定を示していますが、上級ユーザの方は好きなように設定できます)

        wget http://ftp.gnu.org/gnu/libmicrohttpd/libmicrohttpd-0.9.48.tar.gz
        tar xvf libmicrohttpd-0.9.48.tar.gz
        cd libmicrohttpd-0.9.48
        ./configure --disable-messages --disable-postprocessor --disable-dauth
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* ソースから Google Test/Mock をインストールします (このための RPM パッケージがありますが、現在の CMakeLists.txt の設定では動作しません)。以前は URL は http://googlemock.googlecode.com/files/gmock-1.5.0.tar.bz2 でしたが、Google では2016年8月下旬にそのパッケージを削除したため、動作しなくなりました

        yum install perl-Digest-MD5 libxslt
        wget ftp://repositories.lab.fiware.org/gmock-1.5.0.tar.bz2
        tar xfvj gmock-1.5.0.tar.bz2
        cd gmock-1.5.0
        ./configure
        make
        sudo make install  # installation puts .h files in /usr/local/include and library in /usr/local/lib
        sudo ldconfig      # just in case... it doesn't hurt :)

* コードを取得します (または、圧縮されたバージョンや別の URL パターンを使用してダウンロードできます。例えば、`git clone git@github.com:telefonicaid/fiware-orion.git`) :

        sudo yum install git
        git clone https://github.com/telefonicaid/fiware-orion

* ソースをビルドします :

        cd fiware-orion
        make

* (オプションですが強く推奨されます) ユニット・テストの実行です。まず、MongoDB をユニットとしてインストールする必要があり、機能テストは localhost で動作する mongod に依存しています。詳細については、[公式 MongoDB のドキュメント](https://docs.mongodb.com/manual/tutorial/install-mongodb-on-red-hat/)を確認してください。推奨バージョンは 3.6 です。ただし、3.2 および 3.4 も正常に動作するはずです

* yum を使用して、mongodb-org-shell をインストールするために、 /etc/yum.repos.d/mongodb.repo ファイルを作成します。

        [mongodb-org-3.6]
        name=MongoDB Repository
        baseurl=https://repo.mongodb.org/yum/redhat/$releasever/mongodb-org/3.6/x86_64/
        gpgcheck=1
        enabled=1
        gpgkey=https://www.mongodb.org/static/pgp/server-3.6.asc

* バイナリをインストールします。INSTALL_DIR を使用して、インストール・プレフィックス・パス (デフォルトは /usr) を設定することができます。したがって、broker は `$INSTALL_DIR/bin` ディレクトリにインストールされます

        sudo make install INSTALL_DIR=/usr

* broker のバージョン・メッセージを呼び出す、すべてが正常であることを確認してください :

        contextBroker --version

Orion Context Broker には、次の手順 (オプション) に従って実行することができる、valgrind およびエンド・ツー・エンドのテストの機能的なスイートが付属しています :

* 必要なツールをインストールします :

        sudo yum install python curl nc mongodb-org-shell valgrind bc python-devel libffi-devel python-pip

* テスト・ハーネスのための環境を準備します。基本的には、`accumulator-server.py` スクリプトをコントロールの下にあるパスにインストールしなければならず、`~/bin` が推奨です。また、`/usr/bin` のようなシステム・ディレクトリにインストールすることもできますが、RPM インストールと衝突する可能性がありますので、お勧めしません。さらに、ハーネス・スクリプト (`scripts/testEnv.sh` ファイル参照) で使用されるいくつかの環境変数を設定し、CentOS7 のデフォルトの Flask の代わりに Flask version 1.0.2 を使用するために、virtualenv 環境を作成する必要があります。この環境でテスト・ハーネスを実行します。

        mkdir ~/bin
        export PATH=~/bin:$PATH
        make install_scripts INSTALL_DIR=~
        . scripts/testEnv.sh
        pip install virtualenv
        virtualenv /opt/ft_env
        . /opt/ft_env/bin/activate
        pip install Flask==1.0.2 pyOpenSSL==19.0.0 enum34==1.1.6

* テスト・ハーネスを実行してください (時間がかかりますので、気をつけてください)

        make functional_test INSTALL_DIR=~

* すべての機能テストに合格したら、valgrind テストを実行できます (これは機能テストよりも時間がかかります) :

        make valgrind

次の手順を使用して、Orion Context Broker のカバレッジレポートを生成できます (オプション) :

* lcov ツールをインストールします

        sudo yum install lcov

* まず、unit_test と functional_test の成功パスを実行して、すべてが正常であることを確認します (上記参照)

* カバレッジを実行します

        make coverage INSTALL_DIR=~

ソースコードの RPM を生成することができます (オプション) :

* 必要なツールをインストールします

        sudo yum install rpm-build

* RPM を生成します

        make rpm

* 生成された RPM は `~/rpmbuild/RPMS/x86_64` ディレクトリに置かれます

### SASL と SSL をサポートする MongoDB ドライバの構築

手順は次のとおりです :

```
wget https://github.com/mongodb/mongo-cxx-driver/archive/legacy-1.1.2.tar.gz
tar xfvz legacy-1.1.2.tar.gz cd mongo-cxx-driver-legacy-1.1.2
yum install cyrus-sasl-devel
scons --use-sasl-client --ssl                                   # The build/linux2/normal/libmongoclient.a library is generated as outcome
sudo scons install --prefix=/usr/local --use-sasl-client --ssl  # This puts .h files in /usr/local/include/mongo and libmongoclient.a in /usr/local/lib
```

## その他

CentOS 7.x とは異なるシステムで Orion をビルドしている場合は、お気軽にご連絡いただき、このセクションの拡張にご協力ください。おそらく、新しい情報でこのファイルを変更するためのプル・リクエストを実行することが最良の方法です。ありがとう！

### Debian 7

パッケージは基本的に上記の RedHat/CentOS と同じですが、yum ではなく apt-get を使ってパッケージをインストールする必要があります。

Google Test と Google Mock バージョン 1.5 をソースから直接インストールします。

Debian 7.0 (1.9) に付属している lcov にはバグがあります (https://bugs.launchpad.net/ubuntu/+source/lcov/+bug/1163758 を参照)。ソースから lcov 1.10 をインストールします。
