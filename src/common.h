/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_ENUM_H
#define OPTALINKER_ENUM_H

#include <Arduino.h>

namespace optalinker {

/**
 * Monitor message type.
 */
enum MonitorType {
  MonitorNone = 0,
  MonitorInfo,
  MonitorSuccess,
  MonitorFail,
  MonitorWarning,
  MonitorAction,
  MonitorLock,
  MonitorStop,
  MonitorReceive,
  MonitorSend,
  MonitorConfig,
  MonitorPlus,
  MonitorMinus,
};

/**
 * Icons used in monitor message.
 */
static const String MonitorTypeIcons[] = { "", "➡️", "✅", "❌", "⚠️", "⚪️", "🔒", "⛔", "🔍", "🚀", "📝", "➕", "➖" };

/**
 * Board type.
 */
enum BoardType {
  BoardNone = 0,
  BoardRs485,
  BoardWifi,
  BoardLite
};

/**
 * RS485 usage type.
 */
enum Rs485Type {
  Rs485None = 0,
  Rs485Receiver,
  Rs485Sender
};

/**
 * Modbus usage type.
 */
enum ModbusType {
  ModbusNone = 0,
  ModbusRtuServer,
  ModbusTcpServer,
  ModbusRtuClient,
  ModbusTcpClient
};

/**
 * Network connection type.
 */
enum NetworkType {
  NetworkNone = 0,
  NetworkEthernet,    // Ethernet
  NetworkStandard,    // Wifi STA
  NetworkAccessPoint  // Wifi AP
};

/**
 * Library execution status type.
 */
enum StateType {
  StateStop = 0,
  StateFreeze,
  StateSetup,
  StateRun
};

/**
 * Input/Output type.
 */
enum IoType {
  IoNone = 0,
  IoAnalog,
  IoDigital,
  IoRelay,
  IoPwm
};

/**
 * Input/Ouput description field.
 */
enum IoField {
  IoFieldExists = 0,
  IoFieldType,
  IoFieldState,
  IoFieldVoltage,
  IoFieldUpdate,
  IoFieldReset,
  IoFieldPulse,
  IoFieldPartialPulse,
  IoFieldHigh,
  IoFieldPartialHigh
};

/**
 * Input/Output description structure.
 */
struct IoStruct {
  uint8_t exists;
  uint16_t uid;
  uint16_t id; // eii, eoo
  IoType type;
  uint8_t state;
  int voltage;
  uint32_t update;
  uint32_t reset;
  uint32_t pulse;
  uint32_t partialPulse;
  uint32_t high;
  uint32_t partialHigh;
};

/**
 * Expension type.
 */
enum ExpansionType {
  ExpansionNone = 0,
  ExpansionDmec,
  ExpansionDsts,
  ExpansionAnalog
};

/**
 * Expansion decription structure.
 */
struct ExpansionStruct {
  uint8_t exists;
  uint8_t id;
  String name;
  ExpansionType type;
  IoStruct input[16];
  IoStruct output[8];
};


/**
 * Modbus InputRegisters and HoldingRegisters addresses and offset.
 *
 * Types:
 * * T1 : uint16 : 1 offset
 * * T2 : uint32 : 2 offsets
 * * T3 : int16  : 2 offsets : first offset is sign, second offset is absolute value as uint16
 * * T4 : string : 49 offsets : first offset is length of the string, next offsets are the string char by char
 * * T5 : IPv4   : 4 offsets : One offset per Ip bloc
 */

// Groups starting offset and length definition
constexpr uint16_t ModbusRegisterAddress         = 10000; // 10011 to 10019 empty. This address MUST never change through firmware versions
constexpr uint16_t ModbusRegisterFirmware        = 10020; // 10074 to 10079 empty
constexpr uint16_t ModbusRegisterDevice          = 10080; // 10183 to 10189 empty
constexpr uint16_t ModbusRegisterNetwork         = 10190; // 10308 to 10309 empty
constexpr uint16_t ModbusRegisterRs485           = 10310; // 10314 to 10319 empty
constexpr uint16_t ModbusRegisterModbus          = 10320; // 10326 to 10329 empty
constexpr uint16_t ModbusRegisterMqtt            = 10330; // 10487 to 10489 empty
constexpr uint16_t ModbusRegisterExpansion       = 10490; // 10543, 10544, 10598, 10599, 10653, 10654, 10708, 10709, 10763, 10764, 10818, 10819 empty
constexpr uint16_t ModbusRegisterInput           = 10820; // 10839, 10859, 10879, 10899, 10919, 10939, 10959, 10979, 10999, 11019, 11039, 11059, 11079, 11099, 11119, 11139 empty
constexpr uint16_t ModbusRegisterOutput          = 11140; // 11159, 11179, 11199, 11219, 11239, 11259, 11279, 11299 empty. END 11300
constexpr uint16_t ModbusRegisterExpansionLength = 55;
constexpr uint16_t ModbusRegisterIoLength        = 20;
constexpr uint16_t ModbusRegisterTotalLength     = 1300;

// Offset starting at address ModbusRegisterAddress
constexpr uint16_t ModbusRegisterAddressExpansionLength = 0;
constexpr uint16_t ModbusRegisterAddressIoLength        = 1;
constexpr uint16_t ModbusRegisterAddressFirmware        = 2;
constexpr uint16_t ModbusRegisterAddressExpansion       = 3;
constexpr uint16_t ModbusRegisterAddressInput           = 4;
constexpr uint16_t ModbusRegisterAddressOutput          = 5;
constexpr uint16_t ModbusRegisterAddressDevice          = 6;
constexpr uint16_t ModbusRegisterAddressNetwork         = 7;
constexpr uint16_t ModbusRegisterAddressRs485           = 8;
constexpr uint16_t ModbusRegisterAddressModbus          = 9;
constexpr uint16_t ModbusRegisterAddressMqtt            = 10;
// next 11

// Offset starting at address ModbusRegisterFirmware
constexpr uint16_t ModbusRegisterVersionMajor    = 0;  // T1
constexpr uint16_t ModbusRegisterVersionMinor    = 1;  // T1
constexpr uint16_t ModbusRegisterVersionRevision = 2;  // T1
constexpr uint16_t ModbusRegisterConfigValidate  = 3;  // T1, value 0~1, for holding register only, set to 1 to announce end of configuration update. (board reboot after)
constexpr uint16_t ModbusRegisterConfigPassword  = 4;  // T4, Before setting ModbusRegisterConfigValidate, you MUST confirm device password here
constexpr uint16_t ModbusRegisterOtaVersion      = 54; // T2, in form of concat version like 10302
// next 56

// Offset starting at address ModbusRegisterDevice
constexpr uint16_t ModbusRegisterDeviceId       = 0;  // T1, value 0~254
constexpr uint16_t ModbusRegisterTimeOffset     = 1;  // T3, value -24~24
constexpr uint16_t ModbusRegisterDeviceUser     = 3;  // T4
constexpr uint16_t ModbusRegisterDevicePassword = 53; // T4
// next 103

// Offset starting at address ModbusRegisterNetwork
constexpr uint16_t ModbusRegisterNetworkIp       = 0;  // T5
constexpr uint16_t ModbusRegisterNetworkGateway  = 4;  // T5
constexpr uint16_t ModbusRegisterNetworkSubnet   = 8;  // T5
constexpr uint16_t ModbusRegisterNetworkDns      = 12; // T5
constexpr uint16_t ModbusRegisterNetworkDhcp     = 16; // T1, value 0~1
constexpr uint16_t ModbusRegisterNetworkWifi     = 17; // T1, value 0~1
constexpr uint16_t ModbusRegisterNetworkSsid     = 18; // T4
constexpr uint16_t ModbusRegisterNetworkPassword = 68; // T4
// next 118

// Offset starting at address ModbusRegisterRs485
constexpr uint16_t ModbusRegisterRs485Type     = 0; // T1, see Rs485Type
constexpr uint16_t ModbusRegisterRs485Baudrate = 1; // T2
constexpr uint16_t ModbusRegisterRs485ToMqtt   = 3; // T1, value 0~1
// next 4

// Offset starting at address ModbusRegisterModbus
constexpr uint16_t ModbusRegisterModbusType = 0; // T1, see ModbusType
constexpr uint16_t ModbusRegisterModbusIp   = 1; // T5
constexpr uint16_t ModbusRegisterModbusPort = 5; // T1
// next 6

// Offset starting at address ModbusRegisterMqtt
constexpr uint16_t ModbusRegisterMqttIp       = 0;   // T5
constexpr uint16_t ModbusRegisterMqttPort     = 4;   // T1
constexpr uint16_t ModbusRegisterMqttInterval = 5;   // T2
constexpr uint16_t ModbusRegisterMqttUser     = 7;   // T4
constexpr uint16_t ModbusRegisterMqttPassword = 57;  // T4
constexpr uint16_t ModbusRegisterMqttBase     = 107; // T4
// next 157

// Offset starting at address ModbusRegisterExpansion + (expension number * ModbusRegisterExpansionLength)
constexpr uint16_t ModbusRegisterExpansionExists = 0; // T1
constexpr uint16_t ModbusRegisterExpansionId     = 1; // T1
constexpr uint16_t ModbusRegisterExpansionType   = 2; // T1, See ExpansionType
constexpr uint16_t ModbusRegisterExpansionName   = 3; // T4
// next 53

// Offset starting at address ModbusRegisterInput + (expansion number * ModbusRegisterIoLength) + (input number * ModbusRegisterIoLength)
// Offset starting at address ModbusRegisterOutput + (expansion number * ModbusRegisterIoLength) + (output number * ModbusRegisterIoLength)
constexpr uint16_t ModbusRegisterIoExists       = 0;  // T1, value 0~1
constexpr uint16_t ModbusRegisterIoUid          = 1;  // T1, value 0~516
constexpr uint16_t ModbusRegisterIoId           = 2;  // T1, value 0~16
constexpr uint16_t ModbusRegisterIoType         = 3;  // T1, see IoType
constexpr uint16_t ModbusRegisterIoState        = 4;  // T1, value 0~1
constexpr uint16_t ModbusRegisterIoVoltage      = 5;  // T1, value 0~24000 (env)
constexpr uint16_t ModbusRegisterIoUpdate       = 6;  // T2
constexpr uint16_t ModbusRegisterIoReset        = 8;  // T2
constexpr uint16_t ModbusRegisterIoPulse        = 10; // T2
constexpr uint16_t ModbusRegisterIoPartialPulse = 12; // T2
constexpr uint16_t ModbusRegisterIoHigh         = 14; // T2
constexpr uint16_t ModbusRegisterIoPartialHigh  = 16; // T2
constexpr uint16_t ModbusRegisterIoPartialReset = 18; // T1, value 0~1, for holding register only, Expension e input i reset partial counters
// next 19


/**
 * Serial monitor messages.
 */

// Main class
constexpr char LabelOptaLinkerLoop[]             = "Setup completed";
constexpr char LabelOptaLinkerThread[]           = "Starting threaded loop";
constexpr char LabelOptaLinkerApply[]            = "You must reboot device";
constexpr char LabelOptaLinkerBenchmarkStart[]   = "Getting loop time";
constexpr char LabelOptaLinkerBenchmarkLine[]    = "Loops per second: ";
constexpr char LabelOptaLinkerBenchmarkAverage[] = "Average of loops per second: ";

// Monitor
constexpr char LabelMonitorSetup[]     = "\n+—————————————————————————————————————+\n| Arduino Opta Industrial IoT gateway |\n+—————————————————————————————————————+\n\n";
constexpr char LabelMonitorReceive[]   = "Receiving message from serial monitor: ";
constexpr char LabelMonitorHeartbeat[] = "I'm alive";

// Board
constexpr char LabelBoardSetup[]          = "Configuring board";
constexpr char LabelBoardReboot[]         = "Rebooting device";
constexpr char LabelBoardStop[]           = "Opta process failed and stopped";
constexpr char LabelBoardTimeout[]        = "Set watchdog timeout to: ";
constexpr char LabelBoardFreeze[]         = "Entering freeze mode";
constexpr char LabelBoardUnfreeze[]       = "Exiting freeze mode, duration: ";
constexpr char LabelBoardName[]           = "Board name: ";
constexpr char LabelBoardNameNone[]       = "Unknown board name ";
constexpr char LabelBoardNameLite[]       = "Arduino OPTA Lite -- AFX00003";
constexpr char LabelBoardNameRs485[]      = "Arduino OPTA RS485 - AFX00001";
constexpr char LabelBoardNameWifi[]       = "Arduino OPTA Wifi -- AFX00002";
constexpr char LabelBoardButtonDuration[] = "Button was activated: ";

// Store
constexpr char LabelStoreSetup[]           = "Configuring flash memory";
constexpr char LabelStoreReadFail[]        = "Failed to read stored file";
constexpr char LabelStoreInitFail[]        = "QSPI initialization failed";
constexpr char LabelStoreErase[]           = "Erasing partitions, please wait...";
constexpr char LabelStoreExisting[]        = "Existing partition: ";
constexpr char LabelStoreFormat[]          = "Formatting partition: ";
constexpr char LabelStoreFormatFail[]      = "Error formatting partition: ";
constexpr char LabelStoreFirmware[]        = "Flashing firmware";
constexpr char LabelStoreFirmwareFail[]    = "Error writing firmware data";
constexpr char LabelStoreCertificate[]     = "Flashing certificate";
constexpr char LabelStoreCertificateFail[] = "Error writing certificates";
constexpr char LabelStorehMapped[]          = "Flashing memory mapped WiFi firmware";
constexpr char LabelStoreMappedFail[]      = "Error writing memory mapped firmware";

// Config
constexpr char LabelConfigSetup[] = "Configuring parameters";
constexpr char LabelConfigHold[] = "Hold for 5 seconds the user button to fully reset device. Waiting...";
constexpr char LabelConfigReset[] = "Resetting device configuration";
constexpr char LabelConfigJsonRead[] = "Reading configuration from JSON";
constexpr char LabelConfigJsonReadFail[] = "Failed to parse JSON";
constexpr char LabelConfigJsonReadUncomplete[] = "Missing required keys in JSON";
constexpr char LabelConfigDefaultRead[] = "Loading default configuration";
constexpr char LabelConfigFileWrite[] = "Writing configuration to flash memory";
constexpr char LabelConfigFileRead[] = "Reading configuration from flash memory";
constexpr char LabelConfigFileFail[] = "Configuration file not found";
constexpr char LabelConfigSetDeviceId[] = "Set device id to: ";
constexpr char LabelConfigSetDeviceUser[] = "Set device user to: ";
constexpr char LabelConfigSetDevicePassword[] = "Set device password to: ";
constexpr char LabelConfigSetTimeOffset[] = "Set time offset to: ";
constexpr char LabelConfigSetTimeServer[] = "Set time server to: ";
constexpr char LabelConfigSetRs485Type[] = "Set RS485 mode: ";
constexpr char LabelConfigSetRs485Baudrate[] = "Set RS485 baudrate to: ";
constexpr char LabelConfigSetRs485ToMqtt[] = "Set RS485 to MQTT to: ";
constexpr char LabelConfigSetNetworkIp[] = "Set network IP to: ";
constexpr char LabelConfigSetNetworkGateway[] = "Set network Gateway to: ";
constexpr char LabelConfigSetNetworkSubnet[] = "Set network Subnet to: ";
constexpr char LabelConfigSetNetworkDns[] = "Set network DNS to: ";
constexpr char LabelConfigSetNetworkDhcp[] = "Set network DHCP to: ";
constexpr char LabelConfigSetNetworkWifi[] = "Set network Wifi to: ";
constexpr char LabelConfigSetNetworkSsid[] = "Set network SSID to: ";
constexpr char LabelConfigSetNetworkPassword[] = "Set network password to: ";
constexpr char LabelConfigSetMqttIp[] = "Set MQTT server IP: ";
constexpr char LabelConfigSetMqttPort[] = "Set MQTT server port: ";
constexpr char LabelConfigSetMqttUser[] = "Set MQTT user: ";
constexpr char LabelConfigSetMqttPassword[] = "Set MQTT password: ";
constexpr char LabelConfigSetMqttBase[] = "Set MQTT base topic: ";
constexpr char LabelConfigSetMqttInterval[] = "Set MQTT interval: ";
constexpr char LabelConfigSetModbusType[] = "Set Modbus mode: ";
constexpr char LabelConfigSetModbusIp[] = "Set distant Modbus TCP server IP: ";
constexpr char LabelConfigSetModbusPort[] = "Set distant Modbus TCP server port: ";
constexpr char LabelConfigSetUpdateUrl[] = "Set OTA update URL: ";

// Io
constexpr char LabelIoSetup[] = "Configuring IO";
constexpr char LabelIoPoll[] = "Set inputs poll delay to: ";
constexpr char LabelIoExpansionSetup[] = "Configuring expansions";
constexpr char LabelIoExpansionNum[] = "Number of expansions found: ";
constexpr char LabelIoExpansionName[] = "Expansion ";
constexpr char LabelIoExpansionNone[] = "Unknown expansion name ";
constexpr char LabelIoExpansionDmec[] = "Arduino Pro Opta Ext D16O8E - AFX00005 ";
constexpr char LabelIoExpansionDsts[] = "Arduino Pro Opta Ext D16O8S - AFX00006";
constexpr char LabelIoExpansionAnalog[] = "Arduino Pro Opta Ext A06O2 - AFX00007 ";
constexpr char LabelIoStore[] = "Storing inputs and outputs values to flash memory";

// Network
constexpr char LabelNetworkSetup[] = "Configuring network";
constexpr char LabelNetworkTimeout[] = "Set network connection timeout to: ";
constexpr char LabelNetworkPoll[] = "Set network poll delay to: ";
constexpr char LabelNetworkMode[] = "Set network mode as: ";
constexpr char LabelNetworkFail[] = "Communication with network module failed";
constexpr char LabelNetworkDhcpIp[] = "DHCP attributed IP is: ";
constexpr char LabelNetworkStaticIp[] = "Using static IP: ";
constexpr char LabelNetworkSsid[] = "Using SSID and password: ";
constexpr char LabelNetworkApFail[] = "Failed to create Wifi Access Point";
constexpr char LabelNetworkApSuccess[] = "Wifi access point listening";
constexpr char LabelNetworkApConnect[] = "Device connected to Access Point";
constexpr char LabelNetworkApDisconnect[] = "Device disconnected from Access Point";
constexpr char LabelNetworkEthernetFail[] = "Network connection failed";
constexpr char LabelNetworkEthernetSuccess[] = "Network connected with IP: ";
constexpr char LabelNetworkEthernet[] = "Connecting Ethernet network";
constexpr char LabelNetworkEthernetConnect[] = "Ethernet cable connected";
constexpr char LabelNetworkEthernetDisconnect[] = "Ethernet cable disconnected";
constexpr char LabelNetworkSta[] = "Connecting Wifi Standard network";
constexpr char LabelNetworkStaFail[] = "Failed to connect Wifi";
constexpr char LabelNetworkStaSuccess[] = "Wifi connected";

// Clock
constexpr char LabelClockSetup[] = "Configuring time";
constexpr char LabelClockServer[] = "Using NTP server: ";
constexpr char LabelClockUpdate[] = "Updating local time";
constexpr char LabelClockUpdateFail[] = "Failed to update local time";
constexpr char LabelClockUpdateSuccess[] = "Time set to: ";


// RS485
constexpr char LabelRs485Setup[] = "Configuring RS485";
constexpr char LabelRs485Baudrate[] = "Using baudrate: ";

// Modbus
constexpr char LabelModbusSetup[] = "Configuring Modbus";
constexpr char LabelModbusNone[] = "Modbus is disabled";
constexpr char LabelModbusTcpClient[] = "As TCP client";
constexpr char LabelModbusTcpServer[] = "As TCP server";
constexpr char LabelModbusClientFail[] = "Modbus TCP Client failed to connect";
constexpr char LabelModbusRtuClient[] = "Using RTU client";
constexpr char LabelModbusRtuServer[] = "As RTU server";
constexpr char LabelModbusEthernetServer[] = "Creating Modbus Ethernet server";
constexpr char LabelModbusWifiServer[] = "Creating Modbus Wifi server";
constexpr char LabelModbusBeginFail[] = "Failed to start Modbus";
constexpr char LabelModbusRegisterParse[] = "Parsing Modbus HoldingRegisters";
constexpr char LabelModbusRegisterMissmatch[] = "Modbus HoldingRegisters password missmatch";
constexpr char LabelModbusClientConnect[] = "Modbus client connected for request";
constexpr char LabelModbusClientKick[] = "Modbus server kicks client";

// MQTT
constexpr char LabelMqttSetup[] = "Configuring MQTT client";
constexpr char LabelMqttServer[] = "Using broker: ";
constexpr char LabelMqttBroker[] = "Connecting to MQTT broker";
constexpr char LabelMqttBrokerFail[] = "Failed to connect to MQTT broker";
constexpr char LabelMqttBrokerSuccess[] = "MQTT broker found";
constexpr char LabelMqttSubscribe[] = "Subcribed to MQTT topic: ";
constexpr char LabelMqttReceive[] = "Receiving MQTT command: ";
constexpr char LabelMqttPublishDevice[] = "Publishing device informations to MQTT";
constexpr char LabelMqttPublishInput[] = "Publishing inputs informations to MQTT";

// Web server
constexpr char LabelWebSetup[] = "Configuring web server";
constexpr char LabelWebStart[] = "Starting Web server";
constexpr char LabelWebStop[] = "Stopping Web server";
constexpr char LabelWebConfig[] = "Parsing received configuration";
constexpr char LabelWebConfigFail[] = "Failed to load configuration from response";
constexpr char LabelWebConfigFailUser[] = "Missing device user";
constexpr char LabelWebConfigKeepDevice[] = "Get previous device password";
constexpr char LabelWebConfigKeepWifi[] = "Get previous Wifi password";
constexpr char LabelWebConfigKeepMqtt[] = "Get previous MQTT password";

// OTA update
constexpr char LabelUpdateCheck[] = "Checking OTA update";
constexpr char LabelUpdateUpgrade[] = "Higher version bootloader required to perform OTA.\nPlease update the bootloader.\nFile -> Examples -> STM32H747_System -> STM32H747_manageBootloader";
constexpr char LabelUpdateNone[] = "No new OTA firmware version";
constexpr char LabelUpdateBeginFail[] = "Failed to begin OTA firmware ";
constexpr char LabelUpdateDownload[] = "Starting OTA download ";
constexpr char LabelUpdateDownloadFail[] = "Failed to download OTA firmware ";
constexpr char LabelUpdateUncompress[] = "Decompressing LZSS compressed file";
constexpr char LabelUpdateUncompressFail[] = "Failed to uncompress OTA firmware ";
constexpr char LabelUpdateBootloader[] = "Storing parameters for firmware update in bootloader";
constexpr char LabelUpdateBootloaderFail[] = "Failed to set up bootlader ";
constexpr char LabelUpdateSuccess[] = "Performing a reset after which the bootloader will update the firmware";

} // namespace optalinker

#endif // #ifndef OPTALINKER_ENUM_H