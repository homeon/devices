/*
   Sketch Filtro de Linha Homeon v 1.0a
   Arquivo de Funcoes
   Maio/2017
   Desenvolvido:  Gênio Mauro
   Revisado:  Gustavo D. Cardoso / Gênio Mauro
*/

/* Checa Status no Firebase e toma acao - 1 = executa, 0 = nao faz nada e devolucao, > 8 = FORMAT */
byte firecode(byte tipo) {
  /* Linha Unica no Firebase
      if(tipo){
        int f=Firebase.getInt(hard+"flt");
        if (f & 1) {
          lp_bt((f>>1) | B11110000);
        }
      }else{

        byte res=(lp_bt(0) & B00001111)<<1;
         Firebase.setInt(hard+"flt",res);
         while(!Firebase.success()){yield();}
         st_bt=0;
      }
  */

  /* Todas as linhas no firebase */
  if (tipo) {
    if (Firebase.getInt(hard + "st") > 0) {

      if (Firebase.getInt(hard + "st") < 8) {
        unsigned int t1 = (Firebase.getInt(hard + "t1"));
        unsigned int t2 = (Firebase.getInt(hard + "t2"));
        unsigned int t3 = (Firebase.getInt(hard + "t3"));
        unsigned int t4 = (Firebase.getInt(hard + "t4"));
        byte res = t1 | (t2 << 1) | (t3 << 2) | (t4 << 3) | 240;
        lp_bt(res);
        Firebase.setInt(hard + "st", 0);
        
      } else {
        Firebase.setInt(hard + "st", 0);
        Serial.print("Mensagem devolvida para o APP...");
        Serial.print("Formatando...");
        SPIFFS.format();
        Serial.println("... OK.");
        ESP.restart();
      }
    }

  } else {
    byte res = lp_bt(0) & B00001111;
    Firebase.setInt(hard + "t1", res & 1);
    Firebase.setInt(hard + "t2", (res & 2) >> 1);
    Firebase.setInt(hard + "t3", (res & 4) >> 2);
    Firebase.setInt(hard + "t4", (res & 8) >> 3);

    //Firebase.setInt(hard + "st", 1);
    //while(!Firebase.success()){yield();}

    st_bt = 0;
  }

}

/* Botao atraves do I2C BUS */
byte lp_bt(byte lg) {
  if (lg) {
    Wire.beginTransmission(32);
    Wire.write(lg);
    Wire.endTransmission();
  } else {
    Wire.requestFrom(32, 1);
    if (Wire.available()) {
      byte c = Wire.read();
      return c;
    }
  }
}

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

/* Funcao BTOK */
void btok() { // leds:  l1 = p0 | l2= p1 | l3 = p2 | lr = p3
  // bots:  b1 = p4 | b2= p5 | b3 = p6 | br = p7
  if ((millis() - ultint) > 600)ultint = 0;

  byte j = lp_bt(0); //total
  byte y = (j >> 4) ^ 15; //bts
  byte l = j & B00001111; //rl
  byte t = 0; //trocas
  if (y == 8 && ultint == 0) {
    if (l >= 15) {
      lp_bt(B11110000);
    } else {
      lp_bt(B11111111);
    }
    st_bt = 1;
    ultint = millis();
  }

  if ( (y > 0 && y < 15)  && ultint == 0) {
    x = 0;
    while (y) {
      y = y >> 1;
      x++;
    }
    t = 1 << x - 1;
    lp_bt(((l ^ t) | B11110000));
    st_bt = 1;
    ultint = millis();
  }
}

/* Funcao de TEMPO - TESTES */
void inline dt_agora(void) {
  x = 1;
  timer0_write(ESP.getCycleCount() + (80000000 * 10));
}

/* Parametros que serão Publicados no Firebase */
void setaFirebase() {
  Firebase.set(hard + "t1", 1);
  Firebase.set(hard + "t2", 1);
  Firebase.set(hard + "t3", 1);
  Firebase.set(hard + "t4", 1);
  Firebase.set(hard + "ativo", 1);
  Firebase.set(hard + "st", 0);
  Firebase.set(hard + "flt", 0);
}

