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

unsigned long lastSend = 0;

void setup() {

  /**
   * Disable configuration from flash memory and web server 
   * in order to force configuration values from here for this example.
   */
  linker.config->disable();

  // For this example we do not need that
  linker.clock->disable();
  linker.mqtt->disable();
  linker.web->disable();

  /**
   * Force configuration for RS485 mode to "receiver".
   * Both sender and receiver must have same RS485 bauderate.
   */
  linker.config->setRs485Type(Rs485Receiver);
  linker.config->setRs485Baudrate(19200);

  /*
   * Start standard class setup.
   * 
   * On web page configuration, you must set on both devices same RS485 baudrate.
   */
  if (linker.setup()) {

    /**
     * Check if board can use RS485
     */
    if (linker.board->isLite()) {
      Serial.println("Sorry your board does not have RS485 !");
      while(1){}
    }
  
    /**
     * Run class loop in a dedicated thread
     */
    linker.thread();
  }
}

void loop() {

  /**
  * Check if class is already running
  */
  if (linker.state->isStop()) {

    Serial.println("Library is stopped");
    while(1){}

  } else {

    /**
    * Check if a message is received.
    */
    if (linker.rs485->incoming()) {

      /**
      * Read last mesasge.
      *
      * Read the message, erase it and allow opta to receive another message.
      */
      String message = linker.rs485->received();
      linker.monitor->setMessage("RX: " + message, MonitorReceive);
    }
  }

  yield();
}
