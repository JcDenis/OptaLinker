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

Length: 11

* 10000 ~ 10000 : T1 : NC : One Expansion structure length
* 10001 ~ 10001 : T1 : NC : One Input or Output structure length
* 10002 ~ 10002 : T1 : NC : Firmware start address
* 10003 ~ 10003 : T1 : NC : Device start address
* 10004 ~ 10004 : T1 : NC : Network start address
* 10005 ~ 10005 : T1 : NC : RS485 start address
* 10006 ~ 10006 : T1 : NC : Modbus start address
* 10007 ~ 10007 : T1 : NC : MQTT start address
* 10008 ~ 10008 : T1 : NC : Expansions start address
* 10009 ~ 10009 : T1 : NC : Inputs start address
* 10010 ~ 10010 : T1 : NC : Ouputs start address

Structure length is used for each input, output, expansion definition. 
This means for example that definiton of expansion 0 input 1 starts at address of expansion 0 input 0 + structure length.

Values are group by usage at starting addresses:

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

Length: 56

* 10020 ~ 10020 : T1 : NC : Library version major
* 10021 ~ 10021 : T1 : NC : Library version minor
* 10022 ~ 10022 : T1 : NC : Library version revision
* 10023 ~ 10023 : T1 : NC : Configuration validation.
* 10024 ~ 10073 : T4 : NC : Configuration validation password. (this is the device password)
* 10074 ~ 10075 : T2 : CH : OTA update firmware version. (in form of concat version like 10302)

In order to save new configuration after updating Modbus Holding Registers, 
set Device Password in 10024 then set 10023 to 1. 
Modbus server update its configuration and reboot.


### Device

Length: 103

* 10080 ~ 10080 : T1 : CH : Device ID. Must be 0~254
* 10081 ~ 10082 : T3 : CH : Time Offset. First reg is offset sign. Second reg is offset. Must be -24~24
* 10083 ~ 10132 : T4 : CH : Device User Login.
* 10133 ~ 10182 : T4 : CH : Device User Password.

Device User Login and Password are not exposed in Registers, leave them empty to keep old values.


### Network

Length: 118

* 10190 ~ 10193 : T5 : CH : Network static IPv4 if DHCP is off.
* 10194 ~ 10197 : T5 : CH : Network gateway IPv4 if DHCP is off.
* 10198 ~ 10201 : T5 : CH : Network subnet IPv4 if DHCP is off.
* 10202 ~ 10205 : T5 : CH : Network DNS IPv4 if DHCP is off.
* 10206 ~ 10206 : T1 : CH : Network DHCP. 1 to use DHCP, 0 to use static IP.
* 10207 ~ 10208 : T1 : CH : Network WiFi. 1 to use Wifi, 0 to use Ethernet.
* 10208 ~ 10257 : T4 : CH : Network WiFi Standard mode SSID.
* 10258 ~ 10307 : T4 : CH : Network WiFi Standard mode Password.

If board does not support wifi, 10207 is down to 0. 
If WiFi 10207 is on with no SSID 10208 or Password 10258, Wifi is in Access Point mode. 
For Access point mode SSID is `optalinker` + device ID and password is `optalinker`.


### RS485

Length: 4

* 10310 ~ 10310 : T1 : CH : RS485 Type.
* 10311 ~ 10312 : T2 : CH : Bauderate. (should be a standard valid value)
* 10313 ~ 10313 : T1 : CH : Publish received message to MQTT. (only for 10310 RS485 type 1)

RS485 Type can be:
* 0 : Disable.
* 1 : Receiver.
* 2 : Sender.


### Modbus

Length: 6

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

Length: 157

* 10330 ~ 10333 : T5 : CH : MQTT Server Ipv4.
* 10334 ~ 10334 : T1 : CH : MQTT Server port.
* 10335 ~ 10336 : T2 : CH : Update interval in ms.
* 10337 ~ 10386 : T4 : CH : MQTT Server user.
* 10387 ~ 10436 : T4 : CH : MQTT Server password.
* 10437 ~ 10486 : T4 : CH : MQTT base topic.

Set MQTT Server IP to 0.0.0.0 to disable MQTT features. 
Set update interval to 0 to disable automatique update. IO will be updated only on state change.


### Expansion

Length: 53 (one expansion)

* 10490 ~ 10490 : T1 : NC : Expansion 0 exists.
* 10491 ~ 10491 : T1 : NC : Expansion 0 Id. This is the expansion number of current expansion starting at 0 for main board.
* 10492 ~ 10492 : T1 : NC : Expansion 0 Type.
* 10493 ~ 10542 : T4 : NC : Expension 0 name.

* 10490 + "register 10000 value" ~ 10490 + "register 10000 value" : T1 : NC : Expansion 1 exists
* ...

As of now, Opta supports 5 expansions, so there are 6 groups of expansion in Registers with main board included. 
Expansion 1 to 6 starting offset depends on 10000 "Structure length". 

Expansion Type can be:
* 0 : None or main board
* 1 : Arduino Pro Opta Ext D16O8E - AFX00005
* 2 : Arduino Pro Opta Ext D16O8S - AFX00006
* 3 : Arduino Pro Opta Ext A06O2  - AFX00007

### Inputs

Length: 19 (one input)

* 10820 ~ 10820 : T1 : NC : Expansion 0 Input 0 exists
* 10821 ~ 10821 : T1 : NC : Expansion 0 Input 0 uid. This is concatened "expansion num" + "input num"
* 10822 ~ 10822 : T1 : NC : Expansion 0 Input 0 id. This is the input number of current expansion starting at 0.
* 10823 ~ 10823 : T1 : NC : Expansion 0 Input 0 Type.
* 10824 ~ 10824 : T1 : NC : Expansion 0 Input 0 state. 1 = High, 0 = Low.
* 10825 ~ 10825 : T1 : NC : Expansion 0 Input 0 voltage in mV.
* 10826 ~ 10827 : T2 : NC : Expansion 0 Input 0 last update time.
* 10828 ~ 10829 : T2 : NC : Expansion 0 Input 0 last reset time.
* 10830 ~ 10831 : T2 : NC : Expansion 0 Input 0 total pulse count.
* 10832 ~ 10833 : T2 : NC : Expansion 0 Input 0 parial pulse count.
* 10834 ~ 10835 : T2 : NC : Expansion 0 Input 0 total time of high level in ms.
* 10836 ~ 10837 : T2 : NC : Expansion 0 Input 0 partial time of high level in ms.
* 10838 ~ 10838 : T1 : CH : Expansion 0 Input 0 partial counter reset. Set it to 1 in Holding Registers to reset partial counters. (pulse and high)

* 10820 + "Expansion x * register 10001 value" + "Input y * register 10001 value" ~ ... : T1 : NC : Expension x Input y exists
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

Same as input with starting address at 11140.

## Example

Here it is an example to configure device as Ethernet with a static IP of 10.1.5.132:

* 10190 = 0x0a : First part of device static IP (10)
* 10191 = 0x01 : Second part of device static IP (1)
* 10192 = 0x05 : Third part of device static IP (5)
* 10193 = 0x84 : Fourth part of device static IP (132)
* 10196 = 0x00 : Set DHCP off (0)
* 10197 = 0x00 : Set Wifi off (0)
* 10024 = 0x05 : Set device password length (5)
* 10025 = 0x61 : First char of device password (a)
* 10026 = 0x64 : Second char of device password (d)
* 10027 = 0x6d : Thrid char of device password (m)
* 10028 = 0x69 : Fourth char of device password (i)
* 10029 = 0x6e : Fifth char of device password (n)
* 10023 = 0x01 : End of configuration (1)

