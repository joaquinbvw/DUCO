# DUCO

Fork for Duino-Coin Arduino library based on the Arduino sketches on the official repository. Main official repository: https://github.com/revoxhere/duino-coin
This library supports most Arduino boards. This repository has been created in order to test, fix and modify the library before making it official, I believe revox wants this to be official but if we want to include it on the main repository it will change the directory tree. Here is the list of architectures it supports:

- AVR based boards
- Teensy based boards
- SAMD based boards
- SAMD21 based boards
- STM32 boards
- AVRMega based boards
- Raspberry Pi Pico

Any board or architecture not mentioned on the list will not be supported and will not compile, it should show an error message on the Arduino IDE.

## For developers

I've tried to maintain the main code logic of the sketches and mix everything in one library, of course I've had to make some trade off decisions but they're not critical or important and they don't change the SHA1 calculation. Right now there are a lot of commentaries missing that I will try to put on the next few days but please feel free to include them if you think it is needed. The library contains a lot of preprocessor directives to switch between architectures and to separate the Serial devices (I believe I2C, SPI and most Stream classes derivatives are supported too but I haven't tested that) from the network devices (WiFi, Ethernet). To separate the two different ways of communication I had to handle the Serial functions as generic Stream objects, with this I can also use the same functions with the WiFi and Ethernet client classes as they're derivatives from the Stream class. Another thing I want to point out is that I didn't want to give the class complete control of the Stream objects, this is just a personal opinion but I think the user should be free to call the Stream object whenever he wants, not exclusively on DUCO operations. This can be changed in the future if someone shows me a nicer and cleaner way to implement it.

Here is a quick explanation of each function member:

## Global functions for all Stream devices

### DUCO(uint8_t led_pin = LED_BUILTIN, String ducouser = "", String rigname = "")

This is the constructor of the object, it has some predefined arguments in case the user doesn't need to include them, for instance the Serial miners don't need any the ducouser and the rigname.

### void get_DUCOID()

Just to get the unique ID of the microcontroller. This doesn't return the ID, it is just stored inside the object. Also this function needs to be called on the setup section of the sketch, it is actually called inside the constructor but for some reason a lot of architectures don't like that and they want to be called on the actual sketch. Maybe someone can fix that, I'm not sure if it's possible.

### void blink(uint8_t count, uint8_t led_pin = LED_BUILTIN)

I have included this function that is defined on the ESP8266 sketch because I like how it handles the blinks of the led, we can give the function how many times we want the led to blink. This function is called inside other functions of the class but for 

### uintDiff get_result()

This function just returns the last result calculated from a job. uintDiff is a data type that can be uint8_t for AVR 8-bit devices and uint16_t for the rest of architectures.

### uintDiff get_difficulty()

This function just returns the current difficulty obtained.

### String get_lastblockhash()

Returns the latest lastblockhash.

### String get_newblockhash()

Returns the latest newblockhash.

### String get_lastmsg()

Returns the last String object read by the class on any of its member functions.

### String get_difftier()

Returns the difficulty tier associated with the current architecture.

### String get_libver()

Returns the library version.

### uintDiff ducos1a()

Implements the SHA1 loop calculation. In order to make fair calculations of hashrate I have moved the initialization of the SHA1 algorithm to the member function recv_job.

### bool recv_job(Stream& duco_stream)

Receive the data necessary to calculate a job (lastblockhash, newblockhash and difficulty) and also implement the initialization of the SHA1 algorithm.

### void handle(Stream& duco_stream)

Handle the job calculation and send the result through the Stream object.

## Functions exclusive to network devices (WiFi and Ethernet)

### float get_hashrate()

Returns the hashrate of the last job calculated.

### uint32_t get_shares()

Returns the number of the shares or jobs calculated.

### String get_feedback()

Returns the string that the server sends as feedback after the device sends the result.

### String get_server_ver()

Returns the server version sent by the server.

### String get_host()

Returns the address or URL of the Duino-Coin server.

### void change_port()

It increments the index of the ports array so the object can use another port on the next connection.

### uint16_t get_port()

Returns the current port in use.

### bool server_con(WiFiClient& duco_stream) OR bool server_con(EthernetClient& duco_stream)

It tries to connect to the server and waits for the response containing the version of the server.

### bool request(WiFiClient& duco_stream) OR bool request(EthernetClient& duco_stream)

It makes a request for a new job but doesn't wait for the response in this function.

### bool feedback(WiFiClient& duco_stream) OR bool feedback(EthernetClient& duco_stream)

It waits for the feedback from the server. This needs to be called after the result of a job is sent to the server.
