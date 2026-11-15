#ifndef CONFIG_H
#define CONFIG_H

#include <EEPROM.h>

// Struktur für alle konfigurierbaren Parameter
struct ConfigData {
uint16_t version;        // Versionskennung
int RC_System;           // Kennung des verwendeten RC-Systems (z. B. Futaba, FrSky, etc.)
int SBUS_Channel;        // SBUS-Kanalnummer
int einkanal_mode;       // Mode für Einkanalbetrieb
int modul_adress;        // Adresse des Moduls für WM-Modus über CRSF
int Ausgang_Kanal[8];    // Einschaltfunktion der Ausgänge
int pwm_wert[8];         // PWM-Werte für die einzelnen Ausgänge (Impulsbreite)
int mode[8];             // Betriebsmodus je Ausgang
};

// Globale Instanz der Konfiguration
extern ConfigData config;

// Funktionen zum Laden und Speichern
void loadConfig();
void saveConfig();
void Reset_all();

#endif
