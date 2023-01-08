/*
   ESP32-SBus-Switch
   Ziege-One (Der RC-Modelbauer)
 
 ESP32

 /////Pin Belegung////
 GPIO 13: Wifi Pin
 GPIO 16: SBUS in
 GPIO 18: Ausgang1
 GPIO 19: Ausgang2
 GPIO 21: Ausgang3
 GPIO 22: Ausgang4
 GPIO 23: Ausgang5
 GPIO 25: Ausgang6
 GPIO 26: Ausgang7
 GPIO 27: Ausgang8


 */

// ======== ESP32-SBus-Switch  =======================================

/* Boardversion
ESP32 2.0.5
 */

/* Installierte Bibliotheken
Bolder Flight Systems SBUS  8.1.4
 */

#include "sbus.h"
#include <WiFi.h>
#include <EEPROM.h>

const float Version = 0.2; // Software Version

// EEprom

#define EEPROM_SIZE 72

#define adr_eprom_sbus_channel  0   // SBUS Channel
#define adr_eprom_Mode          4   // Mode
#define adr_eprom_Ausgang_0     8   // Ausgang 1 Kanal
#define adr_eprom_Ausgang_1     12  // Ausgang 2 Kanal
#define adr_eprom_Ausgang_2     16  // Ausgang 3 Kanal
#define adr_eprom_Ausgang_3     20  // Ausgang 4 Kanal
#define adr_eprom_Ausgang_4     24  // Ausgang 5 Kanal
#define adr_eprom_Ausgang_5     28  // Ausgang 6 Kanal
#define adr_eprom_Ausgang_6     32  // Ausgang 7 Kanal
#define adr_eprom_Ausgang_7     36  // Ausgang 8 Kanal
#define adr_eprom_PWM_0         40  // PWM Wert Ausgang 1
#define adr_eprom_PWM_1         44  // PWM Wert Ausgang 2
#define adr_eprom_PWM_2         48  // PWM Wert Ausgang 3
#define adr_eprom_PWM_3         52  // PWM Wert Ausgang 4
#define adr_eprom_PWM_4         56  // PWM Wert Ausgang 5
#define adr_eprom_PWM_5         60  // PWM Wert Ausgang 6
#define adr_eprom_PWM_6         64  // PWM Wert Ausgang 7
#define adr_eprom_PWM_7         68  // PWM Wert Ausgang 8

// PWM
const int freq = 5000;
const int ledChannel[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int resolution = 8;

int pwm_wert[8] = {128, 128, 128, 128, 128, 128, 128, 128};

int Ausgang_Kanal[8] = {0, 0, 0, 0, 0, 0, 0, 0};
bool Ausgang[8];

volatile unsigned char OutPin[8]  ={18, 19, 21, 22, 23, 25, 26, 27}; // Pin-Ausgang

// SBUS
bfs::SbusRx sbus_rx(&Serial2,16, 17,true); // Sbus auf Serial2

bfs::SbusData sbus_data;

int sbus_channel = 4; // Kanal 5 default
uint16_t Data;        // Daten für die Ausgänge
volatile int x = 0;

//Wifi
const char* ssid     = "ESP32-SBUS-Switch";
const char* password = "123456789";

//Rest
bool Mode = false;
volatile unsigned char WifiPin = 13;
volatile unsigned char LedPin = 2;

WiFiServer server(80); // die Portnummer des Webservers auf 80 setzen 

// HTTP-GET-Wert decodieren
String valueString = "0";
int pos1 = 0;
int pos2 = 0;

int Menu = 0;

// Variable zum Speichern der HTTP-Anfrage
String header;

// Aktuelle Uhrzeit
unsigned long currentTime = millis();
// Vorherige Zeit
unsigned long previousTime = 0; 
// Timeout-Zeit in Millisekunden definieren (Beispiel: 2000ms = 2s)
const long timeoutTime = 2000;

// ======== Setup  =======================================
void setup() {
  Serial.begin(115200);
  // Beginnen Sie mit der SBUS-Kommunikation
  sbus_rx.Begin();

  pinMode(WifiPin, INPUT_PULLUP);
  pinMode(LedPin, OUTPUT);
 
  ledcSetup(ledChannel[0], freq, resolution);
  ledcSetup(ledChannel[1], freq, resolution);
  ledcSetup(ledChannel[2], freq, resolution);
  ledcSetup(ledChannel[3], freq, resolution);
  ledcSetup(ledChannel[4], freq, resolution);
  ledcSetup(ledChannel[5], freq, resolution);
  ledcSetup(ledChannel[6], freq, resolution);
  ledcSetup(ledChannel[7], freq, resolution);

  ledcAttachPin(OutPin[0], ledChannel[0]);
  ledcAttachPin(OutPin[1], ledChannel[1]);
  ledcAttachPin(OutPin[2], ledChannel[2]);
  ledcAttachPin(OutPin[3], ledChannel[3]);
  ledcAttachPin(OutPin[4], ledChannel[4]);
  ledcAttachPin(OutPin[5], ledChannel[5]);
  ledcAttachPin(OutPin[6], ledChannel[6]);
  ledcAttachPin(OutPin[7], ledChannel[7]);

  if (!digitalRead(WifiPin))
  {
  digitalWrite(LedPin, HIGH);  
  Serial.print("AP (Zugangspunkt) einstellen…");
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP-IP-Adresse: ");
  Serial.println(IP);
  
  server.begin();  // Start Webserver
  }
  else
  {
  Serial.println("Kein AP-Normalmodus");  
  }
  
  EEPROM.begin(EEPROM_SIZE);

  EEprom_Load(); // Einstellung laden
}

// ======== Loop  =======================================
void loop() {
  WiFiClient client = server.available();   // Warte auf eingehende Clients
  
  if (sbus_rx.Read()) {
      sbus_data = sbus_rx.data();
      if (sbus_data.failsafe) {   // bei Failsafe Ausgange ausschalten
         Output(0);
      }
      else
      {
        encodeFunction(sbus_data.ch[sbus_channel]);      
        Output(Data);
      }
  }
  
  if (client) {                             // Wenn sich ein neuer Client verbindet,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // eine Nachricht über die serielle Schnittstelle ausdrucken
    String currentLine = "";                // einen String erstellen, um eingehende Daten vom Client zu speichern
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // Schleife, während der Client verbunden ist
      currentTime = millis();
      if (client.available()) {             // wenn Bytes vom Client zu lesen sind,
        char c = client.read();             // dann ein Byte lesen
        Serial.write(c);                    // Drucken Sie es auf dem seriellen Monitor aus
        header += c;
        if (c == '\n') {                    // wenn das Byte ein Newline-Zeichen ist
          // Wenn die aktuelle Zeile leer ist, haben Sie zwei Zeilenumbrüche hintereinander.
          // das ist das Ende der Client-HTTP-Anfrage, also senden Sie eine Antwort:
          if (currentLine.length() == 0) {
            // HTTP-Header beginnen immer mit einem Antwortcode (z. B. HTTP/1.1 200 OK)
            // und einen Inhaltstyp, damit der Client weiß, was kommt, dann eine Leerzeile:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Webseiten Eingaben abfragen


            if(header.indexOf("GET /?value=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              pwm_wert[Menu-1] = (header.substring(pos1+1, pos2)).toInt();
            }
  
            if(header.indexOf("GET /?mode=true")>=0) {  // Abfrage der Checkbox
              Mode = true;
              Serial.println("Mode 1");
            }  
            else if (header.indexOf("GET /?mode=false")>=0) { 
              Mode = false;  
              Serial.println("Mode 0");
            }

            if(header.indexOf("GET /?SBUS=")>=0) {  // Abfrage des Slider SBUS Kanal
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              sbus_channel = ((header.substring(pos1+1, pos2)).toInt()) - 1;
            }
            
            if (header.indexOf("GET /save") >= 0) {  // Abfrage Button Save
              EEprom_Save();
            } 

            if (header.indexOf("GET /reset") >= 0) {  // Abfrage Button reset
              Reset_all();
            } 

            if (header.indexOf("GET /Vor/on") >= 0) {  // Abfrage Button Next
              Menu ++;
            } 

            if (header.indexOf("GET /Zurueck/on") >= 0) {  // Abfrage Button Back
              Menu --;
            } 
            
            if (header.indexOf("GET /Einzelkanal/on") >= 0) {  // Abfrage Button Einzelkanal
              Ausgang_Kanal[Menu-1] = 0;
            } 

            if (header.indexOf("GET /Kanal_L/on") >= 0) {  // Abfrage Button Kanal_L
              Ausgang_Kanal[Menu-1] = 20;
            } 

            if (header.indexOf("GET /Kanal_H/on") >= 0) {  // Abfrage Button Kanal_H
              Ausgang_Kanal[Menu-1] = 40;
            } 

            if (header.indexOf("GET /Kanal_-/on") >= 0) {  // Abfrage Button Kanal_-
              Ausgang_Kanal[Menu-1] --;
            } 

            if (header.indexOf("GET /Kanal_+/on") >= 0) {  // Abfrage Button Kanal_+
              Ausgang_Kanal[Menu-1] ++;
            } 
            
            if (header.indexOf("GET /Wert_Kanal/on") >= 0) {  // Abfrage Button Wert_Kanal
              pwm_wert[Menu-1] = 0;
            } 
            
            if (header.indexOf("GET /PWM_Kanal/on") >= 0) {  // Abfrage Button PWM_Kanal
              pwm_wert[Menu-1] = 300;
            } 
            
            if (header.indexOf("GET /Kanal_PWM_+/on") >= 0) {  // Abfrage Button Kanal_PWM_+
              pwm_wert[Menu-1] ++;
            } 

            if (header.indexOf("GET /Kanal_PWM_-/on") >= 0) {  // Abfrage Button Kanal_PWM_-
              pwm_wert[Menu-1] --;
            } 

            //Werte begrenzen

            Menu = constrain(Menu, 0, 8); // Begrenzt den Bereich Menu auf 1 bis 8.

            if (Ausgang_Kanal[Menu-1] < 20)  //Einzelkanal 
            {
              Ausgang_Kanal[Menu-1] = constrain(Ausgang_Kanal[Menu-1], 0, 7); // Begrenzt den Bereich Ausgang_Kanal[Menu-1] auf 0 bis 7.
            }
            else if (Ausgang_Kanal[Menu-1] < 40)  //Kanal  L
            {
              Ausgang_Kanal[Menu-1] = constrain(Ausgang_Kanal[Menu-1], 20, 35); // Begrenzt den Bereich Ausgang_Kanal[Menu-1] auf 20 bis 35.
            }
            else if (Ausgang_Kanal[Menu-1] < 60)  //Kanal  H
            {
              Ausgang_Kanal[Menu-1] = constrain(Ausgang_Kanal[Menu-1], 40, 55); // Begrenzt den Bereich Ausgang_Kanal[Menu-1] auf 40 bis 55.
            }

            if (pwm_wert[Menu-1] >= 300)
            {
              pwm_wert[Menu-1] = constrain(pwm_wert[Menu-1], 300, 315); // Begrenzt den Bereich pwm_wert[Menu-1] auf 300 bis 315.              
            }
            
            // Anzeige der HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS zum Stylen der Ein/Aus-Schaltflächen
            // Fühlen Sie sich frei, die Attribute für Hintergrundfarbe und Schriftgröße nach Ihren Wünschen zu ändern
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { border: yes; color: white; padding: 10px 40px; width: 100%;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".slider { -webkit-appearance: none; width: 100%; height: 25px; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; }");
            //client.println(".slider::-moz-range-thumb { width: 25px; height: 25px; background: #04AA6D; cursor: pointer; }");
            client.println(".button1 {background-color: #4CAF50;}");
            client.println(".button2 {background-color: #ff0000;}");
            client.println(".button3 {background-color: #4CAF50; float: left; width: 45%;}");
            client.println(".button4 {background-color: #ff0000; float: right; width: 45%;}");
            client.println(".button5 {background-color: #ffe453; float: left; width: 45%;}");
            client.println(".button6 {background-color: #ffe453; float: center; width: 45%;}");
            client.println(".button7 {background-color: #ffe453; float: right; width: 45%;}");
            client.println(".disabled {opacity: 0.4; cursor: not-alloed;}");
            client.println(".textbox {font-size: 25px; text-align: center;}");
            client.println("</style></head>");    
            
            // Überschrift der Webseite
            client.println("<body><h1>ESP32 SBUS-Switch</h1>");
            client.println("<p>Version : " + String(Version) + "</p>");
            
            switch (Menu) {
              case 0:

            client.println("<body><h2>Einzelkanal Einstellung</h2>");  
            
            client.println("<p><a href=\"/Zurueck/on\"><button class=\"button button3\">Back</button></a>");
            client.println("<a href=\"/Vor/on\"><button class=\"button button4\">Next</button></a></p>");

            client.println("<br />");
            client.println("<br />");
        
            // Checkbox Kompatibilitaets-Mode
            if (Mode == true) {
            client.println("<p><input type=\"checkbox\" id=\"tc\" checked onclick=\"myFunction(this.checked)\"> Kompatibilitaets-Mode </input></p>");     // Checkbox true 
            }
            else
            {
            client.println("<p><input type=\"checkbox\" id=\"tc\" onclick=\"myFunction(this.checked)\"> Kompatibilitaets-Mode </input></p>");             // Checkbox false
            }

            client.println("<script> function myFunction(pos) { ");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?mode=\" + pos + \"&\", true);");
            client.println("xhr.send(); } </script>");

            // Sbus Channel
            valueString = String(sbus_channel +1, DEC);

            client.println("<p>Sbus Channel : <span id=\"textSBUSSliderValue\">" + valueString + "</span>");
            
            client.println("<input type=\"range\" min=\"1\" max=\"16\" step=\"1\" class=\"slider\" id=\"SBUSSlider\" onchange=\"SBUSSpeed(this.value)\" value=\"" + valueString + "\" /></p>");

            client.println("<script> function SBUSSpeed(pos) { ");
            client.println("var sliderValue = document.getElementById(\"SBUSSlider\").value;");
            client.println("document.getElementById(\"textSBUSSliderValue\").innerHTML = sliderValue;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?SBUS=\" + pos + \"&\", true);");
            client.println("xhr.send(); } </script>");

            // Save Button
            client.println("<p><a href=\"/save\"><button class=\"button button2\">Save</button></a></p>");

            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            client.println("<br />");

            client.println("<p><a href=\"/reset\"><button class=\"button button2\">Werkseinstellung</button></a></p>");

            break;

              case 1:
              case 2:
              case 3:
              case 4:
              case 5:
              case 6:
              case 7:
              case 8:

            client.println("<h2>Ausgang : " + String(Menu) + "</h2>");
              
            client.println("<p><a href=\"/Zurueck/on\"><button class=\"button button3\">Back</button></a>");
            client.println("<a href=\"/Vor/on\"><button class=\"button button4\">Next</button></a></p>");

            client.println("<br />");
            client.println("<br />");
            client.println("<br />");

            if (Ausgang_Kanal[Menu-1] < 20)  //Einzelkanal
            {
              client.println("<h3>Mode Einzelkanal</h3>");
              client.println("<p><a href=\"/Kanal_L/on\"><button class=\"button button5\">Kanal L</button></a>");
              client.println("<a href=\"/Kanal_H/on\"><button class=\"button button7\">Kanal H</button></a></p>");
              valueString = String(Ausgang_Kanal[Menu-1]+1, DEC); 
            }
            else if (Ausgang_Kanal[Menu-1] < 40)  //Kanal  L
            {
              client.println("<h3>Mode Kanal L</h3>");
              client.println("<p><a href=\"/Einzelkanal/on\"><button class=\"button button5\">Einzelkanal</button></a>");
              client.println("<a href=\"/Kanal_H/on\"><button class=\"button button7\">Kanal H</button></a></p>");
              valueString = String(Ausgang_Kanal[Menu-1]-20+1, DEC); 
            }  
            else if (Ausgang_Kanal[Menu-1] < 60)  //Kanal  H
            {
              client.println("<h3>Mode Kanal H</h3>");
              client.println("<p><a href=\"/Einzelkanal/on\"><button class=\"button button5\">Einzelkanal</button></a>");
              client.println("<a href=\"/Kanal_L/on\"><button class=\"button button7\">Kanal L</button></a></p>");
              valueString = String(Ausgang_Kanal[Menu-1]-40+1, DEC);               
            }  

            client.println("<br />");
            client.println("<br />");
            client.println("<br />");

            client.println("<p><h3>Kanalnummer : <span id=\"textSetting1Value\">" + valueString + "</span>");
            client.println("<p><a href=\"/Kanal_-/on\"><button class=\"button button5\">Kanal -</button></a>");
            client.println("<a href=\"/Kanal_+/on\"><button class=\"button button7\">Kanal +</button></a></p>");    
             
            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            
            if (pwm_wert[Menu-1] >= 300)
            {
              client.println("<h3>PWM Kanal</h3>");
              client.println("<p><a href=\"/Wert_Kanal/on\"><button class=\"button button6\">Wert</button></a>");
              valueString = String(pwm_wert[Menu-1]-300+1, DEC);
              client.println("<p><h3>Kanalnummer : <span id=\"textSetting1Value\">" + valueString + "</span>");
              client.println("<p><a href=\"/Kanal_PWM_-/on\"><button class=\"button button5\">Kanal -</button></a>");
              client.println("<a href=\"/Kanal_PWM_+/on\"><button class=\"button button7\">Kanal +</button></a></p>"); 
            }
            else
            {
              client.println("<h3>PWM Wert</h3>");
              client.println("<p><a href=\"/PWM_Kanal/on\"><button class=\"button button6\">Kanal</button></a>");
              valueString = String(pwm_wert[Menu-1], DEC);

              client.println("<p><h3>PWM Wert : <span id=\"textSliderValue\">" + valueString + "</span>");
            
              client.println("<input type=\"range\" min=\"0\" max=\"255\" step=\"5\" class=\"slider\" id=\"PWMSlider\" onchange=\"PWMSpeed(this.value)\" value=\"" + valueString + "\" /></p>");

              client.println("<script> function PWMSpeed(pos) { ");
              client.println("var sliderValue = document.getElementById(\"PWMSlider\").value;");
              client.println("document.getElementById(\"textSliderValue\").innerHTML = sliderValue;");
              client.println("var xhr = new XMLHttpRequest();");
              client.println("xhr.open('GET', \"/?value=\" + pos + \"&\", true);");
              client.println("xhr.send(); } </script>");
            }       
            
            // Save Button
            client.println("<p><a href=\"/save\"><button class=\"button button2\">Save</button></a></p>");
              
            break;
            }
            client.println("</body></html>");

            // Die HTTP-Antwort endet mit einer weiteren Leerzeile
            client.println();
            // Aus der while-Schleife ausbrechen
            break;
          } else {           // Wenn Sie eine neue Zeile haben, löschen Sie die aktuelle Zeile
            currentLine = "";
          }
        } else if (c != '\r') {  // Wenn Sie etwas anderes als ein Wagenrücklaufzeichen haben,
          currentLine += c;      // füge es am Ende der aktuellen Zeile hinzu
        }
      }
    }
    // header löschen
    header = "";
    // Schließen Sie die Verbindung
    client.stop();
    Serial.println("Client getrennt.");
    Serial.println("");  
    }
}

// ======== encodeFunction  =======================================
void encodeFunction(uint16_t channel) {
    Data = channel;
  if (Mode == true) {    
    if (Data < 206) { Data = 206;}
    if (Data > 1837) { Data= 1837;}
    Data = Data - 206 ;
    Data = (Data * 10);
    Data = Data + 20;  
    Data = Data  / 8;
    Data = Data  / 8;
  } else {
    Data = Data  / 8;
    Data = Data; 
  }
}

// ======== Output  =======================================
void Output(uint16_t Data) {
  for(x = 0; x <=7; x++)
  {
    Serial.print("Ausgang_Kanal :");
    Serial.print(Ausgang_Kanal[x]); 
    if (Ausgang_Kanal[x] < 20)  //Einzelkanal 
    {
      if (bitRead (Data ,(Ausgang_Kanal[x]))) // Abfrage für Ausgang
      {
        Ausgang[x] = true;
      }
      else
      {
        Ausgang[x] = false;
      } 
    }
    else if (Ausgang_Kanal[x] < 40)  //Kanal  L
    {
      if (sbus_data.ch[Ausgang_Kanal[x]-20] < 800)
      {
        Ausgang[x] = true;
      }
      else
      {
        Ausgang[x] = false;
      } 
    }  
    else if (Ausgang_Kanal[x] < 60)  //Kanal  H
    {
      if (sbus_data.ch[Ausgang_Kanal[x]-40] > 1200)
      {
        Ausgang[x] = true;
      }
      else
      {
        Ausgang[x] = false;
      } 
    }      
    
    if (Ausgang[x]) // Abfrage für Ausgang
      {
        if (pwm_wert[x] >= 300)
        {
          ledcWrite(ledChannel[x], map(sbus_data.ch[pwm_wert[x]-300], 200, 1850, 0, 255));  
        }
        else
        {
         ledcWrite(ledChannel[x], pwm_wert[x]);  
        } 
      }
      else
      {
        ledcWrite(ledChannel[x], 0);
      } 
  }
   
}

// ======== Werkseinstellungen  =======================================
void Reset_all() {
  sbus_channel = 4;
  Mode = 0;
  for(x = 0; x <=7; x++)
  {
    pwm_wert[x] = 255;
    Ausgang_Kanal[x] = x;
  }
}

// ======== Debug  =======================================
void Debug_out() {
  Serial.print("Einzelkanal");
  Serial.print(sbus_channel + 1); 
  Serial.print(" : "); 
  Serial.print(sbus_data.ch[sbus_channel]);
  Serial.print(" : Output ");
  Serial.print(Data);
  Serial.println(".");
}

// ======== EEprom  =======================================
void EEprom_Load() {
  sbus_channel = EEPROM.read(adr_eprom_sbus_channel);
  Mode = EEPROM.read(adr_eprom_Mode);
  Ausgang_Kanal[0] = EEPROM.readInt(adr_eprom_Ausgang_0);
  Ausgang_Kanal[1] = EEPROM.readInt(adr_eprom_Ausgang_1);
  Ausgang_Kanal[2] = EEPROM.readInt(adr_eprom_Ausgang_2);
  Ausgang_Kanal[3] = EEPROM.readInt(adr_eprom_Ausgang_3);
  Ausgang_Kanal[4] = EEPROM.readInt(adr_eprom_Ausgang_4);
  Ausgang_Kanal[5] = EEPROM.readInt(adr_eprom_Ausgang_5);
  Ausgang_Kanal[6] = EEPROM.readInt(adr_eprom_Ausgang_6);
  Ausgang_Kanal[7] = EEPROM.readInt(adr_eprom_Ausgang_7);
  pwm_wert[0] = EEPROM.readInt(adr_eprom_PWM_0);
  pwm_wert[1] = EEPROM.readInt(adr_eprom_PWM_1);
  pwm_wert[2] = EEPROM.readInt(adr_eprom_PWM_2);
  pwm_wert[3] = EEPROM.readInt(adr_eprom_PWM_3);
  pwm_wert[4] = EEPROM.readInt(adr_eprom_PWM_4);
  pwm_wert[5] = EEPROM.readInt(adr_eprom_PWM_5);
  pwm_wert[6] = EEPROM.readInt(adr_eprom_PWM_6);
  pwm_wert[7] = EEPROM.readInt(adr_eprom_PWM_7);
  Serial.println("EEPROM gelesen.");
}
void EEprom_Save() {
  EEPROM.writeInt(adr_eprom_sbus_channel, sbus_channel);
  EEPROM.writeInt(adr_eprom_Mode, Mode);
  EEPROM.writeInt(adr_eprom_Ausgang_0, Ausgang_Kanal[0]);
  EEPROM.writeInt(adr_eprom_Ausgang_1, Ausgang_Kanal[1]);
  EEPROM.writeInt(adr_eprom_Ausgang_2, Ausgang_Kanal[2]);
  EEPROM.writeInt(adr_eprom_Ausgang_3, Ausgang_Kanal[3]);
  EEPROM.writeInt(adr_eprom_Ausgang_4, Ausgang_Kanal[4]);
  EEPROM.writeInt(adr_eprom_Ausgang_5, Ausgang_Kanal[5]);
  EEPROM.writeInt(adr_eprom_Ausgang_6, Ausgang_Kanal[6]);
  EEPROM.writeInt(adr_eprom_Ausgang_7, Ausgang_Kanal[7]);
  EEPROM.writeInt(adr_eprom_PWM_0, pwm_wert[0]);
  EEPROM.writeInt(adr_eprom_PWM_1, pwm_wert[1]);
  EEPROM.writeInt(adr_eprom_PWM_2, pwm_wert[2]);
  EEPROM.writeInt(adr_eprom_PWM_3, pwm_wert[3]);
  EEPROM.writeInt(adr_eprom_PWM_4, pwm_wert[4]);
  EEPROM.writeInt(adr_eprom_PWM_5, pwm_wert[5]);
  EEPROM.writeInt(adr_eprom_PWM_6, pwm_wert[6]);
  EEPROM.writeInt(adr_eprom_PWM_7, pwm_wert[7]);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert.");
}
