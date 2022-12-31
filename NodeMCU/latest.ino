#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLLpFAPG45"
#define BLYNK_DEVICE_NAME "NodeMCU v3"
#define BLYNK_AUTH_TOKEN "cVMttXHXJjS-VDfUB_I1ZuKiLBjyIJQq"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
// #include <Wire.h> //General Stuff
#include <OLED.h>  //Display Driver lib
#include <SPI.h>
#include <MFRC522.h>
#include <ezTime.h>

#define ON true
#define OFF false

// PIN DEFINITIONS --------------------------------------------
  #define DISPLAY_SDA_PIN 4
  #define DISPLAY_SCL_PIN 5
  // #define BUZZER_PIN 7
  #define BUTTON_PIN 15
  // #define RGB_RED_PIN 8
  // #define RGB_GREEN_PIN 6

// DISPLAY ----------------------------------------------------
  #define DISPLAY_ADDRESS 0x3c
  #define DISPLAY_OFFSET 1
  #define DISPLAY_SLEEP_TIMER 10000

  OLED display(DISPLAY_SDA_PIN, DISPLAY_SCL_PIN, DISPLAY_ADDRESS, DISPLAY_OFFSET);

  void runDisplay();
  int displayIdleTimer = 0;
  int displayState = -1;  // s starts with 0

  void s_SplashScreen();                // 0
  void s_Connecting(int state);         // 0 connecting  1 connected 2 cannot connect
  void s_Menu_Main();                   // 2
  void s_Menu_Idle();                   // 3
  void s_Menu_DoorUnlocked();           // 4
  void s_Menu_DoorLocked();             // 5
  void s_Alarm_NoInternetConnection();  // 6
  void s_Alarm_ModuleDisconnect();
  void s_Alarm_IntruderDetected();
  void s_UnknownCardDetected();
  void s_KnownCardDetected();
  void s_CardDetectorDisabled();  // NEW
  void s_SomeoneAtTheDoor();      // NEW

// CONNECTION -------------------------------------------------
  #define RETRYCOUNT 60
  #define CONNECTING 0
  #define CONNECTED 1
  #define DISCONNECTED -1

  int connectionStatus = -1;

  char ssid[] = "Fiber";
  char password[] = "OFZ8cx&85L";

// SERIAL CONNECTION ------------------------------------------
  #define SERIAL_SPEED 9600
  #define LOCK_DOOR 1
  #define UNLOCK_DOOR 0
  #define DOOR_MODULE_DISCONNECT true
  #define DOOR_MODULE_CONNECTED false
  #define DOOR_UNLOCKED false
  #define DOOR_LOCKED true

  int missedPackets = 0;
  bool doorModuleConnection = false;
  bool doorLockStatus = false;

  byte c_SerialReceive();                 // Receives Data - IF NO DATA RECIEVED FOR 3 CYCLES MODULE IS DISCONNECTED (3sn) KEEP READING SERIAL
  void c_SerialSendCommand(int Command);  // Return Receive Status
  void serialDataTransfer();

// RFID -------------------------------------------------------
  #define KEY_DETECTED true
  #define RST_PIN 20  // RST-PIN for RC522 - RFID - SPI - Module GPIO15
  #define SS_PIN 2    // SDA-PIN for RC522 - RFID - SPI - Module GPIO2

  bool unknownCardDetected = false;

  bool RFID_CardReader();

  MFRC522 RFID(SS_PIN, RST_PIN);
  MFRC522::MIFARE_Key key;

  byte key1[4] = { 0xA3, 0x9B, 0xF5, 0xA1 };
  byte key2[4] = { 0x12, 0x97, 0xA0, 0x1A };

// STATS ------------------------------------------------------
  char buffer[32];

// BLYNK CONNECTION -------------------------------------------
  BlynkTimer generalTimer;
  BlynkTimer alarm;

  Timezone local;

  char auth[] = BLYNK_AUTH_TOKEN;
  int someoneAtTheDoor = 0;
  bool infoAlarm = false;
  bool alarmAlarm = false;
  bool allowAlarm = true;

  uint64_t utc_time;

  bool disableRFID = false;
  String btime = "";

  void updateChart();
  void doorControls();
  void info();
  void aalarm();

// FUNCTIONS --------------------------------------------------
//  -------------------------------------------------------- //

void setup() {
  Serial.begin(SERIAL_SPEED);

  SPI.begin();
  display.begin();
  RFID.PCD_Init();

  s_SplashScreen();
  delay(2000);
  display.clear();

  while (!Serial) {
  }

  WiFi.mode(WIFI_STA);

  s_Connecting(CONNECTING);
  Blynk.config(auth);
  Blynk.connectWiFi(ssid, password);
  Blynk.connect();
  display.clear();

  local.setLocation("Europe/Istanbul");

  s_Connecting(Blynk.connected() == true ? CONNECTED : DISCONNECTED);
  connectionStatus = Blynk.connected() == true ? CONNECTED : DISCONNECTED;
  delay(2000);
  display.clear();

  if (connectionStatus == CONNECTED) {
    display.print(const_cast<char *>(WiFi.localIP().toString().c_str()), 4, 2);
    delay(1000);
    display.clear();
  }

  generalTimer.setInterval(1000L, runDisplay);
  generalTimer.setInterval(1000L, serialDataTransfer);
  generalTimer.setInterval(6000L, updateChart);
  generalTimer.setInterval(1000L, doorControls);
  generalTimer.setInterval(100L, RFID_CardReader);

  alarm.setInterval(1000L, info);
  alarm.setInterval(1000L, aalarm);
}

void loop() {
  Blynk.run();
  generalTimer.run();
  alarm.run(); 
}

// SCREEN FUNCS -----------------------------------------------
  void s_SplashScreen() {
    display.print("LutfU Orhun INAN", 2);
    display.print(" IOT Proje Odevi", 3);
    display.print("B201210397", 4, 3);

    display.print("Starting Up", 6, 3);
  }

  void s_Connecting(int state) {
    switch (state) {
      case 0:
        display.print("Baglaniyor", 2, 3);
        display.print("Lutfen Bekleyin", 5);
        break;
      case 1:
        display.print("Baglanti Kuruldu", 3);
        display.print(ssid, 5, 3);
        break;
      case -1:
        display.print("UYARI", 1, 6);
        display.print(ssid, 3);
        display.print("ile baglanti", 5, 3);
        display.print("KURULAMADI", 6, 5);
        break;
    }
  }

  void s_Menu_Main() {
    /*
          display.print("Internet", 0, 5);
          display.print("Baglantisi", 1, 3);
          display.print("Saglandi", 2, 4);

          String f = (doorLockStatus == DOOR_UNLOCKED) ? "Acik" : "Kapali";
          String String_Buffer = "Gun :: " + f;
          display.print(const_cast<char *>(String_Buffer.c_str()), 4);

          f = (doorLockStatus == DOOR_UNLOCKED) ? "Acik" : "Kapali";
          String_Buffer = "Kapi :: " + f;
          display.print(const_cast<char *>(String_Buffer.c_str()), 5);

          String a = deviceTime;
          String_Buffer = "Saat :: " + a;
          display.print(const_cast<char *>(String_Buffer.c_str()), 6);
          sprintf(buffer, "K.A.S :: %d", numberOfTimesDoorUnlocked);
          display.print(buffer, 7);
      */
  }

  void s_Menu_Idle() {
    display.print("IDLE", 3, 6);
  }

  void s_Menu_DoorUnlocked() {
    display.print("Kapi Acik", 3, 4);
  }

  void s_Menu_DoorLocked() {
    display.print("Kapi Kilitli", 3, 2);
  }

  void s_Alarm_NoConnection() {
    display.print("UYARI", 1, 6);
    display.print("Internet ", 4, 4);
    display.print("Baglantisi", 5, 4);
    display.print("Saglanamadi", 6, 3);
  }

  void s_Alarm_NoInternetConnection() {
    display.print("UYARI", 1, 6);
    display.print("Internet ", 3, 4);
    display.print("Baglantisi", 4, 3);
    display.print(ssid, 5, 3);
    display.print("ile saglanamadi", 7, 1);
  }

  void s_Alarm_ModuleDisconnect() {
    display.print("UYARI", 1, 6);
    display.print("Kapi Modulu", 3, 3);
    display.print("Baglantisi", 4, 4);
    display.print("Kesildi", 5, 5);
  }

  void s_Alarm_IntruderDetected() {
    display.print("DIKKAT", 0, 5);
    display.print("IZINSIZ GIRIS", 3, 1);
    display.print("Tespit Edildi", 4, 1);
    display.print("DIKKAT", 7, 5);
  }

  void s_UnknownCardDetected() {
    display.print("Bilinmeyen", 2, 3);
    display.print("Kart Okundu!", 5, 2);
  }

  void s_KnownCardDetected() {
    display.print("Kart Okundu", 2, 3);
    display.print("Kilit Aciliyor!", 5);
  }

  void s_CardDetectorDisabled() {
    display.print("UYARI", 1, 6);
    display.print("Kart Okuyucu", 3, 3);
    display.print("Blynk icinde", 4, 4);
    display.print("Devre Disi", 5, 5);
    display.print("Birakildi", 6, 5);
  }

  void s_SomeoneAtTheDoor() {
    display.print("DIKKAT", 0, 5);
    display.print("Kapida", 3, 5);
    display.print("Birisi Var!", 4, 4);
    display.print("DIKKAT", 7, 5);
  }
// -------------------------------------------------------- //

bool RFID_CardReader() {

  if (!RFID.PICC_IsNewCardPresent()) {
    return false;
  } else if (!RFID.PICC_ReadCardSerial()) {
    return false;
  }

  if (disableRFID == true) {
    displayState = 11;
    return false;
  }

  for (int i = 0; i < 4; i++) {
    if (RFID.uid.uidByte[i] != key1[i] && RFID.uid.uidByte[i] != key2[i]) {
      Blynk.virtualWrite(V4, local.dateTime() + " > " + "Bilinmeyen Kart!");
      unknownCardDetected = true;
      displayState = 9;
      return true;
    }
    unknownCardDetected = false;
    doorLockStatus = !doorLockStatus;
    return true;
  }
}

void c_SerialSendCommand(int Command) {
  Serial.write(Command);
}

byte c_SerialReceive() {
  return Serial.read();
}

int oldState = 0;
void runDisplay() {
  if (oldState != displayState) {
    display.clear();
    oldState = displayState;
  }
  switch (displayState) {
    case 0:
      s_SplashScreen();  // 0
      break;
    case 1:
      break;
    case 2:
      s_Menu_Main();  // 2
      break;
    case 3:
      s_Menu_Idle();  // 3
      break;
    case 4:
      s_Menu_DoorUnlocked();  // 4
      break;
    case 5:
      s_Menu_DoorLocked();  // 5
      break;
    case 6:
      s_Alarm_NoInternetConnection();  // 6
      break;
    case 7:
      s_Alarm_ModuleDisconnect();
      break;
    case 8:
      s_Alarm_IntruderDetected();
      break;
    case 9:
      s_UnknownCardDetected();
      break;
    case 10:
      s_KnownCardDetected();
      break;
    case 11:
      s_CardDetectorDisabled();  // NEW
      break;
    case 12:
      s_SomeoneAtTheDoor();  // NEW
      break;
  }
}

byte oldData = 0x00;
void serialDataTransfer() {
  // 0xff = Inturder Detected
  // 0x0f = Malfunction
  // 0xf0 = OK
  // 0xaa = Someones at the door

  if (Serial.available() > 0) {
    byte data = c_SerialReceive();
    if (data == 0xff) {
      Blynk.virtualWrite(V4, local.dateTime() + " > " + "DİKKAT | Bilinmeyen Kişi Giriş Yaptı!");
      alarmAlarm = true;
      displayState = 8;
    } else if (data == 0x0f) {
      infoAlarm = true;
      displayState = 7;
    } else if (data == 0xf0) {
      alarmAlarm = false;
      displayState = 3;
    } else if (data == 0xaa) {
      Blynk.virtualWrite(V4, local.dateTime() + " > " + "UYARI | Kapıda Birisi Bulunmakta!");
      displayState = 12;
      infoAlarm = true;
      if (oldData != 0xaa) {
        someoneAtTheDoor++;
      }
    }
    missedPackets = 0;
    oldData = data;
  } else if (Serial.available() == 0) {
    missedPackets++;
  }

  if (missedPackets > 50) {
    Blynk.virtualWrite(V4, local.dateTime() + " > " + "UYARI | Kapı Modülü Bağlantısı Kesildi!");
    displayState = 7;
    alarmAlarm = true;
  }

}

void updateChart() {
  Blynk.virtualWrite(V5, someoneAtTheDoor);
  someoneAtTheDoor = 0;
}

bool oldDoorLockStatus = false;
void doorControls() {
  if (doorLockStatus != oldDoorLockStatus) {
    if (doorLockStatus == DOOR_LOCKED) {
      Blynk.virtualWrite(5, 0);
      Blynk.virtualWrite(V4, local.dateTime() + " > " + "BİLGİ | Kapı Kilitlendi!");
      infoAlarm = true;
      displayState = 5;
      c_SerialSendCommand(0x00);
    } else if (doorLockStatus == DOOR_UNLOCKED) {
      Blynk.virtualWrite(5, 1);
      Blynk.virtualWrite(V4, local.dateTime() + " > " + "BİLGİ | Kapı Açıldı!");
      infoAlarm = true;
      displayState = 4;
      c_SerialSendCommand(0x11);
    }
    oldDoorLockStatus = doorLockStatus;
  }
}

void info() {
  if (infoAlarm == true && allowAlarm == true) {
    tone1();
    infoAlarm = false;
  } else {
    notone();
  }
}

void aalarm() {
  if (alarmAlarm == true && allowAlarm == true) {
    Blynk.virtualWrite(V3,1);
    tone5();
  } else {
    Blynk.virtualWrite(V3,0);
    notone();
  }
}

void tone1() {
  tone(15, 2000);
  alarm.setTimeout(100, tone2);
}
void tone2() {
  tone(15, 3000);
  alarm.setTimeout(100, tone3);
}
void tone3() {
  tone(15, 4000);
  alarm.setTimeout(100, tone4);
}
void tone4() {
  tone(15, 5000);
  alarm.setTimeout(100, info);
}

void tone5() {
  tone(15, 50);
  alarm.setTimeout(500, tone6);
}
void tone6() {
  tone(15, 600);
  alarm.setTimeout(500, aalarm);
}

void notone() {
  noTone(15);
}

// BLYNK COMMANDS ---------------------------------------------
BLYNK_WRITE(V0)  // Door Control
{
  int pinValue = param.asInt();
  if (pinValue == LOCK_DOOR) {
    c_SerialSendCommand(0x00);
    while (Serial.available() == false) {
    }
    if (c_SerialReceive() == 0xf0) {
      Blynk.virtualWrite(1, 1);
    }
    doorLockStatus = DOOR_LOCKED;
  } else if (pinValue == UNLOCK_DOOR) {
    c_SerialSendCommand(0x11);
    while (Serial.available() == false) {
    }
    if (c_SerialReceive() == 0xf0) {
      Blynk.virtualWrite(1, 0);
    }
    doorLockStatus = DOOR_UNLOCKED;
  }
}

BLYNK_WRITE(V2) {
  int pinValue = param.asInt();
  allowAlarm = (pinValue == 1 ? true : false);
}

BLYNK_WRITE(V6) {
  int pinValue = param.asInt();
  disableRFID = pinValue == 1 ? true : false;
}

BLYNK_CONNECTED() {
  Blynk.sendInternal("utc", "time");
}

BLYNK_WRITE(InternalPinUTC) {
  utc_time = param[1].asLongLong();
  UTC.setTime(utc_time / 1000, utc_time % 1000);
}

/*
  if (RFID_CardReader() == KEY_DETECTED) {
    if (unknownCardDetected == true) {
      s_UnknownCardDetected();
      delay(2000);
      return;
    } else {
      s_KnownCardDetected();
      delay(2000);
      c_SerialSendCommand(UNLOCK_DOOR);
    }
    display.clear();
  }


  if (Serial.available() > 0){
  int incomingByte = 0;
  incomingByte = Serial.read();
  String String_Buffer = (incomingByte == 0x00 ? "0x00" : incomingByte == 0xff ? "0xff" : "Other?");
  display.print(const_cast<char *>(String_Buffer.c_str()), 3 );
  delay(2000);
  display.clear();

  Serial.write(0xff);
  while (Serial.available() == 0){}

  incomingByte = Serial.read();
  String_Buffer = (incomingByte == 0x00 ? "0x00" : incomingByte == 0xff ? "0xff" : "Other?");
  display.print(const_cast<char *>(String_Buffer.c_str()), 3 );
  delay(2000);
  display.clear();
}
*/