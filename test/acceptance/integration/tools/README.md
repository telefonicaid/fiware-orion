## Mocks for Orion

Mocks are simulated requests that mimic the behavior of real requests in controlled ways.
This mock is used to simulate a behaviour of any kind of http server.

#### Usage:

```
    *****************************************************************************************
    *                                                                                       *
    *  usage: python mock.py [--host host] [--port port]                                    *
    *           ex: python mock.py --host 0.0.0.0 --port 5566                               *
    *  parameters:                                                                          *
    *         host_ip: ip from where a http request can coming from. 0.0.0.0 to everywhere  *
    *         port: http port where the mock is listeing                                    *
    *                                                                                       *
    *  Comments:                                                                            *
    *         Mock is developed thinking in automatic start, but it could be used           *
    *         running it manually*                                                          *
    *                                                                                       *
    *                                ( use <Ctrl-C> to stop in case of run it manually      *
    *****************************************************************************************
```

#### API Rest requests:
Mock has to be configured in execution time, sending to the mock which response has to serve in each url. The responses are queues, once is send, is popped too.
The request did to the mock, are stored in a queue too.
###### GET

- http://ip:port/*/mock_configuration
    - Get the last request to the mock in the url asked, deleting '/mock_configuration' part of the url
- http://ip:port/queues
    - Get the queues for responses and requests
- http://ip:port/*
    - Get the last response saved before in the queue
    
###### DELETE

- http://ip:port/queues
    - Empty the queues of requests and responses
- http://ip:port/*
    - Serve the response set before in the queue with the url
    
###### PUT

- http://ip:port/*
    - Serve the response set before in the queue with the url
    
###### POST

- http://ip:port/*/mock_configurations
    - Configure the response and save it in the queue. the payload of the post request has to have the structure:
    {
        'status_code': HTTP status code to be sent in the response.,
        'headers': headers dict to be included in the response,
        'body': Body to be included in the response,
        'delay': Delay in seconds before sending the response
    }
- http://ip:port/*
    - Serve the response set before in the queue with the url
   