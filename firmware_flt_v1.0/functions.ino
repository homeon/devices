void reset(){
  yield();
  delay(1000);
  if(!digitalRead(bttest)){    
    Serial.print("Formatando...");
    SPIFFS.format();
    Serial.println("... OK.");
    ESP.restart();
  }
}

void btok() { // leds:  l1 = p0 | l2= p1 | l3 = p2 | lr = p3
              // bots:  b1 = p4 | b2= p5 | b3 = p6 | br = p7
              
  if ((millis() - ultint) > 600)ultint=0;
  
  byte j=lp_bt(0); //total
  byte y=(j>>4)^15;  //bts
  byte l=j & B00001111; //rl
  byte t=0; //trocas 
  
  if(y==8 && ultint==0){
    if (l >= 15){
      lp_bt(B11110000);
    }else{
      lp_bt(B11111111);
    } 
    //st_bt=1; 
    ultint=millis();    
  }
  
  if( (y>0 && y<15)  && ultint==0){
    x=0; 
    while(y){
      y=y>>1;
      x++;
    }
    t= 1<<x-1;
    lp_bt(((l^t) | B11110000));  
    //st_bt=1;
    ultint=millis();    
    }
}

byte firecode(byte tipo){
  if(tipo){
    if(Firebase.getInt(hard+"st")){
      unsigned int t1=(Firebase.getInt(hard+"t1"));
      unsigned int t2=(Firebase.getInt(hard+"t2"));
      unsigned int t3=(Firebase.getInt(hard+"t3"));
      byte res= t1 | (t2 << 1) | (t3 << 2) | 240;
      lp_bt(res);
      Firebase.setInt(hard + "st", 0);
    }
  }else{   
   byte res=lp_bt(0) & B00001111;
   Firebase.setInt(hard + "t1",res & 1);
   Firebase.setInt(hard + "t2", (res & 2)>>1);
   Firebase.setInt(hard + "t3", (res & 4)>>2);
   Firebase.setInt(hard + "st", 1);  
   st_bt=0;
  }
}

byte lp_bt(byte lg){
  if (lg){    
    Wire.beginTransmission(32);  
    Wire.write(lg);
    Wire.endTransmission();
  }else{
   Wire.requestFrom(32,1);
   if(Wire.available()){
      byte c = Wire.read();
      return c;
     }
    }
 }

void monta_mac() {
  WiFi.macAddress(mac);
  for (int m = 5; m >= 0; m--) {
    smac = String(mac[m], HEX) + smac;
  }
  smac.toUpperCase();
}

String confs(bool grava, bool apaga, bool ler, String arq, String dado,int bus) {

  File Aq;
  if(!grava){ Aq = SPIFFS.open(arq, "r+"); }else{ Aq = SPIFFS.open(arq, "a"); }
  if(!Aq) return " ";
  
  if (ler) {
    x=0;      
    while (Aq.available()) {     
      String line = Aq.readStringUntil('\n');   
      if(bus == 9 && line.indexOf(dado) != -1) return line.substring(line.indexOf("=")+1,line.length()-1);      
      //Serial.print(x);Serial.print(" - ");Serial.println(line);
      if(bus < 9 && x==bus) return line.substring(line.indexOf("=")+1,line.length()-1);          
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
    confs(1,0,0,"/confs.txt",res,9);
  }
}
