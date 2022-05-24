# サンプル・コード

このセクションには、Orion Context Broker をさまざまなテクノロジで
プログラミングするための例として使用できるコード・スニペットが
含まれています。そのほとんどは外部開発者が提供しています。
Orion Context Broker は時間の経過とともに進化しているので、
コード例がある時点で廃止される可能性があることに注意してください。
公開日を参考として提供しています。

## Java

Orion Context Broker にアクセスする方法を示す Java ライブラリの Kurento (http://www.kurento.org) に感謝します : <https://github.com/KurentoLegacy/kmf-orion-connector/tree/develop> (2014年6月公開)

Alejandro Villamarin に感謝します (2013年10月頃) :

       import com.sun.jersey.api.client.Client;
       import com.sun.jersey.api.client.ClientResponse;
       import com.sun.jersey.api.client.WebResource;

       //Test the Orion Broker, query for Room
       String urlString = "http://orion.lab.fiware.org:1026/v1/queryContext";
       String XML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + 
            "<queryContextRequest>" + 
            "<entityIdList>" + 
            "<entityId type=\"Room\" isPattern=\"true\">" + 
            "<id>Room.*</id>" + 
            "</entityId>" +
            "</entityIdList>" + 
            "<attributeList>" + 
            "<attribute>temperature</attribute>" + 
            "</attributeList>" + 
            "</queryContextRequest>";      
       try {     
          Client client = Client.create();
          WebResource webResource = client.resource(urlString);
          ClientResponse response = webResource.type("application/xml").header("X-Auth-Token", "your_auth_token").post(ClientResponse.class, XML);
       
          if (response.getStatus() != 200) {
             System.err.println(response.getEntity(String.class));
        throw new RuntimeException("Failed : HTTP error code : " + response.getStatus());
          }
       
          System.out.println("Output from Server .... \n");
          String output = response.getEntity(String.class);
          System.out.println(output);
       
       } 
       catch (Exception e) {
          System.err.println("Failed. Reason is " + e.getMessage());
       }

## JavaScript と NodeJS
SmartSDK チームが開発し、[SDK Client](https://github.com/smartsdk/ngsi-sdk-javascript) は NSGIv2 と互換性があります。NodeJS アプリケーションと JavaScript アプリケーションの両方を開発することができます。(2017年3月公開)

NodeJS:
```javascript
var NgsiV2 = require('ngsi_v2');

var defaultClient = NgsiV2.ApiClient.instance;

// Configure API key authorization: fiware_token
var fiware_token = defaultClient.authentications['fiware_token'];
fiware_token.apiKey = "YOUR API KEY";
// Uncomment the following line to set a prefix for the API key, e.g. "Token" (defaults to null)
//fiware_token.apiKeyPrefix['X-Auth-Token'] = "Token";
var api = new NgsiV2.APIEntryPointApi();

var callback = function(error, data, response) {
  if (error) {
    console.error(error);
  } else {
    console.log('API called successfully. Returned data: ' + data);
  }
};
api.retrieveAPIResources(callback);

```

JavaScript:
```html
<script src="https://smartsdk.github.io/ngsi-sdk-javascript/js/ngsi.js"></script>
<script>
  var NgsiV2 = FIWARE.NgsiV2;
  var defaultClient = NgsiV2.ApiClient.instance;
  // Configure API key authorization: fiware_token
  var fiware_token = defaultClient.authentications['fiware_token'];
  fiware_token.apiKey = "My Token";
  // Uncomment the following line to set a prefix for the API key, e.g. "Token" (defaults to null)
  //fiware_token.apiKeyPrefix['X-Auth-Token'] = "Token";
  var api = new NgsiV2.APIEntryPointApi();
  var callback = function(error, data, response) {
    if (error) {
      console.error(error);
      $("#results").html(JSON.stringify(error, null, '\t'));
    } else {
      console.log('API called successfully. Returned data: ' + JSON.stringify(data, null, '\t'));
      $("#results").html(JSON.stringify(data, null, '\t'));
    }
  };
  api.retrieveAPIResources(callback);
</script>
```

## JavaScript

JQuery AJAX を使用。Marco Vereda Manchego に感謝します。([original
107 post](http://marcovereda.blogspot.com.es/2013/10/code-snippet-for-custom-http-post.html)) (2013年10月頃公開) :

     function capture_sensor_data(){  
     var contentTypeRequest = $.ajax({  
          url: 'http://orion.lab.fiware.org:1026/v1/queryContext',  
          data: '<?xml version="1.0" encoding="UTF-8"?>  
                    <queryContextRequest>  
                         <entityIdList>  
                              <entityId type = "Sensor" isPattern="true">  
                                   <id>urn:smartsantander:testbed:.*</id>   
                              </entityId>   
                         </entityIdList>   
                         <attributeList>   
                              <attribute>Latitud</attribute>  
                              <attribute>Longitud</attribute>  
                         </attributeList>       
                    </queryContextRequest>  ',  
          type: 'POST',  
          dataType: 'xml',  
          contentType: 'application/xml',  
          headers: { 'X-Auth-Token' :   
               'you_auth_token'}  
          });  
          contentTypeRequest.done(function(xml){   
               var latitud = new Array();  
               var longitud = new Array();  
               var i = 0;       
               var len = $(xml).find("contextAttribute").children().size();  
               $(xml).find('contextAttribute').each(function(){  
                         if (  $(this).find('type').text() == "urn:x-ogc:def:phenomenon:IDAS:1.0:latitude"  
                               && $(this).find('contextValue').text() != "" )  
                         {   
                              latitud[i] = $(this).find('contextValue').text(); i=i+1;  
                         }  
                         if ($(this).find('type').text() == "urn:x-ogc:def:phenomenon:IDAS:1.0:longitude"  
                                && $(this).find('contextValue').text() != ""       )  
                         {   
                              longitud[i] = $(this).find('contextValue').text();   
                         }  
                                                                      });  
               for (var j=0; j<i; j++){                 
                    console.log("DEBUG :" + latitud[j] + "," + longitud[j]);                 
               }                      
          });                 
          contentTypeRequest.fail(function(jqXHR, textStatus){     
               console.log( "DEBUG :  Ajax request failed... (" + textStatus + ' - ' + jqXHR.responseText + ")." );  
          });  
          contentTypeRequest.always(function(jqXHR, textStatus){       });  
     }

## Arduino

Enrique Hernandez Zurita に感謝します。Orion 0.11.0 と 0.12.0 を使用しています。 (2014年6月頃公開) :

    #include <SPI.h>
    #include <WiFi.h>
    #include <WString.h>
    char ssid[] = "SSID"; 
    char pass[] = "CONTRASE Ñ A";    
              
    int status = WL_IDLE_STATUS;
    IPAddress server(130,206,82,115);
    WiFiClient client;
    String line="";
    int value=0;
    int led=9;

    void setup() {
      
        pinMode(led,OUTPUT);
        Serial.begin(9600); 
        while (!Serial) {  ;
        }
        if (WiFi.status() == WL_NO_SHIELD) {

        Serial.println("WiFi shield not present"); 
        while(true);
        } 
        while (status != WL_CONNECTED) { 
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid); 
        status = WiFi.begin(ssid, pass);
        delay(5000); 
        } 
        Serial.println("Connected to wifi");
        Serial.println("\nStarting connection to server..."); 
        
        if (client.connect(server, 1026)) {
        Serial.println("connected to server");
        client.println("POST /NGSI10/queryContext HTTP/1.1");
        client.println("Host: 130.206.82.115:1026");
        client.println("User-Agent: Arduino/1.1");
        client.println("Connection: close");
        client.println("Content-Type: application/xml");
        client.print("Content-Length: ");
        client.println("227");
        client.println();
        client.println("<?xml version=\"1.0\" encoding=\"UFT-8\"?>");
        client.println("<queryContextRequest>");
        client.println("<entityIdList>");
        client.println("<entityId type=\"UPCT:ACTUATOR\" isPattern=\"false\">");
        client.println("<id>UPCT:ACTUATOR:1</id>");
        client.println("</entityId>");
        client.println("</entityIdList>");
        client.println("<attributeList/>");
        client.println("</queryContextRequest>");
        client.println();
        
        Serial.println("XML Sent");
        
        }else{Serial.println("Impossible to connect");}
    }

    void loop() {
        if(value=1){digitalWrite(led, HIGH);}
      
        while (client.available()) {
        char c = client.read();
        Serial.write(c);
        line=line + c;
            if(c==10){
            value=searchValue(line,value);
            line="";
            }
         }
      
         if (!client.connected()) {
         Serial.println();
         Serial.println("disconnecting from server.");
         Serial.println(value);
         client.stop();
         while(true);
         }
    }


    int searchValue(String s,int i) {
      int beginning,ending;
      String val;
      beginning=s.indexOf('>');
      ending=s.indexOf('<', beginning+1);
        if(s.startsWith("contextValue",s.indexOf('<')+1)){
        val =s.substring(beginning+1,ending);
        return val.toInt();
        }else{return i;}
    }
