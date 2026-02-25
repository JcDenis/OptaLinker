# Opta Linker

Arduino Findernet Opta All In One Industrial IoT gateway

[![Release](https://img.shields.io/github/v/release/JcDenis/OptaLinker?color=lightblue)](https://github.com/JcDenis/OptaLinker/releases)
[![Issues](https://img.shields.io/github/issues/JcDenis/OptaLinker)](https://github.com/JcDenis/OptaLinker/issues)
[![Requests](https://img.shields.io/github/issues-pr/JcDenis/OptaLinker)](https://github.com/JcDenis/OptaLinker/pulls)
[![License](https://img.shields.io/github/license/JcDenis/OptaLinker?color=white)](https://github.com/JcDenis/OptaLinker/blob/master/LICENSE)


## About

This document describe addresses of Modbus Server and how it works.


## Rules

* Holding Registers and Input Registers content same values.
* Holding Registers and Input Registers content device configuration and IO values.
* Changing values of Intputs and Outputs in Holding Registers has no effect.
* To command Ouputs states, use Coils.
* All passwords are NOT exposed in Holding Registers nor Input Registers.
* Expansion 0 is always main board. (The device)

### Type

Types of values of modbus registers for device as Modbus Server are limited to these ones :

* T1  : uint16  :  1 offset
* T2  : uint32  :  2 offsets  : Two consecutive 16-bits registers and compose them into a single 32-bits value, by shifting the first value left by 16 bits.
* T3  :  int16  :  2 offsets  : First offset is sign, second offset is absolute value as uint16.
* T4  : string  : 49 offsets  : First offset is length of the string, next offsets are the string char by char.
* T5  :   IPv4  :  4 offsets  : One offset per Ip bloc.

All strings must have a maximum length of 48 chars.


### Configuration

Some Holding Registers are configurable and some not:

* CH  : Configurable through Holding Registers.
* NC  : Not configurable.

* To have current used values, read Input Registers.
* To have ongoing configuration values, read Holding registers.
* To update configuration values, write Holding Registers.


## Holding Registers and Input Registers


### Groups

* 10000 ~ 10000 : T1 : NC : One Expansion structure length
* 10001 ~ 10001 : T1 : NC : One Input or Output structure length
* 10002 ~ 10002 : T1 : NC : Firmware start address
* 10003 ~ 10003 : T1 : NC : Device start address
* 10004 ~ 10004 : T1 : NC : Network start address
* 10005 ~ 10005 : T1 : NC : RS485 start address
* 10006 ~ 10006 : T1 : NC : Modbus start address
* 10007 ~ 10007 : T1 : NC : MQTT start address
* 10008 ~ 10008 : T1 : NC : Epansion start address
* 10009 ~ 10009 : T1 : NC : Input start address
* 10010 ~ 10010 : T1 : NC : Ouput start address

Structure length is used for each input, output, expansion definition. 
This means for example that definiton of expansion 0 input 1 starts at address of expansion 0 input 0 + structure length.

Values are group by usage  at astaring addresses:

* 10000 : List and addresses and length description
* 10020 : Firmware
* 10080 : Device
* 10190 : Network
* 10310 : RS485
* 10320 : Modbus
* 10330 : MQTT
* 10490 : Expansions
* 10820 : Inputs
* 11140 : Outpus

These groups starting addresses could change at any time on Firmware release.
Use Groups address to check up-to-date starting addresses and "Structure length" to jump from Expansion/IO to another.


### Firmware

* 10020 ~ 10020 : T1 : NC : Library version major
* 10021 ~ 10021 : T1 : NC : Library version minor
* 10022 ~ 10022 : T1 : NC : Library version revision
* 10023 ~ 10023 : T1 : NC : Configuration validation.
* 10024 ~ 10073 : T4 : NC : Configuration validation password. (this is the device password)

In order to save new configuration after updating Modbus Holding Registers, 
set Device Password in 10024 then set 10023 to 1. 
Modbus server update its configuration and reboot.


### Device

* 10080 ~ 10080 : T1 : CH : Device ID. Must be 0~254
* 10081 ~ 10082 : T3 : CH : Time Offset. First reg is offset sign. Second reg is offset. Must be -24~24
* 10083 ~ 10132 : T4 : CH : Device User Login.
* 10133 ~ 10182 : T4 : CH : Device User Password.

Device User Login and Password are not exposed in Registers, leave them empty to keep old values.


### Network

* 10190 ~ 10193 : T5 : CH : Network static IPv4 if DHCP is off.
* 10194 ~ 10197 : T5 : CH : Network gateway IPv4 if DHCP is off.
* 10198 ~ 10201 : T5 : CH : Network subnet IPv4 if DHCP is off.
* 10202 ~ 10205 : T5 : CH : Network DNS IPv4 if DHCP is off.
* 10206 ~ 10206 : T1 : CH : Network DHCP. 1 to use DHCP, 0 to use static IP.
* 10207 ~ 10208 : T1 : CH : Network WiFi. 1 to use Wifi, 0 to use Ethernet.
* 10208 ~ 10257 : T4 : CH : Network WiFi Standard mode SSID.
* 10258 ~ 10307 : T4 : CH : Network WiFi Standard mode Password.

If board does not support wifi, 10207 is down to 0.
If WiFI 10207 is on with no SSID 10208 or Password 10258, Wifi is in Access Point mode.
For Access point mode SSID is `optalinker` + device ID and password is `optalinker`.


### RS485

* 10310 ~ 10310 : T1 : CH : RS485 Type.
* 10311 ~ 10312 : T2 : CH : Bauderate. (should be a standard valid value)
* 10313 ~ 10313 : T1 : CH : Publish received message to MQTT. (only for 10400 RS485 type 1)

RS485 Type can be:
* 0 : Disable.
* 1 : Receiver.
* 2 : Sender.


### Modbus

* 10320 ~ 10320 : T1 : CH : Modbus Type.
* 10321 ~ 10324 : T5 : CH : Modbus distant TCP Server IPv4.
* 10325 ~ 10325 : T1 : CH : Modbus TCP port.

Modbus Type can be:
* 0 : Disable.
* 1 : Modbus RTU Server.
* 2 : Modbus TCP Server.
* 3 : Modbus RTU Client.
* 4 : Modbus TCP client.


### MQTT

* 10420 ~ 10423 : T5 : CH : MQTT Server Ipv4.
* 10424 ~ 10424 : T1 : CH : MQTT Server port.
* 10425 ~ 10426 : T2 : CH : Update interval in ms.
* 10427 ~ 10476 : T4 : CH : MQTT Server user.
* 10477 ~ 10526 : T4 : CH : MQTT Server password.
* 10527 ~ 10576 : T4 : CH : MQTT base topic.

Set MQTT Server IP to 0.0.0.0 to disable MQTT features.
Set update interval to 0 to disable automatique update. IO will be updated only on state change.


### Expansion

* 10620 ~ 10620 : T1 : NC : Expansion 0 exists.
* 10621 ~ 10621 : T1 : NC : Expansion 0 Id. This is the expansion number of current expansion starting at 0 for main board.
* 10622 ~ 10622 : T1 : NC : Expansion 0 Type.

* 10620 + "10000 value" ~ 10620 + "10000 value" : T1 : NC : Expansion 1 exists
* ...

As of now, Opta supports 5 expansions, so there are 6 groups of expansion in Registers with main board included. 
Expansion 1 to 6 starting offset depends on 10000 "Structure length". 

Expansion Type can be:
* 0 : None or main board
* 1 : Arduino Pro Opta Ext D16O8E - AFX00005
* 2 : Arduino Pro Opta Ext D16O8S - AFX00006
* 3 : Arduino Pro Opta Ext A06O2  - AFX00007

### Inputs

* 12220 ~ 12220 : T1 : NC : Expansion 0 Input 0 exists
* 12221 ~ 12221 : T1 : NC : Expansion 0 Input 0 uid. This is concatened "expansion num" + "input num"
* 12222 ~ 12222 : T1 : NC : Expansion 0 Input 0 id. This is the input number of current expansion starting at 0.
* 12223 ~ 12223 : T1 : NC : Expansion 0 Input 0 Type.
* 12224 ~ 12224 : T1 : NC : Expansion 0 Input 0 state. 1 = High, 0 = Low.
* 12225 ~ 12225 : T1 : NC : Expansion 0 Input 0 voltage in mV.
* 12226 ~ 12227 : T2 : NC : Expansion 0 Input 0 last update time.
* 12228 ~ 12229 : T2 : NC : Expansion 0 Input 0 last reset time.
* 12230 ~ 12231 : T2 : NC : Expansion 0 Input 0 total pulse count.
* 12232 ~ 12233 : T2 : NC : Expansion 0 Input 0 parial pulse count.
* 12234 ~ 12235 : T2 : NC : Expansion 0 Input 0 total time of high level in ms.
* 12236 ~ 12237 : T2 : NC : Expansion 0 Input 0 partial time of high level in ms.
* 12238 ~ 12238 : T1 : CH : Expansion 0 Input 0 partial counter reset. Set it to 1 in Holding Registers to reset partial counters. (pulse and high)

* 12320 + "Expansion x * 10001 value" + "Input y * 10001 value" ~ ... : T1 : NC : Expension x Input y exists
* ...

As of now, expansions could have maximum of 16 inputs, so there are 16 groups of inputs per expansion in Registers. 
Input 1 to 16 starting offset depends on 10000 "Structure length".

Example of UID for expansion 2 Input 3 (input 4 print on material) also called I2.3 in industry is 203.
Another example of UID for main board first input also called I0.0 in industry is 0.

Input Type can be:
0 : None (should not be happened)
1 : Analaog
2 : Digital
3 : Relay (only for output)
4 : PWM (only for output)


### Outputs

Same as input with starting address at 12820.

## Example

Here it is an example to configure device as Ethernet with a static IP of 10.1.5.132:

* 10250 = 0x0a : First part of device static IP (10)
* 10251 = 0x01 : Second part of device static IP (1)
* 10252 = 0x05 : Third part of device static IP (5)
* 10253 = 0x84 : Fourth part of device static IP (132)
* 10266 = 0x00 : Set DHCP off (0)
* 10267 = 0x00 : Set Wifi off (0)
* 10014 = 0x05 : Set device password length (5)
* 10015 = 0x61 : First char of device password (a)
* 10016 = 0x64 : Second char of device password (d)
* 10017 = 0x6d : Thrid char of device password (m)
* 10018 = 0x69 : Fourth char of device password (i)
* 10019 = 0x6e : Fifth char of device password (n)
* 10013 = 0x01 : End of configuration (1)

