/*
   ESP32-SBus-Switch
   Ziege-One (Der RC-Modelbauer)
 
 ESP32

 /////Pin Belegung////
 GPIO 13: Wifi Pin
 GPIO 16: SBUS/CRSF in (RX)
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
ESP32 3.3.4
 */

/* Installierte Bibliotheken
Bolder Flight Systems SBUS  8.1.4
 */

#include "sbus.h"            // SBUS-Bibliothek für RC-Signale (z. B. Futaba/FrSky)
#include "crsf_esp32.h"      // CRSF-Protokoll (Crossfire) für serielle Übertragung
#include <WiFi.h>            // WLAN-Funktionalität für ESP32

#include "config.h"          // Eigene Konfigurationsdatei (Version, Pins, Optionen)
#include "WebServerManager.h"// Webseite 

// Versionsinformationen
uint16_t Version = 71;       // Versionsnummer (z. B. 101 = Version 1.01)
char versionString[6];       // Zeichenkette zur Anzeige der Version

// PWM-Konfiguration
const int freq = 5000;       // PWM-Frequenz in Hz
const int resolution = 8;    // PWM-Auflösung in Bit (0–255)

// Array für Ausgangszustände
bool Ausgang[8];             // Speichert den Zustand von 8 Ausgängen

// Pinbelegung für die Ausgänge
volatile unsigned char OutPin[8] = {18, 19, 21, 22, 23, 25, 26, 27}; // GPIO-Pins für die 8 Ausgänge

// SBUS-Empfänger
bfs::SbusRx sbus_rx(&Serial2, 16, 17, true); // SBUS auf Serial2, RX=16, TX=17, invertiertes Signal
bfs::SbusData sbus_data;                     // Datenstruktur für SBUS-Kanäle

// CRSF-Kommunikation
CRSF crsf;                                   // Instanz für CRSF-Protokoll (Crossfire)

// Multiswitch-Kommandos (CRSF Payload-Codes)
#define MWset   0x01   // Setzen eines einzelnen Schalters
#define MWprop  0x02   // Propeller-/Eigenschaftsbefehl
#define MWset4  0x07   // Setzen von 4 Schaltern gleichzeitig
#define MWset4m 0x09   // Setzen von 4 Schaltern mit Maske

// CRSF-Kommandobereich für Multiswitch
#define Multiswitch 0xA1   // Kennung für Multiswitch-Kommandos

// Variablen für Datenhaltung
uint16_t Data;             // Daten für die Ausgänge (z. B. Bitmasken)
long Data2;                // Zusatzdaten (Langformat)
float Data3;               // Zusatzdaten (Fließkommazahl)
uint16_t Data4 = 0;        // Speicher für Multiswitch-Werte
volatile int x = 0;        // Hilfsvariable, volatile für ISR-Sicherheit

// WLAN-Konfiguration
const char* ssid     = "ESP32-SBUS-Switch";  // WLAN-SSID
const char* password = "123456789";          // WLAN-Passwort

// Sonstige Variablen
int RC_System_boot;                          // Bootstatus des RC-Systems
volatile unsigned char WifiPin = 13;         // GPIO für WLAN-Status
volatile unsigned char LedPin  = 2;          // GPIO für LED (Onboard-LED)

// Zeitvariablen für Timeout- und LED-Steuerung
unsigned long currentTime2 = millis();       // zweite Zeitvariable
unsigned long previousTimeLED[8] = {0,0,0,0,0,0,0,0}; // Zeitmarken für jede LED/Ausgang

// ======== Setup  =======================================
void setup() {
  Serial.begin(115200);   // Startet die serielle Schnittstelle für Debug-Ausgaben mit 115200 Baud

  // Konfiguration aus persistentem Speicher laden (z. B. EEPROM oder Flash)
  Serial.println("Einstellung laden");
  loadConfig();

  // Prüfen, welches RC-System konfiguriert ist
  if (config.RC_System == 4) { // 4 = ELRS (CRSF-Protokoll)
    // CRSF-Schnittstelle initialisieren:
    // Übergabe: HardwareSerial-Instanz & RX/TX-Pins für das Crossfire-Modul
    crsf.init_crsf(&Serial1, 16, 17);
  } else {
    // Andernfalls SBUS-Kommunikation starten
    sbus_rx.Begin();
  } 

  // Pin-Modi setzen
  pinMode(WifiPin, INPUT_PULLUP); // WLAN-Pin als Eingang mit Pullup-Widerstand
  pinMode(LedPin, OUTPUT);        // LED-Pin als Ausgang

  // PWM-Kanäle für die Ausgänge konfigurieren
  ledcAttach(OutPin[0], freq, resolution);
  ledcAttach(OutPin[1], freq, resolution);
  ledcAttach(OutPin[2], freq, resolution);
  ledcAttach(OutPin[3], freq, resolution);
  ledcAttach(OutPin[4], freq, resolution);
  ledcAttach(OutPin[5], freq, resolution);
  ledcAttach(OutPin[6], freq, resolution);
  ledcAttach(OutPin[7], freq, resolution);

  // Prüfen, ob der WLAN-Pin LOW ist → dann Access Point starten
  if (!digitalRead(WifiPin))
  {
    digitalWrite(LedPin, HIGH);   // LED einschalten als Statusanzeige
    Serial.print("AP (Zugangspunkt) einstellen…");
    WebServerManager::begin(ssid, password); // SSID, Passwort
  }
  else
  {
    // Falls WLAN-Pin HIGH → kein Access Point, Normalmodus
    Serial.println("Kein AP-Normalmodus");  
  }
  
  // Bootstatus des RC-Systems speichern
  RC_System_boot = config.RC_System;

  // Versionsnummer als Zeichenkette formatieren (z. B. 1.01)
  sprintf(versionString, "%d.%02d", Version / 100, Version % 100);
}

// ======== Loop  =======================================
void loop() {

  // Prüfen, ob der WifiPin LOW ist → Setup-Modus aktiv
  if (!digitalRead(WifiPin))        // Setup Modus
  {     
    WebServerManager::Webpage();    // Webseite laden (z. B. Konfigurationsseite im AP-Modus)
  }
  
  // Prüfen, ob das RC-System beim Booten auf ELRS (CRSF) gesetzt wurde
  if (RC_System_boot == 4) { // ELRS (CRSF)
  
    // CRSF-Pakete vom Empfänger lesen (z. B. Link-Status, Telemetrie, Steuerbefehle)
    crsf.read_packets(0);  // Parameter 0 = keine Debug-Ausgabe
    
    // Prüfen, ob ein direkter Gerätebefehl empfangen wurde
    if (crsf.getDeviceCommandReplyPending())
    {
      // Rückmeldeflag zurücksetzen, damit der Befehl nicht mehrfach verarbeitet wird
      crsf.setDeviceCommandReplyPending(false);

      // Funktion zur Verarbeitung des CRSF-Kommandos aufrufen
      encodeFunctionCRSF();
    }
    Output(Data);

  } else {
    // SBUS-Modus (klassische RC-Signale)
    if (sbus_rx.Read()) {                 // Prüfen, ob neue SBUS-Daten vorliegen
      sbus_data = sbus_rx.data();         // SBUS-Datenstruktur aktualisieren
      
      if (sbus_data.failsafe) {           // Falls Failsafe aktiv → Ausgänge ausschalten
         Output(0);                       // Alle Ausgänge auf 0 setzen
      }
      else
      {
        // SBUS-Kanalwert auslesen und in interne Datenstruktur codieren
        encodeFunction(sbus_data.ch[config.SBUS_Channel]);      
        
        // Ausgangswerte entsprechend der codierten Daten setzen
        Output(Data);
      }
    }
  } 
}

// ======== encodeFunctionCRSF  =======================================
void encodeFunctionCRSF() {
  // Lese den CRSF-Befehl aus dem internen Buffer:
  // Byte 5 = Realm (z. B. 0xA1 für MultiSwitch)
  // Byte 6 = Command (z. B. 0x01, 0x07, 0x09, 0x02)
  uint8_t WMcode  = crsf.get_crfs_buffer(5);
  uint8_t command = crsf.get_crfs_buffer(6);
        
  // Prüfen, ob der Befehl zum Realm 0xA1 (Multiswitch) gehört
  if (WMcode == Multiswitch) {
    switch (command) {

        // ===== Fall 0x07: MWset4 =====
        // Setzt 8 Schalter mit jeweils 4 Zuständen
        // Payload: 1 Byte Adresse + 2 Byte Zustand
        case MWset4: {
            uint8_t address = crsf.get_crfs_buffer(7); // Switch-Adresse auslesen
            // Nur reagieren, wenn die Adresse mit der Moduladresse übereinstimmt
            if (address == config.modul_adress) {
              // Zustand aus zwei Bytes zusammensetzen (High + Low)
              uint16_t state = (crsf.get_crfs_buffer(8) << 8) + crsf.get_crfs_buffer(9);
              // Ausgabe entsprechend dem komprimierten Zustand setzen (PWM)
              Data = (compressSwitches(state));
            }  
            break; 
        }
                    
        // ===== Fall 0x09: MWset4m =====
        // Setzt mehrere 4-Zustands-Schalter mit unterschiedlichen Adressen
        // Payload: <count> × [ <address> <stateH> <stateL> ]
        case MWset4m: {
            int8_t count = crsf.get_crfs_buffer(7); // Anzahl der Gruppen
            uint8_t  address[count];                // Array für Adressen
            uint16_t state[count];                  // Array für Zustände

            // Schleife über alle Gruppen
            for(uint8_t i = 0; i < count; ++i) {
              // Adresse der Gruppe auslesen
              address[i] = crsf.get_crfs_buffer(8 + (3 * i));
              // Nur reagieren, wenn Adresse mit Moduladresse übereinstimmt
              if (address[i] == config.modul_adress) {
                // Zustand aus High- und Low-Byte zusammensetzen
                state[i] = (crsf.get_crfs_buffer(9 + (3 * i)) << 8) + crsf.get_crfs_buffer(10 + (3 * i));
                // Ausgabe entsprechend dem komprimierten Zustand setzen (PWM)
                Data = (compressSwitches(state[i]));
              }
            }
            break; 
        }
    }
  }  
}

// Funktion zur Komprimierung von 16-Bit Switch-Zuständen auf ein 8-Bit Ergebnis
// Jeder Schalter hat 4 mögliche Zustände (2 Bits), wir extrahieren jeweils nur das "aktiv"-Bit.
// Ergebnis: 8 Schalter → 8 Bits in einem Byte.
uint8_t compressSwitches(uint16_t state) {
    uint8_t result = 0;
    result |= ((state >> 0) & 0x1) << 0;    // Switch 1: Bits 0–1 → wir nehmen Bit 0 und setzen es auf Position 0 im Ergebnis
    result |= ((state >> 2) & 0x1) << 1;    // Switch 2: Bits 2–3 → wir nehmen Bit 2 und setzen es auf Position 1
    result |= ((state >> 4) & 0x1) << 2;    // Switch 3: Bits 4–5 → wir nehmen Bit 4 und setzen es auf Position 2
    result |= ((state >> 6) & 0x1) << 3;    // Switch 4: Bits 6–7 → wir nehmen Bit 6 und setzen es auf Position 3    
    result |= ((state >> 8) & 0x1) << 4;    // Switch 5: Bits 8–9 → wir nehmen Bit 8 und setzen es auf Position 4   
    result |= ((state >> 10) & 0x1) << 5;   // Switch 6: Bits 10–11 → wir nehmen Bit 10 und setzen es auf Position 5  
    result |= ((state >> 12) & 0x1) << 6;   // Switch 7: Bits 12–13 → wir nehmen Bit 12 und setzen es auf Position 6   
    result |= ((state >> 14) & 0x1) << 7;   // Switch 8: Bits 14–15 → wir nehmen Bit 14 und setzen es auf Position 7
    return result;   // Ergebnis zurückgeben: jedes Bit repräsentiert einen Schalter (0 = aus, 1 = an)
}

// ======== encodeFunction =======================================
// Wandelt einen RC-Kanalwert abhängig vom System und Modus in ein internes Datenformat um
void encodeFunction(uint16_t channel) {
    Data = channel;   // Ausgangswert übernehmen

    // -------- Mehrkanalbetrieb (Standard) --------
    if (config.einkanal_mode == 0) 
    { 
        // RC-System 0: einfache Skalierung (z. B. FrSky)
        if (config.RC_System == 0) {
            Data = Data / 8;   // Wertebereich reduzieren
            Data = Data;       // (keine weitere Änderung, Platzhalter)
        }

        // RC-System 1: FlySky – Wertebereich begrenzen und skalieren
        if (config.RC_System == 1) {    
            if (Data < 206) { Data = 206; }     // Untergrenze
            if (Data > 1837) { Data = 1837; }   // Obergrenze
            Data = Data - 206;                  // Offset entfernen
            Data = (Data * 10);                 // Skalierung
            Data = Data + 20;                   // kleiner Offset
            Data = Data / 8;                    // mehrfach teilen → Wertebereich anpassen
            Data = Data / 8;
        } 

        // RC-System 2: ELRS/SBUS – lineare Umrechnung
        if (config.RC_System == 2) {
            Data3 = Data - 172;                 // Offset entfernen
            Data3 = Data3 + 1.5;                // Korrekturwert
            Data3 = Data3 * 0.155677655677655;  // Skalierungsfaktor
            Data2 = Data3;                      // Zwischenspeicher
            Data = Data2;                       // Ergebnis übernehmen
        }
    }

    // -------- Einkanalbetrieb (Schaltfunktionen) --------
    if (config.einkanal_mode > 9) 
    {
        uint16_t n;   // Zwischenspeicher für berechneten Kanalwert
        uint8_t  v;   // komprimierter Wert für Schaltlogik

        // RC-System 0: FrSky
        if (config.RC_System == 0) {
            n = (channel >= 172) ? (channel - 172 + 1) : 0;  // Offset entfernen
            v = (n >> 4);                                    // Wertebereich komprimieren
        }

        // RC-System 1: FlySky
        if (config.RC_System == 1) {
            n = (channel >= 220) ? (channel - 220) : 0;      // Offset entfernen
            uint16_t n2 = n + (n >> 6);                      // zusätzliche Anpassung
            v = (n2 >> 4);                                   // komprimieren
        }

        // RC-System 2: ELRS (SBUS)
        if (config.RC_System == 2) {
            n = (channel >= 172) ? (channel - 172) : 0;      // Offset entfernen
            v = (n >> 4);                                    // komprimieren
        }    

        // RC-System 3: Hott
        if (config.RC_System == 3) {
            n = (channel >= 205) ? (channel - 205) : 0;      // Offset entfernen
            v = (n >> 4);                                    // komprimieren
        }    

        // Zerlegen des komprimierten Werts in Adress- und Schaltbits
        uint8_t address = (v >> 4) & 0b11;   // Moduladresse (2 Bit)
        uint8_t sw      = (v >> 1) & 0b111;  // Schalterkennung (3 Bit)
        uint8_t state   = v & 0b1;           // Schaltzustand (1 Bit)

        // Prüfen, ob die Adresse zum einkanal_mode passt
        if (address == (config.einkanal_mode - 10)) {
            bitWrite(Data4, sw, state);      // Schaltzustand ins Datenwort schreiben
        }

        Data = Data4;  // Ergebnis ins Haupt-Datenregister übernehmen
    }
}

// ======== Output =======================================
// Steuert die Ausgänge abhängig von Kanalzuordnung,
// SBUS-Daten und Konfiguration. Unterstützt Einzelkanal- und Schwellenlogik,
// sowie PWM-Ausgabe mit Dauerlicht oder Blinkfunktion.
void Output(uint16_t Data) {
  for (x = 0; x <= 7; x++)   // Schleife über alle 8 Ausgänge
  {
    // -------- Kanalzuordnung prüfen --------
    // Einzelkanal: direkte Bitabfrage aus Data
    if (config.Ausgang_Kanal[x] < 20) {
      if (bitRead(Data, (config.Ausgang_Kanal[x]))) {
        Ausgang[x] = true;   // Ausgang aktiv
      } else {
        Ausgang[x] = false;  // Ausgang inaktiv
      }
    }
    // Kanal L: Aktivierung bei niedrigen SBUS-Werten (< 800)
    else if (config.Ausgang_Kanal[x] < 40) {
      if (sbus_data.ch[config.Ausgang_Kanal[x] - 20] < 800) {
        Ausgang[x] = true;
      } else {
        Ausgang[x] = false;
      }
    }
    // Kanal H: Aktivierung bei hohen SBUS-Werten (> 1200)
    else if (config.Ausgang_Kanal[x] < 60) {
      if (sbus_data.ch[config.Ausgang_Kanal[x] - 40] > 1200) {
        Ausgang[x] = true;
      } else {
        Ausgang[x] = false;
      }
    }

    // -------- Ausgangssteuerung --------
    if (Ausgang[x]) {   // Wenn Ausgang aktiv
      // Blinkmodus (kein Dauerlicht)
      if ((config.mode[x] & 0xFF00) > 0) {
        currentTime2 = millis();   // aktuelle Zeit erfassen

        // LED ist aktuell aus
        if (ledcRead(OutPin[x]) <= 0) {
          // Prüfen, ob Einschaltintervall erreicht
          if (currentTime2 - previousTimeLED[x] >= 50 * (config.mode[x] & 0x00FF)) {
            previousTimeLED[x] = currentTime2;
            // PWM-Wert aus SBUS-Kanal oder statisch
            if (config.pwm_wert[x] >= 300) {
              ledcWrite(OutPin[x], map(sbus_data.ch[config.pwm_wert[x] - 300], 200, 1850, 0, 255));
            } else {
              ledcWrite(OutPin[x], config.pwm_wert[x]);
            }
          }
        }
        // LED ist aktuell an
        else {
          // Prüfen, ob Ausschaltintervall erreicht
          if (currentTime2 - previousTimeLED[x] >= 50 * ((config.mode[x] >> 8) & 0x00FF)) {
            previousTimeLED[x] = currentTime2;
            ledcWrite(OutPin[x], 0);   // LED ausschalten
          }
        }
      }
      // Dauerlicht-Modus
      else {
        if (config.pwm_wert[x] >= 300) {
          // PWM-Wert dynamisch aus SBUS-Kanal
          ledcWrite(OutPin[x], map(sbus_data.ch[config.pwm_wert[x] - 300], 200, 1850, 0, 255));
        } else {
          // Fester PWM-Wert
          ledcWrite(OutPin[x], config.pwm_wert[x]);
        }
      }
    }
    else {
      // Ausgang inaktiv → LED/Pin ausschalten
      ledcWrite(OutPin[x], 0);
    }
  }
}

// ======== Debug  =======================================
void Debug_out() {
  Serial.print("Einzelkanal");
  Serial.print(config.SBUS_Channel + 1); 
  Serial.print(" : "); 
  Serial.print(sbus_data.ch[config.SBUS_Channel]);
  Serial.print(" : Output ");
  Serial.print(Data);
  Serial.println(".");
}