# ウォッチドッグ

Orion Context Broker は非常に安定していますが、失敗する可能性があります (broker の問題検出の詳細については、[問題の診断手順](diagnosis.md)のセクションを参照してください)。したがって、broker プロセスが停止したかどうかを検出するためにウォッチドッグ・プロセスを使用することをお勧めします。自動的に再起動したり、問題の通知を受けたりすることができます。

ウォッチドッグ・プログラムを自分で作成することもできます (例えば、定期的な基本で cron によって呼び出されたスクリプトは /etc/init.d/contextBroker の状態をチェックし、動作していない場合はそれを再起動し、通知メールを送信します) 。このセクションでは、[Monit](http://mmonit.com/monit/) を使用する手順について説明します。

まず、<http://rpmfind.net/linux/rpm2html/search.php?query=monit> にある RPM をインストールします。次の手順は、他のバージョンの RPM でも動作するはずですが、monit-5.1.1-4.el6.x86\_64.rpm を考慮して準備されています。

    sudo rpm -i monit-5.2.5-1.el5.rf.x86_64.rpm

monit 用のディレクトリを作成します。例 :

    /home/orion/monit_CB

そのディレクトリに monitBROKER.conf ファイルを作成します。この例では、CPU 負荷が2サイクルの間に80％を超える場合、または割り当てられたメモリが5サイクルにわたって 200MB を超える場合  (メモリリークの症状になります)、contextBroker を再起動するように monit を設定します。リソースチェックに加えて、プロセスがダウンしている場合、プロセスを再起動します。サイクルの持続時間は、monit コマンドライン・パラメータ (後述) を使用して定義されます。

    ###############################################################################
    ## Monit control file
    ###############################################################################
    ##
    ## Comments begin with a '#' and extend through the end of the line. Keywords
    ## are case insensitive. All path's MUST BE FULLY QUALIFIED, starting with '/'.
    ##
    ##
    ###############################################################################
    ## Global section
    ###############################################################################
    ##

    set logfile /var/log/contextBroker/monitBROKER.log

    set statefile /var/log/contextBroker/monit.state

    ###############################################################################
    ## Services
    ###############################################################################
    ##

    check host localhost with address localhost
       if failed (url http://localhost:1026/version and content == '<version>') for 3 cycles then
          exec "/etc/init.d/contextBroker stop"

    check file monitBROKER.log with path /var/log/contextBroker/monitBROKER.log
       if size > 50 MB then
          exec "/bin/bash -c '/bin/rm /var/log/contextBroker/monitBROKER.log; monit -c /home/localadmin/monit_CB/monitBROKER.conf -p /var/log/contextBroker/monit.pid reload'"

    check process contextBroker  with pidfile /var/log/contextBroker/contextBroker.pid    start program = "/etc/init.d/contextBroker start"    stop program  = "/etc/init.d/contextBroker stop"
        if cpu > 60% for 2 cycles then alert
        if cpu > 80% for 5 cycles then restart
        if totalmem > 200.0 MB for 5 cycles then restart

そのファイルの所有者を root にして、所有者のみのアクセス許可を設定します :

    sudo chown root:root monitBROKER.conf
    sudo chmod 0700 monitBROKER.conf

monit 開始スクリプト start\_monit\_BROKER.sh を作成します。"-d" コマンドライン・パラメータは、チェックサイクルの継続時間を指定するために使用します。この例では10秒に設定しています。

    monit -v -c /home/orion/monit_CB/monitBROKER.conf -d 10 -p /var/log/contextBroker/monit.pid

そのファイルの所有者を root にし、実行権限を設定します :

    sudo chown root:root start_monit_BROKER.sh
    sudo chmod a+x start_monit_BROKER.sh  

monit を実行するには :

    cd /home/orion/monit_CB
    sudo ./start_monit_BROKER.sh

monit が正常に動作していることを確認するには、プロセスが存在するかどうかを確認します。例

    # ps -ef | grep contextBroker
    500      27175     1  0 21:06 ?        00:00:00 monit -v -c /home/localadmin/monit_CB/monitBROKER.conf -d 10 -p /var/log/contextBroker/monit.pid
    500      27205     1  0 21:06 ?        00:00:00 /usr/bin/contextBroker -port 1026 -logDir /var/log/contextBroker -pidpath /var/log/contextBroker/contextBroker.pid -dbhost localhost -db orion;

そして、contextBroker を kill します。例 :

    #kill 27205

context broker がもう一度起動していることを、しばらくしてから (30秒以下) ps コマンドで確認してください。
