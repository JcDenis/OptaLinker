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

  } else {

   // Put personnal code here
  }

  // As OpaLinker uses threads, main loop MUST use yield()
  yield();
}
