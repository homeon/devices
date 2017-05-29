/*
   Sketch Filtro de Linha Homeon v 1.0a
   Maio/2017
   Desenvolvido:  Gênio Mauro
   Revisado:  Gustavo D. Cardoso / Gênio Mauro
*/

/* Inclusao de de Bibliotecas */
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <FirebaseArduino.h>
#include "FS.h"

/* Seta os pinos */
volatile byte bttest = 0;
const byte led = 2;
const byte btliga = D7;
volatile unsigned int x;
volatile unsigned long ultint = 0;
volatile unsigned long ciclo;
volatile unsigned long datas = 0;
volatile byte st_bt = 0;

/* Ativa rede Wi-Fi */
WiFiServer server(80);
WiFiClient client;

/* Criacao de variaveis */
String hard;
String hostn;
volatile bool ap = 0;
//char d_type[9] = "HMNFLT";
byte mac[6];
String smac = "";
String js;


void setup() {
  monta_mac();
  hostn = "HMN-FLT-" + smac.substring(7);

  /* Ativa os botões */
  pinMode(bttest, INPUT);
  pinMode(btliga, INPUT); noInterrupts();

  timer0_isr_init();
  timer0_attachInterrupt(dt_agora);

  //ciclo=ESP.getCycleCount() + 80000000;//ESP.getCycleCount()+1000;

  timer0_write(ESP.getCycleCount() + (80000000 * 10));
  interrupts();

  attachInterrupt(digitalPinToInterrupt(btliga), btok, FALLING);//_PULLUP

  Serial.begin(115200);
  Serial.println(smac);

  /* Ativa I2C BUS */
  Wire.begin();

  /* Ativa File System */
  SPIFFS.begin();

  /* Cria as variaves para receber os dados da rede wifi */
  String ssid = confs(0, 0, 1, "/confs.txt", "ssid=", 9);
  String senha = confs(0, 0, 1, "/confs.txt", "senha=", 9);

  /* Checa modo de WiFi (AP ou Cliente) */
  if (!digitalRead(bttest)) reset();
  Serial.print(" SSid: ");
  Serial.println(ssid);


  if (ssid.length() > 2) {
    ap = 0;
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostn);

    /* Conecta em Wifi com senha em branco */
    if (senha != "---") {
      WiFi.begin(ssid.c_str(), senha.c_str());

    } else {
      WiFi.begin(ssid.c_str());
    }
    Serial.println("Conectando na rede");

    if (confs(0, 0, 1, "/tst.txt", "tst=", 9) == "novo") {
      x = 0;

      /* Tempo para conectar na rede e receber IP */
      while (WiFi.status() != WL_CONNECTED)
      { x++;
        delay(500);
        Serial.print(".");

        /* Checa se o Botao RESET foi presionado */
        if (!digitalRead(bttest)) reset();
        if ((millis() - ultint) > 700)ultint = 0;
        if (x == 180) {
          SPIFFS.format();
          ESP.restart();
        }
      }
      confs(0, 1, 0, "/tst.txt", "", 9);

    }

    /* Cria endereço de publicação */
    hard = confs(0, 0, 1, "/confs.txt", "uid=", 9) + "/HMNFLT" + smac.substring(7) + "/";

    /* Inicia o Firebase */
    Firebase.begin(confs(0, 0, 1, "/confs.txt", "firebaseHost=", 9), confs(0, 0, 1, "/confs.txt", "firebaseAuth=", 9));

    /* Checa se existe o endereco ativo no Firebase, caso contrario, cria */
    int ativo = Firebase.getBool(hard + "/ativo");
    Serial.println(ativo);

    if (!ativo) {
      setaFirebase();
    }

  } else {

    /* Cria modo Access Point */
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
  x = 1;
}

void loop() {

  /* Checa s eesta em modo AP e se o botao de LIGA/DESLIGA foi pressionado para publicar no Firebase - Se o STATUS for > 8 ele RESETA o Hardware */
  if (!ap) {
    if (st_bt) {
      firecode(0);
    } else {
      firecode(1);
    }
  }

  if ((millis() - datas ) >= 1000) {

    // Serial.print(x++);
    // Serial.print(" - ");
    // Serial.println(datas);

    datas = millis();
  }

  /* Funcao RESET caso o botao seja pressionao */
  if (!digitalRead(bttest))reset();

  if ((millis() - ultint) > 600)ultint = 0;

  /* Ativa o Servidor WEB caso esteja em modo AP */
  if (ap) client = server.available();

  /* Recebendo os dados via POST se estiver em modo AP (vai mudar a logica para fazermos modo Offline */
  if (client && ap)
  {
    int tim = 0;
    while (client.connected())
    {
      if (client.available() > 0)
      {
        String  data = client.readStringUntil('\n');
        if (data.indexOf("Auth=") > 5) {
          client.flush();  //Serial.println(data);
          client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin:* \r\nConnection: close\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n");
          confs(0, 1, 0, "/confs.txt", "", 9); //apaga arquivo
          quebra(data);
          confs(1, 0, 0, "/tst.txt", "tst=novo", 9);
          client.stop();
          ESP.restart();
        } else {
          Serial.println("Nao encontrado!");
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
  }
  yield();
}
