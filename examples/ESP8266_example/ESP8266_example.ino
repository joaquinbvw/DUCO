//////////////////////////////////////////////////////////
//  _____        _                    _____      _
// |  __ \      (_)                  / ____|    (_)
// | |  | |_   _ _ _ __   ___ ______| |     ___  _ _ __
// | |  | | | | | | '_ \ / _ \______| |    / _ \| | '_ \ 
// | |__| | |_| | | | | | (_) |     | |___| (_) | | | | |
// |_____/ \__,_|_|_| |_|\___/       \_____\___/|_|_| |_|
//  Code for ESP8266 boards - V2.53
//  © Duino-Coin Community 2019-2021
//  Distributed under MIT License
//////////////////////////////////////////////////////////
//  https://github.com/revoxhere/duino-coin - GitHub
//  https://duinocoin.com - Official Website
//  https://discord.gg/k48Ht5y - Discord
//  https://github.com/revoxhere - @revox
//  https://github.com/JoyBed - @JoyBed
//  https://github.com/kickshawprogrammer - @kickshawprogrammer
//////////////////////////////////////////////////////////
//  If you don't know what to do, visit official website
//  and navigate to Getting Started page. Happy mining!
//////////////////////////////////////////////////////////

#include <ESP8266WiFi.h>
#define WiFi_h
#define WIFI_H
#include <ESP8266mDNS.h> // OTA libraries
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//////////////////////////////////////////////////////////
// NOTE: If during compilation, the below line causes a
// "fatal error: Crypto.h: No such file or directory"
// message to occur; it means that you do NOT have the
// latest version of the ESP8266/Arduino Core library.
//
// To install/upgrade it, go to the below link and
// follow the instructions of the readme file:
//
//       https://github.com/esp8266/Arduino
//////////////////////////////////////////////////////////

#include <Ticker.h>

#include <DUCO.h>

const char* SSID          = "Auditorio";   // Change this to your WiFi name
const char* PASSWORD      = "philadelphia";    // Change this to your WiFi password
const char* USERNAME      = "joaquinbvw";     // Change this to your Duino-Coin username
const char* RIG_IDENTIFIER = "ESP8266";       // Change this if you want a custom miner name

WiFiClient client;

// Loop WDT... please don't feed me...
// See lwdtcb() and lwdtFeed() below
Ticker lwdTimer;
#define LWD_TIMEOUT   60000

#define LED_BUILTIN 2

unsigned long lwdCurrentMillis = 0;
unsigned long lwdTimeOutMillis = LWD_TIMEOUT;
volatile bool OTA_status = false;

DUCO duco = DUCO(LED_BUILTIN,USERNAME,RIG_IDENTIFIER);

void SetupWifi() {
  Serial.println("Connecting to: " + String(SSID));
  WiFi.mode(WIFI_STA); // Setup ESP in client mode
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.begin(SSID, PASSWORD);

  int wait_passes = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++wait_passes >= 10) {
      WiFi.begin(SSID, PASSWORD);
      wait_passes = 0;
    }
  }

  Serial.println("\nConnected to WiFi!");
  Serial.println("Local IP address: " + WiFi.localIP().toString());
}

void SetupOTA() {
  // Prepare OTA handler
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    OTA_status = true;
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    OTA_status = true;
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    OTA_status = false;
  });

  ArduinoOTA.setHostname(RIG_IDENTIFIER); // Give port a name not just address
  ArduinoOTA.begin();
}

void RestartESP(String msg) {
  Serial.println(msg);
  Serial.println("Resetting ESP...");
  duco.blink(BLINK_RESET_DEVICE);
  ESP.reset();
}

// Our new WDT to help prevent freezes
// code concept taken from https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs2_en.html
void ICACHE_RAM_ATTR lwdtcb(void)
{
  if ((millis() - lwdCurrentMillis > LWD_TIMEOUT) || (lwdTimeOutMillis - lwdCurrentMillis != LWD_TIMEOUT))
    RestartESP("Loop WDT Failed!");
}

void lwdtFeed(void) {
  lwdCurrentMillis = millis();
  lwdTimeOutMillis = lwdCurrentMillis + LWD_TIMEOUT;
}

void VerifyWifi() {
  while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0, 0, 0, 0))
    WiFi.reconnect();
}

void handleSystemEvents(void) {
  VerifyWifi();
  ArduinoOTA.handle();
  yield();
  lwdtFeed();
}

bool max_micros_elapsed(unsigned long current, unsigned long max_elapsed) {
  static unsigned long _start = 0;

  if ((current - _start) > max_elapsed) {
    _start = current;
    return true;
  }
  return false;
}

void setup() {
  // Start serial connection
  Serial.begin(500000);
  Serial.println("\nDuino-Coin ESP8266 Miner v" + duco.get_libver());

  SetupWifi();
  OTA_status = false;
  SetupOTA();

  lwdtFeed();
  lwdTimer.attach_ms(LWD_TIMEOUT, lwdtcb);

  // Sucessfull connection with wifi network
  duco.blink(BLINK_SETUP_COMPLETE);
  duco.get_DUCOID();
  Serial.println("Difficulty tier is "+duco.get_difftier());
}

void loop() {
  
  handleSystemEvents();
  uint8_t request_counter = 0;
  const uint8_t request_limit = 5;
  uint8_t task_num = 0; 
  Serial.println("\nTASK" + String(task_num) + " Connecting to Duino-Coin server...");
  if (!duco.server_con(client)) {
    Serial.println("TASK" + String(task_num) + " Connection failed");
	delay(15000);
    return;
  }
  Serial.println("TASK" + String(task_num) + " Succesfully connected to " + duco.get_host() + ":" + String(duco.get_port()) + ". Server version: " + duco.get_server_ver());
  while (client.connected()) {
    handleSystemEvents();
    Serial.println("TASK" + String(task_num) + " Asking for a new job for user: " + String(USERNAME));
    if(!duco.request(client)) {
      request_counter++;
      Serial.println("TASK" + String(task_num) + " Job request failed. Request #"+String(request_counter)+".");
      Serial.println("TASK" + String(task_num) + " Last message: \""+duco.get_lastmsg()+"\"");
      if(request_counter>=request_limit) {
        duco.change_port();
        break;
      }
      continue;
    }
    handleSystemEvents();
    Serial.println("TASK" + String(task_num) + " Waiting for the job from the server.");
    if(!duco.recv_job(client)) {
      request_counter++;
      Serial.println("TASK" + String(task_num) + " Couldn't receive job from the server. Request #"+String(request_counter)+".");
      Serial.println("TASK" + String(task_num) + " Last message: \""+duco.get_lastmsg()+"\"");
      if(request_counter>=request_limit) {
        duco.change_port();
        break;
      }
      continue;
    }
    handleSystemEvents();
    Serial.println("TASK" + String(task_num) + " Job received: " + duco.get_lastblockhash() + " " + duco.get_newblockhash() + " " + String(duco.get_difficulty()));
    Serial.println("TASK" + String(task_num) + " Starting hash calculation.");
    duco.handle(client);
    handleSystemEvents();
    Serial.println("TASK" + String(task_num) + " Result calculated is " + String(duco.get_result()));
    Serial.println("TASK" + String(task_num) + " Posting result and waiting for feedback.");
    if(!duco.feedback(client)) {
      Serial.println("TASK" + String(task_num) + " Failed receiving feedback");
      Serial.println("TASK" + String(task_num) + " Last message: \""+duco.get_lastmsg()+"\"");
      continue;
    }
    request_counter = 0;
    handleSystemEvents();
    Serial.println("TASK" + String(task_num) + " " + duco.get_feedback() + " share #" + String(duco.get_shares()) + " (" + String(duco.get_result()) + ")" + " Hashrate: " + String(duco.get_hashrate()));
  }
  Serial.println("TASK" + String(task_num) + " Not connected. Restarting");
  client.flush();
  client.stop();
}