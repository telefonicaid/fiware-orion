apiary クライアントを使用してローカル・システムの .apib から HTML をレンダリングする方法の簡単なレシピ :

* apiary クライアントをインストールします。https://github.com/apiaryio/apiary-client を参照してください。
  ヒント : インストールに問題がある場合は、Ruby と libssl devel パッケージがインストールされているか
  どうか確認してください。
* 次のように使用します :

```
apiary preview --path=doc/apiary/v2/fiware-ngsiv2-reference.apib --output=/tmp/render.html
```

`scripts/apib_renders/` ディレクトリ内のスクリプトも参照してください。
