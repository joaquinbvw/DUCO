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

#include "Arduino.h"
#include "DUCO.h"

#if defined(ARDUINO_ARCH_AVR)
const char * diff_tier = "AVR";
#elif defined(ARDUINO_ARCH_ESP8266)
const char * diff_tier = "ESP8266";
#elif defined(ARDUINO_ARCH_ESP32)
const char * diff_tier = "ESP32";
#elif defined(ARDUINO_ARCH_SAM)
const char * diff_tier = "DUE";
#elif defined(ARDUINO_ARCH_SAMD)
const char * diff_tier = "ARM";
#elif defined(ARDUINO_ARCH_STM32)
const char * diff_tier = "ARM";
#elif defined(TEENSYDUINO)
const char * diff_tier = "LOW";
#elif defined(ARDUINO_ARCH_MBED_RP2040)
const char * diff_tier = "ARM";
#elif defined(ARDUINO_ARCH_MEGAAVR)
const char * diff_tier = "MEGA";
#else
#error "DUCO library only supports AVR, SAM, SAMD, STM32, Teensy, megaAVR and ESP Architectures"
#endif

DUCO::DUCO(uint8_t led_pin, String ducouser, String rigname)
{
  #if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
  _ducouser = ducouser;
  _rigname = rigname;
  #endif
  _led = led_pin;
  get_DUCOID();
  pinMode(_led, OUTPUT);
}

void DUCO::get_DUCOID()
{
  _DUCOID = "DUCOID";
  char buff[4];
  for (uint8_t i = 0; i < UniqueIDsize; i++)
  {
    sprintf(buff, "%02X", (uint8_t) UniqueID8[i]);
    _DUCOID += buff;
  }
}

void DUCO::blink(uint8_t count, uint8_t led_pin) {
  uint8_t state = HIGH;

  for (int x = 0; x < (count << 1); ++x) {
    #if defined(ARDUINO_ARCH_AVR)
    PORTB ^= B00100000;
    #else
    digitalWrite(led_pin, state ^= HIGH);
    #endif
    delay(50);
  }
}

uintDiff DUCO::get_result()
{
  return _ducosresult;
}

uintDiff DUCO::get_difficulty()
{
  return _difficulty;
}

String DUCO::get_lastblockhash()
{
  return _lastblockhash;
}

String DUCO::get_newblockhash()
{
  return _newblockhash;
}

String DUCO::get_lastmsg()
{
  return _lastmsg;
}

String DUCO::get_difftier()
{
  return String(diff_tier);
}

String DUCO::get_libver()
{
  return String(_lib_ver);
}

uintDiff DUCO::ducos1a()
{
  // Difficulty loop
  #if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)
  // If the difficulty is too high for AVR architecture then return 0
  if (_difficulty > 655)
    return 0;
  #endif
  #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
  _hash1 = "";
  uint8_t payloadLength = 0;
  #endif
  for (uintDiff ducos1res = 0; ducos1res < _diff; ducos1res++)
  {
    #if defined(ARDUINO_ARCH_ESP32)
    _hash1 = _lastblockhash + String(ducos1res);
	payloadLength = _hash1.length();
    esp_sha(SHA1, (const unsigned char*)_hash1.c_str(), payloadLength, _hash_bytes);
    #elif defined(ARDUINO_ARCH_ESP8266)
	yield();
    _hash1 = _lastblockhash + String(ducos1res);
	payloadLength = _hash1.length();
    SHA1::hash((const unsigned char*)_hash1.c_str(), payloadLength, _hash_bytes);
    #else
    Sha1.init();
    Sha1.print(_lastblockhash + String(ducos1res));
    // Get SHA1 result
    _hash_bytes = Sha1.result();
    #endif
    if (memcmp(_hash_bytes, _job, 20) == 0)
    {
      blink(BLINK_SHARE_FOUND);
      // If expected hash is equal to the found hash, return the result
      return ducos1res;
    }
  }
  return 0;
}

bool DUCO::recv_job(Stream& duco_stream)
{
  _lastmsg = "";
  if (duco_stream.available() > 0) {
	_lastblockhash = "";
    _newblockhash = "";
    _difficulty = 0;
    _lastblockhash = duco_stream.readStringUntil(',');
	_lastmsg = _lastblockhash;
    if(!_lastblockhash)
      return false;
    // Read expected hash
    _newblockhash = duco_stream.readStringUntil(',');
	_lastmsg = _lastmsg + " " + _newblockhash;
    if(!_newblockhash)
      return false;
    // Read difficulty
    #if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
    _difficulty = strtoul(duco_stream.readStringUntil('\n').c_str(), NULL, 10);
    #else
    _difficulty = strtoul(duco_stream.readStringUntil(',').c_str(), NULL, 10);
    #endif
	_lastmsg = _lastmsg + " " + String(_difficulty);
    if(!_difficulty)
      return false;
    _diff = _difficulty * 100 + 1;
    // DUCO-S1 algorithm implementation for AVR boards (DUCO-S1A)
    _newblockhash.toUpperCase();
    const char *c = _newblockhash.c_str();
    uint8_t final_len = _newblockhash.length() >> 1;
    memset(_job, 0, job_maxsize);
    for (uint8_t i = 0, j = 0; j < final_len; i += 2, j++)
      _job[j] = ((((c[i] & 0x1F) + 9) % 25) << 4) + ((c[i + 1] & 0x1F) + 9) % 25;
    // Clearing the receive buffer before sending the result.
    while (duco_stream.available())
      duco_stream.read();
    return true;
  }
  else
    return false;
}

void DUCO::handle(Stream& duco_stream)
{
  // Start time measurement
  uint32_t startTime = micros();
  // Call DUCO-S1A hasher
  _ducosresult = ducos1a();
  // Calculate elapsed time
  uint32_t elapsedTime = micros() - startTime;
  // Clearing the receive buffer before sending the result.
  while (duco_stream.available())
    duco_stream.read();
  #if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
  float ElapsedTimeMiliSeconds = elapsedTime/1000;
  float ElapsedTimeSeconds = ElapsedTimeMiliSeconds/1000;
  _hashrate = _ducosresult/ElapsedTimeSeconds;
  // Send result to server
  duco_stream.print(String(_ducosresult) + "," + String(_hashrate) + ","+String(diff_tier)+" Miner v" + String(_lib_ver) + "," + String(_rigname) + "," + _DUCOID + "\n");   
  #else
  // Send result back to the program with share time
  duco_stream.print(String(_ducosresult) + "," + String(elapsedTime) + "," + _DUCOID + "\n");
  #endif
}

#if defined(WIFI_H) || defined(WiFi_h) || defined(ethernet_h_)
float DUCO::get_hashrate()
{
  return _hashrate;
}

uint32_t DUCO::get_shares()
{
  return _shares;
}

String DUCO::get_feedback()
{
  return _feedback;
}

String DUCO::get_server_ver()
{
  return _SERVER_VER;
}

String DUCO::get_host()
{
  return String(_duco_host);
}

void DUCO::change_port()
{
  _port_counter = (_port_counter+1)%(sizeof(_all_ports)/sizeof(uint16_t));
}

uint16_t DUCO::get_port()
{
  return _all_ports[_port_counter];
}
#if defined(WIFI_H) || defined(WiFi_h)
bool DUCO::server_con(WiFiClient& duco_stream)
#elif defined(ethernet_h_)
bool DUCO::server_con(EthernetClient& duco_stream)
#endif
{
  _lastmsg = "";
  _shares = 0; // Share variable
  //duco_stream.setTimeout(1);
  //duco_stream.flush();
  yield();
  if (!duco_stream.connected()) {
    // Trying to connect to the server
    uint16_t connect_start = millis();
    #if defined(ARDUINO_ARCH_ESP32)
    while (!duco_stream.connect(_duco_host, _all_ports[_port_counter], 30000000))
    #else
    while (!duco_stream.connect(_duco_host, _all_ports[_port_counter]))
    #endif
      if(duco_stream.connected())
        break;
      else if(millis()-connect_start>_con_timeout) {
        delay(500);
  	    _port_counter = (_port_counter+1)%(sizeof(_all_ports)/sizeof(uint16_t));
        return false;
	  }
  }	
  // Checking if there is data available
  while(!duco_stream.available()){
    yield();
    if (!duco_stream.connected())
      return false;
    delay(10);
  }
  _SERVER_VER = "";
  _SERVER_VER = duco_stream.readStringUntil('\n'); // Server sends SERVER_VERSION after connecting
  _lastmsg = _SERVER_VER;
  blink(BLINK_CLIENT_CONNECT);
  return true;
}

#if defined(WIFI_H) || defined(WiFi_h)
bool DUCO::request(WiFiClient& duco_stream)
#elif defined(ethernet_h_)
bool DUCO::request(EthernetClient& duco_stream)
#endif
{
  duco_stream.flush();
  duco_stream.print("JOB," + String(_ducouser) + "," + String(diff_tier)); // Ask for new job
  while(!duco_stream.available()){
    yield();
    if (!duco_stream.connected())
      return false;
    delay(10);
  }
  if (!duco_stream.connected())
    return false;
  delay(50);
  yield();
  return true;
}

#if defined(WIFI_H) || defined(WiFi_h)
bool DUCO::feedback(WiFiClient& duco_stream)
#elif defined(ethernet_h_)
bool DUCO::feedback(EthernetClient& duco_stream)
#endif
{
  _lastmsg = "";
  while(!duco_stream.available()){
    if (!duco_stream.connected()) {
      return false;
    }
    delay(10);
    yield();
  }
  delay(50);
  yield();
  _feedback = "";
  _feedback = duco_stream.readStringUntil('\n'); // Receive feedback
  _lastmsg = _feedback;
  if (!_feedback)
    return false;
  duco_stream.flush();
  _shares++;
  return true;
}
#endif
