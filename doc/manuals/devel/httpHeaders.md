# <a name="top"></a>This document describes all the headers used by Orion, as defined in src/lib/rest/HttpHeaders.h file.

1)  [Accept](#1-accept)        
2)  [Content-Length](#2-content-length)         
3)  [Content-Type](#3-content-type)
4)  [Connection](#4-connection)                
5)  [Expect](#5-expect)                                                                                
6)  [Host](#6-host)                        	                                                      
7)  [Origin](#7-origin)                                                       
8)  [User-Agent](#8-user-agent)              
9)  [X-Forwarded-For](#9-x-forwarded-for)                                                               
10) [X-Real-IP](#10-x-real-ip)

11) [Access-Control-Allow-Origin](#11--access-control-allow-origin)
12) [Access-Control-Allow-Headers](#12-access-control-allow-headers)                                                                   
13) [Access-Control-Allow-Methods](#13-access-control-allow-methods)
14) [Access-Control-Max-Age](#14-access-control-max-age)                                 
15) [Access-Control-Expose-Headers](#15-access-control-expose-headers)
16) [Allow](#16-allow)                                                                                    
17) [Location](#17-location)                                                 
18) [Ngsiv2-AttrsFormat](#18-ngsiv2-attrsformat)                                    
19) [Fiware-Service](#19-fiware-service)
20) [Fiware-Servicepath](#20-fiware-servicepath)
21) [Fiware-Total-Count](#21-fiware-total-count)
22) [Fiware-Correlator](#22-fiware-correlator)                                     

23) [X-Auth-Token](#23-x-auth-token)



# HTTP headers used by Orion with their description



## 1. Accept

Accept HTTP header is used in incoming HTTP requests received by Orion. 
Accept HTTP header is used in the request to specify which MIME type the client accepts. Typically its value is `application/json` (for JSON MIME type) although some operations in the NGSIv2 API also allow `text/plain`. 
For example in request URL if we use:
	
    curl ... -H 'Accept: application/json'
		 
On this request we'll get the response data in well defined JSON format.

The API response payloads in this specification are based on `application/json` and  (for attribute value 
type operation) `text/plain` MIME types. Clients issuing HTTP requests with accept types different 
than those will get a `406 Not Acceptable` error.

[Top](#top)

## 2. Content-Length

This header is used by both requests and responses.
Content-length HTTP header is a mandatory header in Orion responses.
It defines the length of the request and response body in bytes.
Orion Context Broker expects always a Content-Length header in all client requests, otherwise the client will receive a `411 Length Required` response. 
This is due to the way the underlying HTTP library (libmicrohttpd) works.
	
    Content-Length: 34	

[Top](#top)
 
## 3. Content-Type 

It is used to specify the MIME type of the request or response. Typically its value is `application/json` (for JSON MIME type) although some operations in the NGSIv2 API also allow `text/plain`.

    curl ... -H 'Content-Type: application/json'

This request will send data in well defined JSON format.

[Top](#top)
 
## 4. Connection
 
 This header is used by both requests and responses.
 Orion is not currently doing anything with this header (apart from checking if its value is `close` to print a log trace).
	
    Connection: Keep-Alive
    	
[Top](#top)	
  
## 5. Expect

Expect HTTP header is used in incoming HTTP requests received by Orion.
It Indicates that behaviour of particular server are required by the client.
It is typically used when sending a large request body. We expect the server to return back `100 Continue` HTTP status if it can handle the request, or `417 Expectation Failed` if not.

    Expect: 100-continue
	
[Top](#top)	
	
## 6. Host

This HTTP header is used in Orion responses.
Orion includes this header in outgoing request (notifications and forwarded updates/queries).This header provides the port number of notification receiver. 	
	
    Host: localhost:1028
		
[Top](#top)

## 7. Origin

Origin is used in incoming HTTP requests received by Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md).
	
    Origin: http://www.example-social-network.com
		
[Top](#top)	
  
## 8. User-Agent

User-Agent HTTP header is used in notifications and forwarded requests.
It describes which version of Orion we are using along with its transfer library.
It gives the string that identifies the user agent.

    User-Agent: orion/2.2.0-next libcurl/7.29.0     (in notifications and forwarded requests)

[Top](#top)		
		
## 9. X-Forwarded-For

It is used in incoming HTTP requests received by Orion.
X-Forwarded-For header overrides the original IP of the HTTP request as source of the transaction.
X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides IP.
X-Real-IP takes preference over X-Forwarded-For, if both appear.
	
    X-Forwarded-For: 129.78.138.66
	
[Top](#top) 
	
## 10. X-Real-IP

X-Forwarded-For header overrides the original IP of the HTTP request as source of the transaction.
X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides IP.
X-Real-IP takes preference over X-Forwarded-For, if both appear.
	
[Top](#top)

## 11.  Access-Control-Allow-Origin

It is an optional header used in outgoing HTTP responses sent by Orion.  
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-origin).
    
    Access-Control-Allow-Origin: *
		
[Top](#top)


## 12. Access-Control-Allow-Headers

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-headers).

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token	
									               								   
[Top](#top)		
		
		
## 13. Access-Control-Allow-Methods

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-methods).

[Top](#top)

									  
## 14. Access-Control-Max-Age

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-max-age).
		
    Access-Control-Max-Age: 86400

[Top](#top)


## 15. Access-Control-Expose-Headers

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-expose-headers).
	
    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location.

[Top](#top)


## 16. Allow

It is used in outgoing HTTP responses sent by Orion.
When the client uses a wrong HTTP method on a given URL resources, this header is used to inform the client which methods are allowed.

    Allow: GET

[Top](#top)
		
    
## 17. Location

It is used in outgoing HTTP responses sent by Orion on subscriptions, entity creations and registrations.
Response contains a Location header which holds the subscription ID, entity ID or registration ID: a 24 digit hexadecimal number used for updating and deleting the subscription, entity or registration. We use `-v` in requests to get the Location header in the response.
	
    curl -v localhost:1026/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
    {
     //payload
    }
    EOF
	
And the response that we will get:
	
    Location: /v2/subscriptions/57458eb60962ef754e7c0998
	
[Top](#top)


## 18. Ngsiv2-AttrsFormat

It is used in outgoing HTTP notifications sent by Orion.
Notifications must include the `Ngsiv2-AttrsFormat` HTTP header with the value of the format of the associated subscription, 
so that notification receivers are aware of the format without needing to infer it from the notification payload.
Note that if a custom payload is used for the notification then a value of `custom` is used for the `Ngsiv2-AttrsFormat` header
in the notification.	
Ngsiv2-AttrsFormat is a non-modifiable header in custom notifications.
Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Ngsiv2-Attrsformat": "something"} ...}` will be ignored.
	
    Ngsiv2-Attrsformat: normalized

[Top](#top)	


## 19. Fiware-Service

Fiware-Service is used in any kind of HTTP transaction (incoming/outcoming requests and outcoming responses) managed in Orion.
When `-multiservice` is used Orion includes the `Fiware-Service` header in the notification requests associated to subscriptions in the given tenant/service (except for the default service/tenant, in which case the header is not present). See related documentation at [../user/multitenancy.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/multitenancy.md)
		
    POST http://127.0.0.1:9977/notify    
    Content-Length: 725    
    User-Agent: orion/2.3.0    
    Host: 127.0.0.1:9977      
    Accept: application/json    
    Fiware-Service: t_02    
    Content-Type: application/json    
    {    
	   ...    
    }
	
[Top](#top)


## 20. Fiware-Servicepath

Fiware-ServicePath is an optional header used in any kind of HTTP transaction (incoming/outcoming requests and outcoming responses) managed in Orion. See specific documentation at [../user/service_path.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/service_path.md).

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte/Parterre1		

[Top](#top)
 
 
## 21. Fiware-Total-Count

It is an optional header used in outgoing HTTP responses sent by Orion.
Its operation is related with pagination, see related documentation at [../user/pagination.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/pagination.md#pagination).
	    
[Top](#top)


## 22. Fiware-Correlator

Fiware-Correlator is a response HTTP header which is used in forwarding messages and notifications.
Fiware-Correlator is a non-modifiable header in custom notifications. Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be ignored.	See related documentation at [../admin/logs.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/admin/logs.md)
    
    Fiware-Correlator: 600119ce-eeaa-11e9-9e0c-080027a71049	

[Top](#top)
	

## 23. X-Auth-Token

X-Auth-Token is an optional HTTP header, which Orion received in requests and propagates transparently to other requests (notifications and forwarded queries/updated) associated with the original one. It is supposed to be used by security enforcement proxies integrated with Orion [such as PEP Steelskin](https://github.com/telefonicaid/fiware-pep-steelskin).
	
    "X-Auth-Token": "fff0f4af447f4b589c835f805fe4be29"

[Top](#top)
