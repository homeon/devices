/* Inclui as bibiliotescas necessárias */
#include <ESP8266WiFi.h>                  // Biblioteca Wifi
#include <ESP8266WebServer.h>             // Servidor WEB - GET, POST
#include <FirebaseArduino.h>              // Biblioteca para acesso ao Firebase
#include <EEPROM.h>                       // Biblioteca para gravacao na EEPROM
// #include <ctype.h>                        // Biblioteca p/ as funções isalpha isdigit toupper tolowe

/* Local dos itens de configuracao na EEPROM */
#define VERSION_START  500
#define CONFIG_START   6

/* ID de Configuração Inicial - EEPROM */
#define CONFIG_VERSION "1c"

/* Define os pinos utilizados pelos sensores no ESP - Simulação de Relés + RESET */
#define t1 D8         // Cor azul = relé 1
#define t2 D7         // Cor verde = relé 2
#define t3 D6         // Cor vermelho = relé 3

/* Cria as variaves que vao ser recebidas pelo APP */
String FIREBASE_HOST = "";
String FIREBASE_AUTH = "";
String WIFI_SSID = "";
String WIFI_PASSWORD = "";
String UID = "" ;

/* Cria as variaveis que serao utilizadas pelo Harware */
/* Cria variavel guardando MAC-ADDRES */
byte mac[6];
String smac = "";

void monta_smac() {
  WiFi.macAddress(mac);
  smac = String(mac[5], HEX);
  smac += String(mac[4], HEX);
  smac += String(mac[3], HEX);
  smac += String(mac[2], HEX);
  smac += String(mac[1], HEX);
  smac += String(mac[0], HEX);
  smac.toUpperCase();
}

/* Sigla do dispositivo - Nao altera */
String DS = "HMN";

/* Sigla do Tipo de Dispositivo - Varia de acordo com dispositivo */
String DT = "FLT";

/* Cria o endereco de Publicacao das informacoes = /UID/DS+DT+SMAC/ */
String endpub = "" ;

/* Estrutura de configuração da EEPROM */
struct ConfigStruct
{
  char ssid[100];
  char senha[100];
  char uid[100];
  char firebaseHost[100];
  char firebaseAuth[100];

} wifiConfig;

/* Configura os pinos GPIO - Padrao tudo ligado */
void setupPins() {

  pinMode(t1, OUTPUT);
  digitalWrite(t1, LOW);

  pinMode(t2, OUTPUT);
  digitalWrite(t2, LOW);

  pinMode(t3, OUTPUT);
  digitalWrite(t3, LOW);

}

/* Inicia Webserver para receber dados do APP */
 ESP8266WebServer server(80);

/* Salva as configuracoes na EEPROM */
void saveConfig()
{
  for (unsigned int t = 0; t < sizeof(wifiConfig); t++) {
    EEPROM.write(CONFIG_START + t, *((char*)&wifiConfig + t));
  }

  /* Salvando o ID da versão para puxar da EEPROM da proxima vez que for carregar */
  EEPROM.write(VERSION_START + 0, CONFIG_VERSION[0]);
  EEPROM.write(VERSION_START + 1, CONFIG_VERSION[1]);
  EEPROM.commit();
}

/* Parametros que serão Publicados no Firebase */
void setupFirebase() {
  Firebase.begin(wifiConfig.firebaseHost, wifiConfig.firebaseAuth);
  Firebase.setBool(endpub + "TMD1_COMANDO", true);
  Firebase.setBool(endpub + "TMD1_STATUS", "on");
  Firebase.setBool(endpub + "TMD2_COMANDO", true);
  Firebase.setBool(endpub + "TMD2_STATUS", "on");
  Firebase.setBool(endpub + "TMD3_COMANDO", true);
  Firebase.setBool(endpub + "TMD2_STATUS", "on");
}

/* Carrega as configurações da EEPROM */
void loadConfig()
{
  byte value;
  int ret = 0;

  for (int i = 0; i < 512; i++) {
    value = EEPROM.read(i);
    Serial.println("Posicao: " + String(i) + " Valor: " + String(value));
    ret = ret + value;
  }

  Serial.println("Value: " + String(ret));

  /* Mostra na console os dados sobre a versao */
  if (EEPROM.read(VERSION_START + 0) == CONFIG_VERSION[0] && EEPROM.read(VERSION_START + 1) == CONFIG_VERSION[1]) {

    // if ( ret != 0 ) {

    /* Carregando a estrutura Mainconfig */
    for (unsigned int t = 0; t < sizeof(wifiConfig); t++)
      *((char*)&wifiConfig + t) = EEPROM.read(CONFIG_START + t);

    Serial.println("Modo cliente iniciado..: " + String(wifiConfig.ssid));

    /* Teste modo Wifi Client */
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiConfig.ssid, wifiConfig.senha);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

  }
  else {
    Serial.println("Modo AP iniciado..: ");

    /* Configuração inicial - Modo AP */
    monta_smac();
    String ssidlong = DS + "-" + DT + "-" + smac;
    String senha = "";
    ssidlong.toCharArray(wifiConfig.ssid, 50);
    senha.toCharArray(wifiConfig.senha, 50);
    UID.toCharArray(wifiConfig.uid, 100);
    FIREBASE_HOST.toCharArray(wifiConfig.firebaseHost, 100);
    FIREBASE_AUTH.toCharArray(wifiConfig.firebaseAuth, 100);

    WiFi.disconnect(true);
    delay(1000);
    WiFi.softAP(wifiConfig.ssid, wifiConfig.senha);
    saveConfig();
  }
}

/* Apenas teste para checar servidor Web - Sera excluido posteriormente 
void chamadaok() {
  server.send(200, "text/html", "<h1>Ta no ar</h1>");
}
*/

/* Estrutura para receber os dados via POST */
void configWifiSubmit()
{
  WIFI_SSID = server.arg("ssid");
  WIFI_PASSWORD = server.arg("senha");
  UID = server.arg("uid");
  FIREBASE_HOST = server.arg("firebaseHost");
  FIREBASE_AUTH = server.arg("firebaseAuth");

  WIFI_SSID.toCharArray(wifiConfig.ssid, 100);
  WIFI_PASSWORD.toCharArray(wifiConfig.senha, 100);
  UID.toCharArray(wifiConfig.uid, 100);
  FIREBASE_HOST.toCharArray(wifiConfig.firebaseHost, 100);
  FIREBASE_AUTH.toCharArray(wifiConfig.firebaseAuth, 100);

  /* Mostra na console os dados recebidos */
  Serial.println("Novo SSID: " + WIFI_SSID);
  Serial.println("Senha Roteador: " + WIFI_PASSWORD);
  Serial.println("UID: " + UID);
  Serial.println("Firebase Host: " + FIREBASE_HOST);
  Serial.println("Firebase Segredo: " + FIREBASE_AUTH);
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());

  /* Devolve status para o APP */
  server.send(200, "text/html");

  /* Salva as configuracoes na EEPROM */
  saveConfig();

  /* Re-inicia ESP */
  WiFi.disconnect(true);
  delay(1000);
  ESP.restart();
}


/* Inicio do Setup */
void setup() {
  delay(1000);
  Serial.begin(115200);

  /* Iniciando EEPROM */
  EEPROM.begin(512);

  /* Configura os pinos - checar status no firebase antes */
  setupPins();

  /* Carrega configuração da EEPROM - Se não existir, cria */
  loadConfig();

  /* Monta a variavel SMAC e ENDPUB para publicacao no Firebase */
  monta_smac();
  endpub = String(wifiConfig.uid) + "/" + DS + DT + smac + "/";

  /* Inicia Firebase */
  setupFirebase();

  /* Mostra os dados atuais (AP) na porta serial */
  IPAddress myIP = WiFi.softAPIP();
  Serial.println("Endereço IP modo Access Point...: ");
  Serial.println(myIP);
  Serial.println("SSID...: ");
  Serial.println(wifiConfig.ssid);
  Serial.println("Senha...: ");
  Serial.println(wifiConfig.senha);
  Serial.println("endpub: " + endpub);
  Serial.println("uid: " + String(wifiConfig.uid));

  /* Configura servidor WEB para receber dados via APP */
  server.on("/ler", HTTP_POST, configWifiSubmit);
  //  server.on("/status", HTTP_GET, chamadaok);
  
  server.begin();

  //Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient();

    /* Verifica o valor da TOMADA1 no Firebase */
  bool tmd1Value = Firebase.getBool(endpub + "/TMD1_COMANDO");
  digitalWrite(t1, tmd1Value ? HIGH : LOW);
  Firebase.setBool(endpub + "TMD1_STATUS", tmd1Value);

  /* Verifica o valor da TOMADA2 no Firebase */
  bool tmd2Value = Firebase.getBool(endpub + "/TMD2_COMANDO");
  digitalWrite(t2, tmd2Value ? HIGH : LOW);
  Firebase.setBool(endpub + "TMD2_STATUS", tmd2Value);

  /* Verifica o valor da TOMADA3 no Firebase */
  bool tmd3Value = Firebase.getBool(endpub + "/TMD3_COMANDO");
  digitalWrite(t3, tmd3Value ? HIGH : LOW);
  Firebase.setBool(endpub + "TMD3_STATUS", tmd3Value);


  /* Mostra na Console dados para depuracao */
  Serial.println("Novo SSID: " + String(wifiConfig.ssid));
  Serial.println("Senha Roteador: " + String(wifiConfig.senha));
  Serial.println("UID: " + String(wifiConfig.uid));
  Serial.println("Firebase Host: " + String(wifiConfig.firebaseHost));
  Serial.println("Firebase Segredo: " + String(wifiConfig.firebaseAuth));
  Serial.println();
  Serial.println("Tomada 1 valor: " + String(tmd1Value));
  Serial.println("Tomada 2 valor: " + String(tmd2Value));
  Serial.println("Tomada 3 valor: " + String(tmd3Value));
  Serial.println("Firebase getbool tms1: " + Firebase.getBool(endpub + "/TMD1"));
  Serial.println("endpub: " + endpub);
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());

  /* Tempo para checagem de status das tomadas */
  delay(500);
}
