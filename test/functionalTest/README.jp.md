## 機能テスト (functional tests) の実行方法

### 前提条件

* `contextBroker` バイナリは実行パスにある
* MongoDB データベースが稼働している
* accumulator-server.py が正しくインストールされ、実行パスで使用可能になっている (以下の特定のセクションを参照)

### accumulator-server.py をインストールする方法

オペレーティング・システムへの Python インストールとの潜在的な競合を回避するために、Python
[virtualenv](https://virtualenv.pypa.io/en/latest) を使用して必要な依存関係をインストールすることをお勧めします。
したがって、最初に仮想環境を作成します (たとえば `ft_env` のような名前を付けます)。

```
pip install virtualenv  # if you don't have virtualenv itself previously installed
virtualenv --python=/usr/bin/python3 /path/to/ft_env
```

次に、仮想環境をアクティブにします:

```
. /path/to/ft_env/bin/activate
```

次に、accumulator-server.py の依存関係をインストールします:

```
pip install Flask==2.0.2
pip install Werkzeug==2.0.2
pip install paho-mqtt==1.6.1
pip install amqtt==0.11.0b1  # Not actually an accumulator-server.py dependency, but needed by some tests
```

次に、accumulator-server.py のスクリプト自体をインストールします:

```
make install_scripts  # add INSTALL_DIR=... if you need to install in a specific place
```

パスに accumulator-server.py があることを確認します:

```
accumulator-server.py -u
```

**重要:** 機能テストを実行する前に、仮想環境 (`./path/to/ft_env/bin/activate`) をアクティブ化することを忘れないでください

#### Dockerを使用した代替インストール

次の Dockerfile を使用して、アキュムレータを構築できます;

```
FROM debian:stable
RUN apt-get update -y
RUN apt-get install -y python3 python3-venv python3-pip

# Create a virtual environment
RUN python3 -m venv /venv
ENV PATH="/venv/bin:$PATH"

# Install required packages within the virtual environment
RUN pip install Flask==2.0.2 Werkzeug==2.0.2 paho-mqtt==1.6.1

COPY . /app
WORKDIR /app
ENTRYPOINT [ "python3", "./accumulator-server.py"]
CMD ["--port", "1028", "--url", "/accumulate", "--host", "0.0.0.0", "-v"]
```

**重要な注意**: `docker build` コマンドを実行する前に、accumulator-server.py を Dockerfile と同じディレクトリにコピーしてください

ビルドしたら（たとえば `accum`という名前で）、次のコマンドで実行できます:

```
docker run -p 0.0.0.0:1028:1028/tcp accum
```

引数 `-p` に注意してください。コンテナ内の accumulator のリスニング・ポートがホスト内のポートにマップされます。

### 機能テストを実行

最も簡単な方法は、次のように実行することです:

```
cd test/functionalTest
./testHarness.sh
```

単一のファイルまたはフォルダのみを実行する場合は、次のようにファイルまたはフォルダへのパスを追加することもできます:

```
./testHarness.sh cases/3949_upsert_with_wrong_geojson/
```

テストの再試行回数を設定する場合は、env var `CB_MAX_TRIES` を使用できます (たとえば、失敗することがわかっているときにテストを
1回だけ実行したいが、出力を確認すると便利です)

もう1つの便利な env var は `CB_DIFF_TOOL` で、これにより、失敗したテストの差分を表示するツールを設定できます。
(例: [meld](https://meldmerge.org/))。

両方の env 変数の使用例として、次の行を参照してください:

```
CB_MAX_TRIES=1 CB_DIFF_TOOL=meld ./testHarness.sh cases/3949_upsert_with_wrong_geojson/
```

## 既知の問題点

### 10進数の四捨五入

テストを実行すると、次のような失敗が見つかる可能性があります:

```
-----  String filters for compound values of attributes: string match  -----
(qfilters_and_compounds_deeper.test) output not as expected
VALIDATION ERROR: input line:
                           "f": 3.1400000000000001,
does not match ref line:
                           "f": 3.14,
```

これは、テストまたは Orion の実際の問題ではありませんが、Python 2.6 に付属するバージョンの json モジュールの丸めの問題です
(testHarness.sh プログラムは通常、`python -mjson.tool` レスポンスを整形するために使用します)。

解決策は簡単です。Python2.6 を使用しないでください。推奨されるバージョンは Python2.7 です。CentOS6 にはシステムレベルで
Python2.6 が付属していますが、[virtualenv](https://virtualenv.pypa.io/en/stable/) を使用して Python2.7 を簡単に使用できることに
注意してください。

## その他

すべての .test ファイルの末尾の空白を削除するための便利なコマンド (テストが失敗した場合の比較のノイズが少なくなります):

```
find cases/ -name *.test -exec sed -i 's/[[:space:]]*$//' {} \;
```
