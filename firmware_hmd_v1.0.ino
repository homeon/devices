#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include "FS.h"
#include <Ticker.h>
#include "DHT.h"

#define DHT_PIN D4
#define DHTTYPE DHT11       // Tipo de sensor DHT

/* Dados para o sensor de temperatura DHT - Será excluido */
float temperatura_aux = 0.0;
float humidade_aux = 0.0;

byte bttest = 0;
byte led = 2;
const byte btliga = D7;
unsigned int x;
static unsigned long ultint = 0;
static unsigned long datas = 0;
byte st_bt = 0;

WiFiServer server(80);
WiFiClient client;

String hard;
String hostn;
bool ap = 0;
char d_type[9] = "HMNHMD";
byte mac[6];
String smac = "";
String js;

/* Define periodicidade do intervalo de publicacao para o Sensor de temperatura e umidade*/
#define PUBLISH_INTERVAL 1000*60*5

DHT dht(DHT_PIN, DHTTYPE);
Ticker ticker;
bool publishNewState = true;


void publish() {
  publishNewState = true;
}


void setup() {
  monta_mac();
  hostn = "HMN-HMD-" + smac.substring(7);

  dht.begin();

  // Registra o ticker para publicar de tempos em tempos
  ticker.attach_ms(PUBLISH_INTERVAL, publish);

  pinMode(bttest, INPUT);
  pinMode(btliga, INPUT);

  Serial.begin(115200);

  SPIFFS.begin();
  
  String ssid = confs(0, 0, 1, "/confs.txt", "ssid=", 9);
  String senha = confs(0, 0, 1, "/confs.txt", "senha=", 9);

  if (ssid.length() > 2) {
    ap = 0;
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostn);

    if (senha != "---") {
      WiFi.begin(ssid.c_str(), senha.c_str());
    } else {
      WiFi.begin(ssid.c_str());
    }

    Serial.println("Conectando na rede");
    if (confs(0, 0, 1, "/tst.txt", "tst=", 9) == "novo") {
      x = 0;
      while (WiFi.status() != WL_CONNECTED)
      { x++;
        delay(500);
        Serial.print(".");
        if (!digitalRead(bttest)) reset();
        if ((millis() - ultint) > 700)ultint = 0;
        if (x == 180) {
          SPIFFS.format();
          ESP.restart();
        }
      }
      confs(0, 1, 0, "/tst.txt", "", 9);

    }

    hard = confs(0, 0, 1, "/confs.txt", "uid=", 9) + "/" + d_type + smac.substring(7) + "/";
    Firebase.begin(confs(0, 0, 1, "/confs.txt", "firebaseHost=", 9), confs(0, 0, 1, "/confs.txt", "firebaseAuth=", 9));

  } else {

    Serial.println("Conectando como ap");
    WiFi.mode(WIFI_AP);
    ap = 1;
    byte msk[] = {255, 255, 255, 0};
    byte ip[] = {10, 10, 10, 1};
    WiFi.softAPConfig(ip, ip, msk);
    WiFi.softAP(hostn.c_str(), NULL);
    server.begin();
  }
  Serial.println(hostn);
  datas = millis();
}

void loop() {

  if (!ap) {

  // Apenas publique quando passar o tempo determinado
  if(publishNewState){
    Serial.println("Publicando novo estado");
    
    // Obtem os dados do sensor DHT 
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

      Serial.println(humidity);
      Serial.println(temperature);

      if(!isnan(humidity) && !isnan(temperature)){
        
        // Manda para o firebase
           if (temperatura_aux != temperature){
                Firebase.pushFloat(hard + "/tmp", temperature);
                temperatura_aux = temperature;
           }

            if (humidade_aux != humidity){
                Firebase.pushFloat(hard + "/hmd", humidity);    
                humidade_aux = humidity;
           }   

        publishNewState = false;
        
     }else{
       Serial.println("Erro Publicando");
     }

  }
  }

  if (!digitalRead(bttest))reset();

  if ((millis() - ultint) > 600)ultint = 0;

  if (ap) client = server.available();

  if (client && ap)
  {
    int tim = 0;
    while (client.connected())
    {
      if (client.available() > 0)
      {
        String  data = client.readStringUntil('\n');

        if (data.indexOf("Auth=") > 5) {
          client.flush();
          Serial.println(data);
          client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin:* \r\nConnection: close\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n");
          confs(0, 1, 0, "/confs.txt", "", 9); //apaga arquivo
          quebra(data);
          confs(1, 0, 0, "/tst.txt", "tst=novo", 9);
          client.stop();
          ESP.restart();

        } else {
          Serial.println("nao tem");

        }
        
      } else {
        delay(100);
        tim++;

        if (tim == 50) {
          client.print("HTTP/1.1 503 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h2>Site nao existe</h2></body></html>");
          return;
        }
      }
    }
    
    delay(1);
    client.stop();
    Serial.println("[Site saiu]");
  }
  yield();
}
