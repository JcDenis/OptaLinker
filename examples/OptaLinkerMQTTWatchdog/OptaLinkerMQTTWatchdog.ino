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
// Last boot timestamp
uint32_t _lastBoot = 0;
// Current timestamp
uint32_t _now = 0;
// Number of cycle where server watchdog fail
uint8_t _countDead = 0;

void setup() {

  /**
   * It is possible to disable some OptaLinker modules here.
   * linker.mqtt->disable();
   */

  // Setup library
  if (linker.setup()) {

    /*
     * It is possible to setup other things here
     */

    // Set boot timestamp
    _lastBoot = millis();

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
  
  _now = millis();

  // Check every 60000
  if ((_now - _lastPoll) > 60000) {
    _lastPoll = _now;
    // Check MQTT distant server watchdog
    if (linker.mqtt->isWatched() == 0) {
      linker.monitor->setMessage("> Server is dead !", MonitorFail);
      _countDead = _countDead + 1;

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
    } else {
      // Server respond, reset counter
      _countDead = 0;
    }
  
    // Reboot device if server watchdog is not received for 10 cycles (10 minutes)
    if (_countDead > 10) {
      linker.monitor->setMessage("> Rebooting device : server dead", MonitorAction);
      linker.board->reboot();
    }

    // Here it is how to force reboot every day
    if ((_now - _lastBoot) > 86400000) {
      linker.monitor->setMessage("> Rebooting device : daily", MonitorAction);
      linker.board->reboot();
    }
  }

  // As OpaLinker uses threads, main loop MUST use yield()
  yield();
}
