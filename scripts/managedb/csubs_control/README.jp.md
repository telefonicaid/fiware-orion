このディレクトリには、コンテキスト・サブスクリプション・コレクション (csubs) の制御に役立つスクリプトがいくつか
含まれています。それらはすべて同じ CLI 引数を共有します:

* `-db <database>` (必須): 処理する DB を指定します
* `--dry-run` (オプション): DB での実際の変更を回避し、レポートするだけです
* `--with-restart <status_file>` (オプション): データベースが操作され、続いて Context Broker を再起動する必要が
  ある場合は、Context Broker サービスを再起動します。これは `/etc/init.d/contextBroker` サービス・スクリプトに
  基づいているため、CB が RPM としてインストールされていない場合は機能しません。ルート権限が必要です。*(非推奨)*
* `-v` (オプション): 冗長モードを有効にします

スクリプトは次のとおりです:

* csub_dups_checker.py: csubs の重複を削除します。重複を検出するために使用されるサブスクリプションの 'signature'
  (署名) の一部と見なされる csub フィールドを知るために、スクリプトの実装を見てください
  * csub_dups_test.sh は、このプログラムのテストに使用できるヘルパー・スクリプトです
* csub_localhost_reference_checker.py: コールバックで "localhost" を使用してサブスクリプションをチェックし、
  許可されていない場合は削除します (valid_localhost_url() 関数の結果に基づいて、独自のチェックを実装するように
  変更する必要があります)
* csub_reference_limit_per_ip_checker.py: 許可されたしきい値を超える ONTIMEINTERVAL サブスクリプションを削除します
  (コードの MAX_PER_IP 変数で設定)
