///Thingspeak üzerinde sıcaklık gösterimi
///Adafruit üzerinden buton kontrolü

#include <SPI.h>
#include <Ethernet.h>
#include <DHT.h> // dht11 kütüphanesini ekliyoruz.
#include "Adafruit_IO_Client.h"

#define DHTPIN 2  //DHT11 data pini
DHT dht(DHTPIN, DHT11); 


// Configure Adafruit IO access.
#define AIO_KEY    "879c33d447504bcf88a8c47cea83cbf1"

int pin = 2; // analog pin
int tempc = 0,tempf=0; // temperature variables
int samples[8]; // variables to make a better precision
int maxi = -100,mini = 100; // to start max/min temperature
int i;

//Ethernet bağlantısı 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //physical mac address 
byte ip[] = { 192, 168, 1, 32 }; // IP address in LAN – need to change according to your Network address 
byte gateway[] = { 192, 168, 1, 1 }; // internet access via router 
byte subnet[] = { 255, 255, 255, 0 }; //subnet mask 

// ThingSpeak Settings
char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "5ZNZ16VSCY8HYSJG";    // Write API Key for a ThingSpeak Channel
const int updateInterval = 10000;        // Time interval in milliseconds to update ThingSpeak   

// Variable Setup
long lastConnectionTime = 0; 
boolean lastConnected = false;
int failedCounter = 0;

// Initialize Arduino Ethernet Client
EthernetClient client;

Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);
Adafruit_IO_Feed testFeed = aio.getFeed("ToogleAnahtar");

void setup()
{
  Serial.begin(9600);
  Ethernet.begin(mac, ip, gateway, subnet);
  delay(1000);
  Serial.print("Ethernet IP Adresi: ");
  Serial.println(Ethernet.localIP()); 
  dht.begin();        //dht başlatılır
   
 //Ethernet Shild çağrılıyor
  startEthernet();
  aio.begin();

  Serial.println(F("Ready!"));
}

void loop()
{
  delay(3000);
  sicaklikkontrol();
  delay(1000);
 // butonkontrol();
  
}

void sicaklikkontrol(){
  
  float t = dht.readTemperature();  //Sıcaklık bilgisi alınır
  float h = dht.readHumidity();//Nem bilgisi alınır

  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }
  if (!client.connected() && lastConnected)
  {
    Serial.println();
    Serial.println("...disconnected.");
    Serial.println();   
    client.stop();
  }
  
  // ThingSpeak veri güncelleniyor
  if(!client.connected() && (millis() - lastConnectionTime > updateInterval))
  {
    updateThingSpeak("field1="+(String)t);
  }
  
  lastConnected = client.connected();
  
  }
void updateThingSpeak(String tsData)
{
  if (client.connect(thingSpeakAddress, 80))
  { 
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);

    Serial.print("Sicaklik: ");
    Serial.print(tsData);
    lastConnectionTime = millis();
    
    if (client.connected())
    {
      Serial.println("ThingSpeak bağlanılıyor...");
      Serial.println();
      
      failedCounter = 0;
    }
    else
    {
      failedCounter++; 
      Serial.println(" ThingSpeak bağlanılamadı ("+String(failedCounter, DEC)+")");   
      Serial.println();
    }   
  }
  else
  {
    failedCounter++;  
    Serial.println("ThingSpeak bağlanılamadı ("+String(failedCounter, DEC)+")");   
    Serial.println();   
    lastConnectionTime = millis(); 
  }
}

void startEthernet()
{  
  client.stop();
  Serial.println("Arduino ağa bağlanıyor");
  Serial.println(); 
  delay(1000);
  
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Arduino Dhcp'den bağlanamadı");
    Serial.println();
  }
  else {
    Serial.println("Arduino DHCP den ağa bağlandı");
    Serial.println();
    Serial.println("Veri Thingspeak'a yükleniyor...");
    Serial.println();
  } 
  delay(1000);
}


void butonkontrol()
{
   FeedData buttonOku=testFeed.receive();
   if(strcmp(buttonOku,"AC")==0)
    digitalWrite(7,HIGH);
   else
    digitalWrite(7,LOW);
  
}
