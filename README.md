# Opta Linker

Arduino Findernet Opta All In One Industrial IoT gateway

[![Release](https://img.shields.io/github/v/release/JcDenis/OptaLinker?color=lightblue)](https://github.com/JcDenis/OptaLinker/releases)
[![Issues](https://img.shields.io/github/issues/JcDenis/OptaLinker)](https://github.com/JcDenis/OptaLinker/issues)
[![Requests](https://img.shields.io/github/issues-pr/JcDenis/OptaLinker)](https://github.com/JcDenis/OptaLinker/pulls)
[![License](https://img.shields.io/github/license/JcDenis/OptaLinker?color=white)](https://github.com/JcDenis/OptaLinker/blob/master/LICENSE)


## About

> Arduino library for the Arduino Opta series https://opta.findernet.com/fr/arduino

The goal of this library is to implement an easy to use MQTT/Modbus gateway and more for industrial equipments.

**Web page and config are inspired by "Remoto" (27-12-2024) by Alberto Perro under CERN-OHL-P license at https://github.com/albydnc/remoto**


## Features

* Support Opta RS485 AFX00001, Opta Wifi AFX00002, Opta Lite AFX00003 
* Support for digital expansion board Arduino Pro Opta Ext D16O8E AFX0000 and Arduino Pro Opta Ext D16O8S AFX00006
* Partition formating (first boot or on demand) with Wifi firmware update
* Configurable Ethernet with DHCP or static IP
* Configurable Wifi STA or AP and with DHCP or static IP
* Configurable bidirectionnal MQTT Client with password support
* Configurable Modbus RTU or TCP, Client or Server
* Abstract inputs and ouputs structure to expose many informations
* Inputs and outputs counter and timer
* RS485 helpers
* Serial monitor and commands
* Password protected Web server for visualization and configuration
* Persistent configuration storage in flash memory
* Watchdog
* Multithreading loop
* Lots of simple methods to deal with inputs/outputs/storage...
* Lite and fast loop to keep MQTT publishing of input change state under 50ms
* ...


## To do

* Support for OTA update
* Support for analog expansions boards
* Code documentation
* More examples


## USAGE

### Notes

As this library is for industrial usage, expansion numbering and input numbering and output numbering start at 0, 
not 1 as printed on Opta device. This means that the first input of the opta board is named as I0.0 in industry.


### Network

**Wifi AP mode**  
If Wifi SSID is not configured and Wifi is set as prefered network, the Wifi goes into Access Point mode. 
Default SSID is `optalinker` + device ID and password is `optalinker`.

**Wifi STA mode**  
If Wifi SSID and password are configured and Wifi is set as prefered network, the wifi goes into Standard mode.

**Ethernet mode**  
If Ethernet is set as prefered network, wifi is disbaled.


**DHCP**  
If DHCP mode is enabled in configuration, connection is tried to be established with dynamic IP, 
else configured static IP address is used. By default static IP is `192.168.1.231`.

Network settings are configured in setup process and can not be changed without a device reboot.


### Serial

This library display activity on serial port and also support several commands.  
These commands are not case sensitive.
 
* `print version`	: Send to serial monitor the OptaLinker library version
* `print config ` 	: Send to serial monitor the contents of device configuration backup file
* `print io`		: Send to serial monitor the contents of io backup file
* `print store`		: Send to serial monitor the list of flash memory stored file
* `print boot` 		: Send tto serial the number of time device reboot
* `print boot`    	: Send to serial monitor the number of loops per second
* `print ip`      	: Send to serial monitor the device IPv4 address
* `switch dhcp`    	: Switch ethernet DHCP mode in configuration
* `switch wifi`    	: Switch Wifi/Ethernet mode in configuration
* `print time`		: Send to serial monitor the device time
* `update time` 	: Query NTP server to update device local time
* `flash memory`  	: Create/format partitions and reboot
* `reset config` 	: Reset to default device configuration and reboot
* `reset io`	 	: Rest IO values to default and reboot
* `publish mqtt` 	: Publish to MQTT device inforamtions and IO states
* `reboot`  		: Reboot device

You should do a `REBOOT` after `SWITCH DHCP`, `SWITCH WIFI` actions to take effect.


### MQTT

This library has MQTT client that supports MQTT 3.3.1 and broker with password credentials.

MQTT broker IP, port, user, password, topic, delay can be configured through web server interface (recommanded) or define.h file.

Publishing inputs statistics topics:
* `<base_topic>/<device_id>/input/x/state` The digital state of the input
* `<base_topic>/<device_id>/input/x/voltage` The voltage of the input
* `<base_topic>/<device_id>/input/x/pulse` The total number of pulse of the input
* `<base_topic>/<device_id>/input/x/partialPulse` The partial number of pulse of the input (resetable)
* `<base_topic>/<device_id>/input/x/high` The total time (ms) of high level of the input
* `<base_topic>/<device_id>/input/x/partialHigh` The partial time (ms) of high level of the input (resetable)

Publishing outputs statistics topics:
* `<base_topic>/<device_id>/output/x/state` The digital state of the output
* `<base_topic>/<device_id>/output/x/voltage` The voltage of the output (not implemented)
* `<base_topic>/<device_id>/output/x/pulse` The total number of pulse of the output
* `<base_topic>/<device_id>/output/x/partialPulse` The partial number of pulse of the output (resetable)
* `<base_topic>/<device_id>/output/x/high` The total time (ms) of high level of the output
* `<base_topic>/<device_id>/output/x/partialHigh` The partial time (ms) of high level of the output (resetable)

Publishing device informations topics:
* `<base_topic>/<device_id>/device/type` The human readable device type (Opta Lite...)
* `<base_topic>/<device_id>/device/ip` The device IPv4 address
* `<base_topic>/<device_id>/device/revision` The device OptaLinker library version
* `<base_topic>/<device_id>/rs485` The RS485 received values

Command output state and device counters and device information topics:
* `<base_topic>/<device_id>/input/x/reset` To reset partial counters for an input (value doesn't matter)
* `<base_topic>/<device_id>/output/x/reset` To reset partial counters for an ouput (value doesn't matter)
* `<base_topic>/<device_id>/output/x/set` To set state of an output with `0` = OFF, `1` = ON
* `<base_topic>/<device_id>/device/get` to force device information publishing (value doesn't matter)

In notation, x mean Input or Output uniq ID, it is made of "Expansion number" and "Input Number" starting at 0 for main board. 
For example to set the output O1.3 of the device 98 (device 98, expansion 1, Output 3, the ouput print as 4 on the device) to 1 : `opta/98/output/103/set = 1`.

Input state can also be published on demand by sending an HTTP request to the `/publish` URL.


### Modbus

See dedicated [Modbus document](https://github.com/JcDenis/OptaLinker/blob/master/docs/modbusserver.md)


### Web server

This library provides a web interface for visualization and configuration through a web server with basic authentication.
Web server is available in both Ethernet and Wifi mode.

* Default static IP address for web server is `192.168.1.231`
* Default user and password for web interface are `admin`:`admin` and can be changed in configuration.

Available web server entrypoints are:
* `GET /` : HTML IO visualization page
* `POST /form` : send new json data to this URL to udpate configuration
* `GET /config` : json device configuration data
* `GET /data` : json device information data
* `GET /io` : json device IO data
* `GET /device` : HTML device configration page
* `GET /favicon.ico` : Icon for HTML pages
* `GET /publish` : Publish to MQTT device and inputs state

**Note:** All pages require basic authentication !


### LED

During boot:
* Fast blink Green : Waiting for user to press button to fully reset device
* Fast blink Green to Red : Device is going to reboot
* Fix Green and red : Device is doning a long task (connection...)

After boot:
* Fix Green and Red with no blue : Connecting networks (this freeze device)
* Fast blink Green to Red : Device is going to reboot
* Fast short blink Green and Red : heartbeat
* Slow blink Red : No network connection
* Slow blink Green : No MQTT connection (network connection is OK)
* Fix Green : MQTT connection OK

Wifi device:
* Fix Blue : Wifi in STA mode
* Slow blink Blue : Wifi in AP mode
* No Blue : Not in Wifi mode


### Button

Button pushed duration make different actions :

During boot:
* +5s : On fast blink green LED, user can reset device to default by pressing button more than 5 seconds

After boot:
* +5s : User can reset device to default by pressing button more than 5 seconds
* +1s : Without network, user can switch WIFI mode by pressing button more then 1 second
* -1s : Without network, user can change DHCP mode by pressing button less than 1 second
* -1s : With network and MQTT connection, user can force publishing input state to MQTT by pressing button less than 1 second

Note that actions take effect on button release. WIFI and DHCP actions reboot device.


### Watchdog

A watchog is present. If device does not respond until 30s, the device reboot.
(Maximum timeout of Opta board is 32270 milliseconds and cannot be stop.)


### USB

There is a bug on USB, there is no way to detect if cable is disconnected. 
And on cable disconnetion, board reboot after about 10 seconds.


## ARDUINO IDE

For Arduino Finder Opta on its M7 core.


### Boards manager

From Arduino IDE menu: _Tools > Boards > Boards Manager_, you must install: 

* `Arduino Mbed OS Opta Boards` by Arduino


### Library manager

From Arduino IDE menu: _Tools > Manage libraries_, you must install: 

* `ArduinoHttpClient` by Arduino at https://github.com/arduino-libraries/ArduinoHttpClient
* `ArduinoMqttClient` by Arduino at https://github.com/arduino-libraries/ArduinoMqttClient
* `Arduinojson` by Benoit Blanchon at https://github.com/bblanchon/ArduinoJson.git
* `ArduinoRS485` by Arduino at https://github.com/arduino-libraries/ArduinoRS485
* `ArduinoModbus` by Arduino at https://docs.arduino.cc/libraries/arduinomodbus/
* `Arduino_KVStore` by arduino at https://github.com/arduino-libraries/Arduino_KVStore
* `Arduino_Opta_Blueprint` by Daniele Aimo at https://github.com/arduino-libraries/Arduino_Opta_Blueprint
* `NTPClient` by Fabrice Weinberg at https://github.com/arduino-libraries/NTPClient


### Settings

* Tools > Boards > Arduino Mbed OS Opta Boards > Opta
* Tools > Security > None
* Tools > Flash split > 2Mb M7
* Tools > Target core > Main Core


### Install

* Copy folder `OptaLinker` to your Arduino IDE `libraries` folder, 
* Restart your Arduino IDE
* Select your Opta board and port
* In menu go to: _file > Examples > Examples from Custom Libraries > Opta Industrial IoT_ and select an example.
* Upload sketch to your Opta board. Enjoy.

To use OptaLinker in your sketch, see example `OptaLinkerAuto.ino`


## CONTRIBUTORS

* Alberto Perro (remoto author)
* Jean-Christian Paul Denis (OptaLinker author)

You are welcome to contribute to this code.


## LICENSE

CERN-OHL-P-2.0 license