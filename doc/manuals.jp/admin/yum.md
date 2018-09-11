## Yum リポジトリの使用

このドキュメントでは、FIWARE Yum リポジトリを使用して Orion Context Broker をインストールする際のガイドラインについて説明します。提供される構成は、x86_64 アーキテクチャ および CentOS/RHEL 7 OS に対応します。

利用可能なリポジトリは 2つあります :

* Nightly、ナイトリー・パッケージ
* Release、リリース・パッケージ

[ここ](install.md#installation)でパッケージの違いについて読むことができます。

リポジトリの設定を手動で追加することも、FIWARE パブリック・リポジトリからダウンロードすることもできます。同じサーバ上で両方のリポジトリを一緒に使用すると、ナイトリー・パッケージは常にリリース・パッケージより先になることに注意してください。

リリース・リポジトリには、この構成を使用します :

```
[fiware-release]
name=FIWARE release repository
baseurl=https://nexus.lab.fiware.org/repository/el/7/x86_64/release
enabled=1
gpgcheck=0
priority=1

```
または、[FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo) からダウンロードします。

```
sudo wget -P /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-release.repo
```

ナイトリー・リポジトリには、この設定を使用します :

```
[fiware-nightly]
name=FIWARE nightly repository
baseurl=https://nexus.lab.fiware.org/repository/el/7/x86_64/nightly
enabled=1
gpgcheck=0
priority=1

```
または、[FIWARE public repository](https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo) からダウンロードします。

```
sudo wget -P /etc/yum.repos.d/ https://nexus.lab.fiware.org/repository/raw/public/repositories/el/7/x86_64/fiware-nightly.repo

```

次に、ContextBroker をインストールするだけです

```
sudo yum install contextBroker 
```
