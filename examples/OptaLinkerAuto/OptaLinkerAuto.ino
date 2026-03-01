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

// Use OptaLinker namspace
using namespace optalinker;

// Get OptaLinker instance
OptaLinker linker = OptaLinker::getInstance();

void setup() {

  // Setup library.
  if (linker.setup()) {

    // Start library loop in a dedicated thread.
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

    /**
     * Read board and expansion input and report their states to the outputs
     */
    ExpansionStruct *expansion = linker.io->getExpansions();
    for (uint8_t e = 0; e < linker.io->getExpansionsNum(); e++) {
      if (expansion[e].exists) {
        for (uint8_t i = 0; i < linker.io->getMaxInputNum(); i++) {
          if (expansion[e].input[i].exists) {
            linker.io->setOutput(e, i, linker.io->getInput(e, i, IoFieldState));
          }
        }
      }
    }
  }

  // As OpaLinker use thread, main loop must use yield()
  yield();
}
