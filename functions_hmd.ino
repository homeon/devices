/*
   Sketch Filtro de Linha Homeon v 1.0a
   Arquivo de Funcoes
   Maio/2017
   Desenvolvido:  Gênio Mauro
   Revisado:  Gustavo D. Cardoso 
*/

/* Função Reset */
void reset() {
  yield();
  delay(1000);
  if (!digitalRead(bttest)) {
    Serial.print("Formatando...");
    SPIFFS.format();
    Serial.println("... OK.");
    ESP.restart();
  }
}


/* Funcao montar MAC */
void monta_mac() {
  WiFi.macAddress(mac);
  for (int m = 5; m >= 0; m--) {
    smac = String(mac[m], HEX) + smac;
  }
  smac.toUpperCase();
}

/* Funcao gravar arquivos FS */
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
      Serial.println("LINHA: " + line);

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

/* Funcao quebrar */
void quebra(String cod) {
  int valcf[5] = {0, 0, 0, 0, 0};
  String res;
  for (x = 0; x < 5; x++) {
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


