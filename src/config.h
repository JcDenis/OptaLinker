/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_CONFIG_H
#define OPTALINKER_CONFIG_H

#include <ArduinoJson.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerVersion;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerStore;

/**
 * OptaLinker Library configuration module.
 */
class OptaLinkerConfig : public OptaLinkerModule {

private:
  OptaLinkerVersion &version;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerStore &store;

  uint8_t _deviceId = 0;
  String _deviceUser = "admin";
  String _devicePassword = "admin";

  int8_t _timeOffset = 0;
  String _timeServer = "pool.ntp.org";

  uint8_t _rs485Type = 0;
  uint32_t _rs485Baudrate = 9600;
  uint8_t _rs485ToMqtt = 0;

  uint8_t _modbusType = 0;
  IPAddress _modbusIp = { 0, 0, 0, 0};
  uint16_t _modbusPort = 502;

  IPAddress _networkIp = { 192, 168, 1, 231 };
  IPAddress _networkGateway= { 192, 168, 1, 1 };
  IPAddress _networkSubnet = { 255, 255, 255, 0 };
  IPAddress _networkDns = { 4, 4, 4, 4 };
  uint8_t _networkDhcp = 0;
  uint8_t _networkWifi = 0;
  String _networkSsid = "";
  String _networkPassword = "";

  IPAddress _mqttIp = { 0, 0, 0, 0 };
  uint16_t _mqttPort = 1883;
  String _mqttUser = "";
  String _mqttPassword = "";
  String _mqttBase = "/opta/";
  uint32_t _mqttInterval = 0;

  String _updateUrl = "";

  /**
   * Convert IP addresse from string 0.0.0.0 to IPAddress object.
   *
   * @param 	The string IP
   *
   * @return 	The IPAddress object
   */
	IPAddress stringToIp(String ip) {
	  unsigned int res[4];
	  sscanf(ip.c_str(), "%u.%u.%u.%u", &res[0], &res[1], &res[2], &res[3]);
	  IPAddress ret(res[0], res[1], res[2], res[3]);

	  return ret;
	}

public:
  OptaLinkerConfig(OptaLinkerVersion &_version, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerStore &_store) : version(_version), monitor(_monitor), board(_board), store(_store) {}

  static const uint8_t MaxStringLength = 48; // Limit string length, usefull for modbus, must be even.

  uint8_t setup() {

	  if (isEnabled()) {

		  monitor.setMessage(LabelConfigSetup, MonitorAction);

		  readFromDefault();

	  	if (readFromFile()) {

		  	// wait user to push reset button
		    monitor.setMessage(LabelConfigHold, MonitorWarning);

		    uint32_t resetPushStart = 0;
		    uint8_t resetLedState = 0;
		    for (uint8_t i = 4; i-- > 0;) {  // boot delay of 3 seconds
		      for (uint8_t j = 0; j < 20; j++) {
		        delay(50);
		        resetLedState = resetLedState ? 0 : 1;
		        board.setGreen(resetLedState);

		        resetPushStart = millis();
		        while (board.isPushed()) {
		          board.setGreen(1);
		          if (resetPushStart + 5000 < millis()) {
		            reset();
		            board.reboot();
		            i = 0;
		            break;
		          }
		        }

		        board.pingTimeout();
		      }
		      if (i > 0) {
		        monitor.setMessage(String(i), MonitorNone);
		      }
		    }
		    board.setGreen(0);

		    writeToFile(); // update new keys
		  }
		}

	  board.pingTimeout();

    return 1;
  }

  uint8_t loop() {
    // if button press > 5s: reset config and reboot
    if (board.isPushDuration(5000, 20000)) {
      reset();
      board.reboot();
    }

    return 1;
  }

  /**
   * Reset configuration.
   *
   * Using values from define.
   * Device must be rebooted after.
   */
	void reset() {
	  monitor.setMessage(LabelConfigReset, MonitorAction);

	  store.eraseKey("config");
	  delay(10);
	  readFromDefault();
	  delay(10);
	  writeToFile();
	  delay(10);
	}

	/**
	 * Read configuration from a JSON string.
	 *
	 * @return 	1 on success, else 0
	 */
	uint8_t readFromJson(const char *buffer, size_t length) {

	  JsonDocument doc;
	  DeserializationError error = deserializeJson(doc, buffer, length);

	  if (error) {
	    monitor.setMessage(LabelConfigJsonReadFail, MonitorWarning);

	    return 0;
	  }

  	monitor.setMessage(LabelConfigJsonRead, MonitorSuccess);

	  if (doc["deviceId"].isNull()
	      || doc["deviceUser"].isNull()
	      || doc["devicePassword"].isNull()
	      || doc["timeOffset"].isNull()
	      || doc["timeServer"].isNull()
	      || doc["rs485Type"].isNull()
	      || doc["rs485Baudrate"].isNull()
	      || doc["rs485ToMqtt"].isNull()
	      || doc["modbusType"].isNull()
	      || doc["modbusPort"].isNull()
	      || doc["modbusIp"].isNull()
	      || doc["networkIp"].isNull()
	      || doc["networkGateway"].isNull()
	      || doc["networkSubnet"].isNull()
	      || doc["networkDns"].isNull()
	      || doc["networkDhcp"].isNull()
	      || doc["networkWifi"].isNull()
	      || doc["networkSsid"].isNull()
	      || doc["networkPassword"].isNull()
	      || doc["mqttIp"].isNull()
	      || doc["mqttPort"].isNull()
	      || doc["mqttUser"].isNull()
	      || doc["mqttPassword"].isNull()
	      || doc["mqttBase"].isNull()
	      || doc["mqttInterval"].isNull()
	      || doc["updateUrl"].isNull()) {
	    monitor.setMessage(LabelConfigJsonReadUncomplete, MonitorWarning);
	  }

	  if (!doc["deviceId"].isNull()) {
	    setDeviceId(doc["deviceId"].as<int>());
	  }
	  if (!doc["deviceUser"].isNull()) {
	    setDeviceUser(doc["deviceUser"].as<String>());
	  }
	  if (!doc["devicePassword"].isNull() && doc["devicePassword"].as<String>().length() > 4) {
	    setDevicePassword(doc["devicePassword"].as<String>());
	  }

	  if (!doc["timeOffset"].isNull()) {
	    setTimeOffset(doc["timeOffset"].as<int>());
	  }
	  if (!doc["timeServer"].isNull()) {
	    setTimeServer(doc["timeServer"].as<String>());
	  }

	  if (!doc["rs485Type"].isNull()) {
	    setRs485Type(doc["rs485Type"].as<int>());
	  }
	  if (!doc["rs485Baudrate"].isNull()) {
	    setRs485Baudrate(doc["rs485Baudrate"].as<int>());
	  }
	  if (!doc["rs485ToMqtt"].isNull()) {
	    setRs485ToMqtt(doc["rs485ToMqtt"].as<int>());
	  }

	  if (!doc["modbusType"].isNull()) {
	    setModbusType(doc["modbusType"].as<int>());
	  }
	  if (!doc["modbusPort"].isNull()) {
	    setModbusPort(doc["modbusPort"].as<int>());
	  }
	  if (!doc["modbusIp"].isNull() && !doc["modbusIp"][3].isNull()) {
	    IPAddress modbusIp(doc["modbusIp"][0].as<int>(), doc["modbusIp"][1].as<int>(), doc["modbusIp"][2].as<int>(), doc["modbusIp"][3].as<int>()); 
	    setModbusIp(modbusIp);
	  }

	  if (!doc["networkIp"].isNull() && !doc["networkIp"][3].isNull()) {
	    IPAddress networkIp(doc["networkIp"][0].as<int>(), doc["networkIp"][1].as<int>(), doc["networkIp"][2].as<int>(), doc["networkIp"][3].as<int>()); 
	    setNetworkIp(networkIp);
	  }
	  if (!doc["networkGateway"].isNull() && !doc["networkGateway"][3].isNull()) {
	    IPAddress networkGateway(doc["networkGateway"][0].as<int>(), doc["networkGateway"][1].as<int>(), doc["networkGateway"][2].as<int>(), doc["networkGateway"][3].as<int>()); 
	    setNetworkGateway(networkGateway);
	  }
	  if (!doc["networkSubnet"].isNull() && !doc["networkSubnet"][3].isNull()) {
	    IPAddress networkSubnet(doc["networkSubnet"][0].as<int>(), doc["networkSubnet"][1].as<int>(), doc["networkSubnet"][2].as<int>(), doc["networkSubnet"][3].as<int>()); 
	    setNetworkSubnet(networkSubnet);
	  }
	  if (!doc["networkDns"].isNull() && !doc["networkDns"][3].isNull()) {
	    IPAddress networkDns(doc["networkDns"][0].as<int>(), doc["networkDns"][1].as<int>(), doc["networkDns"][2].as<int>(), doc["networkDns"][3].as<int>()); 
	    setNetworkDns(networkDns);
	  }
	  if (!doc["networkDhcp"].isNull()) {
	    setNetworkDhcp(doc["networkDhcp"].as<int>());
	  }
	  if (!doc["networkWifi"].isNull()) {
	    setNetworkWifi(doc["networkWifi"].as<int>());
	  }
	  if (!doc["networkSsid"].isNull()) {
	    setNetworkSsid(doc["networkSsid"].as<String>());
	  }
	  if (!doc["networkPassword"].isNull() && doc["networkPassword"].as<String>().length() > 4) {
	    setNetworkPassword(doc["networkPassword"].as<String>());
	  }

	  if (!doc["mqttIp"].isNull() && !doc["mqttIp"][3].isNull()) {
	    IPAddress mqttIp(doc["mqttIp"][0].as<int>(), doc["mqttIp"][1].as<int>(), doc["mqttIp"][2].as<int>(), doc["mqttIp"][3].as<int>()); 
	    setMqttIp(mqttIp);
	  }
	  if (!doc["mqttPort"].isNull()) {
	    setMqttPort(doc["mqttPort"].as<int>());
	  }
	  if (!doc["mqttUser"].isNull()) {
	    setMqttUser(doc["mqttUser"].as<String>());
	  }
	  if (!doc["mqttPassword"].isNull() && doc["mqttPassword"].as<String>().length() > 1) {
	    setMqttPassword(doc["mqttPassword"].as<String>());
	  }
	  if (!doc["mqttBase"].isNull()) {
	    setMqttBase(doc["mqttBase"].as<String>());
	  }
	  if (!doc["mqttInterval"].isNull()) {
	    setMqttInterval(doc["mqttInterval"].as<int>());
	  }

	  if (!doc["updateUrl"].isNull()) {
	    setUpdateUrl(doc["updateUrl"].as<String>());
	  }

	  return 1;
	}

	/**
	 * Write current configuration to JSON string.
	 *
	 * @return 	The JSON configuration string
	 */
	String writeToJson(const bool nopass) {
	  JsonDocument doc;

	  doc["version"] = version.toString();
	  doc["name"] = board.getName();
    doc["type"] = board.getType();

	  doc["deviceId"] = getDeviceId();
	  doc["deviceUser"] = getDeviceUser();
	  doc["devicePassword"] = nopass ? "" : getDevicePassword();
	  doc["timeOffset"] = getTimeOffset();
	  doc["timeServer"] = getTimeServer();
	  doc["rs485Type"] = getRs485Type();
	  doc["rs485Baudrate"] = getRs485Baudrate();
	  doc["rs485ToMqtt"] = getRs485ToMqtt();
	  doc["modbusType"] = getModbusType();
	  doc["modbusPort"] = getModbusPort();
	  doc["networkDhcp"] = getNetworkDhcp();
	  doc["networkWifi"] = getNetworkWifi();
	  doc["networkSsid"] = getNetworkSsid();
	  doc["networkPassword"] = nopass ? "" : getNetworkPassword();
	  doc["mqttPort"] = getMqttPort();
	  doc["mqttUser"] = getMqttUser();
	  doc["mqttPassword"] = nopass ? "" : getMqttPassword();
	  doc["mqttBase"] = getMqttBase();
	  doc["mqttInterval"] = getMqttInterval();
	  doc["updateUrl"] = getUpdateUrl();

	  for (uint8_t i; i < 4; i++) {
	    doc["modbusIp"][i] = getModbusIp()[i];
	    doc["networkIp"][i] = getNetworkIp()[i];
	    doc["networkGateway"][i] = getNetworkGateway()[i];
	    doc["networkSubnet"][i] = getNetworkSubnet()[i];
	    doc["networkDns"][i] = getNetworkDns()[i];
	    doc["mqttIp"][i] = getMqttIp()[i];
	  }

	  String jsonString;
	  serializeJson(doc, jsonString);

	  return jsonString;
	}

	/**
	 * Read configuration from default define.
	 */
	void readFromDefault() {

	  setDeviceId(OPTALINKER_DEVICE_ID);
	  setDeviceUser(OPTALINKER_DEVICE_USER);
	  setDevicePassword(OPTALINKER_DEVICE_PASSWORD);

	  setTimeOffset(OPTALINKER_TIME_OFFSET);
	  setTimeServer(OPTALINKER_TIME_SERVER);

	  setRs485Type(OPTALINKER_RS485_TYPE);
	  setRs485Baudrate(OPTALINKER_RS485_BAUDRATE);
	  setRs485ToMqtt(OPTALINKER_RS485_TOMQTT);

	  setModbusType(OPTALINKER_MODBUS_TYPE);
	  setModbusIp(stringToIp(OPTALINKER_MODBUS_IP));
	  setModbusPort(OPTALINKER_MODBUS_PORT);

	  setNetworkIp(stringToIp(OPTALINKER_NETWORK_IP));
	  setNetworkGateway(stringToIp(OPTALINKER_NETWORK_GATEWAY));
	  setNetworkSubnet(stringToIp(OPTALINKER_NETWORK_SUBNET));
	  setNetworkDns(stringToIp(OPTALINKER_NETWORK_DNS));
	  setNetworkDhcp(OPTALINKER_NETWORK_DHCP);
	  setNetworkWifi(OPTALINKER_NETWORK_WIFI);
	  setNetworkSsid(OPTALINKER_NETWORK_SSID);
	  setNetworkPassword(OPTALINKER_NETWORK_PASSWORD);

	  setMqttIp(stringToIp(OPTALINKER_MQTT_IP));
	  setMqttPort(OPTALINKER_MQTT_PORT);
	  setMqttUser(OPTALINKER_MQTT_USER);
	  setMqttPassword(OPTALINKER_MQTT_PASSWORD);
	  setMqttBase(OPTALINKER_MQTT_BASE);
	  setMqttInterval(OPTALINKER_MQTT_INTERVAL);

	  setUpdateUrl(OPTALINKER_UPDATE_URL);

	  monitor.setMessage(LabelConfigDefaultRead, MonitorSuccess);
	}

	/**
	 * Write configuration to falsh memory.
	 *
	 * @return 	1 on success, else 0
	 */
	uint8_t writeToFile() {
	  String str = writeToJson(false);
	  uint8_t ret = store.writeKey("config", str.c_str());

	  monitor.setMessage(LabelConfigFileWrite, ret ? MonitorSuccess : MonitorFail);

	  return ret;
	}

	/**
	 * Read configuration from flash memory.
	 *
	 * If configuration file can not be read, the configuration is resetted to default.
	 *
	 * @return 	1 on success, else 0
	 */
	uint8_t readFromFile() {

	  String str = store.readKey("config");
	  if (readFromJson(str.c_str(), str.length()) < 1) {
	    monitor.setMessage(LabelConfigFileFail, MonitorFail);
	    reset();

	    return 0;
	  }
	  monitor.setMessage(LabelConfigFileRead, MonitorSuccess);

	  return 1;
	}

	uint8_t getDeviceId() const {

	  return _deviceId;
	}

	void setDeviceId(int value) {
	  if (value >= 0 && value < 255 && value != _deviceId) {
	    monitor.setMessage(LabelConfigSetDeviceId + String(value), MonitorSuccess);
	    _deviceId = value;
	  }
	}

	String getDeviceUser() const {

	  return _deviceUser;
	}

	void setDeviceUser(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_deviceUser)) {
		  monitor.setMessage(LabelConfigSetDeviceUser + String(value), MonitorSuccess);
		  _deviceUser = value;
		}
	}

	String getDevicePassword() const {

	  return _devicePassword;
	}

	void setDevicePassword(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_devicePassword)) {
		  monitor.setMessage(LabelConfigSetDevicePassword + String(value), MonitorSuccess);
		  _devicePassword = value;
		}
	}

	int8_t getTimeOffset() const {

	  return _timeOffset;
	}

	void setTimeOffset(int16_t value) {
	  if (value > -24 && value < 24 && value != _timeOffset) {
	    monitor.setMessage(LabelConfigSetTimeOffset + String(value), MonitorSuccess);
	    _timeOffset = value;
	  }
	}

	String getTimeServer() const {

	  return _timeServer;
	}

	void setTimeServer(String value) {
		if (!value.equals(_timeServer)) {
		  monitor.setMessage(LabelConfigSetTimeServer + String(value), MonitorSuccess);
		  _timeServer = value;
		}
	}

	uint8_t getRs485Type() const {

	  return _rs485Type;
	}

	void setRs485Type(uint8_t value) {
	  if (value < 3 && value != _rs485Type) {
	    monitor.setMessage(LabelConfigSetRs485Type + String(value), MonitorSuccess);
	    _rs485Type = value;
	  }
	}

	uint32_t getRs485Baudrate() const {

	  return _rs485Baudrate;
	}

	void setRs485Baudrate(uint32_t value) {
	  if (value > 0 && value < 921600 && value != _rs485Baudrate) {
	    monitor.setMessage(LabelConfigSetRs485Baudrate + String(value), MonitorSuccess);
	    _rs485Baudrate = value;
	  }
	}

	uint8_t getRs485ToMqtt() const {

	  return _rs485ToMqtt;
	}

	void setRs485ToMqtt(int value) {
		if ((value == 0 || value == 1) && value != _rs485ToMqtt) {
		  monitor.setMessage(LabelConfigSetRs485ToMqtt + String(value ? "Enable" : "Disable"), MonitorSuccess);
		  _rs485ToMqtt = value;
		}
	}

	IPAddress getNetworkIp() const {

	  return _networkIp;
	}

	void setNetworkIp(IPAddress value) {
		if (value != _networkIp) {
		  monitor.setMessage(LabelConfigSetNetworkIp + value.toString(), MonitorSuccess);
		  _networkIp = value;
		}
	}

	IPAddress getNetworkGateway() const {

	  return _networkGateway;
	}

	void setNetworkGateway(IPAddress value) {
		if (value != _networkGateway) {
		  monitor.setMessage(LabelConfigSetNetworkGateway + value.toString(), MonitorSuccess);
		  _networkGateway = value;
		}
	}

	IPAddress getNetworkSubnet() const {

	  return _networkSubnet;
	}

	void setNetworkSubnet(IPAddress value) {
		if (value != _networkSubnet) {
		  monitor.setMessage(LabelConfigSetNetworkSubnet + value.toString(), MonitorSuccess);
	  	_networkSubnet = value;
	  }
	}

	IPAddress getNetworkDns() const {

	  return _networkDns;
	}

	void setNetworkDns(IPAddress value) {
		if (value != _networkDns) {
	  	monitor.setMessage(LabelConfigSetNetworkDns + value.toString(), MonitorSuccess);
		  _networkDns = value;
		}
	}

	uint8_t getNetworkDhcp() const {

	  return _networkDhcp;
	}

	void setNetworkDhcp(int value) {
		if ((value == 0 || value == 1) && value != _networkDhcp) {
		  monitor.setMessage(LabelConfigSetNetworkDhcp + String(value ? "Enable" : "Disable"), MonitorSuccess);
		  _networkDhcp = value;
		}
	}

	uint8_t getNetworkWifi() const {

	  return _networkWifi;
	}

	void setNetworkWifi(int value) {
		if ((value == 0 || value == 1) && value != _networkWifi) {
		  monitor.setMessage(LabelConfigSetNetworkWifi + String(value ? "Enable" : "Disable"), MonitorSuccess);
		  _networkWifi = value;
		}
	}

	String getNetworkSsid() const {

	  return _networkSsid;
	}

	void setNetworkSsid(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_networkSsid)) {
	  	monitor.setMessage(LabelConfigSetNetworkSsid + String(value), MonitorSuccess);
	  	_networkSsid = value;
	  }
	}

	String getNetworkPassword() const {

	  return _networkPassword;
	}

	void setNetworkPassword(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_networkPassword)) {
		  monitor.setMessage(LabelConfigSetNetworkPassword + String(value), MonitorSuccess);
		  _networkPassword = value;
		}
	}

	IPAddress getMqttIp() const {

	  return _mqttIp;
	}

	void setMqttIp(IPAddress value) {
		if (value != _mqttIp) {
	  	monitor.setMessage(LabelConfigSetMqttIp + value.toString(), MonitorSuccess);
	  	_mqttIp = value;
	  }
	}

	uint16_t getMqttPort() const {

	  return _mqttPort;
	}

	void setMqttPort(int value) {
		if ((value > 0 && value < 65536) && value != _mqttPort) {
		  monitor.setMessage(LabelConfigSetMqttPort + String(value), MonitorSuccess);
		  _mqttPort = value;
		}
	}

	String getMqttUser() const {

	  return _mqttUser;
	}

	void setMqttUser(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_mqttUser)) {
	  	monitor.setMessage(LabelConfigSetMqttUser + value, MonitorSuccess);
	  	_mqttUser = value;
	  }
	}

	String getMqttPassword() const {

	  return _mqttPassword;
	}

	void setMqttPassword(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_mqttPassword)) {
	  	monitor.setMessage(LabelConfigSetMqttPassword + value, MonitorSuccess);
	  	_mqttPassword = value;
	  }
	}

	String getMqttBase() const {

	  return _mqttBase;
	}

	void setMqttBase(String value) {
		if (value.length() <= MaxStringLength && !value.equals(_mqttBase)) {
	  	monitor.setMessage(LabelConfigSetMqttBase + value, MonitorSuccess);
	  	_mqttBase = value;
	  }
	}

	uint32_t getMqttInterval() const {

	  return _mqttInterval;
	}

	void setMqttInterval(uint32_t value) {
	  if (value != _mqttInterval) {
	    monitor.setMessage(LabelConfigSetMqttInterval + String(value), MonitorSuccess);
	    _mqttInterval = value;
	  }
	}

	uint8_t getModbusType() const {

	  return _modbusType;
	}

	void setModbusType(uint8_t value) {
	  if (value < 5 && value != _modbusType) {
	    monitor.setMessage(LabelConfigSetModbusType + String(value), MonitorSuccess);
	    _modbusType = value;
	  }
	}

	IPAddress getModbusIp() const {

	  return _modbusIp;
	}

	void setModbusIp(IPAddress value) {
		if (value != _modbusIp) {
	  	monitor.setMessage(LabelConfigSetModbusIp + value.toString(), MonitorSuccess);
	  	_modbusIp = value;
	  }
	}

	uint16_t getModbusPort() const {

	  return _modbusPort;
	}

	void setModbusPort(int value) {
		if (value > 0 && value < 65536 && value != _modbusPort) {
		  monitor.setMessage(LabelConfigSetModbusPort + String(value), MonitorSuccess);
		  _modbusPort = value;
		}
	}

	// OTA update URL is not manage in modbus registers
	String getUpdateUrl() const {

	  return _updateUrl;
	}

	void setUpdateUrl(String value) {
		if (!value.equals(_updateUrl)) {
		  monitor.setMessage(LabelConfigSetUpdateUrl + String(value), MonitorSuccess);
		  _updateUrl = value;
		}
	}

}; // class OptaLinkerConfig

} // namespace optalinker

#endif // #ifndef OPTALINKER_CONFIG_H