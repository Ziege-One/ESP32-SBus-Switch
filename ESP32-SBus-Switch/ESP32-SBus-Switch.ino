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
ESP32 3.2.0
 */

/* Installierte Bibliotheken
Bolder Flight Systems SBUS  8.1.4
 */

#include "sbus.h"
#include <WiFi.h>
#include <EEPROM.h>

const float Version = 0.6; // Software Version

// EEprom

#define EEPROM_SIZE 114

#define adr_eprom_RC_System     0   // RC_System Elrs Flysky ...
#define adr_eprom_SBUS_Channel  4   // SBUS_Channel
#define adr_eprom_einkanal_mode 8   // Einkanal Mode
#define adr_eprom_Ausgang_0     12   // Ausgang 1 Kanal
#define adr_eprom_Ausgang_1     16  // Ausgang 2 Kanal
#define adr_eprom_Ausgang_2     20  // Ausgang 3 Kanal
#define adr_eprom_Ausgang_3     24  // Ausgang 4 Kanal
#define adr_eprom_Ausgang_4     28  // Ausgang 5 Kanal
#define adr_eprom_Ausgang_5     32  // Ausgang 6 Kanal
#define adr_eprom_Ausgang_6     36  // Ausgang 7 Kanal
#define adr_eprom_Ausgang_7     40  // Ausgang 8 Kanal
#define adr_eprom_PWM_0         44  // PWM Wert Ausgang 1
#define adr_eprom_PWM_1         48  // PWM Wert Ausgang 2
#define adr_eprom_PWM_2         52  // PWM Wert Ausgang 3
#define adr_eprom_PWM_3         56  // PWM Wert Ausgang 4
#define adr_eprom_PWM_4         60  // PWM Wert Ausgang 5
#define adr_eprom_PWM_5         64  // PWM Wert Ausgang 6
#define adr_eprom_PWM_6         68  // PWM Wert Ausgang 7
#define adr_eprom_PWM_7         72  // PWM Wert Ausgang 8
#define adr_eprom_Mode_0        76  // Mode Ausgang 1
#define adr_eprom_Mode_1        80  // Mode Ausgang 2
#define adr_eprom_Mode_2        84  // Mode Ausgang 3
#define adr_eprom_Mode_3        88  // Mode Ausgang 4
#define adr_eprom_Mode_4        92  // Mode Ausgang 5
#define adr_eprom_Mode_5        96  // Mode Ausgang 6
#define adr_eprom_Mode_6        100  // Mode Ausgang 7
#define adr_eprom_Mode_7        104  // Mode Ausgang 8


// PWM
const int freq = 5000;
const int resolution = 8;

int pwm_wert[8] = {128, 128, 128, 128, 128, 128, 128, 128};

int mode[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int Ausgang_Kanal[8] = {0, 0, 0, 0, 0, 0, 0, 0};
bool Ausgang[8];

volatile unsigned char OutPin[8]  ={18, 19, 21, 22, 23, 25, 26, 27}; // Pin-Ausgang

// SBUS
bfs::SbusRx sbus_rx(&Serial2,16, 17,true); // Sbus auf Serial2

bfs::SbusData sbus_data;

int SBUS_Channel = 4; // Kanal 5 default
uint16_t Data;        // Daten für die Ausgänge
long Data2;
float Data3;
uint16_t Data4 = 0;  // Speicher WM
volatile int x = 0;

//Wifi
const char* ssid     = "ESP32-SBUS-Switch";
const char* password = "123456789";

//Rest
int einkanal_mode = 0;
int RC_System = 0;
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
unsigned long currentTime2 = millis();
// Vorherige Zeit
unsigned long previousTime = 0; 
unsigned long previousTimeLED[8] =  {0, 0, 0, 0, 0, 0, 0, 0};
// Timeout-Zeit in Millisekunden definieren (Beispiel: 2000ms = 2s)
const long timeoutTime = 2000;

// ======== Setup  =======================================
void setup() {
  Serial.begin(115200);
  // Beginnen Sie mit der SBUS-Kommunikation
  sbus_rx.Begin();

  pinMode(WifiPin, INPUT_PULLUP);
  pinMode(LedPin, OUTPUT);

  ledcAttach(OutPin[0], freq, resolution);
  ledcAttach(OutPin[1], freq, resolution);
  ledcAttach(OutPin[2], freq, resolution);
  ledcAttach(OutPin[3], freq, resolution);
  ledcAttach(OutPin[4], freq, resolution);
  ledcAttach(OutPin[5], freq, resolution);
  ledcAttach(OutPin[6], freq, resolution);
  ledcAttach(OutPin[7], freq, resolution);

  if (!digitalRead(WifiPin))
  {
    digitalWrite(LedPin, HIGH);  
    Serial.print("AP (Zugangspunkt) einstellen…");
    WiFi.softAP(ssid, password);

    Serial.println("IP Adresse einstellen");
    IPAddress Ip(192, 168, 1, 1);
    IPAddress NMask(255, 255, 255, 0);
    WiFi.softAPConfig(Ip, Ip, NMask);
  
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

  if (!digitalRead(WifiPin))        // Setup Modus
  {     
    Webpage();                      // Webseite laden
  }
  
  if (sbus_rx.Read()) {
      sbus_data = sbus_rx.data();
      if (sbus_data.failsafe) {   // bei Failsafe Ausgange ausschalten
         Output(0);
      }
      else
      {
        encodeFunction(sbus_data.ch[SBUS_Channel]);      
        Output(Data);
      }
  }
  
}

// ======== encodeFunction  =======================================
void encodeFunction(uint16_t channel) {
    Data = channel;

  if (einkanal_mode == 0) 
  { 

    if (RC_System == 0) {
      Data = Data  / 8;
      Data = Data; 
    }

    if (RC_System == 1) {    
      if (Data < 206) { Data = 206;}
     if (Data > 1837) { Data= 1837;}
     Data = Data - 206 ;
     Data = (Data * 10);
     Data = Data + 20;  
     Data = Data  / 8;
     Data = Data  / 8;
    } 
  
    if (RC_System == 2) {
      Data3 = Data - 172;
      Data3 = Data3 + 1.5;
      Data3 = Data3 * 0.155677655677655;
      Data2 = Data3;
      Data = Data2;
    }
  }

  if (einkanal_mode > 9) 
  {
    uint16_t n;
    uint8_t  v;
    if (RC_System == 0) //FrSky
    {
      n = (channel >= 172) ? (channel - 172 + 1)  : 0;
      v = (n >> 4);
    }
    if (RC_System == 1) //FlySky
    {
      n = (channel >= 220) ? (channel - 220) : 0;
      uint16_t n2 = n + (n >> 6);
      v = (n2 >> 4);
    }
    if (RC_System == 2) //ELRS
    {
      n = (channel >= 172) ? (channel - 172)  : 0;
      v = (n >> 4);
    }    
    if (RC_System == 3) //Hott
    {
      n = (channel >= 205) ? (channel - 205)  : 0;
      v = (n >> 4);
    }    
    //uint16_t n = (channel >= 220) ? (channel - 220) : 0;
    //uint8_t  v = (n >> 4);
    uint8_t address = (v >> 4) & 0b11;
    uint8_t sw      = (v >> 1) & 0b111;
    uint8_t state   = v & 0b1;

    if (address == (einkanal_mode - 10))  // Adresse ?
    {
      bitWrite(Data4, sw, state);
    }

    Data = Data4;  // Verschiebe zum Output

  /* 
  Serial.print("WM: ");
  Serial.print(SBUS_Channel + 1); 
  Serial.print(" : "); 
  Serial.print(sbus_data.ch[SBUS_Channel]);
  Serial.print(" : N ");
  Serial.print(n);
  Serial.print(" : V ");
  Serial.print(v);
  Serial.print(" : Adress ");
  Serial.print(address);
  Serial.print(" : sw ");
  Serial.print(sw);
  Serial.print(" : state ");
  Serial.print(state);
  Serial.println(".");
  */
  }
 
}

// ======== Output  =======================================
void Output(uint16_t Data) {
  for(x = 0; x <=7; x++)
  {
    //Serial.print("Ausgang_Kanal :");
    //Serial.print(Ausgang_Kanal[x]); 
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
        if ((mode[x] & 0xFF00) > 0) // kein Dauerlicht
        {
          currentTime2 = millis();
          if (ledcRead(OutPin[x]) <= 0)  // LED aus
          {
            if (currentTime2 - previousTimeLED[x] >= 50 * (mode[x] & 0x00FF))
            {
              previousTimeLED[x] = currentTime2;
              if (pwm_wert[x] >= 300)
              {
                ledcWrite(OutPin[x], map(sbus_data.ch[pwm_wert[x]-300], 200, 1850, 0, 255));  
              }
              else
              {
                ledcWrite(OutPin[x], pwm_wert[x]);  
              }  
            }
          }  
          else
          {
            if (currentTime2 - previousTimeLED[x] >= 50 * ((mode[x] >> 8) & 0x00FF))
            {
              previousTimeLED[x] = currentTime2;
              ledcWrite(OutPin[x], 0);      
            }
          }
        }
        else //Dauerlicht
        {
          if (pwm_wert[x] >= 300)
          {
            ledcWrite(OutPin[x], map(sbus_data.ch[pwm_wert[x]-300], 200, 1850, 0, 255));  
          }
          else
          {
          ledcWrite(OutPin[x], pwm_wert[x]);  
          } 
        }
      }
      else
      {
        ledcWrite(OutPin[x], 0);
      }   
  }
}

// ======== Werkseinstellungen  =======================================
void Reset_all() {
  RC_System = 0;
  SBUS_Channel = 4;
  einkanal_mode = 0;
  for(x = 0; x <=7; x++)
  {
    mode[x] = 0;
    pwm_wert[x] = 255;
    Ausgang_Kanal[x] = x;
  }
}

// ======== Debug  =======================================
void Debug_out() {
  Serial.print("Einzelkanal");
  Serial.print(SBUS_Channel + 1); 
  Serial.print(" : "); 
  Serial.print(sbus_data.ch[SBUS_Channel]);
  Serial.print(" : Output ");
  Serial.print(Data);
  Serial.println(".");
}

// ======== EEprom  =======================================
void EEprom_Load() {
  RC_System = EEPROM.read(adr_eprom_RC_System);  
  SBUS_Channel = EEPROM.read(adr_eprom_SBUS_Channel);
  einkanal_mode = EEPROM.read(adr_eprom_einkanal_mode);
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
  mode[0] = EEPROM.readInt(adr_eprom_Mode_0);
  mode[1] = EEPROM.readInt(adr_eprom_Mode_1);
  mode[2] = EEPROM.readInt(adr_eprom_Mode_2);
  mode[3] = EEPROM.readInt(adr_eprom_Mode_3);
  mode[4] = EEPROM.readInt(adr_eprom_Mode_4);
  mode[5] = EEPROM.readInt(adr_eprom_Mode_5);
  mode[6] = EEPROM.readInt(adr_eprom_Mode_6);
  mode[7] = EEPROM.readInt(adr_eprom_Mode_7);
  Serial.println("EEPROM gelesen.");
}
void EEprom_Save() {
  EEPROM.writeInt(adr_eprom_RC_System, RC_System);
  EEPROM.writeInt(adr_eprom_SBUS_Channel, SBUS_Channel);
  EEPROM.writeInt(adr_eprom_einkanal_mode, einkanal_mode);
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
  EEPROM.writeInt(adr_eprom_Mode_0, mode[0]);
  EEPROM.writeInt(adr_eprom_Mode_1, mode[1]);
  EEPROM.writeInt(adr_eprom_Mode_2, mode[2]);
  EEPROM.writeInt(adr_eprom_Mode_3, mode[3]);
  EEPROM.writeInt(adr_eprom_Mode_4, mode[4]);
  EEPROM.writeInt(adr_eprom_Mode_5, mode[5]);
  EEPROM.writeInt(adr_eprom_Mode_6, mode[6]);
  EEPROM.writeInt(adr_eprom_Mode_7, mode[7]);
  EEPROM.commit();
  Serial.println("EEPROM gespeichert.");
}

// ======== Webseite  =======================================  
void Webpage()
{
  WiFiClient client = server.available();   // Warte auf eingehende Clients
  
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

            if(header.indexOf("GET /?AusgangKanal=")>=0) {  // Abfrage Ausgang Einstellung
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              Ausgang_Kanal[Menu-1] = (valueString.toInt());
            }

            if(header.indexOf("GET /?AusgangKanalMODE=")>=0) {  // Abfrage Ausgang PWM Einstellung
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              mode[Menu-1] = (mode[Menu-1] & 0x00FF) | (valueString.toInt() << 8);
            }   

            if(header.indexOf("GET /?AusgangKanalMODE2=")>=0) {  // Abfrage Ausgang PWM Einstellung
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              mode[Menu-1] = (mode[Menu-1] & 0xFF00) | valueString.toInt();
            }   

            if(header.indexOf("GET /?AusgangKanalPWM=")>=0) {  // Abfrage Ausgang PWM Einstellung
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              pwm_wert[Menu-1] = (valueString.toInt());
            }            

            if(header.indexOf("GET /?RCSytem=")>=0) { // Abfrage RC-Sytem
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              RC_System = (valueString.toInt());
            }

            if(header.indexOf("GET /?SBUSChannel=")>=0) { // Abfrage SBUS-Channel
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              SBUS_Channel = (valueString.toInt());
            }

            if(header.indexOf("GET /?EinkanalMode=")>=0) { // Abfrage Einkanal-Mode
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1+1, pos2);
              einkanal_mode = (valueString.toInt());
            }  
            
            if (header.indexOf("GET /save") >= 0) {  // Abfrage Button Save
              EEprom_Save();
            } 

            if (header.indexOf("GET /reset") >= 0) {  // Abfrage Button reset
              Reset_all();
            } 

            if (header.indexOf("GET /next") >= 0) {  // Abfrage Button Next
              Menu ++;
            } 

            if (header.indexOf("GET /back") >= 0) {  // Abfrage Button Back
              Menu --;
            } 
            
            //Werte begrenzen

            Menu = constrain(Menu, 0, 9); // Begrenzt den Bereich Menu auf 1 bis 8.



            
            //HTML Seite angezeigen:
            client.println("<!DOCTYPE html><html>");
            //client.println("<meta http-equiv='refresh' content='5'>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS zum Stylen der Ein/Aus-Schaltflächen
            // Fühlen Sie sich frei, die Attribute für Hintergrundfarbe und Schriftgröße nach Ihren Wünschen zu ändern
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { border: 4px solid black; color: white; padding: 10px 40px; width: 100%; border-radius: 10px;");
            client.println("text-decoration: none; font-size: 20px; margin: 2px; cursor: pointer;}");
            client.println(".slider { -webkit-appearance: none; width: 80%; height: 30px; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; }");
            client.println(".slider::-moz-range-thumb { width: 30px; height: 30px; background: #04AA6D; cursor: pointer; }");
            client.println(".buttonA {outline: none; cursor: pointer; padding: 14px; margin: 10px; width: 90%; font-family: Verdana, Helvetica, sans-serif; font-size: 20px; background-color: #2222; color: #111; border: 4px solid black; border-radius: 10px; text-align: center;}");
            client.println(".text1 {font-family: Verdana, Helvetica, sans-serif; font-size: 25px; color: #111; text-align: center; margin-top: 0px; margin-bottom: 0px; }");
            client.println(".text2 {font-family: Verdana, Helvetica, sans-serif; font-size: 20px; color: #111; text-align: center; margin-top: 0px; margin-bottom: 0px; }");
            client.println(".text3 {font-family: Verdana, Helvetica, sans-serif; font-size: 15px; color: #111; text-align: center; margin-top: 0px; margin-bottom: 0px; }");
            client.println(".text4 {font-family: Verdana, Helvetica, sans-serif; font-size: 10px; color: #111; text-align: center; margin-top: 0px; margin-bottom: 0px; }");

            client.println(".center {text-align: center; }");
            client.println(".left {text-align: left; }");
            client.println(".right {text-align: right; }");
            
          
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
            
            // Webseiten-Überschrift
            client.println("<body>");
            client.println("<p class=\"text1\" ><b>ESP32 SBUS-Switch</b></p>"); 
            client.println("<p class=\"text3\" >Version : " + String(Version) + "</p>");
            
            switch (Menu) {
              case 0:

            client.println("<p class=\"text2\" ><b>Einzelkanal Einstellung</b></p>"); 
            
            client.println("<p><a href=\"/back\"><button class=\"button button3\">Back</button></a>");
            client.println("<a href=\"/next\"><button class=\"button button4\">Next</button></a></p>");

            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            client.println("<br />"); 

            // SBUS Kanal; id= SBUSChannel; script= setsbuschannel(); value= SBUS_Channel;
            client.println("<p class=\"text2\" >SBUS Kanal</p>");

            client.println("<p><select id=\"SBUSChannel\" class=\"buttonA\" onchange=\"setsbuschannel()\">");
            client.println("<optgroup label=\"SBUS Kanal\">");
      	    client.println("<option value=\"0\">SBUS Kanal 01</option>");
            client.println("<option value=\"1\">SBUS Kanal 02</option>");
            client.println("<option value=\"2\">SBUS Kanal 03</option>");
            client.println("<option value=\"3\">SBUS Kanal 04</option>");
            client.println("<option value=\"4\">SBUS Kanal 05</option>");
            client.println("<option value=\"5\">SBUS Kanal 06</option>");
            client.println("<option value=\"6\">SBUS Kanal 07</option>");
            client.println("<option value=\"7\">SBUS Kanal 08</option>");
            client.println("<option value=\"8\">SBUS Kanal 09</option>");
            client.println("<option value=\"9\">SBUS Kanal 10</option>");
            client.println("<option value=\"10\">SBUS Kanal 11</option>");
            client.println("<option value=\"11\">SBUS Kanal 12</option>");
            client.println("<option value=\"12\">SBUS Kanal 13</option>");
            client.println("<option value=\"13\">SBUS Kanal 14</option>");
            client.println("<option value=\"14\">SBUS Kanal 15</option>");
            client.println("<option value=\"15\">SBUS Kanal 16</option>");
            client.println("</optgroup>");
            client.println("</select><br></p>");

            client.println("<script> function setsbuschannel() { ");
            client.println("var sel = document.getElementById(\"SBUSChannel\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?SBUSChannel=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(SBUS_Channel, DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"SBUSChannel\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>");

            // RC-Sytem; id= RCSytem; script= setrcsytem(); value= RC_System;
            client.println("<p class=\"text2\" >RC-Sytem</p>");

            client.println("<p><select id=\"RCSytem\" class=\"buttonA\" onchange=\"setrcsytem()\">");
      	    client.println("<option value=\"0\">FrSky</option>");
            client.println("<option value=\"1\">FlySky</option>");
            client.println("<option value=\"2\">ELRS</option>");
            client.println("<option value=\"3\">Hott</option>");            
            client.println("</select><br></p>");

            client.println("<script> function setrcsytem() { ");
            client.println("var sel = document.getElementById(\"RCSytem\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?RCSytem=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(RC_System, DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"RCSytem\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>");

            // Einkanal-Mode; id= EinkanalMode; script= seteinkanalmode(); value= einkanal_mode;
            client.println("<p class=\"text2\" >Einkanal-Mode</p>");

            client.println("<p><select id=\"EinkanalMode\" class=\"buttonA\" onchange=\"seteinkanalmode()\">");
      	    client.println("<option value=\"0\">Normal</option>");
            client.println("<option value=\"10\">SBUS WM Adr 0</option>");
            client.println("<option value=\"11\">SBUS WM Adr 1</option>");
            client.println("<option value=\"12\">SBUS WM Adr 2</option>");         
            client.println("<option value=\"13\">SBUS WM Adr 3</option>");    
            client.println("</select><br></p>");

            client.println("<script> function seteinkanalmode() { ");
            client.println("var sel = document.getElementById(\"EinkanalMode\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?EinkanalMode=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(einkanal_mode, DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"EinkanalMode\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>");

            client.println("<hr>");            
        
            // Save Button
            client.println("<p><a href=\"/save\"><button class=\"button button2\">Save</button></a></p>");



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

            client.println("<p class=\"text2\" ><b>Ausgang : " + String(Menu) + "</b></p>");
              
            client.println("<p><a href=\"/back\"><button class=\"button button3\">Back</button></a>");
            client.println("<a href=\"/next\"><button class=\"button button4\">Next</button></a></p>");

            client.println("<br />");
            client.println("<br />");
            client.println("<br />");
            client.println("<br />"); 

            client.println("<p class=\"text2\" >Quelle Ausgang</p>");

	          client.println("<p><select id=\"ausgangkanal\" class=\"buttonA\" onchange=\"setausgangkanal()\">");
            client.println("<optgroup label=\"Einzelkanal\">");
      	    client.println("<option value=\"0\">Einzelkanal 01</option>");
            client.println("<option value=\"1\">Einzelkanal 02</option>");
            client.println("<option value=\"2\">Einzelkanal 03</option>");
            client.println("<option value=\"3\">Einzelkanal 04</option>");
            client.println("<option value=\"4\">Einzelkanal 05</option>");
            client.println("<option value=\"5\">Einzelkanal 06</option>");
            client.println("<option value=\"6\">Einzelkanal 07</option>");
            client.println("<option value=\"7\">Einzelkanal 08</option>");
            client.println("</optgroup>");
            client.println("<optgroup label=\"Kanal_L\">");
      	    client.println("<option value=\"20\">Kanal_L 01</option>"); 
            client.println("<option value=\"21\">Kanal_L 02</option>"); 
            client.println("<option value=\"22\">Kanal_L 03</option>"); 
            client.println("<option value=\"23\">Kanal_L 04</option>"); 
            client.println("<option value=\"24\">Kanal_L 05</option>"); 
            client.println("<option value=\"25\">Kanal_L 06</option>"); 
            client.println("<option value=\"26\">Kanal_L 07</option>"); 
            client.println("<option value=\"27\">Kanal_L 08</option>"); 
            client.println("<option value=\"28\">Kanal_L 09</option>"); 
            client.println("<option value=\"29\">Kanal_L 10</option>"); 
            client.println("<option value=\"30\">Kanal_L 11</option>"); 
            client.println("<option value=\"31\">Kanal_L 12</option>"); 
            client.println("<option value=\"32\">Kanal_L 13</option>"); 
            client.println("<option value=\"33\">Kanal_L 14</option>"); 
            client.println("<option value=\"34\">Kanal_L 15</option>"); 
            client.println("<option value=\"35\">Kanal_L 16</option>"); 
            client.println("</optgroup>");
            client.println("<optgroup label=\"Kanal_H\">");
            client.println("<option value=\"40\">Kanal_H 01</option>"); 
            client.println("<option value=\"41\">Kanal_H 02</option>"); 
            client.println("<option value=\"42\">Kanal_H 03</option>"); 
            client.println("<option value=\"43\">Kanal_H 04</option>"); 
            client.println("<option value=\"44\">Kanal_H 05</option>"); 
            client.println("<option value=\"45\">Kanal_H 06</option>"); 
            client.println("<option value=\"46\">Kanal_H 07</option>"); 
            client.println("<option value=\"47\">Kanal_H 08</option>"); 
            client.println("<option value=\"48\">Kanal_H 09</option>"); 
            client.println("<option value=\"49\">Kanal_H 10</option>"); 
            client.println("<option value=\"50\">Kanal_L 11</option>"); 
            client.println("<option value=\"51\">Kanal_L 12</option>"); 
            client.println("<option value=\"52\">Kanal_L 13</option>"); 
            client.println("<option value=\"53\">Kanal_L 14</option>"); 
            client.println("<option value=\"54\">Kanal_L 15</option>"); 
            client.println("<option value=\"55\">Kanal_L 16</option>"); 
            client.println("</optgroup>");
            client.println("</select><br></p>");

            client.println("<script> function setausgangkanal() { ");
            client.println("var sel = document.getElementById(\"ausgangkanal\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?AusgangKanal=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(Ausgang_Kanal[Menu-1], DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"ausgangkanal\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>");   
             
            //client.println("<br />");
            //client.println("<br />");
            //client.println("<br />");
            
            // Funktion Ausgang 1

            client.println("<p class=\"text2\" >Funktion Ausgang</p>");

            client.println("<p><select id=\"ausgangkanalmode\" class=\"buttonA\" onchange=\"setausgangkanalmode()\">");
            client.println("<optgroup label=\"Funtion\">");
            client.println("<option value=\"0\">Dauerlicht</option>");
            client.println("<option value=\"1\">Blinken 0,05s an</option>");
            client.println("<option value=\"2\">Blinken 0,1s an</option>");
            client.println("<option value=\"5\">Blinken 0,25s an</option>");
            client.println("<option value=\"10\">Blinken 0,5s an</option>");
            client.println("<option value=\"15\">Blinken 0,75s an</option>");
            client.println("<option value=\"20\">Blinken 1s an</option>");
            client.println("<option value=\"40\">Blinken 2s an</option>");
            client.println("<option value=\"60\">Blinken 3s an</option>");
            client.println("<option value=\"80\">Blinken 4s an</option>");
            client.println("<option value=\"100\">Blinken 5s an</option>");
            client.println("<option value=\"120\">Blinken 6s an</option>");
            client.println("<option value=\"140\">Blinken 7s an</option>");
            client.println("<option value=\"160\">Blinken 8 an</option>");
            client.println("<option value=\"180\">Blinken 9s an</option>");
            client.println("<option value=\"200\">Blinken 10s an</option>");
            client.println("</optgroup>");
            client.println("</select><br></p>");
            
            client.println("<script> function setausgangkanalmode() { ");
            client.println("var sel = document.getElementById(\"ausgangkanalmode\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?AusgangKanalMODE=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String((mode[Menu-1] >> 8) & 0xFF, DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"ausgangkanalmode\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>"); 

            // Funktion Ausgang 2 nur beim blinken Anzeigen 2te Zeit

            if ((mode[Menu-1] & 0xFF00) > 0)
            {

            // client.println("<p class=\"text2\" >Zeit aus</p>");

            client.println("<p><select id=\"ausgangkanalmode2\" class=\"buttonA\" onchange=\"setausgangkanalmode2()\">");
            client.println("<optgroup label=\"Funtion\">");
            client.println("<option value=\"1\">Blinken 0,05s aus</option>");
            client.println("<option value=\"2\">Blinken 0,1s aus</option>");
            client.println("<option value=\"5\">Blinken 0,25s aus</option>");
            client.println("<option value=\"10\">Blinken 0,5s aus</option>");
            client.println("<option value=\"15\">Blinken 0,75s aus</option>");
            client.println("<option value=\"20\">Blinken 1s aus</option>");
            client.println("<option value=\"40\">Blinken 2s aus</option>");
            client.println("<option value=\"60\">Blinken 3s aus</option>");
            client.println("<option value=\"80\">Blinken 4s aus</option>");
            client.println("<option value=\"100\">Blinken 5s aus</option>");
            client.println("<option value=\"120\">Blinken 6s aus</option>");
            client.println("<option value=\"140\">Blinken 7s aus</option>");
            client.println("<option value=\"160\">Blinken 8 aus</option>");
            client.println("<option value=\"180\">Blinken 9s aus</option>");
            client.println("<option value=\"200\">Blinken 10s aus</option>");
            client.println("</optgroup>");
            client.println("</select><br></p>");
            
            client.println("<script> function setausgangkanalmode2() { ");
            client.println("var sel = document.getElementById(\"ausgangkanalmode2\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?AusgangKanalMODE2=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(mode[Menu-1] & 0xFF, DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"ausgangkanalmode2\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>");
            }

            client.println("<p class=\"text2\" >PWM Ausgang</p>");

            client.println("<p><select id=\"ausgangkanalpwm\" class=\"buttonA\" onchange=\"setausgangkanalpwm()\">");
            client.println("<optgroup label=\"Wert\">");
      	    client.println("<option value=\"255\">Wert PWM</option>");
            client.println("</optgroup>");
            client.println("<optgroup label=\"SBUS Kanal\">");
      	    client.println("<option value=\"300\">SBUS Kanal 01</option>"); 
            client.println("<option value=\"301\">SBUS Kanal 02</option>"); 
            client.println("<option value=\"302\">SBUS Kanal 03</option>"); 
            client.println("<option value=\"303\">SBUS Kanal 04</option>"); 
            client.println("<option value=\"304\">SBUS Kanal 05</option>"); 
            client.println("<option value=\"305\">SBUS Kanal 06</option>"); 
            client.println("<option value=\"306\">SBUS Kanal 07</option>"); 
            client.println("<option value=\"307\">SBUS Kanal 08</option>"); 
            client.println("<option value=\"308\">SBUS Kanal 09</option>"); 
            client.println("<option value=\"309\">SBUS Kanal 10</option>"); 
            client.println("<option value=\"310\">SBUS Kanal 11</option>"); 
            client.println("<option value=\"311\">SBUS Kanal 12</option>"); 
            client.println("<option value=\"312\">SBUS Kanal 13</option>"); 
            client.println("<option value=\"313\">SBUS Kanal 14</option>"); 
            client.println("<option value=\"314\">SBUS Kanal 15</option>"); 
            client.println("<option value=\"315\">SBUS Kanal 16</option>"); 
            client.println("</optgroup>");
            client.println("</select><br></p>");
            
            client.println("<script> function setausgangkanalpwm() { ");
            client.println("var sel = document.getElementById(\"ausgangkanalpwm\");");
            client.println("var opt = sel.options[sel.selectedIndex];");
            client.println("var val = opt.value;");
            client.println("var xhr = new XMLHttpRequest();");
            client.println("xhr.open('GET', \"/?AusgangKanalPWM=\" + val + \"&\", true);");
            client.println("xhr.send(); } </script>");

            valueString = String(pwm_wert[Menu-1], DEC);

            client.println("<script> ");
            client.println("var selectedOption =" + valueString + ";");
            client.println("var selectElement = document.getElementById(\"ausgangkanalpwm\");");
            client.println("for (var i = 0; i < selectElement.options.length; i++) {");
            client.println("var option = selectElement.options[i];");
            client.println("if (option.value == selectedOption) {");
            client.println("option.selected = true;");
            client.println("break; }  } </script>"); 


            if (pwm_wert[Menu-1] < 300)

            {
              valueString = String(pwm_wert[Menu-1], DEC);

              client.println("<p><h3>PWM Wert : <span id=\"textSliderValue\">" + valueString + "</span>");
            
              client.println("<input type=\"range\" min=\"0\" max=\"255\" step=\"5\" class=\"slider\" id=\"PWMSlider\" onchange=\"PWMSpeed(this.value)\" value=\"" + valueString + "\" /></p>");

              client.println("<script> function PWMSpeed(pos) { ");
              client.println("var sliderValue = document.getElementById(\"PWMSlider\").value;");
              client.println("document.getElementById(\"textSliderValue\").innerHTML = sliderValue;");
              client.println("var xhr = new XMLHttpRequest();");
              client.println("xhr.open('GET', \"/?AusgangKanalPWM=\" + pos + \"&\", true);");
              client.println("xhr.send(); } </script>");
            }       

            client.println("<hr>"); 
            
            // Save Button
            client.println("<p><a href=\"/save\"><button class=\"button button2\">Save</button></a></p>");
              
            break;

              case 9:
              
            client.println("<p class=\"text2\" ><b>Debug Info</b></p>");  
            
            client.println("<p><a href=\"/back\"><button class=\"button button3\">Back</button></a>");
            client.println("<a href=\"/next\"><button class=\"button button4\">Next</button></a></p>");
            
            client.println("<br />");
            client.println("<br />");
            
            // SBUS Kanal 1 bis 8 Ausgeben
            client.println("<div class=\"left\" >");
            valueString = String(sbus_data.ch[0], DEC);
            client.println("<p><br> SUBS K1: " + valueString);        
            valueString = String(sbus_data.ch[1], DEC);
            client.println("<br> SUBS K2: " + valueString);
            valueString = String(sbus_data.ch[2], DEC);
            client.println("<br> SUBS K3: " + valueString);
            valueString = String(sbus_data.ch[3], DEC);
            client.println("<br> SUBS K4: " + valueString);
            valueString = String(sbus_data.ch[4], DEC);
            client.println("<br> SUBS K5: " + valueString);
            valueString = String(sbus_data.ch[5], DEC);
            client.println("<br> SUBS K6: " + valueString);
            valueString = String(sbus_data.ch[6], DEC);
            client.println("<br> SUBS K7: " + valueString);
            valueString = String(sbus_data.ch[7], DEC);
            client.println("<br> SUBS K8: " + valueString);
            valueString = String(sbus_data.ch[8], DEC);
            client.println("<br> SUBS K9: " + valueString);
            valueString = String(sbus_data.ch[9], DEC);
            client.println("<br> SUBS K10: " + valueString);
            valueString = String(sbus_data.ch[10], DEC);
            client.println("<br> SUBS K11: " + valueString);
            valueString = String(sbus_data.ch[11], DEC);
            client.println("<br> SUBS K12: " + valueString);
            valueString = String(sbus_data.ch[12], DEC);
            client.println("<br> SUBS K13: " + valueString);
            valueString = String(sbus_data.ch[13], DEC);
            client.println("<br> SUBS K14: " + valueString);
            valueString = String(sbus_data.ch[14], DEC);
            client.println("<br> SUBS K15: " + valueString);
            valueString = String(sbus_data.ch[15], DEC);
            client.println("<br> SUBS K16: " + valueString + "</p> <hr>");

            // Konfig  Ausgeben
            valueString = String(SBUS_Channel, DEC);
            client.println("<br> Konfig SBUS Channel: " + valueString); 
            valueString = String(RC_System, DEC);
            client.println("<br> Konfig RC-System: " + valueString);
            valueString = String(einkanal_mode, DEC);
            client.println("<br> Konfig Einkanal Mode: " + valueString);
            valueString = String(Ausgang_Kanal[0], DEC);
            client.println("<br> Konfig Ausgang 1: " + valueString);        
            valueString = String(Ausgang_Kanal[1], DEC);
            client.println("<br> Konfig Ausgang 2: " + valueString);
            valueString = String(Ausgang_Kanal[2], DEC);
            client.println("<br> Konfig Ausgang 3: " + valueString);
            valueString = String(Ausgang_Kanal[3], DEC);
            client.println("<br> Konfig Ausgang 4: " + valueString);
            valueString = String(Ausgang_Kanal[4], DEC);
            client.println("<br> Konfig Ausgang 5: " + valueString);
            valueString = String(Ausgang_Kanal[5], DEC);
            client.println("<br> Konfig Ausgang 6: " + valueString);
            valueString = String(Ausgang_Kanal[6], DEC);
            client.println("<br> Konfig Ausgang 7: " + valueString);
            valueString = String(Ausgang_Kanal[7], DEC);
            client.println("<br> Konfig Ausgang 8: " + valueString);

            valueString = String(mode[0], DEC);
            client.println("<br> Mode Ausgang 1: " + valueString);
            valueString = String(mode[1], DEC);
            client.println("<br> Mode Ausgang 2: " + valueString);
            valueString = String(mode[2], DEC);
            client.println("<br> Mode Ausgang 3: " + valueString);
            valueString = String(mode[3], DEC);
            client.println("<br> Mode Ausgang 4: " + valueString);
            valueString = String(mode[4], DEC);
            client.println("<br> Mode Ausgang 5: " + valueString);
            valueString = String(mode[5], DEC);
            client.println("<br> Mode Ausgang 6: " + valueString);
            valueString = String(mode[6], DEC);
            client.println("<br> Mode Ausgang 7: " + valueString);
            valueString = String(mode[7], DEC);
            client.println("<br> Mode Ausgang 8: " + valueString);
          
            valueString = String(pwm_wert[0], DEC);
            client.println("<br> Konfig Ausgang PWM 1: " + valueString);        
            valueString = String(pwm_wert[1], DEC);
            client.println("<br> Konfig Ausgang PWM 2: " + valueString);
            valueString = String(pwm_wert[2], DEC);
            client.println("<br> Konfig Ausgang PWM 3: " + valueString);
            valueString = String(pwm_wert[3], DEC);
            client.println("<br> Konfig Ausgang PWM 4: " + valueString);
            valueString = String(pwm_wert[4], DEC);
            client.println("<br> Konfig Ausgang PWM 5: " + valueString);
            valueString = String(pwm_wert[5], DEC);
            client.println("<br> Konfig Ausgang PWM 6: " + valueString);
            valueString = String(pwm_wert[6], DEC);
            client.println("<br> Konfig Ausgang PWM 7: " + valueString);
            valueString = String(pwm_wert[7], DEC);
            client.println("<br> Konfig Ausgang PWM 8: " + valueString + "</p> </div>");
          
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
