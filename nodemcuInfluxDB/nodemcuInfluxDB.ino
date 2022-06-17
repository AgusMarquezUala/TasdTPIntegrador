#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <DHT.h> // libreria para el sensor de humedad y temperatura

#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DHTTYPE DHT22 // DHT 22 (tambien se utilizo el 11)
#define DHTPIN 5


// InfluxDB v2 Url del server, e.g. https://eu-central-1-1.aws.cloud2.influxdata.com (Use: InfluxDB UI -> Load Data -> Client Libraries)
#define INFLUXDB_URL "http://192.168.1.8:8086"
// InfluxDB v2 API token (Use: InfluxDB UI -> Data -> API Tokens -> <select token>)
#define INFLUXDB_TOKEN "wFy5MpbVcMfVK0cxlXZ42ri_7EaiwzAmXU0P4X9JyUXQECz-FaHCjM3xPrj9pgSKPMiPshJh5xd8miFhI5Mc-Q=="
// InfluxDB v2 Id del organization (Use: InfluxDB UI -> User -> About -> Common Ids )
#define INFLUXDB_ORG "utn"
// InfluxDB v2 Nombre del bucket (Use: InfluxDB UI ->  Data -> Buckets)
#define INFLUXDB_BUCKET "sensores"

#define TZ_INFO "WART4WARST,J1/0,J365/25"


const char* ssid = "AGUSCASA";
const char* password = "";
const char* serverName = "http://192.168.1.10/api/v1/metrics";

DHT dht(DHTPIN, DHTTYPE); 

//configTzTime(TZ_INFO "pool.ntp.org", "time.nis.gov");


// Cliente de influx
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
Point sensor("sensores_dht");

long contador = 0;

String json;
DeserializationError jsonError;
char * macAddress;

DynamicJsonDocument doc(1024);
DynamicJsonDocument docSendArray(1024);

unsigned long lastTime = 0;

void setup()
{
  dht.begin();
  Serial.begin(115200);
          while (!Serial) {
  ; // espera a que levante el puerto serial del node
  }
  setUpWifi();
  delay(5000);

  sensor.addTag("ubicacion", "CABA");
}

void loop()
{

 
      sensor.clearFields();
      Serial.print("Writing: ");
      Serial.println(sensor.toLineProtocol());
  
     // Se leen los valores de humedad y temperatura y se construye la Query

     float h = dht.readHumidity();
     float t = dht.readTemperature();
     Serial.print("Current humidity = ");
     Serial.print(h);
     Serial.print("% ");
     Serial.print("temperature = ");
     Serial.print(t);
     Serial.println("C "); 

    Serial.println(client.getServerUrl());

    sensor.addField("temperature",t);
    sensor.addField("humidity",h);
    
    //Envia la informacion a InfluxDB
    if(!client.writePoint(sensor))
    {
      Serial.print("Error al escribir en influxDB: ");
      Serial.println(client.getLastErrorMessage());
    }else{
            Serial.println("Envio exitoso");
    }

    
    delay(1500);

    /* Pendiente envio por post custom
    while(Serial.available()) {
  
      String a = Serial.readString();
      
      if(!(a.isEmpty()))
        sendPostRequests(a);
      else
        Serial.println("No se recibe informacion");
  
        a = "";
      }
  
      delay(700);
      */

     
}

void setUpWifi() 
{
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void sendPostRequests(String _msg) 
{
      String msg = _msg;
      if(msg[0] != '[')
        msg = "[" + _msg;
      
      Serial.print("Se recibe: ");
      Serial.println(msg);
    //Wifi Status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
      
      jsonError = deserializeJson(doc, msg);

      if(!jsonError)
        {
              Serial.print("----------------------------------------------------------------------------------------------------------- CYCLE: ");
              Serial.println("-------------------------------------------------------------------------------------------------------------------------------------------");

              String aux = doc["mac"];
              
              docSendArray["mac"] = WiFi.macAddress() + aux;
              docSendArray["sensor_type"] = "Temperatura";
              docSendArray["value"] = "30";
              docSendArray["location"] = "CABA";
              docSendArray["measure_type"] = "ºC";
              
              serializeJson(docSendArray, json);
  
              Serial.println("--------------------------------------------------------- REQUEST STARTS ----------------------------------------------------------------------");
              Serial.print("JSON: ");
              Serial.println(json);
              
              contador++;
              
              Serial.print("CONTADOR DE REQUESTS REALIZADAS: ");
              Serial.println(contador);
                      
              http.begin(client, serverName);
        
              http.addHeader("Content-Type", "application/json");
        
              int httpResponseCode = http.POST(json);
              
              Serial.print("HTTP Response code: ");
              Serial.println(httpResponseCode);
                
              http.end();
  
              json = "";
              Serial.println("--------------------------------------------------------- END OF REQUEST ----------------------------------------------------------------------");
        }
        else
        {
           Serial.print("Error al deserealizar: ");
           Serial.print(msg);
           Serial.print(" error de tipo: ");
           Serial.println(jsonError.c_str());
        }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}
