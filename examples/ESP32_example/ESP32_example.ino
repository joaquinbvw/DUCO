//  Code for ESP32 boards v2.3
//  © Duino-Coin Community 2019-2021
//  Distributed under MIT License
//////////////////////////////////////////////////////////
//  https://github.com/revoxhere/duino-coin - GitHub
//  https://duinocoin.com - Official Website
//  https://discord.gg/k48Ht5y - Discord
//  https://github.com/revoxhere - @revox
//  https://github.com/JoyBed - @JoyBed
//////////////////////////////////////////////////////////
//  If you don't know what to do, visit official website
//  and navigate to Getting Started page. Happy mining!
//////////////////////////////////////////////////////////

#include <WiFi.h>
#define WiFi_h
#define WIFI_H
#include <esp_task_wdt.h>
#include <ESPmDNS.h>
#include "WiFiUdp.h"

const char* ssid     = "Auditorio";            // Change this to your WiFi SSID
const char* password = "philadelphia";        // Change this to your WiFi password
const char* ducouser = "joaquinbvw";  // Change this to your Duino-Coin username
const char* rigname  = "ESP32";                     // Change this if you want to display a custom rig name in the Wallet
#define LED_BUILTIN 33                           // Change this if your board has built-in led on non-standard pin (NodeMCU - 16 or 2)
#define WDT_TIMEOUT 60                              // Define watchdog timer seconds

//////////////////////////////////////////////////////////
//  If you're using the ESP32-CAM board or other board
//  that doesn't support OTA (Over-The-Air programming)
//  comment the ENABLE_OTA definition line
//  (#define ENABLE_OTA)
//////////////////////////////////////////////////////////

#define ENABLE_OTA

//////////////////////////////////////////////////////////
//  If you don't want to use the Serial interface uncomment
//  the DISABLE_SERIAL definition line (#define DISABLE_SERIAL)
//////////////////////////////////////////////////////////

//#define DISABLE_SERIAL

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

#include <DUCO.h>

DUCO duco[4] = {DUCO(LED_BUILTIN,ducouser,rigname), DUCO(LED_BUILTIN,ducouser,rigname), DUCO(LED_BUILTIN,ducouser,rigname), DUCO(LED_BUILTIN,ducouser,rigname)};

TaskHandle_t WiFirec;
TaskHandle_t Task1;
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
SemaphoreHandle_t xMutex;

volatile int wifiStatus = 0;
volatile int wifiPrev = WL_CONNECTED;
volatile bool OTA_status = false;

WiFiClient client[4];

void WiFireconnect( void * pvParameters ) {
  int n = 0;
  unsigned long previousMillis = 0;
  const long interval = 500;
  esp_task_wdt_add(NULL);
  for(;;) {
    wifiStatus = WiFi.status();
    
    #ifdef ENABLE_OTA
    ArduinoOTA.handle();
    #endif
    
    if (OTA_status)  // If the OTA is working then reset the watchdog.
      esp_task_wdt_reset();
    // check if WiFi status has changed.
    if ((wifiStatus == WL_CONNECTED)&&(wifiPrev != WL_CONNECTED)) {
      esp_task_wdt_reset(); // Reset watchdog timer
      Serial.println(F("\nConnected to WiFi!"));
      Serial.println("Local IP address: " + WiFi.localIP().toString());
      Serial.println();
    }
    else if ((wifiStatus != WL_CONNECTED)&&(wifiPrev == WL_CONNECTED)) {
      esp_task_wdt_reset(); // Reset watchdog timer
      Serial.println(F("\nWiFi disconnected!"));
      WiFi.disconnect();
      Serial.println(F("Scanning for WiFi networks"));
      n = WiFi.scanNetworks(false, true);
      Serial.println(F("Scan done"));
      if (n == 0) {
          Serial.println(F("No networks found. Resetting ESP32."));
          esp_restart();
      } else {
          Serial.print(n);
          Serial.println(F(" networks found"));
          for (int i = 0; i < n; ++i) {
              // Print SSID and RSSI for each network found
              Serial.print(i + 1);
              Serial.print(F(": "));
              Serial.print(WiFi.SSID(i));
              Serial.print(F(" ("));
              Serial.print(WiFi.RSSI(i));
              Serial.print(F(")"));
              Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
              delay(10);
          }
      }
      esp_task_wdt_reset(); // Reset watchdog timer
      Serial.println();
      Serial.println(F("Please, check if your WiFi network is on the list and check if it's strong enough (greater than -90)."));
      Serial.println("ESP32 will reset itself after "+String(WDT_TIMEOUT)+" seconds if can't connect to the network");
      Serial.print("Connecting to: " + String(ssid));
      WiFi.reconnect();
    }
    else if ((wifiStatus == WL_CONNECTED)&&(wifiPrev == WL_CONNECTED)) {
      esp_task_wdt_reset(); // Reset watchdog timer
      delay(1000);
    }
    else {
       // Don't reset watchdog timer. If the timer gets to the timeout then it will reset the MCU
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        Serial.print(F("."));
      }
    }
    wifiPrev = wifiStatus;
  }
}

void duco_loop (DUCO duco_object, WiFiClient duco_client, uint8_t task_num) {
  uint8_t request_counter = 0;
  const uint8_t request_limit = 5;
  for(;;) {
    esp_task_wdt_reset();
    request_counter = 0;
    if (OTA_status)  // If the OTA is working then reset the watchdog.
      esp_task_wdt_reset();
    while(wifiStatus != WL_CONNECTED){
      delay(1000);
      esp_task_wdt_reset();
    }
    Serial.println("\nTASK" + String(task_num) + " Connecting to Duino-Coin server...");
    if (!duco_object.server_con(duco_client)) {
      Serial.println("TASK" + String(task_num) + " Connection failed.");
      continue;
    }
    Serial.println("TASK" + String(task_num) + " Succesfully connected to " + duco_object.get_host() + ":" + String(duco_object.get_port()) + ". Server version: " + duco_object.get_server_ver());
    while (duco_client.connected()) {
      esp_task_wdt_reset();
        Serial.println("TASK" + String(task_num) + " Asking for a new job for user: " + String(ducouser));
        if(!duco_object.request(duco_client)) {
        request_counter++;
        Serial.println("TASK" + String(task_num) + " Job request failed. Request #"+String(request_counter)+".");
        Serial.println("TASK" + String(task_num) + " Last message: \""+duco_object.get_lastmsg()+"\"");
        if(request_counter>=request_limit) {
          duco_object.change_port();
          break;
        }
        continue;
      }
      esp_task_wdt_reset();
      Serial.println("TASK" + String(task_num) + " Waiting for the job from the server.");
      if(!duco_object.recv_job(duco_client)) {
        request_counter++;
        Serial.println("TASK" + String(task_num) + " Couldn't receive job from the server. Request #"+String(request_counter)+".");
        Serial.println("TASK" + String(task_num) + " Last message: \""+duco_object.get_lastmsg()+"\"");
        if(request_counter>=request_limit) {
          duco_object.change_port();
          break;
        }
        continue;
      }
      esp_task_wdt_reset();
      Serial.println("TASK" + String(task_num) + " Job received: " + duco_object.get_lastblockhash() + " " + duco_object.get_newblockhash() + " " + String(duco_object.get_difficulty()));
      Serial.println("TASK" + String(task_num) + " Starting hash calculation.");
      duco_object.handle(duco_client);
      esp_task_wdt_reset();
      Serial.println("TASK" + String(task_num) + " Result calculated is " + String(duco_object.get_result()));
      Serial.println("TASK" + String(task_num) + " Posting result and waiting for feedback.");
      if(!duco_object.feedback(duco_client)) {
        Serial.println("TASK" + String(task_num) + " Failed receiving feedback");
        Serial.println("TASK" + String(task_num) + " Last message: \""+duco_object.get_lastmsg()+"\"");
        continue;
      }
      request_counter = 0;
      esp_task_wdt_reset();
      Serial.println("TASK" + String(task_num) + " " + duco_object.get_feedback() + " share #" + String(duco_object.get_shares()) + " (" + String(duco_object.get_result()) + ")" + " Hashrate: " + String(duco_object.get_hashrate()));
    }
    Serial.println("TASK" + String(task_num) + " Not connected. Restarting");
    duco_client.flush();
    duco_client.stop();
  }
}

//Task1code
void Task1code( void * pvParameters ) {
  esp_task_wdt_add(NULL);
  const uint8_t thread_num = 0;
  duco[thread_num].get_DUCOID();
  duco_loop(duco[thread_num],client[thread_num],thread_num);
}

// Task2code
void Task2code( void * pvParameters ) {
  esp_task_wdt_add(NULL);
  const uint8_t thread_num = 1;
  duco[thread_num].get_DUCOID();
  duco_loop(duco[thread_num],client[thread_num],thread_num);
}

// Task3code
void Task3code( void * pvParameters ) {
  esp_task_wdt_add(NULL);
  const uint8_t thread_num = 2;
  duco[thread_num].get_DUCOID();
  duco_loop(duco[thread_num],client[thread_num],thread_num);
}

// Task4code
void Task4code( void * pvParameters ) {
  esp_task_wdt_add(NULL);
  const uint8_t thread_num = 3;
  duco[thread_num].get_DUCOID();
  duco_loop(duco[thread_num],client[thread_num],thread_num);
}

void setup() {
  //disableCore0WDT();
  //disableCore1WDT();
  Serial.begin(500000); // Start serial connection
  Serial.println("\n\nDuino-Coin ESP32 Miner v2.3");
  
  WiFi.mode(WIFI_STA); // Setup ESP in client mode
  btStop();
  WiFi.begin(ssid, password); // Connect to wifi
  
  OTA_status = false;
  
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);
  
  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");
  
  // No authentication by default
  // ArduinoOTA.setPassword("admin");
  
  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  #ifdef ENABLE_OTA
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
      OTA_status = true;
    })
    .onEnd([]() {
      Serial.println(F("\nEnd"));
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      OTA_status = true;
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
      else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
      else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
      else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
      else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
      OTA_status = false;
      esp_restart();
    });
  
  ArduinoOTA.begin();
  #endif
  
  esp_task_wdt_init(WDT_TIMEOUT, true); // Init Watchdog timer
  
  xMutex = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore(WiFireconnect, "WiFirec", 10000, NULL, 3, &WiFirec, 0); //create a task with priority 3 and executed on core 0
  delay(250);
  xTaskCreatePinnedToCore(Task1code, "Task1", 10000, NULL, 1, &Task1, 0); //create a task with priority 2 and executed on core 0
  delay(250);
  xTaskCreatePinnedToCore(Task2code, "Task2", 10000, NULL, 2, &Task2, 1); //create a task with priority 2 and executed on core 1
  delay(250);
  //xTaskCreatePinnedToCore(Task3code, "Task3", 5000, NULL, 1, &Task3, 0); //create a task with priority 2 and executed on core 0
  //delay(250);
  //xTaskCreatePinnedToCore(Task4code, "Task4", 5000, NULL, 2, &Task4, 1); //create a task with priority 2 and executed on core 1
  //delay(250);
}

void loop() {}