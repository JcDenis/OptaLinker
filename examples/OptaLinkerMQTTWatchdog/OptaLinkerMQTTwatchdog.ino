/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * see README.md file
 */

// Include OptaLinker library
#include <OptaLinker.h>

// Use OptaLinker namespace
using namespace optalinker;

// Get OptaLinker instance
OptaLinker linker = OptaLinker::getInstance();

// Last poll timestamp
uint32_t _lastPoll = 0;

void setup() {

  /**
   * It is possible to disable some OptaLinker modules here.
   */

  // Setup library
  if (linker.setup()) {

    /*
     * It is possible to setup other things here
     */

    // Start library loop in a dedicated thread. Hightly recommanded.
    linker.thread();
  }
}

void loop() {

  // Check if library is not stopped.
  if (linker.state->isStop()) {

    Serial.println("");
    Serial.println("Library is stopped !");
    Serial.println("");
    while(1){}

  }

  // Check every 60000
  if (millis() - _lastPoll > 60000) {
    _lastPoll = millis();
    // Check MQTT distant server watchdog
    if (linker.mqtt->isWatched() == 0) {
      linker.monitor->setMessage("> Server is dead !", MonitorFail);

      /**
      * Loop through board outputs to set them to 0 if distant server is dead.
      */
      ExpansionStruct *expansion = linker.io->getExpansions();
      for (uint8_t e = 0; e < linker.io->getExpansionsNum(); e++) {
        if (expansion[e].exists) {
          for (uint8_t i = 0; i < linker.io->getMaxOutputNum(); i++) {
            if (expansion[e].output[i].exists) {
              linker.io->setOutput(e, i, 0);
            }
          }
        }
      }
    }
  }

  // As OpaLinker uses threads, main loop MUST use yield()
  yield();
}
