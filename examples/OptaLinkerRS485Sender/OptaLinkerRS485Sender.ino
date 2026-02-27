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
unsigned long countSend = 0;

void setup() {

  /**
   * Disable configuration from flash memory and web server 
   * in order to force configuration value for this example.
   */
  linker.config->disable();

  // For this example we do not need that
  linker.clock->disable();
  linker.mqtt->disable();
  linker.web->disable();

  /**
   * Force RS485 mode to "sender".
   * Both sender and receiver must have same RS485 bauderate.
   */
  linker.config->setRs485Type(Rs485Sender);
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
    * Every 10 seconds, send device id as message on serial port
    */
  if (millis() - lastSend > 10000) {
    lastSend = millis();

    /**
    * Check if class is already running
    */
    if (linker.state->isStop()) {

      Serial.println("Library is stopped");
      while(1){}

    } else {

      String message = linker.board->getName() + ", test " + countSend++;
      linker.monitor->setMessage("TX: " + message, MonitorReceive);

      /**
       * Try to send message till serial is avalable
       */
      while (!linker.rs485->send(message)) {

        /**
         * Wait 2 second, then stop trying to send message
         */
        if (millis() - lastSend > 2000) {
          linker.monitor->setMessage("Failed to send message", MonitorReceive);
          break;
        }
      }
    }
  }

  /**
   * Required for threaded loop
   */
  yield();
}
