## Mocks for Orion

Mocks are simulated requests that mimic the behavior of real requests in controlled ways.
This mock is used to simulate a behaviour of any kind of http server.

#### Usage:

```
    *****************************************************************************************
    *                                                                                       *
    *  usage: python mock.py <bind_ip> <port>                                               *
    *           ex: python mock.py 0.0.0.0 5566                                             *
    *  parameters:                                                                          *
    *         bind_ip: ip from where a http request can coming from. 0.0.0.0 to everywhere  *
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

###### Default

- http://ip:port/*
    - The mock store the information received, and return "Saved" and a 200 status code. The way to store is in a dict, an element for each request with all information (headers, parms, payload and path)
    
###### Retrieve de requests information

- http://ip:port/get_data
    - The mock return all requests done to it since it was started, or since the last reset, and reset the responses information
    
###### Set a personalized response

- http://ip:port/set_response
    - A personalized response is set in the mock, this response will be response to all petitions, instead the "Saved" and 200. The response has to go in a json in the following format:
    {
        'headers': {dict/json with the headers},
        'payload': string with the payload,
        'status_code': an int with the status code to return
    }

###### Reset the response to the default

- http://ip:port/reset_response
    - Reset the response to the default "Saved" and a 200 status code.
