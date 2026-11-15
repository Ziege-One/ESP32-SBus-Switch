#include "config.h"

// Globale Instanz
ConfigData config;

 // Software Version
extern uint16_t Version; 

// ======== Werkseinstellungen  =======================================
void Reset_all() {
  config.RC_System = 0;  
  config.SBUS_Channel = 4;
  config.einkanal_mode = 0;
  config.modul_adress = 0;
  config.Ausgang_Kanal[0] = 0;
  config.Ausgang_Kanal[1] = 1;
  config.Ausgang_Kanal[2] = 2;
  config.Ausgang_Kanal[3] = 3;
  config.Ausgang_Kanal[4] = 4;
  config.Ausgang_Kanal[5] = 5;
  config.Ausgang_Kanal[6] = 6;
  config.Ausgang_Kanal[7] = 7;
  config.pwm_wert[0] = 255;
  config.pwm_wert[1] = 255;
  config.pwm_wert[2] = 255;
  config.pwm_wert[3] = 255;
  config.pwm_wert[4] = 255;
  config.pwm_wert[5] = 255;
  config.pwm_wert[6] = 255;
  config.pwm_wert[7] = 255;
  config.mode[0] = 0;
  config.mode[1] = 0;
  config.mode[2] = 0;
  config.mode[3] = 0;
  config.mode[4] = 0;
  config.mode[5] = 0;
  config.mode[6] = 0;
  config.mode[7] = 0;
}

void loadConfig() {
  EEPROM.begin(sizeof(ConfigData));
  EEPROM.get(0, config);

  if (config.version != Version) {
    Serial.println("EEPROM-Version passt nicht – Werkseinstellungen werden geladen.");
    Reset_all();
  }

  EEPROM.end();
}

void saveConfig() {
  config.version = Version;
  EEPROM.begin(sizeof(ConfigData));
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}