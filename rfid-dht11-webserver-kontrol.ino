/*
Bu kod blogu rfid ile kapı kontrolü, lcd ve web serverda sıcaklık bilgisi ve ışık açma kapatmayı içermektedir.
*/

#include <EEPROM.h> //Okuma ve yazma işini UID ile EEPROM a kaydedicez.
#include <SPI.h>
#include <MFRC522.h>
#include <Ethernet.h>
#include <dht11.h>
#include <LiquidCrystal.h>


///DHT ALANI
int DHT11_pin=9; // DHT11_pin olarak Dijital 3'yi belirliyoruz.
dht11 DHT11_sensor; 
int led = 3;

///Ethernet alanı
EthernetServer server(80);  //http port

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  
byte ip[] = { 192, 168, 4, 50 };                     
byte gateway[] = { 192, 168, 4, 1 };                   
byte subnet[] = { 255, 255, 255, 0 };  


 //LCD ekran alanı
const int rs = A0, en = A1, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define COMMON_ANODE 
#ifdef COMMON_ANODE
#define LED_ON HIGH
#define LED_OFF LOW
#else
#define LED_ON LOW
#define LED_OFF HIGH
#endif

 
#define yesilled1 7 // led pinleri
#define sariled 6
#define yesilled2 22

#define role 4 //Röle pinini seçtik.
#define delete 2 //2. Pini GND'ye bağlayarak master kartı silebiliriz
 

#define RST_PIN 5    //rfid reset pini
#define SS_PIN 53   //rfid sda pini

boolean match=false;
boolean programMode=false; //Programlama modu başlatma.
 
int successRead; //Başarılı bir şekilde sayı okuyabilmek için integer atıyoruz.
 
byte storedCard[4]; //Kart EEPROM tarafından okundu.
byte readCard[4]; //RFID modül ile ID tarandı.
byte masterCard[4]; //Master kart ID'si EEPROM'a aktarıldı.


MFRC522 mfrc522(SS_PIN, RST_PIN);   // mfrc522 nesnesi oluşturuldu

MFRC522::MIFARE_Key key;
String readString;

void setup()
{
      //Protokol konfigrasyonu
      Serial.begin(9600); //PC ile seri iletim başlat.
      
      //LCD nin satır ve sutun sayısını giriyoruz:
      lcd.begin(16, 2);
      pinMode(led, OUTPUT);
      // start the Ethernet connection and the server:
      Ethernet.begin(mac, ip, gateway, subnet);
      server.begin();
      Serial.println(Ethernet.localIP());      
      lcd.setCursor(0,1);
      lcd.print(Ethernet.localIP());    
      delay(1000);  
      //Arduino Pin Konfigrasyonu
      pinMode(yesilled1,OUTPUT);
      pinMode(sariled,OUTPUT);
      pinMode(yesilled2,OUTPUT);
      pinMode(delete,INPUT_PULLUP);
      pinMode(role,OUTPUT);       
      digitalWrite(role,HIGH); //Kapının kilitli
      digitalWrite(yesilled1,LED_OFF); 
      digitalWrite(sariled,LED_OFF);
      digitalWrite(yesilled2,LED_OFF); 
             

      SPI.begin(); //MFRC522 donanımı SPI protokolünü kullanır.
      mfrc522.PCD_Init(); //MFRC522 donanımını başlat.
             
      Serial.println(F("Giris Kontrol v1.0")); //Hata ayıklama amacıyla
      ShowReaderDetails(); //PCD ve MFRC522 kart okuyucu ayrıntılarını göster.
 
      //2. pini GNS'ye bağlarsak EEPROM temizlenir
      if(digitalRead(delete)==LOW)
      {
        //Butona bastığınızda pini düşük almalısınız çünkü buton toprağa bağlı.
        digitalWrite(yesilled1,LED_ON); //Kırmızı led sildiğimizi bilgilendirmek amacıyla yanık kalır.
        Serial.println(F("Silme butonuna basildi."));
        Serial.println(F("5 saniye icinde iptal edebilirsiniz.."));
        lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
        lcd.print("Kayitlar Siliniyor");
        Serial.println(F("Tum kayitlar silinecek ve bu islem geri alinamayacak."));
        delay(5000); //Kullanıcıya iptal işlemi için yeterli zaman verin.
      if (digitalRead(delete)==LOW) //
          {
          Serial.println(F("EEPROM silinmeye baslaniyor."));
          for (int x=0; x<EEPROM.length();x=x+1) //EEPROM adresinin döngü sonu
          {
          if(EEPROM.read(x)==0) //EEPROM adresi sıfır olursa
          {    
          }
        else
        {
                EEPROM.write(x,0);
                }
                }
                Serial.println(F("EEPROM Basariyla Silindi.."));
                lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
                lcd.print("Silme Basarili");
                digitalWrite(yesilled1,LED_OFF);
                delay(200);
                digitalWrite(yesilled1,LED_ON);
                delay(200);
                digitalWrite(yesilled1,LED_OFF);
                delay(200);
                digitalWrite(yesilled1,LED_ON);
                delay(200);
                digitalWrite(yesilled1,LED_OFF);
        }
 
        else
        {
              Serial.println(F("Silme islemi iptal edildi."));
               lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
               lcd.print("Silme iptal");
              digitalWrite(yesilled1,LED_OFF);
              delay(1000);
        }
        }
 
//Kart tanımlımı ona bakılacak
 
    if (EEPROM.read(1) != 143)
    {
      Serial.println(F("Master Kart Secilmedi."));
      Serial.println(F("Master Karti secmek icin kartinizi okutunuz.."));
        lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
        lcd.print("Master Kart Okut");
    do
    {
        successRead=getID(); //successRead 1 olduğu zaman okuyucu düzenlenir aksi halde 0 olucaktır.
        digitalWrite(yesilled2,LED_ON); //Master Kartın kaydedilmesi için gösterildiğini ifade eder.
        delay(200);
        digitalWrite(yesilled2,LED_OFF);
        delay(200);
    }
    while(!successRead); //Başarılı bir şekilde okuyamadıysa Başarılı okuma işlemini yapmicaktır.
        for (int j=0; j<4; j++) //4 kez döngü
        {
          EEPROM.write(2+j,readCard[j]); //UID EPPROM a yazıldı, 3. adres başla.
        }
          EEPROM.write(1,143); //EEPROM a Master Kartı kaydettik.
          Serial.println(F("Master Kart kaydedildi.."));
        lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
        lcd.print("Master Kart Tamam");
        delay(1400);
        }
         
    Serial.println(F("*****************"));
    Serial.println(F("Master Kart UID:"));
    for(int i=0; i<4; i++) //EEPROM da Master Kartın UID'si okundu.
      { //Master Kart yazıldı.
          masterCard[i]=EEPROM.read(2+i);
          Serial.print(masterCard[i],HEX);
      }
     
    Serial.println(F("***************"));
    Serial.println(F("Hersey Hazir!!"));
    Serial.println(F("Kart okutulmasi icin bekleniyor..."));
    ledYak(); //Herşeyin hazır olduğunu kullanıcıya haber vermek için geri bildirim.
}
void loop()
{
      int chk = DHT11_sensor.read(DHT11_pin);
      float nem = DHT11_sensor.humidity;
      float sicaklik = DHT11_sensor.temperature;

        Serial.print("Nem: ");
        Serial.print(nem);
        Serial.print(" %\t");
        Serial.print("Sıcaklık: ");
        Serial.print(sicaklik);
        Serial.println(" *C");

        
    do{
        successRead=getID(); //SuccessRead 1 olursa kartı oku aksi halde 0
        if(programMode){
            ledYak(); 
        }
        else{
        normalModeOn(); //Normal mod
        sicaklikolc();
        }
 
    }
    while(!successRead);
    if(programMode){
          if(isMaster(readCard)) //Master Kart tekrar okutulursa programdan çıkar.
          {
              Serial.println(F("Master Kart Okundu.."));
              Serial.println(F("Programdan cikis yapiliyor.."));
              Serial.println(F("*********************"));
              programMode=false;
              return;
          }
          else
          {
                 if(findID(readCard)) //Okunan kart silinmek isteniyorsa
              {
                  Serial.println(F("Okunan kart siliniyor.."));
                  deleteID(readCard);
                   lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
                   lcd.print("Kart Silindi....");
                  Serial.println(F("****************"));
              }
              else //Okunan kart kaydedilmek isteniyorsa
              {
                  Serial.println(F("Okunan kart hafizaya kaydediliyor.."));
                  writeID(readCard);
                   lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
                  lcd.print("Kart Kayit Tamam");
                  Serial.println(F("**************************"));
              }
        }
    }
    else
    {
          if(isMaster(readCard)){
              programMode=true;
              int count=EEPROM.read(0);
              Serial.println(F("Eklemek veya cikarmak istediginiz karti okutunuz."));
              Serial.println(F("*****************************"));
              lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
              lcd.print("Kart Ekle-Cikart");
          }
          else{
              if(findID(readCard))
          {
              Serial.println(F("Hosgeldin, gecis izni verildi."));
              lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
              lcd.print("Giris Basarili");
              granted(300); //300 milisaniyede kilitli kapıyı aç.
          }
          else{
              Serial.println(F("Gecis izni verilmedi."));
              lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
              lcd.print("Giris Basarisiz");
              denied();
          }
        }
    }
}

////Ethernet Client
void sicaklikolc(){ 
      lcd.clear();
      int chk = DHT11_sensor.read(DHT11_pin);
      float nem = DHT11_sensor.humidity;
      float sicaklik = DHT11_sensor.temperature;
      // Mesajımızı LCD'ye yazdırıyoruz.
      lcd.print("ISI:");
    //DHT11 en aldığımız sıcaklık verisini Celcius olarak ekrana yazdırıyoruz
      lcd.print(sicaklik,0);
      lcd.print(" \337C ");//Celcuis işareti
     // lcd.print("NEM ");
      lcd.print("% ");
    //DHT 11 den aldığımız nem verisini ekrana yazdırıyoruz
      lcd.print((float)DHT11_sensor.humidity, 2);
      
      if (isnan(sicaklik) || isnan(nem)) {
        Serial.println("DHT veri okuma hatasi");
      } else {
        Serial.print("Nem: ");
        Serial.print(nem);
        Serial.print(" %\t");
        Serial.print("Sıcaklık: ");
        Serial.print(sicaklik);
        Serial.println(" *C");
      }

  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();  
        if (readString.length() < 100) {
          readString += c;
         }
       
         if (c == '\n') {          
           Serial.println(readString);    
           client.println("HTTP/1.1 200 OK");
           client.println("Content-Type: text/html");
           client.println();     
           client.println("<HTML>");
           client.println("<HEAD>");
           client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
           client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
           client.println("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />");
           client.println("<TITLE>Birol Emekli Proje</TITLE>");
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>Birol Emekli Proje</H1>");
           client.println("<hr />");
           client.println("<br />");  
           client.println("<a href=\"/?button1on\"\">Isik Ac</a>");
           client.println("<a href=\"/?button1off\"\">Isik Kapat</a><br />");   
           client.println("<br />");      
           client.println("</BODY>");
             // DHT11 değerlerini gönderme
            client.println("<H2>");
            client.print("Nem: ");
            client.println("</H2>");
            client.println("<p />");
            client.println("<H1>");
            client.print(nem);
            client.println("</H1>");
            client.println("<p />"); 
            client.println("<H2>");
            client.print("Sicaklik: ");
            client.println("</H2>");
            client.println("<H1>");
            client.print(sicaklik);
            client.println("C");
            client.println("</H1>");
           client.println("</HTML>");   
           client.stop();
           if (readString.indexOf("?button1on") >0){
               digitalWrite(led, HIGH);
              lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
              lcd.print("LED Yandi");
           }
           if (readString.indexOf("?button1off") >0){
               digitalWrite(led, LOW);   
              lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
              lcd.print("LED Sondu");
           }
            readString="";  

         }
       }
    }
  }
 delay(500);
}

//ERİŞİM İZNİ
 
void granted(int setDelay)
{
      digitalWrite(yesilled2,LED_OFF);
      digitalWrite(yesilled1,LED_OFF);
      digitalWrite(sariled,LED_ON);
      digitalWrite(role,LOW);
      delay(setDelay); //kapının açılması için zaman kazanıyoruz.
      digitalWrite(role,HIGH); //kapı kilitlendi
      delay(1000); //1 saniye sonra yeşil led de sönücek
}
 
//ERİŞİM İZNİ VERİLMEDİ
 
void denied()
{
      digitalWrite(sariled,LED_OFF); 
      digitalWrite(yesilled2,LED_OFF); 
      digitalWrite(yesilled1,LED_ON); 
      delay(1000);
}
 
///KART OKUYUCU UID AYARLAMA
 
int getID()
{
      //Kart okuyucuyu hazır ediyoruz
      if(!mfrc522.PICC_IsNewCardPresent()){
      return 0;
      }
      if(!mfrc522.PICC_ReadCardSerial()){
      return 0;
      }
      //4 ve 7 byte UID'ler mevcut biz 4 byte olanı kullanıcaz
      Serial.println(F("Kartin UID'sini taratin..."));
      for (int i=0; i<4; i++){
          readCard[i]=mfrc522.uid.uidByte[i];
          Serial.print(readCard[i],HEX);
      }
      Serial.println("");
      mfrc522.PICC_HaltA(); //Okuma durduruluyor.
      return 1;
}
 
void ShowReaderDetails() {
      byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
      Serial.print(F("MFRC522 Software Version: 0x"));
      Serial.print(v, HEX);
      if (v == 0x91){  Serial.print(F(" = v1.0"));
        Serial.print(v);
        }
      else if (v == 0x92)
        Serial.print(F(" = v2.0"));
      else{
          Serial.print(F(" (bilinmeyen)"));
          Serial.println("");
            if ((v == 0x00) || (v == 0xFF)) {
                  Serial.println(F("DİKKAT! Haberlesme yetmezligi, MFRC522'yi dogru bagladiginizdan emin olun."));
                  while(true);
            }
      }
}

///Normal Led Modu
void normalModeOn () {
      digitalWrite(yesilled2, LED_ON); 
      digitalWrite(yesilled1, LED_OFF); 
      digitalWrite(sariled, LED_OFF); 
      digitalWrite(role, HIGH); 
}
 
//EEPROM için ID Okuma
void readID( int number )
{
  int start=(number*4)+2; //başlama pozisyonu
      for (int i=0; i<4; i++) //4 byte alamabilmek için 4 kez döngü kurucaz
      {
      storedCard[i]=EEPROM.read(start+i); // EEPROM dan diziye okunabilen değerler atayın.
      }
}
 
///EEPROM a ID Ekleme
void writeID(byte a[])
{
      if (!findID(a)) //biz eeprom a yazmadan önce önceden yazılıp yazılmadığını kontrol edin.
      {
          int num=EEPROM.read(0);
          int start= (num*4)+6;
          num++;
          EEPROM.write(0,num);
            for(int j=0;j<4;j++){
            EEPROM.write(start+j,a[j]);
            }
            ledYak();
            Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'a eklendi.."));
            }
            else{
            ledYak();
            Serial.println(F("Basarisiz! Yanlıs ID"));
            }
}

//EEPROM'dan ID Silme
void deleteID(byte a[])
{
      if (!findID(a)){
          ledYak(); //değilse
          Serial.println(F("Basarisiz! Yanlıs ID"));
      }
      else{
      int num=EEPROM.read(0);
      int slot;
      int start;
      int looping;
      int j;
      int count=EEPROM.read(0); //Kart numarasını saklayan ilk EEPROM'un ilk byte'ını oku
      slot=findIDSLOT(a);
      start=(slot*4)+2;
      looping=((num-slot)*4);
      num--; //tek sayacı azaltma
      EEPROM.write(0,num);
      for(j=0;j<looping;j++){
            EEPROM.write(start+j,EEPROM.read(start+4+j));
         }
      for(int k=0;k<4;k++) {
            EEPROM.write(start+j+k,0);
        }
      ledYak();
      Serial.println(F("Basarili bir sekilde ID kaydi EEPROM'dan silinmistir.."));
      }
}

//Byte Kontrolü
boolean checkTwo (byte a[],byte b[])
{
        if (a[0] != NULL)
          match=true;
            for (int k=0; k<4; k++){
            if(a[k] != b[k]){
                match=false;
              }
            if(match){
               return true;
            }
        else{
             return false;
        }
        }
}
 
//Boş id bulma
int findIDSLOT (byte find[] )
{
        int count = EEPROM.read(0); //EEPROM ile ilk byte ı okuyacağız.
        for(int i=1; i<=count; i++) //Döngüdeki her EEPROM girişi için
        {
            readID(i); //EEPROM daki ID yi okuyacak ve Storedcard[4] de saklayacağız.
                if (checkTwo(find, storedCard)) // Saklı Kartlar da olup olmadığının kontrolü.
                    { //aynı ID'e sahip kart bulursa geçişe izin vericek.
                      return i; //kartın slot numarası
                      break; //aramayı durdurucak.
                    }
        }
}
 
//EEPROM Id bulma
boolean findID (byte find[] )
{
          int count = EEPROM.read(0); //EEPROM'daki ilk byte'ı oku
          for(int i=1; i <= count; i++) //Önceden giriş yapılmış mı kontrolü.
          {
          readID(i);
          if(checkTwo(find,storedCard) )
              {
              return true;
              break;
              }
          else
            {
            //değilse return false
            }
          }
          return false;
}

//Led gösterisi
void ledYak()
{
    digitalWrite(yesilled2, LED_OFF); 
    digitalWrite(yesilled1, LED_OFF); 
    digitalWrite(sariled, LED_OFF); 
    delay(100);
    digitalWrite(yesilled1, LED_ON);
    delay(100);
    digitalWrite(yesilled1, LED_OFF); 
    delay(100);
    digitalWrite(sariled, LED_ON); 
    delay(100);
    digitalWrite(sariled, LED_OFF); 
    delay(100);
    digitalWrite(yesilled2, LED_ON); 
    delay(100);
    digitalWrite(yesilled2, LED_OFF);
    delay(100);
}

//Master Kart Dogruluğu Test
boolean isMaster (byte test[])
{
  if(checkTwo(test,masterCard))
      return true;
  else
      return false;
}
