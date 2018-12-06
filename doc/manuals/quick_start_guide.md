# Orion Context Broker Quick Start Guide

Welcome to Orion Context Broker! In this brief guide you will find information about some initial steps to work with the
Orion Context Broker global instance in [FIWARE Lab](https://lab.fiware.org) (owned and managed by the FIWARE Foundation) in an easy way.

Orion Context Broker implements the [FIWARE NGSI version 2](http://fiware.github.io/context.Orion/api/v2/stable/) API.
A good learning resource for such API is the
[NGSI version 2 Cookbook](http://fiware.github.io/context.Orion/api/v2/stable/cookbook/).

First of all, you need an account in FIWARE Lab, so register for one in [the following link](https://account.lab.fiware.org/sign_up) if you don't have one (it's free, all you need is a valid email adress :). With that account you can obtain a valid authentication token to use in the REST API calls to Orion. To get that token, get and run the `token_script.sh` script. Introduce your FIWARE Lab user and password when the scripts ask for it (**note you have to use the complete username, including email domain**, e.g. if you email were "foo@gmail.com" you have to use "foo@gmail.com", not just "foo"):

    # wget --no-check-certificate https://raw.githubusercontent.com/FIWARE-Ops/Tools/master/GetToken/get_token.sh
    # bash get_token.sh orion-gi
    Username: your_email@example.com
    Password:
    Token: <this is the token you need>

(Take into account that tokens generated in this way
[will expire after 1 hour](http://stackoverflow.com/questions/39835218/orion-context-broker-global-instance-token),
time enough to complete the rest of the steps in this guide ;)

Let's assume that the authentication token you got is in the AUTH_TOKEN shell variable. Now, let's start querying some real-time information from the city sensors of Santander (in particular, a sound level meter):
``` 
curl orion.lab.fiware.org:1026/v2/entities/urn:smartsantander:testbed:357 \
   -X GET -s -S --header 'Accept: application/json' \
   --header  "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
``` 
You will get a JSON document including the time of the last measure (TimeInstant), sound level (sound), sensor battery charge (batteryCharge) and sensor location (Latitud and Longitud... sorry for the Spanish in these last ones ;) for the sensor identified by "urn:smartsantander:testbed:357".

Let's query another sensor, this time one related to road traffic:
``` 
curl orion.lab.fiware.org:1026/v2/entities/urn:smartsantander:testbed:3332 \
   -X GET -s -S  --header 'Accept: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
``` 
The data you find in the returned JSON about the "urn:smartsantander:testbed:3332" sensor is:

* time of measurement (TimeInstant),
* "traffic intensity" (in vehicles per hour),
* occupancy (percentage), and
* sensor location (Latitud and Longitud).

The Orion Context Broker global instance can also be used to create new entities. First, chose a unique entity ID (uniqueness is important, given that the Orion Context Broker instance is shared and you could be modifying the entity of some other user):

    # RANDOM_NUMBER=$(cat /dev/urandom | tr -dc '0-9' | fold -w 10 | head -n 1)
    # ID=MyEntity-$RANDOM_NUMBER
    # echo $ID

The following command creates an entity with the attributes "city_location" and "temperature" in the Orion Context Broker:

``` 
curl orion.lab.fiware.org:1026/v2/entities -X POST -s -S \
   --header 'Content-Type: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d @- <<EOF
{
  "id": "$ID",
  "type": "User",
  "city_location": {
    "value": "Madrid",
    "type": "City"
  },
  "temperature": {
    "value": 23.8,
    "type": "Number"
  }
}
EOF
``` 

In order to check that the entity is there, you can query it the same way you queried the public sensors:

``` 
curl orion.lab.fiware.org:1026/v2/entities/$ID -X GET -s -S \
    --header 'Accept: application/json'\
    --header "X-Auth-Token: $AUTH_TOKEN" | python -mjson.tool
``` 
And you can, of course, modify the values for the attributes, e.g. to modify the temperature:

```
curl orion.lab.fiware.org:1026/v2/entities/$ID/attrs/temperature \
   -X PUT -s -S --header  'Content-Type: application/json' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d @- <<EOF
{
  "value": 18.4,
  "type": "Number"
}
EOF
```

or (more compact):

```
curl orion.lab.fiware.org:1026/v2/entities/$ID/attrs/temperature/value \
   -X PUT -s -S --header  'Content-Type: text/plain' \
   --header "X-Auth-Token: $AUTH_TOKEN" -d 18.4
```

If you re-run the query command above, you will see that the temperature has changed to 18.4 ºC.

This concludes this small introduction to Orion Context Broker. If you want to know more about this FIWARE enabler (including the API details, how to deploy your own private instances, how to subscribe/receive notifications, geo-localization queries and much more), please go to the [documentation home](http://github.com/telefonicaid/fiware-orion). 
