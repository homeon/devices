/* Arquivo de Funcoes - Homeon Tomada v1.0a
    Desenvolvido por Genio Mauro - Revisado por Gustavo D. Cardoso
    06/2017
*/

/* Monta MAC-ADDRESS */
void monta_mac() {
  WiFi.macAddress(mac);
  for (int m = 5; m >= 0; m--) {
    smac = String(mac[m], HEX) + smac;
  }
  smac.toUpperCase();
}

/* Estrutura do File System */
String confs(bool grava, bool apaga, bool ler, String arq, String dado, int bus) {
  File Aq;
  if (!grava) {
    Aq = SPIFFS.open(arq, "r+");
  } else {
    Aq = SPIFFS.open(arq, "a");
  }
  if (!Aq) return " ";

  if (ler) {
    x = 0;
    while (Aq.available()) {
      String line = Aq.readStringUntil('\n');
      if (bus == 9 && line.indexOf(dado) != -1) return line.substring(line.indexOf("=") + 1, line.length() - 1);
      //Serial.print(x);Serial.print(" - ");Serial.println(line);
      if (bus < 9 && x == bus) return line.substring(line.indexOf("=") + 1, line.length() - 1);
      x++;
    }
  }
  if (grava) {
    Aq.println(dado);
  }
  if (apaga) {
    SPIFFS.remove(arq);

  } else {
    Aq.close();
  }
}

/* Quebra os dados recebidos em arquivo */
void quebra(String cod) {
  int valcf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  String res;
  for (x = 0; x < 8; x++) {
    if (x == 0) {
      valcf[x] = cod.indexOf("&");
      res = cod.substring(valcf[x], 0);
    } else {
      valcf[x] = cod.indexOf("&", valcf[x - 1] + 1);
      res = cod.substring(valcf[x], valcf[x - 1] + 1);
    }
    confs(1, 0, 0, "/confs.txt", res, 9);
  }
}

/* Ações do botão SONOFF */
void tick()
{
  //toggle state
  int state = digitalRead(SONOFF_LED);  // get the current state of GPIO1 pin
  digitalWrite(SONOFF_LED, !state);     // set pin to the opposite state
}

void setState(int state, int channel) {
  //relay
  //digitalWrite(SONOFF_RELAY_PINS[channel], state);

  //led
  if (SONOFF_LED_RELAY_STATE) {
    digitalWrite(SONOFF_LED, (state + 1) % 2); // led is active low
  }
}

void turnOn(int channel = 0) {
  int relayState = HIGH;
  setState(relayState, channel);
}

void turnOff(int channel = 0) {
  int relayState = LOW;
  setState(relayState, channel);
}

void toggleState() {
  cmd = CMD_BUTTON_CHANGE;
}

/* Checa o estado e liga ou desliga */
void toggle(int channel = 0) {
  Serial.println("toggle state");
  //  Serial.println(digitalRead(SONOFF_RELAY_PINS[channel]));
  //  int relayState = digitalRead(SONOFF_RELAY_PINS[channel]) == HIGH ? LOW : HIGH;
  setState(relayState, channel);
}

/* Reinicia Equipamento */
void restart() {
  ESP.restart();
  delay(1000);
}

/* Reseta Equipamento */
void reset() {
  //reset settings to defaults
  //TODO turn off relays before restarting
  yield();
  delay(1000);
  Serial.print("Formatando...");
  SPIFFS.format();
  Serial.println("... OK.");
  ESP.restart();
  // ESP.reset();
  delay(1000);
}

/* Parametros que serão Publicados no Firebase */
void setaFirebase() {
  Firebase.set(hard + "ativo", 1);
  Firebase.set(hard + "topico", 1);
}

/* Troca de status */
void troca () {
  Serial.println(digitalRead(gpio12Relay));
  int relayState = digitalRead(gpio12Relay) == HIGH ? LOW : HIGH;

  if (relayState > 0) {
    setState(gpio12Relay, HIGH);
    if (!ap) {
      Firebase.setInt(hard + "topico", 1);
      // while (!Firebase.success()) {
      //   yield();
      // }
    }
  } else {
    setState(gpio12Relay, LOW);
    if (!ap) {
      Firebase.setInt(hard + "topico", 3);
      // while (!Firebase.success()) {
      //   yield();
      // }
    }
  }
}


