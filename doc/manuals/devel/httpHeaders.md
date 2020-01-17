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

Accept HTTP header is used in requests. 
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

Expect HTTP header is used in requests.
It Indicates that behaviour of particular server are required by the client.
It is typically used when sending a large request body. We expect the server to return back `100 Continue` HTTP status if it can handle the request, or `417 Expectation Failed` if not.

    Expect: 100-continue
	
[Top](#top)	
	
## 6. Host

This HTTP header is used in Orion responses.
When we make any request then the corresponding Notification will come on accumulator server, this header gives the port number of accumulator server. 	
	
    Host: localhost:1028
		
[Top](#top)

## 7. Origin

Origin is used in requests.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md).
	
    Origin: http://www.example-social-network.com
		
[Top](#top)	
  
## 8. User-Agent

User-Agent HTTP header is used in both requests and responses.
It describes which version of Orion we are using along with its transfer library.
It gives the string that identifies the user agent.

    User-Agent: orion/2.2.0-next libcurl/7.29.0     (in notification upon subscription on Accumulator-server)
    User-Agent: curl/7.29.0                         (Response upon subscription on Orion)

[Top](#top)		
		
## 9. X-Forwarded-For

It is a request HTTP header in Orion.
Source IP of the HTTP request associated to the transaction, except if the request includes X-Forwarded-For header which overrides the former IP.
X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides IP.
X-Real-IP takes preference over X-Forwarded-For, if both appear.
	
    X-Forwarded-For: 129.78.138.66
	
[Top](#top) 
	
## 10. X-Real-IP

Source IP of the HTTP request associated to the transaction, except if the request includes X-Real-IP which overrides X-Forwarded-For and source IP.
X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides IP.
X-Real-IP takes preference over X-Forwarded-For, if both appear.
	
[Top](#top)

## 11.  Access-Control-Allow-Origin

It is an optional header used in Orion responses.  
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-origin).
    
    Access-Control-Allow-Origin: *
		
[Top](#top)


## 12. Access-Control-Allow-Headers

It is a response HTTP header in Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-headers).

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token	
									               								   
[Top](#top)		
		
		
## 13. Access-Control-Allow-Methods

It is a response HTTP header in Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-methods).

[Top](#top)

									  
## 14. Access-Control-Max-Age

It is a response HTTP header in Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-max-age).
		
    Access-Control-Max-Age: 86400

[Top](#top)


## 15. Access-Control-Expose-Headers

It is a response HTTP header in Orion.
Its operation is related with CORS, see specific documentation at [../user/cors.md](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-expose-headers).
	
    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location.

[Top](#top)


## 16. Allow

Allow HTTP header is used in response notification.
It notify that which methods are allowed by the specified host/resource.

    Allow: GET

[Top](#top)
		
    
## 17. Location

Location HTTP header is used in responses.
Response contains a Location header which holds the subscription ID: a 24 digit hexadecimal number used for updating and cancelling the subscription.
we use `-v` in requests to get the Location header in the response.
	
    curl -v localhost:1026/v2/subscriptions -s -S -H 'Content-Type: application/json' -d @- <<EOF
    {
     //payload
    }
    EOF
	
And the response that we will get:
	
    Location: /v2/subscriptions/57458eb60962ef754e7c0998
	
[Top](#top)


## 18. Ngsiv2-AttrsFormat

It is a response HTTP header in Orion.
Notifications must include the `Ngsiv2-AttrsFormat` HTTP header with the value of the format of the associated subscription, 
so that notification receivers are aware of the format without needing to process the notification payload.
Note that if a custom payload is used for the notification then a value of `custom` is used for the `Ngsiv2-AttrsFormat` header
in the notification.	
Ngsiv2-AttrsFormat is a NON-MODIFIABLE header in custom notifications.
Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Ngsiv2-Attrsformat": "something"} ...}` will be ignored.
	
    Ngsiv2-Attrsformat: normalized

[Top](#top)	


## 19. Fiware-Service

Fiware-Service is an optional header used in requests.
When `-multiservice` is used Orion includes the `Fiware-Service` header in the notifyContextRequest and notifyContextAvailability request messages associated to subscriptions in the given tenant/service (except for the default service/tenant, in which case the header is not present).
		
    POST http://127.0.0.1:9977/notify    
    Content-Length: 725    
    User-Agent: orion/0.13.0    
    Host: 127.0.0.1:9977      
    Accept: application/json    
    Fiware-Service: t_02    
    Content-Type: application/json    
    {    
	   ...    
    } 
      
 Regarding service/tenant name syntax, it must be a string of alphanumeric characters (and the `_` symbol). 
 Maximum length is 50 characters, which should be enough for most use cases. 
 Orion Context Broker interprets the tenant name in lowercase, thus, although you can use tenants such as in updateContext `MyService` 
 it is not advisable, as the notifications related with that tenant will be sent with `myservice` and, 	in that sense, 
 it is not coherent the tenant you used in updateContext compared with the one that Orion sends in notifyContextRequest.
	
[Top](#top)


## 20. Fiware-Servicepath

Fiware-ServicePath is an optional header used in requests.
Fiware-ServicePath header is included in notification requests sent by Orion.
When `-multiservice` is used, Orion uses the `Fiware-Service` HTTP header in the request to identify the service/tenant.
If the header is not present in the HTTP request, the default service/tenant is used.	
The scope to use is specified using the Fiware-ServicePath HTTP in update/query request. 

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte/Parterre1
		
Queries without Fiware-ServicePath resolve to `/#`.
	
It is assumed that all the entities created without Fiware-ServicePath (or that don't include service path information in the database) 
belongs to a root scope `/` implicitely. All the queries without using Fiware-ServicePath (including subscriptions) are on `/#` implicitly. 

While entities belong to services and servicepaths, subscriptions and registrations belong only to the service. 
The servicepath in subscriptions and registrations doesn't denote sense of belonging, but is the expression of the 
query associated to the subscription or registration.

Fiware-ServicePath header is ignored in `GET /v2/subscriptions/{id}` and `GET /v2/registrations/{id}` operations, 
as the id fully qualifies the subscription or registration to retrieve.

Fiware-ServicePath header is taken into account in `GET /v2/subscriptions` and `GET /v2/registrations` in order to narrow 
down the results to subscriptions/registrations that use exactly that service path as query.

At the present moment hierarchical service paths (i.e. the ones using ending with #) are not allowed in registrations.
Fiware-Service and Fiware-ServicePath are set at entity creation time using HTTP headers in the entity creation REST request.
		
    "servicePath": "test/example" 		

[Top](#top)
 
 
## 21. Fiware-Total-Count

It is an optional header used in responses.
NGSIv2 implements a pagination mechanism in order to help clients to retrieve large sets of resources. This mechanism works for all listing operations in the API (e.g. `GET /v2/entities`, `GET /v2/subscriptions`, `POST /v2/op/query`, etc.).

The mechanism is based on three URI parameters:

i)   limit, in order to specify the maximum number of elements (default is 20, maximum allowed is 1000).

ii)  offset, in order to skip a given number of elements at the beginning (default is 0)

iii) count (as option), if activated then a Fiware-Total-Count header is added to the response, with a count of total elements.
	    
[Top](#top)


## 22. Fiware-Correlator

Fiware-Correlator is a response HTTP header.
Fiware-Correlator is used as HTTP header in forwarding messages, notifications and responses.
This correlator id is either transferred from an incoming request, or, if the incoming request doesn't carry any HTTP header Fiware-Correlator, the correlator is generated by the Orion context broker and then used in the log file. 
It can be either `N/A` or it is a string in the UUID format.
The correlator id is a common identifier among all applications involved in the `message chain` for one specific request.	
Fiware-Correlator is a NON-MODIFIABLE header in custom notifications.    
Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be ignored.	
    
    Fiware-Correlator: 600119ce-eeaa-11e9-9e0c-080027a71049	

[Top](#top)
	

## 23. X-Auth-Token

X-Auth-Token is an optional HTTP header used in requests.
It is used as a Access-Control HTTP header.	
This header is used to provide security to Orion and it is frequently used in FIWARE-CEPHEUS(i.e. an another component used in fiware).
X-Auth-Token forwarded in ngsi10 notifications.
	
    "authToken": "OAUTH_TOKEN"

[Top](#top)
