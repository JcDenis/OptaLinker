/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_MODBUS_H
#define OPTALINKER_MODBUS_H

#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <Ethernet.h>
#include <WiFi.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerVersion;
class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;
class OptaLinkerNetwork;
class OptaLinkerIo;
class OptaLinkerRs485;

class OptaLinkerModbus : public OptaLinkerModule {

private:
  OptaLinkerVersion &version;
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;
  OptaLinkerNetwork &network;
  OptaLinkerIo &io;
  OptaLinkerRs485 &rs485;

  ModbusRTUClientClass _rtuClient;

  ModbusRTUServerClass _rtuServer;

  EthernetClient _ethernetClient;
  ModbusTCPClient _tcpEthernetClient;
  WiFiClient _wifiClient;
  ModbusTCPClient _tcpWifiClient;

  EthernetServer _ethernetServer;
  WiFiServer _wifiServer;
  ModbusTCPServer _tcpServer;

  /**
   * The distant server ID for TCP client request.
   */
  uint8_t _clientServerId = 0;

  /**
   * Last update time of IO.
   */
  uint32_t _updateLast = 0;

  void prepareRS485() {
     auto bitduration{ 1.f / config.getRs485Baudrate() };
     auto preDelayBR{ bitduration * 9.6f * 3.5f * 1e6 };
     auto postDelayBR{ bitduration * 9.6f * 3.5f * 1e6 };
     RS485.setDelays(preDelayBR, postDelayBR);
  }

  /**
   * Connect lcoal modbus client to distant modbus TCP server.
   *
   * @return 1 on success, else 0
   */
  uint8_t connectToTcpServer() {
    uint8_t ret = 0;

    // Wait previous for connection handle
    lock();

    if (network.isEthernet()) {
      if (!_tcpEthernetClient.connected()) {
        board.setFreeze();
        _tcpEthernetClient.setTimeout(5000);
        if (!_tcpEthernetClient.begin(config.getModbusIp(), config.getModbusPort())) {
          monitor.setWarning(LabelModbusClientFail);
        } else {
          ret = _tcpEthernetClient.connected() == 1 ? 1 : 0;
        }
        board.unsetFreeze();
      }
    } else if (network.isStandard()) {
      if (!_tcpWifiClient.connected()) {
        board.setFreeze();
        _tcpWifiClient.setTimeout(5000);
        if (!_tcpWifiClient.begin(config.getModbusIp(), config.getModbusPort())) {
          monitor.setWarning(LabelModbusClientFail);
        } else {
          ret = _tcpWifiClient.connected() == 1 ? 1 : 0;
        }
        board.unsetFreeze();
      }
    }

    unlock();

    return ret;
  }

  /**
   * Disconnect local modbus client from distant modbus TCP server.
   *
   * We connect/disconnect on each request to prevent server kick.
   */
  void disconnectFromTcpServer() {
    if (network.isEthernet()) {
      _tcpEthernetClient.stop();
    } else if (network.isStandard()) {
      _tcpWifiClient.stop();
    }
  }

  /**
   * Configure modbus local modbus server registers and discretes and coils.
   *
   * On startup.
   */
  void configureServerRegisters() {
    if (isRtuServer()) {
      _rtuServer.configureInputRegisters(RegisterOffsetAddress, ModbusRegisterTotalLength);
      _rtuServer.configureHoldingRegisters(RegisterOffsetAddress, ModbusRegisterTotalLength);
      _rtuServer.configureDiscreteInputs(0, (io.getMaxExpansionsNum() * io.getMaxInputNum()));
      _rtuServer.configureCoils(0, (io.getMaxExpansionsNum() * io.getMaxOutputNum()));
    } else if (isTcpServer()) {
      _tcpServer.configureInputRegisters(RegisterOffsetAddress, ModbusRegisterTotalLength);
      _tcpServer.configureHoldingRegisters(RegisterOffsetAddress, ModbusRegisterTotalLength);
      _tcpServer.configureDiscreteInputs(0, (io.getMaxExpansionsNum() * io.getMaxInputNum()));
      _tcpServer.configureCoils(0, (io.getMaxExpansionsNum() * io.getMaxOutputNum()));
    }
  }

  /**
   * Update local modbus server registers.
   */
  void setServerRegisters() {
    _updateLast = state.getTime() + 1;
    uint16_t offset = 0;
    ExpansionStruct *expansion = io.getExpansions();

    // Address
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressExpansionLength, ModbusRegisterExpansionLength);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressIoLength, ModbusRegisterIoLength);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressFirmware, ModbusRegisterFirmware);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressExpansion, ModbusRegisterExpansion);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressInput, ModbusRegisterInput);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressOutput, ModbusRegisterOutput);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressDevice, ModbusRegisterDevice);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressNetwork, ModbusRegisterNetwork);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressRs485, ModbusRegisterRs485);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressModbus, ModbusRegisterModbus);
    setRegisterUint16(ModbusRegisterAddress + ModbusRegisterAddressMqtt, ModbusRegisterMqtt);

    // Firmware
    setRegisterUint16(ModbusRegisterFirmware + ModbusRegisterVersionMajor, version.getMajor());
    setRegisterUint16(ModbusRegisterFirmware + ModbusRegisterVersionMinor, version.getMinor());
    setRegisterUint16(ModbusRegisterFirmware + ModbusRegisterVersionRevision, version.getRevision());
    setRegisterUint16(ModbusRegisterFirmware + ModbusRegisterConfigValidate, 0);
    setRegisterString(ModbusRegisterFirmware + ModbusRegisterConfigPassword, String(""));

    // Config - Device
    setRegisterUint16(ModbusRegisterDevice + ModbusRegisterDeviceId, config.getDeviceId());
    setRegisterInt16(ModbusRegisterDevice + ModbusRegisterTimeOffset, config.getTimeOffset());
    setRegisterString(ModbusRegisterDevice + ModbusRegisterDeviceUser, String("")); // Do not expose device user
    setRegisterString(ModbusRegisterDevice + ModbusRegisterDevicePassword, String("")); // Do not expose device password

    // Config - Network
    setRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkIp, config.getNetworkIp());
    setRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkGateway, config.getNetworkGateway());
    setRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkSubnet, config.getNetworkSubnet());
    setRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkDns, config.getNetworkDns());
    setRegisterUint16(ModbusRegisterNetwork + ModbusRegisterNetworkDhcp, config.getNetworkDhcp());
    setRegisterUint16(ModbusRegisterNetwork + ModbusRegisterNetworkWifi, config.getNetworkWifi());
    setRegisterString(ModbusRegisterNetwork + ModbusRegisterNetworkSsid, config.getNetworkSsid());
    setRegisterString(ModbusRegisterNetwork + ModbusRegisterNetworkPassword, String("")); // Do not expose wifi password

    // Config - RS485
    setRegisterUint16(ModbusRegisterRs485 + ModbusRegisterRs485Type, config.getRs485Type());
    setRegisterUint32(ModbusRegisterRs485 + ModbusRegisterRs485Baudrate, config.getRs485Baudrate());
    setRegisterUint16(ModbusRegisterRs485 + ModbusRegisterRs485ToMqtt, config.getRs485ToMqtt());

    // Config - Modbus
    setRegisterUint16(ModbusRegisterModbus + ModbusRegisterModbusType, config.getModbusType());
    setRegisterIp(ModbusRegisterModbus + ModbusRegisterModbusIp, config.getModbusIp());
    setRegisterUint16(ModbusRegisterModbus + ModbusRegisterModbusPort, config.getModbusPort());

    // Config - MQTT
    setRegisterIp(ModbusRegisterMqtt + ModbusRegisterMqttIp, config.getMqttIp());
    setRegisterUint16(ModbusRegisterMqtt + ModbusRegisterMqttPort, config.getMqttPort());
    setRegisterUint32(ModbusRegisterMqtt + ModbusRegisterMqttInterval, config.getMqttInterval());
    setRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttUser, String("")); // Do not expose mqtt user
    setRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttPassword, String("")); // Do not expose mqtt password
    setRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttBase, config.getMqttBase());

    // Expansions
    for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
      offset = ModbusRegisterExpansion + (e * ModbusRegisterExpansionLength);
      setRegisterUint16(offset + ModbusRegisterExpansionExists, expansion[e].exists);
      setRegisterUint16(offset + ModbusRegisterExpansionId, expansion[e].id);
      setRegisterUint16(offset + ModbusRegisterExpansionType, expansion[e].type);
      setRegisterString(offset + ModbusRegisterExpansionName, expansion[e].name);
    }

    // Inputs
    for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
      for(uint8_t i = 0; i < io.getMaxInputNum(); i++) {
        setRegisterIo(ModbusRegisterInput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength), expansion[e].input[i], 1);
        //max 6*16*20
      }
    }

    // Output
    for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
      for(uint8_t i = 0; i < io.getMaxOutputNum(); i++) {
        setRegisterIo(ModbusRegisterOutput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength), expansion[e].output[i], 0);
        //max 6*8*20
      }
    }
  }

  /**
   * Update local modbus server IO registers.
   *
   * @param   offset    The starting address of the IO registers
   * @param   ios       The IO structure
   * @param   isInput   True if IO is an input
   */
  void setRegisterIo(uint16_t offset, IoStruct ios, uint8_t isInput = true) {
    setRegisterUint16(offset + ModbusRegisterIoExists, ios.exists);
    setRegisterUint16(offset + ModbusRegisterIoUid, ios.uid);
    setRegisterUint16(offset + ModbusRegisterIoId, ios.id);
    setRegisterUint16(offset + ModbusRegisterIoType, ios.type);
    setRegisterUint16(offset + ModbusRegisterIoState, ios.state); 
    setRegisterUint16(offset + ModbusRegisterIoVoltage, ios.voltage);
    setRegisterUint32(offset + ModbusRegisterIoUpdate, ios.update);
    setRegisterUint32(offset + ModbusRegisterIoReset, ios.reset);
    setRegisterUint32(offset + ModbusRegisterIoPulse, ios.pulse);
    setRegisterUint32(offset + ModbusRegisterIoPartialPulse, ios.partialPulse);
    setRegisterUint32(offset + ModbusRegisterIoHigh, ios.high);
    setRegisterUint32(offset + ModbusRegisterIoPartialHigh, ios.partialHigh);
    setRegisterUint16(offset + ModbusRegisterIoPartialReset, 0);

    if (isInput) {
      setDiscreteInput(ios.uid, ios.state);
    } else {
      setCoil(ios.uid, ios.state);
    }
  }

  /**
   * Parser received holding registers.
   */
  void parseServerHoldingRegisters() {
    monitor.setAction(LabelModbusRegisterParse);

    // prevent infinite loop
    setHoldingRegister(ModbusRegisterFirmware + ModbusRegisterConfigValidate, 0);

    if (!getHoldingRegisterString(ModbusRegisterFirmware + ModbusRegisterConfigPassword).equals(config.getDevicePassword())) {
      monitor.setWarning(LabelModbusRegisterMissmatch);
    } else {

      // Device
      config.setDeviceId(getHoldingRegisterUint16(ModbusRegisterDevice + ModbusRegisterDeviceId));
      config.setTimeOffset(getHoldingRegisterInt16(ModbusRegisterDevice + ModbusRegisterTimeOffset));
      String deviceUser = getHoldingRegisterString(ModbusRegisterDevice + ModbusRegisterDeviceUser);
      if (deviceUser.length() > 1 && !deviceUser.equals(config.getDeviceUser())) {
        config.setDeviceUser(deviceUser);
      }
      String devicePassword = getHoldingRegisterString(ModbusRegisterDevice + ModbusRegisterDevicePassword);
      if (devicePassword.length() > 1 && !devicePassword.equals(config.getDevicePassword())) {
        config.setDevicePassword(devicePassword);
      }

      // Network
      config.setNetworkIp(getHoldingRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkIp));
      config.setNetworkGateway(getHoldingRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkGateway));
      config.setNetworkSubnet(getHoldingRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkSubnet));
      config.setNetworkDns(getHoldingRegisterIp(ModbusRegisterNetwork + ModbusRegisterNetworkDns));
      config.setNetworkDhcp(getHoldingRegisterUint16(ModbusRegisterNetwork + ModbusRegisterNetworkDhcp));
      config.setNetworkWifi(getHoldingRegisterUint16(ModbusRegisterNetwork + ModbusRegisterNetworkWifi));
      config.setNetworkSsid(getHoldingRegisterString(ModbusRegisterNetwork + ModbusRegisterNetworkSsid));
      String networkPassword = getHoldingRegisterString(ModbusRegisterNetwork + ModbusRegisterNetworkPassword);
      if (networkPassword.length() > 1 && !networkPassword.equals(config.getNetworkPassword())) {
        config.setNetworkPassword(networkPassword);
      }

      // RS485
      config.setRs485Type(getHoldingRegisterUint16(ModbusRegisterRs485 + ModbusRegisterRs485Type));
      config.setRs485Baudrate(getHoldingRegisterUint32(ModbusRegisterRs485 + ModbusRegisterRs485Baudrate));
      config.setRs485ToMqtt(getHoldingRegisterUint16(ModbusRegisterRs485 + ModbusRegisterRs485ToMqtt));

      // Modbus
      config.setModbusType(getHoldingRegisterUint16(ModbusRegisterModbus + ModbusRegisterModbusType));
      config.setModbusIp(getHoldingRegisterIp(ModbusRegisterModbus + ModbusRegisterModbusIp));
      config.setModbusPort(getHoldingRegisterUint16(ModbusRegisterModbus + ModbusRegisterModbusPort));

      // MQTT
      config.setMqttIp(getHoldingRegisterIp(ModbusRegisterMqtt + ModbusRegisterMqttIp));
      config.setMqttPort(getHoldingRegisterUint16(ModbusRegisterMqtt + ModbusRegisterMqttPort));
      config.setMqttInterval(getHoldingRegisterUint32(ModbusRegisterMqtt + ModbusRegisterMqttInterval));
      String mqttUser = getHoldingRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttUser);
      if (mqttUser.length() > 1 && !mqttUser.equals(config.getMqttUser())) {
        config.setMqttUser(mqttUser);
      }
      String mqttPassword = getHoldingRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttPassword);
      if (mqttPassword.length() > 1 && !mqttPassword.equals(config.getMqttPassword())) {
        config.setMqttPassword(mqttPassword);
      }
      config.setMqttBase(getHoldingRegisterString(ModbusRegisterMqtt + ModbusRegisterMqttBase));

      // Write config and reboot
      delay(10);
      config.writeToFile();
      io.writeToFile();
      board.reboot();

    }
  }

  /**
   * Get registers.
   *
   * @param   response  The reponse container
   * @param   type      The regiter type (holding or input)
   * @param   start     The starting address
   * @param   length    The number of regieter to read
   *
   * @return  1 on success, else 0
   */
  uint8_t getRegisters(int *response, uint8_t type, uint16_t start, uint16_t length) { // only for client
    uint8_t ret = 0;

    if (isRtuClient()) {
      if (_rtuClient.requestFrom(_clientServerId, type, start, length) == 0) {
          monitor.setWarning(String(_rtuClient.lastError()));
      } else {
        for (uint16_t index = 0; index < length; index++) {
          response[index] = _rtuClient.read();
        }
        ret = 1;
      }
    } else if (isTcpClient()) {
      if (!connectToTcpServer()) {
        ret = 0;
      } else {
        if (network.isEthernet()) {
          board.setFreeze();
          if (_tcpEthernetClient.requestFrom(_clientServerId, type, start, length) == 0) {
              monitor.setWarning(String(_tcpEthernetClient.lastError()));
          } else {
            for (uint16_t index = 0; index < length; index++) {
              response[index] = _tcpEthernetClient.read();
            }
            ret = 1;
          }
          board.unsetFreeze();

        } else if (network.isStandard()) {
          board.setFreeze();
          if (_tcpWifiClient.requestFrom(_clientServerId, type, start, length) == 0) {
              board.unsetFreeze();
              monitor.setWarning(String(_tcpWifiClient.lastError()));
          } else {
            for (uint16_t index = 0; index < length; index++) {
              response[index] = _tcpWifiClient.read();
            }
            ret = 1;
          }
          board.unsetFreeze();
        }
      }
      disconnectFromTcpServer();
    } else if (isRtuServer()) {
      for (uint16_t index = 0; index < length; index++) {
        if (type == INPUT_REGISTERS) {
          response[index] = _rtuServer.inputRegisterRead(start + index);
        } else if (type == HOLDING_REGISTERS) {
          response[index] = _rtuServer.holdingRegisterRead(start + index);
        }
      }
      ret = 1;
    } else if (isTcpServer()) {
      for (uint16_t index = 0; index < length; index++) {
        if (type == INPUT_REGISTERS) {
          response[index] = _tcpServer.inputRegisterRead(start + index);
        } else if (type == HOLDING_REGISTERS) {
          response[index] = _tcpServer.holdingRegisterRead(start + index);
        }
      }
      ret = 1;
    }

    return ret;
  }

public:
  OptaLinkerModbus(
    OptaLinkerVersion &_version,
    OptaLinkerState &_state,
    OptaLinkerMonitor &_monitor,
    OptaLinkerBoard &_board,
    OptaLinkerConfig &_config,
    OptaLinkerNetwork &_network,
    OptaLinkerIo &_io,
    OptaLinkerRs485 &_rs485) : 
    version(_version),
    state(_state),
    monitor(_monitor),
    board(_board),
    config(_config),
    network(_network),
    io(_io),
    rs485(_rs485),
    _tcpEthernetClient(_ethernetClient),
    _tcpWifiClient(_wifiClient) {}

  static const uint32_t RegisterOffsetAddress       = 10000;
  static const uint32_t RegisterOffsetFirmware      = 11000;
  static const uint32_t RegisterOffsetConfigDevice  = 20000;
  static const uint32_t RegisterOffsetConfigNetwork = 21000;
  static const uint32_t RegisterOffsetConfigRs485   = 22000;
  static const uint32_t RegisterOffsetConfigModbus  = 23000;
  static const uint32_t RegisterOffsetConfigMqtt    = 24000;
  static const uint32_t RegisterOffsetExpansion     = 30000;
  static const uint32_t RegisterOffsetInput         = 31000;
  static const uint32_t RegisterOffsetOutput        = 32000;

  uint8_t setup() {
    monitor.setAction(LabelModbusSetup);

    uint8_t ret = 1;
    uint8_t type = config.getModbusType();
    switch (type) {

      case ModbusType::ModbusRtuServer:
        monitor.setInfo(LabelModbusRtuServer);

        // Check Modbus RTU capability
        if (!board.isLite() && !rs485.isEnabled()) {
          prepareRS485();
          if (!_rtuServer.begin(config.getDeviceId(), config.getRs485Baudrate(), SERIAL_8E1)) {
            monitor.setWarning(LabelModbusBeginFail);
            ret = 0;
          } else {
            configureServerRegisters();
            setServerRegisters();
          }
        }
        break;

      case ModbusType::ModbusTcpServer:
        monitor.setInfo(LabelModbusTcpServer);

        _ethernetServer = EthernetServer(config.getModbusPort());
        _wifiServer = WiFiServer(config.getModbusPort());

        if (network.isEthernet()) {
          monitor.setInfo(LabelModbusEthernetServer);
          _ethernetServer.begin();
        } else {
          monitor.setInfo(LabelModbusWifiServer);
          _wifiServer.begin();
        }
        board.pingTimeout();

        if (!_tcpServer.begin()) {
          monitor.setWarning(LabelModbusBeginFail);
        } else {
          configureServerRegisters();
          setServerRegisters();
        }
        break;

      case ModbusType::ModbusRtuClient:
        monitor.setInfo(LabelModbusRtuClient);

        // Check Modbus RTU capability
        if (!board.isLite() && !rs485.isEnabled()) {
          prepareRS485();
          if (!_rtuClient.begin(config.getRs485Baudrate(), SERIAL_8E1)) {
            monitor.setWarning(LabelModbusBeginFail);
            ret = 0;;
          }
          _rtuClient.setTimeout(1000);
        }
       break;

      case ModbusType::ModbusTcpClient:
        monitor.setInfo(LabelModbusTcpClient);

        // Check distant server IP
        if (config.getModbusIp().toString().equals("0.0.0.0")) {
          ret = 0;
        }
        // Do not connect on setup but when required
        break;

      default:
        ret = 0;;
    }

    board.pingTimeout();

    if (ret == 0) {
      config.setModbusType(ModbusType::ModbusNone);
      monitor.setWarning(LabelModbusNone);
    }

    return 1;
  }

  uint8_t loop() {
    if (isRtuServer()) {
      _rtuServer.poll();
    } else if (isTcpServer()) {

      // Check if ehternet tcp client is trying to connect
      if (network.isEthernet()) {
        EthernetClient modbusEthernetClient = _ethernetServer.accept();
        if (modbusEthernetClient) {
          monitor.setInfo(LabelModbusClientConnect);
          _tcpServer.accept(modbusEthernetClient);

          while (modbusEthernetClient.connected()) {
            _tcpServer.poll();
            board.pingTimeout();
            if (state.getDuration() > 1000) { // use timer instead of freeze, for debug purpose...
                monitor.setWarning(String(LabelModbusClientKick));
                break;
            }

            _tcpServer.poll();
            board.pingTimeout();
          }
          modbusEthernetClient.stop();
        }
      // Check if wifi tcp client is trying to connect
      } else {
        WiFiClient modbusWifiClient = _wifiServer.accept();
        if (modbusWifiClient) {
          monitor.setInfo(LabelModbusClientConnect);
          _tcpServer.accept(modbusWifiClient);

          while (modbusWifiClient.connected()) {
            _tcpServer.poll();
            board.pingTimeout();
            if (state.getDuration() > 1000) { // use timer instead of freeze, for debug purpose...
                monitor.setWarning(String(LabelModbusClientKick));
                break;
            }
          }
          modbusWifiClient.stop();
        }
      }
    }

    // For server only
    if (isRtuServer() || isTcpServer()) {
      uint16_t offset = 0;
      int rcv = -1;

      ExpansionStruct *expansion = io.getExpansions();

      /*
       * Update Io <=> modbus
       */

      // Inputs
      for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
        for(uint8_t i = 0; i < io.getMaxInputNum(); i++) {
          if (expansion[e].exists && expansion[e].input[i].exists) {

            // Check changes to inputs from local then update HoldingRegisters, InputRegisters, DiscreteInputs
            offset = ModbusRegisterInput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength);
            if (expansion[e].input[i].update >= _updateLast) {
              //monitor.setInfo("Modbus registers update input " + String(expansion[e].input[i].uid) + " starting at offset " + offset);
              setRegisterIo(offset, expansion[e].input[i], 1);
              setDiscreteInput(expansion[e].input[i].uid, expansion[e].input[i].state);
            }

            // Check distant changes to intputs from HoldingRegisters then update local inputs. Manage only "input reset" from HoldingRegisters
            offset = ModbusRegisterInput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength) + ModbusRegisterIoPartialReset;
            rcv = getHoldingRegister(offset);
            if (rcv == 1) {
              io.resetIo(expansion[e].input[i]);
            }
          }
        }
      }

      // Outputs
      for (uint8_t e = 0; e < io.getExpansionsNum(); e++) {
        for(uint8_t i = 0; i < io.getMaxOutputNum(); i++) {
          if (expansion[e].exists && expansion[e].output[i].exists) {

            // Check changes to outputs from local then update HoldingRegisters, InputRegisters, Coils
            offset = ModbusRegisterOutput + (e * ModbusRegisterIoLength) +  (i * ModbusRegisterIoLength);
            if (expansion[e].output[i].update >= _updateLast) {
              //monitor.setInfo("Modbus registers update output " + String(expansion[e].output[i].uid) + " starting at offset " + offset);
              setRegisterIo(offset, expansion[e].output[i], 0);
              setCoil(expansion[e].output[i].uid, expansion[e].output[i].state);
            }

            // Check command to outputs from Coils
            offset = ModbusRegisterOutput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength) + ModbusRegisterIoState;
            rcv = getCoil(expansion[e].output[i].uid);
            if (rcv != -1 && rcv != getInputRegister(offset)) {
              io.setOutput(e, i, rcv);
            }

            // Check distant changes to intputs from HoldingRegisters then update local inputs. Manage only "input reset" from HoldingRegisters
            offset = ModbusRegisterOutput + (e * ModbusRegisterIoLength) + (i * ModbusRegisterIoLength) + ModbusRegisterIoPartialReset;
            rcv = getHoldingRegister(offset);
            if (rcv == 1) {
              io.resetIo(expansion[e].output[i]);
            }
          }
        }
      }
      _updateLast = state.getTime();

      // Check if "End of configuration" is received from HoldingRegisters
      if (getHoldingRegister(ModbusRegisterFirmware + ModbusRegisterConfigValidate) == 1) {
        parseServerHoldingRegisters();
      }
    }

    return 1;
  }

  uint8_t isRtuClient() {
    return config.getModbusType() == ModbusType::ModbusRtuClient;
  }

  uint8_t isTcpClient() {
    return config.getModbusType() == ModbusType::ModbusTcpClient;
  }

  uint8_t isRtuServer() {
    return config.getModbusType() == ModbusType::ModbusRtuServer;
  }

  uint8_t isTcpServer() {
    return config.getModbusType() == ModbusType::ModbusTcpServer;
  }

  void setServer(uint8_t server) {
    _clientServerId = server;
  }

  int getInputRegister(uint16_t offset) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.inputRegisterRead(offset);
    } else if (isTcpServer()) {
      ret = _tcpServer.inputRegisterRead(offset);
    }

    return ret;
  }

  int setInputRegister(uint16_t offset, uint16_t value) { // only for server
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.inputRegisterWrite(offset, value) == 0 ? -1 : 1;
    } else if (isTcpServer()) {
      ret = _tcpServer.inputRegisterWrite(offset, value) == 0 ? -1 : 1;
    } else {
      ret = 0;
    }

    return ret;
  }

  int getHoldingRegister(uint16_t offset) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.holdingRegisterRead(offset);
    } else if (isTcpServer()) {
      ret = _tcpServer.holdingRegisterRead(offset);
    } else if (isRtuClient()) {
      ret = _rtuClient.holdingRegisterRead(_clientServerId, offset);
    } else if (isTcpClient()) {
      if (connectToTcpServer()) {
        if (network.isEthernet()) {
          ret = _tcpEthernetClient.holdingRegisterRead(offset);
        } else if (network.isStandard()) {
          ret = _tcpWifiClient.holdingRegisterRead(offset);
        }
      }
      disconnectFromTcpServer();
    }

    return ret;
  }

  int setHoldingRegister(uint16_t offset, uint16_t value) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.holdingRegisterWrite(offset, value) == 0 ? -1 : 1;
    } else if (isTcpServer()) {
      ret = _tcpServer.holdingRegisterWrite(offset, value) == 0 ? -1 : 1;
    } else if (isRtuClient()) {
      ret = _rtuClient.holdingRegisterWrite(_clientServerId, offset, value) == 0 ? -1 : 1;
    } else if (isTcpClient()) {
      if (connectToTcpServer()) {
        if (network.isEthernet()) {
          ret = _tcpEthernetClient.holdingRegisterWrite(offset, value) == 0 ? -1 : 1;
        } else if (network.isStandard()) {
          ret = _tcpWifiClient.holdingRegisterWrite(offset, value) == 0 ? -1 : 1;
        }
      }
      disconnectFromTcpServer();
    }

    return ret;
  }

  int getDiscreteInput(uint16_t offset) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.discreteInputRead(offset);
    } else if (isTcpServer()) {
      ret = _tcpServer.discreteInputRead(offset);
    } else if (isRtuClient()) {
      ret = _rtuClient.discreteInputRead(_clientServerId, offset);
    } else if (isTcpClient()) {
      if (connectToTcpServer()) {
        if (network.isEthernet()) {
          ret = _tcpEthernetClient.discreteInputRead(offset);
        } else if (network.isStandard()) {
          ret = _tcpWifiClient.discreteInputRead(offset);
        }
      }
      disconnectFromTcpServer();
    }

    return ret;
  }

  int setDiscreteInput(uint16_t offset, uint8_t state) { // only for server
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.discreteInputWrite(offset, state) == 0 ? -1 : 1;
    } else if (isTcpServer()) {
      ret = _tcpServer.discreteInputWrite(offset, state) == 0 ? -1 : 1;
    }

    return ret;
  }

  int getCoil(uint16_t offset) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.coilRead(offset);
    } else if (isTcpServer()) {
      ret = _tcpServer.coilRead(offset);
    } else if (isRtuClient()) {
      ret = _rtuClient.coilRead(_clientServerId, offset);
    } else if (isTcpClient()) {
      if (connectToTcpServer()) {
        if (network.isEthernet()) {
          ret = _tcpEthernetClient.coilRead(offset);
        } else if (network.isStandard()) {
          ret = _tcpWifiClient.coilRead(offset);
        }
      }
      disconnectFromTcpServer();
    }

    return ret;
  }

  int setCoil(uint16_t offset, uint8_t state) {
    int ret = -1;
    if (isRtuServer()) {
      ret = _rtuServer.coilWrite(offset, state) == 0 ? -1 : 1;
    } else if (isTcpServer()) {
      ret = _tcpServer.coilWrite(offset, state) == 0 ? -1 : 1;
    } else if (isRtuClient()) {
      ret = _rtuClient.coilWrite(_clientServerId, offset, state) == 0 ? -1 : 1;
    } else if (isTcpClient()) {
      if (connectToTcpServer()) {
        if (network.isEthernet()) {
          ret = _tcpEthernetClient.coilWrite(offset, state) == 0 ? -1 : 1;
        } else if (network.isStandard()) {
          ret = _tcpWifiClient.coilWrite(offset, state) == 0 ? -1 : 1;
        }
      }
      disconnectFromTcpServer();
    }

    return ret;
  }

  uint8_t getInputRegisters(int *response, uint16_t start, uint16_t length) {
    return getRegisters(response, INPUT_REGISTERS, start, length);
  }

  uint8_t getHoldingRegisters(int *response, uint16_t start, uint16_t length) {
    return getRegisters(response, HOLDING_REGISTERS, start, length);
  }

  /**
   * Helpers
   */

  uint16_t getInputRegisterUint16(uint16_t start) {
    int response[1];
    return getRegisters(response, INPUT_REGISTERS, start, 1) == 0 ? 0 : (uint16_t)response[0];
  }

  uint16_t getHoldingRegisterUint16(uint16_t start) {
    int response[1];
    return getRegisters(response, HOLDING_REGISTERS, start, 1) == 0 ? 0 : (uint16_t)response[0];
  }

  void setRegisterUint16(uint16_t offset, uint32_t value) {
    setInputRegister(offset, value);
    setHoldingRegister(offset, value);
  }

  int16_t getInputRegisterInt16(uint16_t start) {
    int response[2];
    return getRegisters(response, INPUT_REGISTERS, start, 2) == 0 ? 0 : toInt16(response, 0);
  }

  int16_t getHoldingRegisterInt16(uint16_t start) {
    int response[2];
    return getRegisters(response, HOLDING_REGISTERS, start, 2) == 0 ? 0 : toInt16(response, 0);
  }

  void setRegisterInt16(uint16_t offset, int value) {
    setInputRegister(offset, value < 0 ? 0 : 1);
    setHoldingRegister(offset++, value < 0 ? 0 : 1);
    setInputRegister(offset, (uint16_t)abs(value));
    setHoldingRegister(offset, (uint16_t)abs(value));
  }

  uint32_t getInputRegisterUint32(uint16_t start) {
    int response[2];
    return getRegisters(response, INPUT_REGISTERS, start, 2) == 0 ? 0 : toUint32(response, 0);
  }

  uint32_t getHoldingRegisterUint32(uint16_t start) {
    int response[2];
    return getRegisters(response, HOLDING_REGISTERS, start, 2) == 0 ? 0 : toUint32(response, 0);
  }

  void setRegisterUint32(uint16_t offset, uint32_t value) {
    uint16_t high = (value >> 16) & 0xFFFF;
    uint16_t low  = value & 0xFFFF;

    setInputRegister(offset, high);
    setHoldingRegister(offset++, high);
    setInputRegister(offset, low);
    setHoldingRegister(offset, low);
  }

  String getInputRegisterString(uint16_t start) {
    int response[config.MaxStringLength + 1];
    return getRegisters(response, INPUT_REGISTERS, start, config.MaxStringLength + 1) == 0 ? String("") : toString(response, 0);
  }

  String getHoldingRegisterString(uint16_t start) {
    int response[config.MaxStringLength + 1];
    return getRegisters(response, HOLDING_REGISTERS, start, config.MaxStringLength + 1) == 0 ? String("") : toString(response, 0);
  }

  void setRegisterString(uint16_t offset, String str) {
    if (str.length() <= config.MaxStringLength) {
      setInputRegister(offset, str.length());
      setHoldingRegister(offset++, str.length());
      for (uint8_t n = 0; n < str.length(); n++) {
        setInputRegister(offset, str.c_str()[n]);
        setHoldingRegister(offset++, str.c_str()[n]);
      }
      setInputRegister(offset, '\0');
      setHoldingRegister(offset, '\0');
    }
  }

  IPAddress getInputRegisterIp(uint16_t start) {
    int response[4];
    return getRegisters(response, INPUT_REGISTERS, start, 4) == 0 ? IPAddress(0, 0, 0, 0) : toIp(response, 0);
  }

  IPAddress getHoldingRegisterIp(uint16_t start) {
    int response[4];
    return getRegisters(response, HOLDING_REGISTERS, start, 4) == 0 ? IPAddress(0, 0, 0, 0) : toIp(response, 0);
  }

  void setRegisterIp(uint16_t offset, IPAddress Ip) {
    for (uint8_t n = 0; n < 4; n++) {
      setInputRegister(offset, Ip[n]);
      setHoldingRegister(offset++, Ip[n]);
    }
  }

  int16_t toInt16(int *response, uint16_t start) {
    return response[start] == 0 ? 0 - response[start + 1] : response[start + 1];
  }

  uint32_t toUint32(int *response, uint16_t start) {
    return (uint32_t)(response[start] << 16 | response[start + 1]);
  }

  String toString(int *response, uint16_t start) {
    char ret[config.MaxStringLength];
    for (uint8_t i = 0; i < response[start]; i++) {
      ret[i] = (char)response[start + 1 + i];
    }
    ret[response[start]] = '\0';

    return String(ret);
  }

  IPAddress toIp(int *response, uint16_t start) {
    return IPAddress(response[start], response[start + 1], response[start + 2 ], response[start + 3 ]);
  }

}; // class OptaLinkerModbus

} // namespace optalinker

#endif // #ifndef OPTALINKER_MODBUS_H