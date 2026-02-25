/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * see README.md file
 */

#ifndef OPTALINKER_DEFINE_H
#define OPTALINKER_DEFINE_H

// Device

// Unit: 0~254. Default: 0. Action: Device uniq ID, this is also the modbus server ID
#define OPTALINKER_DEVICE_ID 0

// Unit: string. Default: admin. Action: Web interface basic auth login
#define OPTALINKER_DEVICE_USER "admin"

// Unit: string. Default: admin. Action: Web interface basic auth password
#define OPTALINKER_DEVICE_PASSWORD "admin"


// Network

// Unit: 0~1. Default: 0. Action: Network DHCP mode. (0 = Static IP)
#define OPTALINKER_NETWORK_DHCP 0

// Unit: IPv4 string. Default: 192.168.1.213. Action: Device static IP (no HDCP)
#define OPTALINKER_NETWORK_IP "192.168.1.231"

// Unit: IPv4 string. Default: 192.168.1.1. Action: Device gateway IP (no DHCP)
#define OPTALINKER_NETWORK_GATEWAY "192.168.1.1"

// Unit: IPv4 string. Default: 255.255.0.0. Action: Device subnet mask (no DHCP)
#define OPTALINKER_NETWORK_SUBNET "255.255.0.0"

// Unit: IPv4 string. Default: 4.4.4.4. Action: Device DNS IP (no DHCP)
#define OPTALINKER_NETWORK_DNS "4.4.4.4"

// Unit: 0~1. Default: 0. Action: Use wifi netwaork. (0 = ethernet)
#define OPTALINKER_NETWORK_WIFI 0

// Unit: string. Default: "". Action: Device Wifi SSID (empty for Access point mode)
#define OPTALINKER_NETWORK_SSID ""

// Unit: string. Default: "". Action: Device Wifi password (for standard mode)
#define OPTALINKER_NETWORK_PASSWORD ""


// MQTT

// Unit: IPv4 string. Default: 0.0.0.0. Action: MQTT server IP. (0.0.0.0 disable MQTT features)
#define OPTALINKER_MQTT_IP "0.0.0.0"

// Unit: 0~65535 Default: 1883. Action: MQTT server RCP port.
#define OPTALINKER_MQTT_PORT 1883

// Unit: string. Default: mqtt_user. Action: MQTT server user
#define OPTALINKER_MQTT_USER "mqtt_user"

// Unit: string. Default: mqtt_password. Action: MQTT server password
#define OPTALINKER_MQTT_PASSWORD "mqtt_password"

// Unit: string. Default: /opta/. Action: MQTT base topic
#define OPTALINKER_MQTT_BASE "opta/"

// Unit: Second. Default: 0. Action: MQTT periodical publish delay. (0 to disbale periodical update)
#define OPTALINKER_MQTT_INTERVAL 0


// Time

// Unit: -23~23. Default: 0. Action: GMT time offset
#define OPTALINKER_TIME_OFFSET 0

// Unit: Domain name. Default: poll.ntp.org. Action: NTP server
#define OPTALINKER_TIME_SERVER "pool.ntp.org"


// Serial RS485

// Units: 0~2. Default: 0. Action: Serial RS485 type: 0=disable, 1=receiver, 2=sender
#define OPTALINKER_RS485_TYPE 0

// Units: baud. Default: 19200. Action: RS485 port speed
#define OPTALINKER_RS485_BAUDRATE 19200

// Untis: 0~1. Default: 0. Action: Redirect RS485 received message to MQTT
#define OPTALINKER_RS485_TOMQTT 0


// Modbus

// Unit: 0~4. Default: 0. Action: This device modbus mode: 0=none, 1=RTU server, 2=TCP server, 3=RTU client, 4=TCP client
#define OPTALINKER_MODBUS_TYPE 0

// Unit: IPv4 string. Action: The modbus TCP server IP
#define OPTALINKER_MODBUS_IP "0.0.0.0"

// Unit: 0~65535. Defaut: 502. Action: The modbus TCP server port
#define OPTALINKER_MODBUS_PORT 502

// Unit: Millisecond. Default: 50. Action: modbus server poll loop delay
#define OPTALINKER_MODBUS_POLL 50


// OTA update

#define OPTALINKER_UPDATE_URL ""


#endif  // #ifndef OPTALINKER_DEFINE_H