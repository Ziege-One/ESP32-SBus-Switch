# ESP32-SBUS-Switch für RC Models

ESP32-SBUS-Switch das Projekt stammt von Voodoo-68 auf www.rc-network.de

https://www.rc-network.de/threads/sbus-switch.696022/

und der SBUS library :https://github.com/bolderflight/sbus

Webserver Beispiel :https://randomnerdtutorials.com/esp32-web-server-arduino-ide/

# Funktionen:
- 8 Schaltzustände über einen RC-Kanal übermitteln
- Einlesen des RC-Kanals über SBSUS
- 8 PWM Ausgänge (Werte können individuell eingestellt werden)
- Einstellen über Wi-Fi AP (SBUS-Kanal PWM Werte und Kompatibilität Mode)


# Kanalübertragung über SBSUS:
Funktioniert mit openTX oder edgeTX.
Der zu übertragende Kanal wird mithilfe von Mischern generiert.
Zum Erstellen der Mischer (Kurve) wird eine LUA Skript verwendet. (im Ordner LUA Tool SBUS-Switch V0.1.2)

# Beschreibung


# Schaltplan
Positive Schaltend IC1 = UDN2981A
![](https://github.com/Ziege-One/ESP32-SBus-Switch/blob/main/docs/ESP32_SBUS-Switch_+_sch.png?raw=true)
Negative Schaltend IC1 = ULN2803
![](https://github.com/Ziege-One/ESP32-SBus-Switch/blob/main/docs/ESP32_SBUS-Switch_-_sch.png?raw=true)

# Bilder
![](https://github.com/Ziege-One/ESP32-SBus-Switch/blob/main/docs/ESP32%20SBUS-Switch_F360.png?raw=true)

# Video
Teil 1
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/-PHlG2yzvQ4/0.jpg)](https://www.youtube.com/watch?v=-PHlG2yzvQ4)
Teil 2
[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/tFa6pBXM2Os/0.jpg)](https://www.youtube.com/watch?v=tFa6pBXM2Os)