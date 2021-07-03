//  Code for Arduino boards v2.53
//  © Duino-Coin Community 2019-2021
//  Distributed under MIT License
//////////////////////////////////////////////////////////
//  https://github.com/revoxhere/duino-coin - GitHub
//  https://duinocoin.com - Official Website
//  https://discord.gg/k48Ht5y - Discord
//////////////////////////////////////////////////////////
//  If you don't know what to do, visit official website
//  and navigate to Getting Started page. Happy mining!
//////////////////////////////////////////////////////////

#ifndef DUCO_h
#define DUCO_h

#include "Arduino.h"

#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
#ifndef WiFi_h
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#endif
#define ENABLE_OTA
#ifdef DISABLE_SERIAL
#define Serial DummySerial
static class {
public:
    void begin(...) {}
    void connect(...) {}
    void connected(...) {}
    void available(...) {}
    void read(...) {}
    void readStringUntil(...) {}
    void write(...) {}
    void print(...) {}
    void println(...) {}
    void printf(...) {}
    void setTimeout(...) {}
    void flush(...) {}
} Serial;
#endif
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include "hwcrypto/sha.h" // Include hardware accelerated hashing library
#elif defined(ARDUINO_ARCH_ESP8266)
#ifndef HASH_H_
#define HASH_H_
void sha1(const uint8_t* data, uint32_t size, uint8_t hash[20]);
void sha1(const char* data, uint32_t size, uint8_t hash[20]);
void sha1(const String& data, uint8_t hash[20]);
String sha1(const uint8_t* data, uint32_t size);
String sha1(const char* data, uint32_t size);
String sha1(const String& data);
#endif
#include <Crypto.h>  // experimental SHA1 crypto library
using namespace experimental::crypto;
#endif

#include "stdint.h"
#include "sha1.h"
#include "uniqueID.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define BLINK_SHARE_FOUND    1
#define BLINK_SETUP_COMPLETE 2
#define BLINK_CLIENT_CONNECT 3
#define BLINK_RESET_DEVICE   5

#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
typedef uint16_t uintDiff;
#else
typedef uint32_t uintDiff;
#endif
const uint8_t job_maxsize = 104; // 40+40+20+3 is the maximum size of a job

#if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
const uint16_t legacy_port = 2811;
const uint16_t wallet_port = 2812;
const uint16_t pc_mining_port = 2813;
const uint16_t pc_mining_port2 = 2816;
const uint16_t avr_port = 2814;
const uint16_t esp_port = 2815;
const uint16_t esp8266_port = 2825;
const uint16_t esp32_port = 2820;
const uint16_t websocket_port = 15808;
const uint16_t rest_api_port = 80;
//const char * duco_host = "51.15.127.80"; // Static server IP
#endif

class DUCO
{
  public:
    DUCO(uint8_t led_pin = LED_BUILTIN, String ducouser = "", String rigname = "");
    void get_DUCOID();
    void blink(uint8_t count, uint8_t led_pin = LED_BUILTIN);
    uintDiff get_result();
    uintDiff get_difficulty();
    String get_lastblockhash();
    String get_newblockhash();
    String get_lastmsg();
    String get_difftier();
    String get_libver();
    uintDiff ducos1a();
    #if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
    float get_hashrate();
    uint32_t get_shares();
    String get_feedback();
    String get_server_ver();
    String get_host();
	void change_port();
	uint16_t get_port();
    #if defined(WIFI_H) || defined(WiFi_h)
    bool server_con(WiFiClient& duco_stream);
    #elif defined(ethernet_h_)
    bool server_con(EthernetClient& duco_stream);
    #endif
    #if defined(WIFI_H) || defined(WiFi_h)
    bool request(WiFiClient& duco_stream);
    #elif defined(ethernet_h_)
    bool request(EthernetClient& duco_stream);
    #endif
    #if defined(WIFI_H) || defined(WiFi_h)
    bool feedback(WiFiClient& duco_stream);
    #elif defined(ethernet_h_)
    bool feedback(EthernetClient& duco_stream);
    #endif
    #endif
    bool recv_job(Stream& duco_stream);
    void handle(Stream& duco_stream);
  private:
    uint8_t _led = LED_BUILTIN;
    String _lastblockhash = "";
    String _newblockhash = "";
    String _DUCOID = "";
	String _lastmsg = "";
    uintDiff _difficulty = 0;
    uintDiff _diff = 0;
    uintDiff _ducosresult = 0;
    uint8_t _job[job_maxsize];
    const char * _lib_ver = "2.3";
    #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
    uint8_t _hash_bytes[20];
	String _hash1 = "";
    #else
    uint8_t *_hash_bytes;
    #endif
    #if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
    uint32_t _shares = 0;
    String _SERVER_VER = "";
    String _ducouser = "";
    String _rigname = "";
    String _feedback = "";
    float _hashrate = 0.0;
    uint16_t _all_ports[10] = {esp8266_port,esp32_port,esp_port,avr_port,pc_mining_port,pc_mining_port2,legacy_port,wallet_port,websocket_port,rest_api_port};
    uint8_t _port_counter = 0;
	const char * _duco_host = "51.15.127.80"; // Static server IP
	const uint16_t _con_timeout = 30000; // connection timeout in milliseconds
    #endif
};

#endif
