This test was conducted during the development of flow control feature (version: 2.3.0-next).

Assuming the DB is empty (i.e. there aren't any entity or subscription previous to the start of the test), start ContextBroker the following way:

```
contextBroker -logForHumans -fg -notifFlowControl 1:1000:10000000 -notificationMode threadpool:1000:1 -t 160 -logLevel DEBUG | grep -v INFO
```

This is the startup log:

```
DEBUG@11:20:23  contextBroker.cpp[953]: notification mode: 'threadpool', queue size: 1000, num threads 1
DEBUG@11:20:23  contextBroker.cpp[964]: notification flow control: enabled - gauge: 1.000000, stepDelay: 1000, maxInterval: 10000000
DEBUG@11:20:23  QueueNotifier.cpp[43]: Setting up queue and threads for notifications
```

Note this configuration means:

* ContextBroker uses a threadpool as notification node, with just one worker and queue length of 1000. Given there is only one worker, slowness
  in the processing of the notifications is expected (and this is what we want, in order to perceive the effect of the flow control mechanism)
* The notification flow control algorithm is configured as follows:
    * gauge: 1
    * stepDelay: 1000 ms (1 second)
    * maxInterval: 10000000 ms. This is abnormaly high (for the purpose of this test, this is like "infinite") so we can see the effect of flow control properly

In a separate terminal, start the accumulator server:

```
accumulator-server.py --port 1028 --url /accumulate --host ::1 --pretty-print -v
```

Create two entities (E1 and E10) at ContextBroker:

```
curl localhost:1026/v2/entities -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E1",
  "type": "T",
  "temp": {
    "value": 23,
    "type": "Float"
  }
}
EOF

curl localhost:1026/v2/entities -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "id": "E10",
  "type": "T",
  "temp": {
    "value": 23,
    "type": "Float"
  }
}
EOF
```

Create 1 subscription for entity E1. Thus, each time E1 gets updated a notification will be sent (1:1 relationship).
Note we use `skipInitialNotification` to avoid a "noisy" initial notification to be sent.

```
curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E1 subscription",
  "subject": {
    "entities": [
      {
        "id": "E1",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF
```

Create 10 subscriptions for entity E10. Thus, each time E10 gets updated ten notifications will be sent (1:10 relationship).
Note we use `skipInitialNotification` to avoid a "noisy" initial notification to be sent.

```
curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 1",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 2",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 3",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 4",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 5",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 6",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 7",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 8",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 9",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF

curl -v localhost:1026/v2/subscriptions?options=skipInitialNotification -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "description": "E10 subscription 10",
  "subject": {
    "entities": [
      {
        "id": "E10",
        "type": "T"
      }
    ]    
  },
  "notification": {
    "http": {
      "url": "http://localhost:1028/givemeDelay"
    }
  }
}
EOF
```

Note we are using the `/givemeDelay` endpoint in accumulator-server in every subscriptions. Thus, accumulator-server waits
60 seconds before responding the notification.

We will use the following update requests to update E1 and E10 respectively (note the usage of `forceUpdate`, to trigger notifications
even if we are using 23 as attribute value all the time, to avoid the need of changing the value each time we use the request).
The first one (for E1) doesn't use flow control, the second onde (for E10) uses flow control.

```
# update1request (without flow control)
curl -X PATCH localhost:1026/v2/entities/E1/attrs?options=forcedUpdate -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "temp": {
    "value": 23,
    "type": "Float"
  }
}
EOF

# update10request (with flow control)
curl -X PATCH localhost:1026/v2/entities/E10/attrs?options=forcedUpdate,flowControl -s -S -H 'Content-Type: application/json' -d @- <<EOF
{
  "temp": {
    "value": 23,
    "type": "Float"
  }
}
EOF
```

So, first we will generate a notifications queue at CB, so next we can see the impact of flow control on it. To do
so, we execute update1request ten times. After that, we wait a couple of seconds, then execute update10request one time,
which will cause a block of 10 notifications to be added to the queue and start the flow control mechanism before
responding the update request (we will use `time` bash command to know how much time it takes this response). Basically,
this is the test script we are executing:

```
for i in $(seq 1 10)
do
  update1request
done
sleep 2
time update10request
```

Let's examine the ContextBroker log. The first block corresponds to the ten update1request executions: 

```
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 0
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 0
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 2
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 3
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 4
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:43  mongoUpdateContext.cpp[126]: notification queue size before processing update: 5
DEBUG@12:30:43  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:44  mongoUpdateContext.cpp[126]: notification queue size before processing update: 6
DEBUG@12:30:44  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:44  mongoUpdateContext.cpp[126]: notification queue size before processing update: 7
DEBUG@12:30:44  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
DEBUG@12:30:44  mongoUpdateContext.cpp[126]: notification queue size before processing update: 8
DEBUG@12:30:44  mongoUpdateContext.cpp[161]: total notifications sent during update: 1
```

At this point of time, ContextBroker worker is processing the first notification (corresponds to line: "worker sending to:") and
there are 9 additional notifications waiting in the queue.

Two seconds after that, update10request is received. This adds 10 additional notifications to the queue (so now
we have 19). The response to the update is not done inmediatelly, is put on hold until the flow control mechanism
returns control. The target queue size to return control is 9. This corresponds to the size of the queue at
the moment before starting processing the update. Remember the formula is 
`target = queue_size_before_processing_update + (1 - gauge) * notifications_added_by_the_update`,
so given that gauge set at ContextBroker startup is 1, then `target = queue_size_before_processing_update = 9`.

```
DEBUG@12:30:46  mongoUpdateContext.cpp[126]: notification queue size before processing update: 9
DEBUG@12:30:46  mongoUpdateContext.cpp[161]: total notifications sent during update: 10
DEBUG@12:30:46  mongoUpdateContext.cpp[174]: start notification flow control algorithm
DEBUG@12:30:46  mongoUpdateContext.cpp[60]: flow control target is 9
```
 	
The flow control mechanism starts...

```
DEBUG@12:30:46  mongoUpdateContext.cpp[70]: flow control pass 0, delay 0, notification queue size is: 19
DEBUG@12:30:47  mongoUpdateContext.cpp[83]: flow control pass 1, delay 1000, notification queue size is: 19
DEBUG@12:30:48  mongoUpdateContext.cpp[83]: flow control pass 2, delay 2000, notification queue size is: 19
WARN@12:30:48  AlarmManager.cpp[328]: Raising alarm NotificationError localhost:1028/givemeDelay: notification failure for queue worker: Timeout was reached
DEBUG@12:30:48  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:30:49  mongoUpdateContext.cpp[83]: flow control pass 3, delay 3000, notification queue size is: 18
DEBUG@12:30:50  mongoUpdateContext.cpp[83]: flow control pass 4, delay 4000, notification queue size is: 18
DEBUG@12:30:51  mongoUpdateContext.cpp[83]: flow control pass 5, delay 5000, notification queue size is: 18
DEBUG@12:30:52  mongoUpdateContext.cpp[83]: flow control pass 6, delay 6000, notification queue size is: 18
DEBUG@12:30:53  mongoUpdateContext.cpp[83]: flow control pass 7, delay 7000, notification queue size is: 18
DEBUG@12:30:53  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:30:54  mongoUpdateContext.cpp[83]: flow control pass 8, delay 8000, notification queue size is: 17
DEBUG@12:30:55  mongoUpdateContext.cpp[83]: flow control pass 9, delay 9000, notification queue size is: 17
DEBUG@12:30:56  mongoUpdateContext.cpp[83]: flow control pass 10, delay 10000, notification queue size is: 17
DEBUG@12:30:57  mongoUpdateContext.cpp[83]: flow control pass 11, delay 11000, notification queue size is: 17
DEBUG@12:30:58  mongoUpdateContext.cpp[83]: flow control pass 12, delay 12000, notification queue size is: 17
DEBUG@12:30:58  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:30:59  mongoUpdateContext.cpp[83]: flow control pass 13, delay 13000, notification queue size is: 16
DEBUG@12:31:00  mongoUpdateContext.cpp[83]: flow control pass 14, delay 14000, notification queue size is: 16
...
```

In each pass, the queue length is checked. Note each pass is done with an interval of 1 second (the stepDelay
configured at ContextBroker startup). From pass 0 to 2 there isn't any change. But at 12:30:48 the default HTTP timeout
expires (note that `-httpTimeout` is not used in ContextBroker startup, so default is 5 seconds), so the worker stops
waiting for the notification response and takes the next notification from queue (again, the "worker sending to" message).
The timeout condition is shown by the WARN message (*). Thus, in the next pass of the algorithm (pass 3) we see a
decrease of the queue, from 19 to 18.

This happens again at pass 8 and pass 13. Each time the worker stops waiting for its current notification and takes the
next one, the queue length decreases (to 17, to 16, etc.).

```
...
DEBUG@12:31:01  mongoUpdateContext.cpp[83]: flow control pass 15, delay 15000, notification queue size is: 16
DEBUG@12:31:02  mongoUpdateContext.cpp[83]: flow control pass 16, delay 16000, notification queue size is: 16
DEBUG@12:31:03  mongoUpdateContext.cpp[83]: flow control pass 17, delay 17000, notification queue size is: 16
DEBUG@12:31:03  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:04  mongoUpdateContext.cpp[83]: flow control pass 18, delay 18000, notification queue size is: 15
DEBUG@12:31:05  mongoUpdateContext.cpp[83]: flow control pass 19, delay 19000, notification queue size is: 15
DEBUG@12:31:06  mongoUpdateContext.cpp[83]: flow control pass 20, delay 20000, notification queue size is: 15
DEBUG@12:31:07  mongoUpdateContext.cpp[83]: flow control pass 21, delay 21000, notification queue size is: 15
DEBUG@12:31:08  mongoUpdateContext.cpp[83]: flow control pass 22, delay 22000, notification queue size is: 15
DEBUG@12:31:08  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:09  mongoUpdateContext.cpp[83]: flow control pass 23, delay 23000, notification queue size is: 14
DEBUG@12:31:10  mongoUpdateContext.cpp[83]: flow control pass 24, delay 24000, notification queue size is: 14
DEBUG@12:31:11  mongoUpdateContext.cpp[83]: flow control pass 25, delay 25000, notification queue size is: 14
DEBUG@12:31:12  mongoUpdateContext.cpp[83]: flow control pass 26, delay 26000, notification queue size is: 14
DEBUG@12:31:13  mongoUpdateContext.cpp[83]: flow control pass 27, delay 27000, notification queue size is: 14
DEBUG@12:31:13  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:14  mongoUpdateContext.cpp[83]: flow control pass 28, delay 28000, notification queue size is: 13
DEBUG@12:31:15  mongoUpdateContext.cpp[83]: flow control pass 29, delay 29000, notification queue size is: 13
DEBUG@12:31:16  mongoUpdateContext.cpp[83]: flow control pass 30, delay 30000, notification queue size is: 13
DEBUG@12:31:17  mongoUpdateContext.cpp[83]: flow control pass 31, delay 31000, notification queue size is: 13
DEBUG@12:31:18  mongoUpdateContext.cpp[83]: flow control pass 32, delay 32000, notification queue size is: 13
DEBUG@12:31:18  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:19  mongoUpdateContext.cpp[83]: flow control pass 33, delay 33000, notification queue size is: 12
DEBUG@12:31:20  mongoUpdateContext.cpp[83]: flow control pass 34, delay 34000, notification queue size is: 12
DEBUG@12:31:21  mongoUpdateContext.cpp[83]: flow control pass 35, delay 35000, notification queue size is: 12
DEBUG@12:31:22  mongoUpdateContext.cpp[83]: flow control pass 36, delay 36000, notification queue size is: 12
DEBUG@12:31:23  mongoUpdateContext.cpp[83]: flow control pass 37, delay 37000, notification queue size is: 12
DEBUG@12:31:23  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:24  mongoUpdateContext.cpp[83]: flow control pass 38, delay 38000, notification queue size is: 11
DEBUG@12:31:25  mongoUpdateContext.cpp[83]: flow control pass 39, delay 39000, notification queue size is: 11
DEBUG@12:31:26  mongoUpdateContext.cpp[83]: flow control pass 40, delay 40000, notification queue size is: 11
DEBUG@12:31:27  mongoUpdateContext.cpp[83]: flow control pass 41, delay 41000, notification queue size is: 11
DEBUG@12:31:28  mongoUpdateContext.cpp[83]: flow control pass 42, delay 42000, notification queue size is: 11
DEBUG@12:31:28  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:29  mongoUpdateContext.cpp[83]: flow control pass 43, delay 43000, notification queue size is: 10
DEBUG@12:31:30  mongoUpdateContext.cpp[83]: flow control pass 44, delay 44000, notification queue size is: 10
DEBUG@12:31:31  mongoUpdateContext.cpp[83]: flow control pass 45, delay 45000, notification queue size is: 10
...
```

At pass 45 the flow control mechanism has almost reached the target. Target is 9 and queue size is 10.

```
...
DEBUG@12:31:32  mongoUpdateContext.cpp[83]: flow control pass 46, delay 46000, notification queue size is: 10
DEBUG@12:31:33  mongoUpdateContext.cpp[83]: flow control pass 47, delay 47000, notification queue size is: 10
DEBUG@12:31:33  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:34  mongoUpdateContext.cpp[83]: flow control pass 48, delay 48000, notification queue size is: 9
DEBUG@12:31:34  mongoUpdateContext.cpp[176]: end notification flow control algorithm
```

At 12:31:33, just after pass 47, the worker takes next notification and queue size decreases to 9. Thus,
in the next pass at 12:31:34 (pass 48) we have reached the target. The notification flow control mechanism
returns control and ContextBroker finally responses to the client of the update.

Thus, the client sending update10request gets its response not inmediatelly (as it would be the case without `flowControl`
option in the update request) but 48 seconds after.

After responding the client update, the worker continues sending pending notifications. Thus, we can see nine new messages in the
log (with an interval of 5 seconds) corresponding to the last 9 notifications in the queue.

```
DEBUG@12:31:38  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:43  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:48  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:53  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:31:58  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:32:03  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:32:08  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:32:13  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
DEBUG@12:32:18  QueueWorkers.cpp[121]: worker sending to: host='localhost', port=1028, verb=POST, tenant='', service-path: '', xauthToken: '', path='/givemeDelay', content-type: application/json
```

Note that we have used a maxInterval value large enough to let the flow control mechanism to reach the target.
However, we could use it to put a maximum waiting limit for clients. For instance, if we haved used maxInterval 20 seconds
then we have reached the max delay at pass 20 and the flow control mechanism would have returned control at that point,
although the notifications queue at this point was 15 (higher than the target by 6). The client would have waited
~20 seconds for the response (instead of 48 seconds).

(*) Note that although each notification fails for the same reason (timeout) you will see only one WARN message in the
logs. This is due to this WARN messge raises a notification alarm for `localhost:1028/givemeDelay` and Orion by default doesn't
re-log alarm conditions for alarms already risen (except if `-relogAlarms` is used).
