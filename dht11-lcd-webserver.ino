///LCD ve Web Serverda Sıcaklık Bilgisi Gösterme
#include <SPI.h>
#include <Ethernet.h>
#include <dht11.h>
#include <LiquidCrystal.h>
int DHT11_pin=3; // DHT11_pin olarak Dijital 3'yi belirliyoruz.

dht11 DHT11_sensor; 

int led = 8;
int pos = 0; 
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   
byte ip[] = { 192, 168, 3, 50 };                      
byte gateway[] = { 192, 168, 3, 1 };                   
byte subnet[] = { 255, 255, 255, 0 };                 
EthernetServer server(80);     

const int rs = A0, en = A1, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

String readString;

void setup() {
  Serial.begin(9600);
   while (!Serial) {
    ; 
  }
  pinMode(led, OUTPUT);
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("Server IP Adresi ");
  Serial.println(Ethernet.localIP());
}


void loop() {
  lcd.clear();
  int chk = DHT11_sensor.read(DHT11_pin);
  float nem = DHT11_sensor.humidity;
  float sicaklik = DHT11_sensor.temperature;

// LCD nin satır ve sutun sayısını giriyoruz:
  lcd.begin(16, 2);
  // Mesajımızı LCD'ye yazdırıyoruz.
  lcd.print("ISI:");
//DHT11 en aldığımız sıcaklık verisini Celcius olarak ekrana yazdırıyoruz
  lcd.print(sicaklik, 0);
  lcd.print(" \337C");//Celcuis işareti
 // lcd.print("NEM ");
  lcd.print("% ");
//DHT 11 den aldığımız nem verisini ekrana yazdırıyoruz
  lcd.print(nem,0);

  if(led==HIGH){
     lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
     lcd.print("LED Yandi");
  }
  else if (led==LOW){
     lcd.setCursor(0, 1);//LCD nin ikinci satırına geçiyoruz
    lcd.print("LED Kapali");
    }
    
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
          //Serial.print(c);
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
           client.println("<TITLE>Proje</TITLE>");
           client.println("</HEAD>");
           client.println("<BODY>");
           client.println("<H1>Proje</H1>");
           client.println("<hr />");
           client.println("<br />");  
           client.println("<a href=\"/?button1on\"\">LED Yak</a>");
           client.println("<a href=\"/?button1off\"\">LED Sondur</a><br />");   
           client.println("<br />");      
           client.println("</BODY>");
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
delay(2000);
}
