Run the broker in multiservice way, with the following configuration:

* Default queue size 3 and 2 workers
* service `serv1` queue size 1 and 2 workers
* service `serv2` queue size 2 and 2 workers

```
contextBroker -fg -logLevel INFO -multiservice -notificationMode threadpool:3:2 -serviceQueues serv1:1:2:serv2:2:2
```

Run the accumulator server, as notification receptor.

```
accumulator-server.py --port 1028 --url /accumulate --host ::1 --pretty-print -v
```

We set up three subscriptions, for `serv1`, `serv2` and `serv3`. We use special URLs in the accumulator
that nerver get a response, in order to see the effect of a "blocked" worker.

```
curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'fiware-service: serv1' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "serv1 subscriptions",
  "subject": {
    "entities": [
      {
        "id": "E",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/waitForever"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'fiware-service: serv2' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "serv2 subscriptions",
  "subject": {
    "entities": [
      {
        "id": "E",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/waitForever"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'fiware-service: serv3' -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "serv3 subscriptions",
  "subject": {
    "entities": [
      {
        "id": "E",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/waitForever"
    }
  }
}
EOF
```

Now, let's stimulate the subscriptions to see how it works. Repeat 4 times the following request:

```
curl -v localhost:1026/v2/entities?options=upsert,forcedUpdate -s -S -H 'fiware-service: serv1' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E",
  "type": "T",
  "A": {
     "value": 1,
	 "type": "Number"
  }
}
EOF
```

We will see in the logs something like this:`

```
time=2021-05-28T13:47:08.129Z | lvl=INFO | corr=31d06648-bfbb-11eb-9f32-000c29df7912 | trans=1622197996-631-00000000008 | from=0.0.0.0 | srv=serv1 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:47:11.524Z | lvl=INFO | corr=33e29942-bfbb-11eb-981f-000c29df7912 | trans=1622197996-631-00000000010 | from=0.0.0.0 | srv=serv1 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:47:14.066Z | lvl=INFO | corr=35667b76-bfbb-11eb-b321-000c29df7912 | trans=1622197996-631-00000000012 | from=0.0.0.0 | srv=serv1 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:47:17.180Z | lvl=ERROR | corr=3741e840-bfbb-11eb-a449-000c29df7912 | trans=1622197996-631-00000000013 | from=0.0.0.0 | srv=serv1 | subsrv=<none> | comp=Orion | op=QueueNotifier.cpp[150]:sendNotifyContextRequest | msg=Runtime Error (serv1 notification queue is full)
time=2021-05-28T13:47:17.181Z | lvl=INFO | corr=3741e840-bfbb-11eb-a449-000c29df7912 | trans=1622197996-631-00000000013 | from=0.0.0.0 | srv=serv1 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
```

The notifications triggered by the first and second requests are taken by the workers, that keep waiting for a response that never comes.
The third notification is put in queue, which gets full (as queue size is only one). Thus, the fourth notification is discarded
and we see `Runtime Error (serv1 notification queue is full)`.

But the good thing is that queue saturation is isolated to `serv1`. In `serv2` notifications are still working as expected.
Let's repeat the following request five times:

```
curl -v localhost:1026/v2/entities?options=upsert,forcedUpdate -s -S -H 'fiware-service: serv2' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E",
  "type": "T",
  "A": {
     "value": 1,
	 "type": "Number"
  }
}
EOF
```

We will see something like this in the logs:

```
time=2021-05-28T13:50:07.222Z | lvl=INFO | corr=9c8c2bc0-bfbb-11eb-96ab-000c29df7912 | trans=1622197996-631-00000000014 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:50:10.546Z | lvl=INFO | corr=9e9495ba-bfbb-11eb-a880-000c29df7912 | trans=1622197996-631-00000000016 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:50:14.069Z | lvl=INFO | corr=a0b0ae2e-bfbb-11eb-ac1f-000c29df7912 | trans=1622197996-631-00000000018 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:50:18.628Z | lvl=INFO | corr=a36898ac-bfbb-11eb-8311-000c29df7912 | trans=1622197996-631-00000000019 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:50:20.955Z | lvl=ERROR | corr=a4cbb094-bfbb-11eb-a299-000c29df7912 | trans=1622197996-631-00000000020 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=QueueNotifier.cpp[150]:sendNotifyContextRequest | msg=Runtime Error (serv2 notification queue is full)
time=2021-05-28T13:50:20.955Z | lvl=INFO | corr=a4cbb094-bfbb-11eb-a299-000c29df7912 | trans=1622197996-631-00000000020 | from=0.0.0.0 | srv=serv2 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
```

This time, first 2 notifications are for the workers, 2 next ones are stored in the queue (which size is 2). The fifth
one doesn't fit in the queue, so it's discarded and we see the `Runtime Error (serv2 notification queue is full)` error trace.

But the default queue (i.e. the queue used by services that doesn't have a dedicated queue) is still working. Let's repeat
the following request six times:

```
curl -v localhost:1026/v2/entities?options=upsert,forcedUpdate -s -S -H 'fiware-service: serv3' -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E",
  "type": "T",
  "A": {
     "value": 1,
	 "type": "Number"
  }
}
EOF
```

We will see something like this in the logs:

```
time=2021-05-28T13:52:52.009Z | lvl=INFO | corr=fec817fe-bfbb-11eb-9bd3-000c29df7912 | trans=1622197996-631-00000000021 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:52:53.166Z | lvl=INFO | corr=ff85265a-bfbb-11eb-9bf7-000c29df7912 | trans=1622197996-631-00000000023 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:52:55.025Z | lvl=INFO | corr=00a0963c-bfbc-11eb-8e39-000c29df7912 | trans=1622197996-631-00000000025 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:52:57.782Z | lvl=INFO | corr=02459398-bfbc-11eb-bd61-000c29df7912 | trans=1622197996-631-00000000026 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:52:59.314Z | lvl=INFO | corr=032f447a-bfbc-11eb-a951-000c29df7912 | trans=1622197996-631-00000000027 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
time=2021-05-28T13:53:01.820Z | lvl=ERROR | corr=04ada9cc-bfbc-11eb-878f-000c29df7912 | trans=1622197996-631-00000000028 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=QueueNotifier.cpp[150]:sendNotifyContextRequest | msg=Runtime Error (default notification queue is full)
time=2021-05-28T13:53:01.820Z | lvl=INFO | corr=04ada9cc-bfbc-11eb-878f-000c29df7912 | trans=1622197996-631-00000000028 | from=0.0.0.0 | srv=serv3 | subsrv=<none> | comp=Orion | op=logTracing.cpp[130]:logInfoRequestWithPayload | msg=Request received: POST /v2/entities?options=upsert,forcedUpdate, request payload (72 bytes): {  "id": "E",  "type": "T",  "A": {     "value": 1, "type": "Number"  }}, response code: 204
```

This time, first 2 notifications are for the workers, 3 next ones are stored in the queue (which size is 3). The sixth
one doesn't fit in the queue, so it's discarded and we see the `Runtime Error (default notification queue is full)` error trace.

