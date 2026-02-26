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

uint32_t lastPoll = 0;

void setup() {

  /**
   * Setup library.
   */
  if (linker.setup()) {

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
    if (!linker.modbus->isEnabled() || (!linker.modbus->isRtuServer() && !linker.modbus->isTcpServer())) {
      linker.monitor->setMessage(">>> You must configure this device as modbus server <<<", MonitorReceive);

      while(1) {}
    }

    /**
     * There are nothing to do in loop for Modbus Server.
     * Note: If Modbus RTU is used, RS485 baudrate of client and server must be the same.
     */

    // Poll itself every 30000
    if (millis() - lastPoll > 25000) {
      uint16_t addr = 0;
      lastPoll = millis();

      // Get Expansion 0 (main board) name
      addr = ModbusRegisterExpansion + ModbusRegisterExpansionName;
      linker.monitor->setMessage(">>> Reading board name starting at offset : " + String(addr));
      linker.monitor->setMessage(linker.modbus->getInputRegisterString(addr), MonitorReceive);
    }
  }

  // As OpaLinker use thread, main loop MUST use yield()
  yield();
}
