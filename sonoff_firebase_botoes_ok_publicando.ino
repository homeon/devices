/* Firmware Tomada - Homeon Tomada v1.0a
    Desenvolvido por Genio Mauro - Revisado por Gustavo D. Cardoso
    06/2017
   1 - vcc 3v3
   2 - rx
   3 - tx
   4 - gnd
   5 - gpio 14
   gpio  0 - button
   gpio 12 - relay
   gpio 13 - green led - active low
   gpio 14 - pin 5 on header
*/

#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include "FS.h"
#include <Ticker.h>


/* Define os pinos do SONOFF */
#define   SONOFF_BUTTON             0
#define   SONOFF_INPUT              14
#define   SONOFF_LED                13
#define   SONOFF_AVAILABLE_CHANNELS 1

//const int SONOFF_RELAY_PINS[4] =    {12, 12, 12, 12};
int gpio12Relay = 12;

//if this is false, led is used to signal startup state, then always on
//if it s true, it is used to signal startup state, then mirrors relay state
//S20 Smart Socket works better with it false
#define SONOFF_LED_RELAY_STATE      false

//for LED status
Ticker ticker;

const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;
int cmd = CMD_WAIT;

int relayState = HIGH;
// inverted button state

int buttonState = HIGH;
static long startPress = 0;

/* Ativa rede Wi-Fi */
WiFiServer server(80);
WiFiClient client;

/* Criacao de variaveis */
String hard;
String hostn;
volatile bool ap = 0;
byte mac[6];
String smac = "";
String js;

// int gpio13Led = 13;
// int gpio12Relay = 12;

volatile unsigned int x;
volatile unsigned long ultint = 0;
volatile unsigned long ciclo;
volatile unsigned long datas = 0;

void setup(void) {

  monta_mac();
  hostn = "HMN-TMD-" + smac.substring(7);

  Serial.begin(115200);
  delay(5000);
  Serial.println(".");

  /* Ativa File System */
  SPIFFS.begin();

  /* Cria as variaves para receber os dados da rede wifi */
  String ssid = confs(0, 0, 1, "/confs.txt", "ssid=", 9);
  String senha = confs(0, 0, 1, "/confs.txt", "senha=", 9);

  /* Prepara os pinos */
  // pinMode(gpio13Led, OUTPUT);
  // digitalWrite(gpio13Led, HIGH);

  pinMode(gpio12Relay, OUTPUT);
  // digitalWrite(gpio12Relay, HIGH);

  pinMode(SONOFF_LED, OUTPUT);

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

    //if you get here you have connected to the WiFi
    ticker.detach();

    if (confs(0, 0, 1, "/tst.txt", "tst=", 9) == "novo") {
      x = 0;

      /* Tempo para conectar na rede e receber IP */
      while (WiFi.status() != WL_CONNECTED)
      { x++;
        delay(500);
        Serial.print(".");
      }
      confs(0, 1, 0, "/tst.txt", "", 9);
    }

    /* Cria endereço de publicação */
    hard = "users/" + confs(0, 0, 1, "/confs.txt", "uid=", 9) + "/dispositivos/TMD/HMNTMD" + smac.substring(7) + "/";

    /* Inicia o Firebase */
    Firebase.begin(confs(0, 0, 1, "/confs.txt", "firebaseHost=", 9), confs(0, 0, 1, "/confs.txt", "firebaseAuth=", 9));

    /* Checa se existe o endereco ativo no Firebase, caso contrario, cria */
    int ativo = Firebase.getBool(hard + "/ativo");
    Serial.println(ativo);

    if (!ativo) {
      setaFirebase();
    }

    /* SONOFF */
    //setup button
    pinMode(SONOFF_BUTTON, INPUT);
    attachInterrupt(SONOFF_BUTTON, toggleState, CHANGE);

    //setup relay
    //TODO multiple relays
    // pinMode(SONOFF_RELAY_PINS[0], OUTPUT);

    //TODO this should move to last state maybe
    //TODO multi channel support
    // if (strcmp(settings.bootState, "on") == 0) {
    //   turnOn();
    //  } else {
    //  turnOff();
    // }

    //setup led
    if (!SONOFF_LED_RELAY_STATE) {
      digitalWrite(SONOFF_LED, LOW);
    }

  } else {

    /* Cria modo Access Point */
    Serial.println("Conectando como ap");

    // start ticker with 0.5 because we start in AP mode and try to connect
    ticker.attach(0.6, tick);

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

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

}

void loop(void) {

  /* Verifica o valor do rele no Firebase
    int f = Firebase.getInt(hard + "topico");
    digitalWrite(gpio12Relay, f ? HIGH : LOW);
  */

  /* Checa o botao SONOFF */
  // Serial.println(digitalRead(SONOFF_BUTTON));

  switch (cmd) {
    case CMD_WAIT:
      break;

    case CMD_BUTTON_CHANGE:
      int currentState = digitalRead(SONOFF_BUTTON);

      if (currentState != buttonState) {
        if (buttonState == LOW && currentState == HIGH) {
          long duration = millis() - startPress;

          if (duration < 1000) {
            Serial.println("short press - trocar status");
            // toggle();
            troca ();
          } else if (duration < 5000) {
            Serial.println("medium press - reset");
            restart();

          } else if (duration < 60000) {
            Serial.println("long press - reset settings");
            reset();
          }

        } else if (buttonState == HIGH && currentState == LOW) {
          startPress = millis();
        }
        buttonState = currentState;
      }
      break;
  }

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
          client.flush();
          Serial.println(data);
          client.print("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin:* \r\nConnection: close\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n");
          confs(0, 1, 0, "/confs.txt", "", 9); //apaga arquivo
          quebra(data);
          confs(1, 0, 0, "/tst.txt", "tst=novo", 9);
          client.stop();
          // ESP.restart();
          ESP.reset();
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

  /* Checa s eesta em modo AP e o Status da tomada no Firebase   */
    if (!ap) {
    /* Verifica o valor da TOMADA2 no Firebase */
    int valor = Firebase.getInt(hard + "/topico");

    if (valor > 1) {
      valor == 0;
      digitalWrite(SONOFF_LED, HIGH);
      digitalWrite(gpio12Relay, LOW);
    } else {
      valor == 1;
      digitalWrite(SONOFF_LED, LOW);
      digitalWrite(gpio12Relay, HIGH);
    }

    Serial.println(valor);
    Serial.println(digitalRead(gpio12Relay));
    }
  yield();

}

