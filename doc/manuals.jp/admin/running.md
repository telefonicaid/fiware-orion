# Orion をシステムサービスとして実行

Orion Context Broker をインストールすると、Orion Context Broker を実行する2つの方法があります : コマンドラインから手動実行、またはシステムサービスとして実行。後で Orion を RPM パッケージとしてインストールした場合のみ利用可能です。両方の方法を混在させることは推奨されません。たとえば、コマンドラインから context broker を起動し、/etc/init.d/contextBroker status その状態を確認するなどです。このセクションでは、Orion をシステム・サービスとして実行していることを前提としています。コマンドラインの代わりに、[このドキュメント](cli.md)をチェックしてください。

Orion Context Broker をシステムサービスとして使用するには、通常、スーパーユーザ権限が必要です。したがって、root または sudo コマンドを使用して次のコマンドを実行する必要があります。

broker サービスを開始するには、次のコマンドを実行します :

```
/etc/init.d/contextBroker start
```

コンテキスト・broker を停止するには、次のコマンドを実行します :

```
/etc/init.d/contextBroker stop
```

再起動するには、次のコマンドを実行します :

```
/etc/init.d/contextBroker restart
```

chkconfig コマンドを使用すると、システムのブート/シャットダウン時に contextBroker を自動的に起動/停止させることができます。詳細は、[chkconfig のドキュメント](http://www.centos.org/docs/5/html/Deployment_Guide-en-US/s1-services-chkconfig.html) を参照してください。

## 設定ファイル

contextBroker サービスによって使用される設定は、/etc/sysconfig/contextBroker ファイルに格納されます。典型的な内容は次のとおりです :

```
# BROKER_USER - Who to run orion-broker as. Note that you may need to use root if you want
# to run Orion in a privileged port (<1024)
BROKER_USER=orion

# BROKER_PORT - the port/socket where orion-broker will listen for connections
BROKER_PORT=1026

# BROKER_LOG_DIR - Where to log to
BROKER_LOG_DIR=/var/log/contextBroker

# BROKER_LOG_LEVEL - Log File Level
BROKER_LOG_LEVEL=WARN

# BROKER_PID_FILE - Where to store the pid for orion-broker
BROKER_PID_FILE=/var/run/contextBroker/contextBroker.pid

## Database configuration for orion-broker
BROKER_DATABASE_HOST=localhost
BROKER_DATABASE_NAME=orion

## Replica set configuration. Note that if you set this parameter, the BROKER_DATABASE_HOST
## is interpreted as the list of host (or host:port) separated by commas to use as
## replica set seed list (single element lists are also allowed). If BROKER_DATABASE_RPL_SET
## parameter is unset, Orion CB assumes that the BROKER_DATABASE_HOST is an stand-alone
## mongod instance
#BROKER_DATABASE_RPLSET=orion_rs

# Database authentication (not needed if MongoDB doesn't use --auth)
#BROKER_DATABASE_USER=orion
#BROKER_DATABASE_PASSWORD=orion

# Use the following variable if you need extra ops
#BROKER_EXTRA_OPS="-t 0-255"
```

BROKER\_USER および BROKER\_EXTRA\_OPS を除くすべてのフィールド は、次のように [コマンドライン・オプション](cli.md#command-line-options)で説明されているオプションのいずれかにマップされます :

-   BROKER\_USER は CLI オプションにはマッピングされませんが、init.d スクリプトは contextBroker プロセスの所有者を設定するために使用されます
-   BROKER\_PORT は -port にマップされます
-   BROKER\_LOG\_DIR は -logDir にマップされます
-   BROKER\_LOG\_LEVEL は -logLevel にマップされます
-   BROKER\_PID\_FILE は -pidpath にマップされます
-   BROKER\_DATABASE\_HOST は -dbhost にマップされます
-   BROKER\_DATABASE\_NAME は -db にマップされます
-   BROKER\_DATABASE\_RPLSET は -rplSet にマップされます
-   BROKER\_DATABASE\_USER は -dbuser にマップされます
-   BROKER\_DATABASE\_PASSWORD は -dbpwd にマップされます

BROKER\_EXTRA\_OPS に関しては、上記のフィールドで扱われていない他のオプションを、起動時に broker のコマンドラインに追加される文字列として指定するために使用されます。この文字列は "やみくも" に付加されていることに注意してください。つまり、service スクリプトは何もチェックしないので、注意してください。ここで有効なオプションを提供していることを確認し、他の BROKER\_\* フィールドにオプションを複製していないことを確認してください。例えば、BROKER\_PORT が使用されていれば、BROKER\_EXTRA\_OPS="-port 1026" は設定されません。

BROKER\_USER に関しては、起動時に contextBroker プロセスを所有するのはそのユーザです。デフォルトでは、RPM のインストールによって 'orion' という名前のユーザが作成されます。特権ポート (1024以下) で broker を実行する場合は、BROKER\_USER として 'root' を使用する必要があることに注意してください。

## ステータスの確認

broker の状態を確認するには、スーパーユーザ権限で、root ユーザまたは sudo コマンドを使用して、次のコマンドを使用します :

```
/etc/init.d/contextBroker status
```

broker が稼動している場合は、次のようになります :

```
Checking contextBroker...                         Running
```

broker が稼働していない場合、次のようになります :

```
Checking contextBroker...                         Service not running
```
