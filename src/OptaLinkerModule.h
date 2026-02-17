/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_MODULE_H
#define OPTALINKER_MODULE_H

#include <Arduino.h>

#include "define.h"
#include "common.h"

namespace optalinker {

/**
 * OptaLinker base modules class.
 *
 * All library modules extend this class
 */
class OptaLinkerModule {

private:

  /**
   * Memorized if this OptaLinker libreary module is enabled.
   */
  uint8_t _isEnabled = 1;

protected:

  /**
   * Internal lock feature.
   */
  uint8_t _isLock = 0;

  /**
   * Active lock feature.
   */
  void lock() {
    while(_isLock) {
      delay(1);
    }
    _isLock = 1;
  }

  /**
   * Deactive lock feature.
   */
  void unlock() {
    _isLock = 0;
  }

public:
	virtual ~OptaLinkerModule() {}

  /**
   * OptaLinker library module setup.
   *
   * This is called in the main OptaLinker class.
   */
  uint8_t setup() {

    return 1;
  }

  /**
   * OptaLinker library module loop.
   *
   * This is called in the main OptaLinker class.
   */
  uint8_t loop() {

    return 1;
  }

  /**
   * Disable class features.
   *
   * By default, class is enabled.
   * Once disbaled, a class can NOT be re-enabled.
   */
  void disable() {
    _isEnabled = 0;
  }

  /**
   * Check if a class is enabled.
   *
   * @return  1 if it is enabled, else 0
   */
  uint8_t isEnabled() {

    return _isEnabled;
  }

}; // class OptaLinkerModule

} // namespace optalinker

#endif // #ifndef OPTALINKER_BASE_H