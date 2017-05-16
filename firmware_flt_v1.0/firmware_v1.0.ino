#include <ESP8266WiFi.h>
#include <Wire.h>
#include <FirebaseArduino.h>
#include "FS.h"


byte bttest=0;
byte led=2;
const byte btliga = D7;
unsigned int x;
static unsigned long ultint = 0;
static unsigned long datas = 0;
byte st_bt = 0;

String hard;
WiFiServer server(80);
WiFiClient client;
String hostn;
bool ap=0;
char d_type[9]="HMNFLT";
byte mac[6];
String smac = "";
String js;


void setup() { 
  monta_mac();  
  hostn= "HMN-FLT-"+smac.substring(7); 
  
  pinMode(bttest,INPUT);   
  pinMode(btliga, INPUT);
  attachInterrupt(digitalPinToInterrupt(btliga), btok, FALLING);//_PULLUP
  Serial.begin(115200);
  Serial.println(smac);
  
  Wire.begin();
  SPIFFS.begin();    
  String ssid=confs(0,0,1,"/confs.txt","ssid=",9);
  String senha=confs(0,0,1,"/confs.txt","senha=",9);

  if(ssid.length() > 2){
    ap=0;     
    WiFi.mode(WIFI_STA);    
    WiFi.hostname(hostn);
    
    if(senha != "---"){ WiFi.begin(ssid.c_str(), senha.c_str()); }else{ WiFi.begin(ssid.c_str());}
    
    Serial.println("Conectando na rede");
    if(confs(0,0,1,"/tst.txt","tst=",9) == "novo"){
      x=0;
      while (WiFi.status() != WL_CONNECTED)
      { x++;     
        delay(500);      
        Serial.print(".");          
          if(!digitalRead(bttest)) reset();
          if ((millis() - ultint) > 700)ultint=0;
          if (x==180) {
            SPIFFS.format();
            ESP.restart();
          }
      }
      confs(0,1,0,"/tst.txt","",9);
      
    }
    
    hard=confs(0,0,1,"/confs.txt","uid=",9) + "/" + d_type + smac.substring(7) + "/";
    Firebase.begin(confs(0,0,1,"/confs.txt","firebaseHost=",9), confs(0,0,1,"/confs.txt","firebaseAuth=",9));
    
    //Firebase.get(hard);
    //Serial.print(Firebase.success());
    
        Firebase.setInt(hard + "t1", 1);
        Firebase.setInt(hard + "t2", 1);
        Firebase.setInt(hard + "t3", 1);
        Firebase.setInt(hard + "st", 0);     
    
   Serial.print(Firebase.getInt(hard+"t1"));
   Serial.print(Firebase.getInt(hard+"t2"));
   Serial.print(Firebase.getInt(hard+"t3"));
   Serial.print(Firebase.getInt(hard+"st"));
   
   /*SFirebase.begin(confs(0,0,1,"/confs.txt","firebaseHost=",9), confs(0,0,1,"/confs.txt","firebaseAuth=",9));
  
   SFirebase.PUT("/"+hard + "t1","1");
   SFirebase.PUT("/"+hard + "t2","1");
   SFirebase.PUT("/"+hard + "t3","1");
   Serial.println(hard);
   Serial.println(SFirebase.GET("/"+hard+ "t1"));
   Serial.println(SFirebase.GET("/"+hard+ "t2"));
   Serial.println(SFirebase.GET("/"+hard+ "t2"));*/
   
  }else{
    Serial.println("Conectando como ap");
    WiFi.mode(WIFI_AP); 
    ap=1;   
    byte msk[] = {255,255,255,0};    
    byte ip[] = {10,10,10,1};
    WiFi.softAPConfig(ip,ip,msk);
    WiFi.softAP(hostn.c_str(),NULL); 
    server.begin();   
  }  
  Serial.println(hostn);
  datas=millis();
}

void loop() {
  if(!ap){
    
    if(st_bt) firecode(0);
    //firecode(1);
    
    if ((millis() - datas ) >= 1000 && !st_bt){
    
    //if ((millis() - datas ) >= 1000){    
      firecode(1);
      Serial.println(datas);
      datas=millis();
    }
  }  
  
  if(!digitalRead(bttest))reset(); 
     
  //Serial.println(SFirebase.GET("/"+hard + "t1"));
  //Serial.print(Firebase.getInt(endpub + "t1")) ;
  
  if ((millis() - ultint) > 600)ultint=0;
  
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
          confs(0,1,0,"/confs.txt","",9); //apaga arquivo
          quebra(data);
          confs(1,0,0,"/tst.txt","tst=novo",9);
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
