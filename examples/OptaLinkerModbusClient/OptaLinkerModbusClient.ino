/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * see README.md file
 */

#include <OptaLinker.h>

using namespace optalinker;

OptaLinker linker = OptaLinker::getInstance();

uint32_t _lastPoll = 0;
uint8_t _lastState = 0;

/**
 * Serer device password, to validate its configuration edition
 */
String _serverPassword = "admin";

void setup() {

  /**
   * Setup library.
   */
  if (linker.setup()) {

      /**
      * Set second Opta as modbus server.
      * If modbus RTU is used, set distant server id here :
      */
      linker.modbus->setServer(98);

    /**
     * Start library loop in a dedicated thread.
     */
    linker.thread();
  }
}

void loop() {

  /**
   * Check if library is not stopped.
   */
  if (linker.state->isStop()) {
    Serial.println("\nLibrary is stopped !\n");

    while(1){}

  } else {

    // Check device configuration
    if (!linker.modbus->isEnabled() || linker.modbus->isRtuServer() || linker.modbus->isTcpServer()) {
      // Note: If Modbus RTU is used, RS485 baudrate of client and server must be the same
      Serial.println(">>> You must configure this device as modbus client <<<");

      while(1) {}
    }

    uint16_t addr = 0;

    // Poll second Opta every 30000
    if (millis() - _lastPoll > 30000) {
      _lastPoll = millis();

      int response[100];

      addr = ModbusRegisterInput + (0 * ModbusRegisterIoLength) + (0 * ModbusRegisterIoLength);
      Serial.println(">>> Reading Input Registers expansion 0 ouput 0 brut values : " + String(addr) + " = ");
      if (linker.modbus->getInputRegisters(response, addr, ModbusRegisterIoLength)) {
        for (uint8_t v = 0; v < ModbusRegisterIoLength; v++) {
          Serial.println(String("E0 O0 V") + v + " : " + String((uint16_t)response[v]));
        }

        // expansion 0 input 1 state
        addr = ModbusRegisterInput + (0 * ModbusRegisterIoLength) + (1 * ModbusRegisterIoLength) + ModbusRegisterIoState;
        Serial.println(">>> Reading Input Register : E0 I1 V3 : I0.1 state : " + String(addr) + " = ");
        Serial.println(String(linker.modbus->getInputRegisterUint16(addr)));
        // expansion 0 output 2 state
        addr = ModbusRegisterOutput + (0 * ModbusRegisterIoLength) + (2 * ModbusRegisterIoLength) + ModbusRegisterIoState;
        Serial.println(">>> Reading Output Register : E0 O2 V3 : O0.2 state : " + String(addr) + " = ");
        Serial.println(String(linker.modbus->getInputRegisterUint16(addr)));

        // Switch expansion 0 ouput 2 every 30s
        _lastState = _lastState == 0 ? 1 : 0;
        Serial.println(">>> Writing Coil 2 : main board output 3 : E0 O2 : O0.2 : " + String(_lastState));
        if (linker.modbus->setCoil(2, _lastState) == -1) {
          Serial.println("failed to update coil");
        } else {
          Serial.println("coil updated");
        }

      } else {
        Serial.println(">>> Failed to get modbus registers");
      }

      Serial.println(">>> End of poll");
    }

    // Send "modbus" to serial monitor to update modbus server configuration
    if (linker.monitor->hasIncoming() && linker.monitor->getIncoming().equals("modbus")) {

      // Change device timeOffset
      addr = ModbusRegisterDevice + ModbusRegisterTimeOffset;
      Serial.println(">>> Writing time offset : " + String(addr));
      linker.modbus->setRegisterInt16(addr, _lastState);

      // Set password for verifiation
      addr = ModbusRegisterFirmware + ModbusRegisterConfigPassword;
      Serial.println(">>> Writing password verification : " + String(addr));
      linker.modbus->setRegisterString(addr, String(_serverPassword));

      // Set end of configuration
      addr = ModbusRegisterFirmware + ModbusRegisterConfigValidate;
      Serial.println(">>> Writing config end : " + String(addr));
      linker.modbus->setRegisterUint16(addr, 1);

    }
  }

  // As OpaLinker use thread, main loop MUST use yield()
  yield();
}
