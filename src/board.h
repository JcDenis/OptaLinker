/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_BOARD_H
#define OPTALINKER_BOARD_H

#include <mbed.h>
#include <drivers/Watchdog.h> //?
#include <opta_info.h>

#include "OptaLinkerBase.h"

// Board info
OptaBoardInfo *boardInfo();

namespace optalinker {

class OptaLinkerState;
class OptaLinkerMonitor;

class OptaLinkerBoard : public OptaLinkerBase {

private:
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;

  /**
   * The board type.
   */
  uint8_t _boardType = BoardType::BoardNone;

  /**
   * Memorized previous green LED state.
   */
  uint8_t _previousLedGreen = 0;

  /**
   * Memorized previous red LED state.
   */
  uint8_t _previousLedRed = 0;

  /**
   * Memorized previous blue LED state.
   */
  uint8_t _previousLedBlue = 0;

  /**
   * Last heartbeat launch time.
   */
  uint32_t _heartbeatLast = 0;

  /**
   * Current heartbeat step.
   */
  uint8_t _heartbeatStep = 0;

  /**
   * Memorized button push state.
   */
  uint32_t _buttonStart = 0;

  /**
   * Current button push duration.
   */
  uint32_t _buttonDuration = 0;

  /**
   * Last button duration.
   */
  uint32_t _buttonLast = 0;

  /**
   * Watchdog timeout.
   *
   * Opta baord max timrout should be 32720.
   * Most long operation such as network/mqtt connection timeout can be 30000.
   * Note: Opta boad does not support to stop watchdog.
   */
  uint32_t _timeout = 31000;

  /**
   * Freeze mode start timestamp.
   */
  uint32_t _freezeStart[10];

  /**
   * Freeze mode level.
   *
   * Number of time setFreeze() was called without unsetFreeze().
   */
  uint16_t _freezeLevel = 0;

public:
	OptaLinkerBoard(OptaLinkerState &_state, OptaLinkerMonitor &_monitor) : state(_state), monitor(_monitor) {}

  uint8_t setup() {
    // Display board setup message
    monitor.setAction(LabelBoardSetup);

    // Setup watchdog timeout
    if (_timeout > mbed::Watchdog::get_instance().get_max_timeout()) {
      _timeout = mbed::Watchdog::get_instance().get_max_timeout();
    }
    mbed::Watchdog::get_instance().start(_timeout);
    monitor.setInfo(LabelBoardTimeout + String(_timeout));

    // Retrieve board info
    OptaBoardInfo *info = boardInfo();
    if (info->magic == 0xB5) {
      if (info->_board_functionalities.wifi == 1) {
        _boardType = BoardType::BoardWifi;
      } else if (info->_board_functionalities.rs485 == 1) {
        _boardType = BoardType::BoardRs485;
      } else if (info->_board_functionalities.ethernet == 1) {
        _boardType = BoardType::BoardLite;
      }
    }

    // Display board name
    monitor.setInfo(LabelBoardName + getName());

    return isNone() ? 0 : 1;
  }

  uint8_t loop() {
    // Ping watchdog on each loop
    pingTimeout();

    // Heartbeat using LEDs
    if (state.getTime() - _heartbeatLast > 10000) {

      switch (_heartbeatStep) {
        case 0:
          _heartbeatStep = 1;
          _previousLedGreen = isGreen();
          _previousLedRed = isRed();
          setGreen(0);
          setRed(0);

          break;

        case 1:
          if (state.getTime() - _heartbeatLast > 10150) {
            _heartbeatStep = 2;
            setGreen(1);
            setRed(1);
          }
          break;

        case 2:
          if (state.getTime() - _heartbeatLast > 10200) {
            _heartbeatStep = 3;
            setGreen(0);
            setRed(0);
          }

          break;

        case 3:
          if (state.getTime() - _heartbeatLast > 10350) {
            _heartbeatStep = 0;
            _heartbeatLast = state.getTime();
            setGreen(_previousLedGreen);
            setRed(_previousLedRed);
          }

          break;
      }
    }

    // Button duration
    if (isPushed()) {
      if (_buttonStart == 0) {
        delay(1);
        _buttonStart = state.getTime();
      }
      _buttonDuration = state.getTime() - _buttonStart;

    } else if (_buttonStart > 0 && _buttonDuration > 0) {
      _buttonStart = 0;
      _buttonLast = _buttonDuration;
      monitor.setInfo(LabelBoardButtonDuration + String(_buttonDuration));

      return _buttonDuration;
    }

    return 1;
  }

  /**
   * Get the board type.
   *
   * @see   OptaLinker::BoardType
   *
   * @return  The board type
   */
  uint8_t getType() {

    return _boardType;
  }

  /**
   * Check if board type is unknow.
   *
   * @see OptaLinker::BoardType
   *
   * @return  1 if it is unknown, else 0
   */
  uint8_t isNone() {

    return _boardType == BoardType::BoardNone ? 1 : 0;
  }

  /**
   * Check if board type is Opta Lite.
   *
   * @see   OptaLinker::BoardType
   *
   * @return  1 if it is Opta Lite, else 0
   */
  uint8_t isLite() {

    return _boardType == BoardType::BoardLite ? 1 : 0;
  }

  /**
   * Check if board type is Opta RS485.
   *
   * @see   OptaLinker::BoardType
   *
   * @return  1 if it is Opta RS485, else 0
   */
  uint8_t isRs485() {

    return _boardType == BoardType::BoardRs485 ? 1 : 0;
  }

  /**
   * Check if board type is Opta Wifi.
   *
   * @see   OptaLinker::BoardType
   *
   * @return  1 if it is Opta Wifi, else 0
   */
  uint8_t isWifi() {

    return _boardType == BoardType::BoardWifi ? 1 : 0;
  }

  /**
   * Get board human readable name.
   *
   * @return The board real name
   */
  String getName() {
    String name;
    switch(_boardType) {
      case BoardType::BoardLite:
        name = LabelBoardNameLite;
        break;
      case BoardType::BoardRs485:
        name = LabelBoardNameRs485;
        break;
      case BoardType::BoardWifi:
        name = LabelBoardNameWifi;
        break;
      default:
        name = LabelBoardNameNone;
      }

      return name;
  }

  /**
   * Set green LED state.
   *
   * @param   set 0 = off, 1 = on
   */
  void setGreen(uint8_t set) {
    if (set < 2) {
      digitalWrite(LEDG, set ? HIGH : LOW);
    }
  }

  /**
   * Check if green LED is on.
   *
   * @return  The green LED state, 0 = off, 1 = on
   */
  uint8_t isGreen() {

    return digitalRead(LEDG) == HIGH ? 1 : 0;
  }

  /**
   * Set red LED state.
   *
   * @param   set 0 = off, 1 = on
   */
  void setRed(uint8_t set) {
    if (set < 2) {
      digitalWrite(LEDR, set ? HIGH : LOW);
    }
  }

  /**
   * Check if red LED is on.
   *
   * @return  The red LED state, 0 = off, 1 = on
   */
  uint8_t isRed() {

    return digitalRead(LEDR) == HIGH ? 1 : 0;
  }

  /**
   * Set blue LED state.
   *
   * @param   set 0 = off, 1 = on
   */
  void setBlue(uint8_t set) {
    if (set < 2) {
      digitalWrite(LEDB, set ? HIGH : LOW);
    }
  }

  /**
   * Check if blue LED is on.
   *
   * @return  The blue LED state, 0 = off, 1 = on
   */
  uint8_t isBlue() {

    return digitalRead(LEDB) == HIGH ? 1 : 0;
  }

  /**
   * Check user button state.
   *
   * @return  The button state, 1 = pressed, else 0
   */
  uint8_t isPushed() {
    return digitalRead(BTN_USER) == LOW ? 1 : 0;
  }

  /**
   * Check if last user button pressed duration is between to values.
   *
   * @param   min     The minimum time
   * @param   max     The maximum time
   * @param   reset   Reset memorized duration if the check is true
   *
   * @return  1 if push duration is in the window, else 0
   */
  uint8_t isPushDuration(uint16_t min, uint16_t max, uint8_t reset = true) {
    if (_buttonLast > min && _buttonLast < max) {
      if (reset) {
        _buttonLast = 0;
      }
      return 1;
    }

    return 0;
  }

  /**
   * Get watchdog timeout value.
   *
   * @return The timeout value.
   */
  uint32_t getTimeout() {

    return _timeout;
  }

  /**
   * Ping watchdog to extend timeout.
   */
  void pingTimeout() {
    mbed::Watchdog::get_instance().kick();
  }

  /**
   * Enter freeze mode.
   *
   * This do not freeze board,
   * but set a state to announce there is a time consuming operation.
   * (netowrk connection...)
   */
  void setFreeze() {
    // perform raise only if not already in long timeout
    if (_freezeLevel < 1) {
      state.setType(StateType::StateFreeze);
      setRed(1);
      setGreen(1);

      _freezeStart[_freezeLevel] = millis();
    }
    _freezeLevel++;
    monitor.setWarning(LabelBoardFreeze + String(" (") + _freezeLevel + ")");
    pingTimeout();
  }

  /**
   * Exit freeze mode.
   *
   * If multiple setFreeze() have been set, 
   * this decrease from one level till it is being in lower timeout. (level 0)
   */
  void unsetFreeze() {
    // perform lower only if not already in short timeout
    if (_freezeLevel > 0) {
      state.setType(StateType::StateRun);
      setRed(0);
      setGreen(0);

      _freezeLevel--;
      monitor.setInfo(LabelBoardUnfreeze + String((millis() - _freezeStart[_freezeLevel])) + String(" (") + _freezeLevel + ")");
    }
    pingTimeout();
  }

  /**
   * Stop execution.
   *
   * This does not stop the loop, but stop library execution.
   */
  uint8_t stop() {
    // Display stop message
    monitor.setWarning(LabelBoardStop);

    // Set fixed red LED
    setGreen(0);
    setRed(1);
    setBlue(0);

    // Change process state
    state.setType(StateType::StateStop);

    return state.getType();
  }

  /**
   * Reboot device.
   */
  void reboot() {
    // Display reboot message
    monitor.setAction(LabelBoardReboot);

    // Blink LEDs
    uint8_t on = 1;
    for (int i = 0; i < 10; i++) {
      on = on ? 0 : 1;
      setRed(on ? 1 : 0);
      setGreen(on ? 0 : 1);
      delay(100);
    }

    // Reboot
    NVIC_SystemReset();
  }

}; // class OptaLinkerState

} // namespace optalinker

#endif // #ifndef OPTALINKER_BOARD_H