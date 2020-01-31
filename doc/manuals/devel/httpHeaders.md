# <a name="top"></a>Orion HTTP headers
This document describes all the headers used by Orion, as defined in **src/lib/rest/HttpHeaders.h**.

1)  [Accept](#1-accept)        
2)  [Content-Length](#2-content-length)         
3)  [Content-Type](#3-content-type)                
4)  [Expect](#4-expect)                                                                                
5)  [Host](#5-host)                        	                                                      
6)  [Origin](#6-origin)                                                       
7)  [User-Agent](#7-user-agent)    
8)  [X-Forwarded-For and X-Real-IP](#8-x-forwarded-for-and-x-real-ip)

9) [Access-Control-Allow-Origin](#9-access-control-allow-origin)
10) [Access-Control-Allow-Headers](#10-access-control-allow-headers)                                                                   
11) [Access-Control-Allow-Methods](#11-access-control-allow-methods)
12) [Access-Control-Max-Age](#12-access-control-max-age)                                 
13) [Access-Control-Expose-Headers](#13-access-control-expose-headers)
14) [Allow](#14-allow)                                                                                    
15) [Location](#15-location)                                                 
16) [Ngsiv2-AttrsFormat](#16-ngsiv2-attrsformat)                                    
17) [Fiware-Service](#17-fiware-service)
18) [Fiware-Servicepath](#18-fiware-servicepath)
19) [Fiware-Total-Count](#19-fiware-total-count)
20) [Fiware-Correlator](#20-fiware-correlator)                                     

21) [X-Auth-Token](#21-x-auth-token)



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
  
## 4. Expect

Orion sends a blank `Expect:` header in outgoing requests (notifications and forwarded queries/updates). For more details, follow general documentation of Expect HTTP header at [flaviocopes.com](https://flaviocopes.com/http-request-headers/), [ietf.org](https://tools.ietf.org/html/rfc7231#section-5.1.1) and [wikipedia.org](https://en.wikipedia.org/wiki/List_of_HTTP_header_fields).

    Expect:
	
[Top](#top)	
	
## 5. Host

Orion includes this header in outgoing request (notifications and forwarded updates/queries).This header provides the port number of notification receiver. 	
	
    Host: localhost:1028
		
[Top](#top)

## 6. Origin

Origin is used in incoming HTTP requests received by Orion.
Its operation is related with CORS, see specific documentation at [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md).
	
    Origin: http://www.example-social-network.com
		
[Top](#top)	
  
## 7. User-Agent

User-Agent HTTP header is used in notifications and forwarded requests.
It describes which version of Orion we are using along with its transfer library.
It gives the string that identifies the user agent.

    User-Agent: orion/2.2.0-next libcurl/7.29.0     (in notifications and forwarded requests)

[Top](#top)		
		
## 8. X-Forwarded-For and X-Real-IP

Both headers are used in incoming HTTP requests received by Orion.
X-Forwarded-For header overrides the original IP of the HTTP request as source of the transaction.
X-Real-IP and X-Forwarded-For (used by a potential proxy on top of Orion) overrides IP.
X-Real-IP takes preference over X-Forwarded-For, if both appear.
	
    X-Forwarded-For: 129.78.138.66
	
[Top](#top)

## 9. Access-Control-Allow-Origin

It is an optional header used in outgoing HTTP responses sent by Orion. Its operation is related with CORS, see specific documentation [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-origin).
    
    Access-Control-Allow-Origin: *
		
[Top](#top)


## 10. Access-Control-Allow-Headers

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-headers).

    Access-Control-Allow-Headers: Content-Type, Fiware-Service, Fiware-Servicepath, Ngsiv2-AttrsFormat, Fiware-Correlator, X-Forwarded-For, X-Real-IP, X-Auth-Token	
									               								   
[Top](#top)		
		
		
## 11. Access-Control-Allow-Methods

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-allow-methods).

[Top](#top)

									  
## 12. Access-Control-Max-Age

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-max-age).
		
    Access-Control-Max-Age: 86400

[Top](#top)


## 13. Access-Control-Expose-Headers

It is used in outgoing HTTP responses sent by Orion.
Its operation is related with CORS, see specific documentation [about CORS in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/cors.md#access-control-expose-headers).
	
    Access-Control-Expose-Headers: Fiware-Correlator, Fiware-Total-Count, Location.

[Top](#top)


## 14. Allow

It is used in outgoing HTTP responses sent by Orion.
When the client uses a wrong HTTP method on a given URL resources, this header is used to inform the client which methods are allowed.

    Allow: GET

[Top](#top)
		
    
## 15. Location

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


## 16. Ngsiv2-AttrsFormat

It is used in outgoing HTTP notifications sent by Orion.
Notifications must include the `Ngsiv2-AttrsFormat` HTTP header with the value of the format of the associated subscription, 
so that notification receivers are aware of the format without needing to infer it from the notification payload.
Note that if a custom payload is used for the notification then a value of `custom` is used for the `Ngsiv2-AttrsFormat` header
in the notification.	
Ngsiv2-AttrsFormat is a non-modifiable header in custom notifications.
Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Ngsiv2-Attrsformat": "something"} ...}` will be ignored.
	
    Ngsiv2-Attrsformat: normalized

[Top](#top)	


## 17. Fiware-Service

Fiware-Service is used in any kind of HTTP transaction (incoming/outcoming requests and outcoming responses) managed in Orion.
When `-multiservice` is used Orion includes the `Fiware-Service` header in the notification requests associated to subscriptions in the given tenant/service (except for the default service/tenant, in which case the header is not present). See related documentation [about multitenancy in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/multitenancy.md).
		
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


## 18. Fiware-Servicepath

Fiware-ServicePath is an optional header used in any kind of HTTP transaction (incoming/outcoming requests and outcoming responses) managed in Orion. See specific documentation [about service_path in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/service_path.md).

    Fiware-ServicePath: /Madrid/Gardens/ParqueNorte/Parterre1		

[Top](#top)
 
 
## 19. Fiware-Total-Count

It is an optional header used in outgoing HTTP responses sent by Orion.
Its operation is related with pagination, see related documentation [about pagination in the user manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/user/pagination.md#pagination).
	    
[Top](#top)


## 20. Fiware-Correlator

Fiware-Correlator is used in outgoing responses. It is also sent (or propagated) in notifications and forwarded queries/updates.
Fiware-Correlator is a non-modifiable header in custom notifications. Any attempt of doing so (e.g. `"httpCustom": { ... "headers": {"Fiware-Correlator": "foo"} ...}` will be ignored.	See related documentation [about logs in the admin manual](https://github.com/telefonicaid/fiware-orion/blob/master/doc/manuals/admin/logs.md).
    
    Fiware-Correlator: 600119ce-eeaa-11e9-9e0c-080027a71049	

[Top](#top)
	

## 21. X-Auth-Token

X-Auth-Token is an optional HTTP header, which Orion received in requests and propagates transparently to other requests (notifications and forwarded queries/updated) associated with the original one. It is supposed to be used by security enforcement proxies integrated with Orion [such as PEP Steelskin](https://github.com/telefonicaid/fiware-pep-steelskin).
	
    "X-Auth-Token": "fff0f4af447f4b589c835f805fe4be29"

[Top](#top)
