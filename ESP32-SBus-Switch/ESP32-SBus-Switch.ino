
#include "sbus.h"
#include <WiFi.h>
#include <EEPROM.h>

const float Version = 0.1; // Software Version

// EEprom

#define EEPROM_SIZE 20

#define adr_eprom_sbus_channel 0          // SBUS Channel
#define adr_eprom_PWM_0 2                 // PWM Wert Ausgang 1
#define adr_eprom_PWM_1 4                 // PWM Wert Ausgang 2
#define adr_eprom_PWM_2 6                 // PWM Wert Ausgang 3
#define adr_eprom_PWM_3 8                 // PWM Wert Ausgang 4
#define adr_eprom_PWM_4 10                // PWM Wert Ausgang 5
#define adr_eprom_PWM_5 12                // PWM Wert Ausgang 6
#define adr_eprom_PWM_6 14                // PWM Wert Ausgang 7
#define adr_eprom_PWM_7 16                // PWM Wert Ausgang 8
#define adr_eprom_Mode  18                // Mode

// PWM
const int freq = 5000;
const int ledChannel[8] = {0, 1, 2, 3, 4, 5, 6, 7};
const int resolution = 8;

int pwm_wert[8] = {128, 128, 128, 128, 128, 128, 128, 128};

volatile unsigned char OutPin[8]  ={18, 19, 21, 22, 23, 25, 26, 27}; // Pin-Ausgang

// SBUS
bfs::SbusRx sbus_rx(&Serial2); // Sbus auf Serial2

std::array<int16_t, bfs::SbusRx::NUM_CH()> sbus_data;

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
  sbus_rx.Begin(16, 17);

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
      sbus_data = sbus_rx.ch();
      if (sbus_rx.failsafe()) {   // bei Failsafe Ausgange ausschalten
         encodeFunction(sbus_data[0]);
      }
      else
      {
        encodeFunction(sbus_data[sbus_channel]);
      }
    
      Serial.print("Kanal");
      Serial.print(sbus_channel + 1); 
      Serial.print(" : "); 
      Serial.print(sbus_data[sbus_channel]);
      Serial.print(" : Output ");
      Serial.print(Data);
      
      Output(Data);
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

            for(x = 0; x <=7; x++)    //Abfrage der Slider PWm 0-7
            {
            if(header.indexOf("GET /?value"+ String(x, DEC)+"=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              pwm_wert[x] = (header.substring(pos1+1, pos2)).toInt();
            }
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

            // Anzeige der HTML web page
            client.println("<!DOCTYPE html><html>");
            //client.println("<meta http-equiv='refresh' content='5'>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS zum Stylen der Ein/Aus-Schaltflächen
            // Fühlen Sie sich frei, die Attribute für Hintergrundfarbe und Schriftgröße nach Ihren Wünschen zu ändern
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: yes; color: white; padding: 3px 3px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".slider { -webkit-appearance: none; width: 100%; height: 25px; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; }");
            //client.println(".slider::-moz-range-thumb { width: 25px; height: 25px; background: #04AA6D; cursor: pointer; }");
            client.println(".button2 {background-color: #555555;}</style></head>");     
            
            // Überschrift der Webseite
            client.println("<body><h1>ESP32 SBUS-Switch</h1>");
            client.println("<p>Version : " + String(Version) + "</p>");

            // PWM Werte 0-255
            for(x = 0; x <=7; x++)
            {
            valueString = String(pwm_wert[x], DEC);

            client.println("<p>PWM Kanal"+ String(x+1, DEC)+" : <span id=\"textSliderValue"+ String(x, DEC)+"\">" + valueString + "</span>");
            
            client.println("<input type=\"range\" min=\"0\" max=\"255\" step=\"5\" class=\"slider\" id=\"PWMSlider"+ String(x, DEC)+"\" onchange=\"PWMSpeed"+ String(x, DEC)+"(this.value)\" value=\"" + valueString + "\" /></p>");

            client.println("<script> function PWMSpeed"+ String(x, DEC)+"(pos) { ");
            client.println("var sliderValue = document.getElementById(\"PWMSlider"+ String(x, DEC)+"\").value;");
            client.println("document.getElementById(\"textSliderValue"+ String(x, DEC)+"\").innerHTML = sliderValue;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?value"+ String(x, DEC)+"=\" + pos + \"&\", true);");
            client.println("xhr.send(); } </script>");
            }

            // Checkbox Kompatibilitaets-Mode
            if (Mode == true) {
            client.println("<input type=\"checkbox\" id=\"tc\" checked onclick=\"myFunction(this.checked)\"> Kompatibilitaets-Mode </input>");     // Checkbox true 
            }
            else
            {
            client.println("<input type=\"checkbox\" id=\"tc\" onclick=\"myFunction(this.checked)\"> Kompatibilitaets-Mode </input>");             // Checkbox false
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
            client.println("<p><a href=\"/save\"><button class=\"button\">Save</button></a></p>");
            
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
      Serial.print(" : 1 ");
      Serial.print(Data);
    Data = (Data * 10);
      Serial.print(" : 2 ");
      Serial.print(Data);
    Data = Data + 20;  
      Serial.print(" : 3 ");
      Serial.print(Data);
    Data = Data  / 8;
      Serial.print(" : 4 ");
      Serial.print(Data);
    Data = Data  / 8;
      Serial.print(" : 5 ");
      Serial.println(Data);  
  } else {
    Data = Data  / 8;
    Serial.print(" : 1 ");
    Serial.println(Data); 
    Data = Data; 
  }
}

// ======== Output  =======================================
void Output(uint16_t Data) {
  for(x = 0; x <=7; x++)
  {
    if (bitRead (Data ,(x ))) // Abfrage für Ausgang
    {
      ledcWrite(ledChannel[x], pwm_wert[x]);   
    }
    else
    {
      ledcWrite(ledChannel[x], 0);
    }
  }
}

// ======== EEprom  =======================================
void EEprom_Load() {
  sbus_channel = EEPROM.read(adr_eprom_sbus_channel);
  pwm_wert[0] = EEPROM.read(adr_eprom_PWM_0);
  pwm_wert[1] = EEPROM.read(adr_eprom_PWM_1);
  pwm_wert[2] = EEPROM.read(adr_eprom_PWM_2);
  pwm_wert[3] = EEPROM.read(adr_eprom_PWM_3);
  pwm_wert[4] = EEPROM.read(adr_eprom_PWM_4);
  pwm_wert[5] = EEPROM.read(adr_eprom_PWM_5);
  pwm_wert[6] = EEPROM.read(adr_eprom_PWM_6);
  pwm_wert[7] = EEPROM.read(adr_eprom_PWM_7);
  Mode = EEPROM.read(adr_eprom_Mode);
  Serial.println("EEPROM gelesen.");
}
void EEprom_Save() {
  EEPROM.write(adr_eprom_sbus_channel, sbus_channel);
  EEPROM.write(adr_eprom_PWM_0, pwm_wert[0]);
  EEPROM.write(adr_eprom_PWM_1, pwm_wert[1]);
  EEPROM.write(adr_eprom_PWM_2, pwm_wert[2]);
  EEPROM.write(adr_eprom_PWM_3, pwm_wert[3]);
  EEPROM.write(adr_eprom_PWM_4, pwm_wert[4]);
  EEPROM.write(adr_eprom_PWM_5, pwm_wert[5]);
  EEPROM.write(adr_eprom_PWM_6, pwm_wert[6]);
  EEPROM.write(adr_eprom_PWM_7, pwm_wert[7]);
  EEPROM.write(adr_eprom_Mode, Mode);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert.");
}
