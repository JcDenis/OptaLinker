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

    Serial.println("");
    Serial.println("Library is stopped !");
    Serial.println("");
    while(1){}

  } else {

   // ...
  }

  // As OpaLinker use thread, main loop must use yield()
  yield();
}
