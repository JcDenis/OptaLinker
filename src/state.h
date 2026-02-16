/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_STATE_H
#define OPTALINKER_STATE_H

#include "OptaLinkerBase.h"

namespace optalinker {

class OptaLinkerState : public OptaLinkerBase {

private:
  /**
   * Current state of the process.
   */
  uint8_t _type = StateType::StateStop;

  /**
   * Current odd/even loop.
   */
  uint8_t _odd = 0;

  /**
   * Current loop start time.
   */
  uint32_t  _time = 0;

public:
  OptaLinkerState() {}

  uint8_t setup() {
    _type = StateType::StateSetup;
    _time = millis();

    return 1;
  }

  uint8_t loop() {
    _type = StateType::StateRun;
    _odd = _odd == 1 ? 0 : 1;
    _time = millis();

    return 1;
  }

  /**
   * Get loop start time.
   *
   * @return The loop start time
   */
  uint32_t getTime() {

    return _time;
  }

  /**
   * Check loop even/odd state.
   *
   * @return  1 if odd loop, 0 is even loop
   */
  uint8_t isOdd() {

    return _odd;
  }

  /**
   * Set process state.
   *
   * @param   type   The process state type to set
   */
  void setType(uint8_t type) {
    if (type < 4) {
      _type = type;
    }
  }

  /**
   * Get current process state.
   *
   * @return The current process StateType
   */
  uint8_t getType() {

    return _type;
  }

  /**
   * Check if process is stopped.
   *
   * @return  1 if process is StateStop, else 0
   */
  uint8_t isStop() {

    return _type == StateType::StateStop ? 1 : 0;
  }

  /**
   * Check if process is freezed.
   *
   * @return  1 if process is StateFreeze, else 0
   */
  uint8_t isFreeze() {

    return _type == StateType::StateFreeze ? 1 : 0;
  }

  /**
   * Check if process is runing.
   *
   * @return  1 if process is StateRun, else 0
   */
  uint8_t isRun() {

    return _type == StateType::StateRun ? 1 : 0;
  }

  uint32_t getDuration() {
    return millis() - _time;
  }

}; // class OptaLinkerState

} // namespace optalinker

#endif // #ifndef OPTALINKER_STATE_H