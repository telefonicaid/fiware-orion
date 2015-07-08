# Rush relayer

Apart from running Orion Context Broker in "stand alone" mode, you can
also take advanage of [Rush](https://github.com/telefonicaid/Rush) as
notification relayer. Thus, instead of managing the notifications itself
(including waiting for the HTTP timeout while the notification receives
responses), Orion passes the notification to Rush, which in turn deals
with it. Thus, Orion can implement a "fire and forget" policy for
notification sending, realying in Rush (a piece of sofware hihgly
specialized in that task) for that.

In addition, you can send notifications using HTTPS using Rush ,see
[security section in Users and Programmers
manual](../user/security.md).

In order to use Rush you need:

-   A running Rush instance network-reachable from Orion, e.g. in the
    same host and reachable using "localhost". The installation of Rush
    is out the scope of this manual, please check the [Rush
    documentation](https://github.com/telefonicaid/Rush/wiki) for that.
-   Run Orion using the -rush command line interface, which value has to
    be the Rush host and port, eg. `-rush localhost:1234` means that
    Rush is listening in port 1234 in localhost.

HTTP Rush relayer is used only for notifications. Other cases in which
Orion acts as HTTP client (eg. forwarding query/updates to Context Providers)
doesn't use Rush and Orion always sends the HTTP request itself.