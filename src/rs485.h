/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_RS485_H
#define OPTALINKER_RS485_H

#include <ArduinoRS485.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;

/**
 * OptaLinker Library RS485 module.
 *
 * Manage RS485 transmissions.
 */
class OptaLinkerRs485 : public OptaLinkerModule {

private:
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;

  uint8_t _sleep = 1;
  String _received = "";

  /**
   * Prepare RS485 transmission.
   */
  void prepare() {
    auto bitduration{ 1.f / config.getRs485Baudrate() };
    auto wordlen{ 9.6f };  // required for modbus, OR 10.0f depending on the channel configuration for rs485
    //auto wordlen{ 10.0f };  // required for modbus, OR 10.0f depending on the channel configuration for rs485
    auto preDelayBR{ bitduration * wordlen * 3.5f * 1e6 };
    auto postDelayBR{ bitduration * wordlen * 3.5f * 1e6 };
    RS485.setDelays(preDelayBR, postDelayBR);
  }

public:
  OptaLinkerRs485(OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerConfig &_config) : monitor(_monitor), board(_board), config(_config) {}

  uint8_t setup() {
    // Disable RS485 feature
    if (board.isLite() || board.isNone() || config.getRs485Type() == Rs485Type::Rs485None) {
      disable();

      return 1;
    }

    monitor.setMessage(LabelRs485Setup, MonitorAction);
    monitor.setMessage(LabelRs485Baudrate + String(config.getRs485Baudrate()), MonitorInfo);

    RS485.begin(config.getRs485Baudrate());
    prepare();
    //RS485.receive();

    board.pingTimeout();

    return 1;
  }

  /**
   * Check if board is setup as RS485 receiver.
   *
   * @return  1 if receiver, else 0
   */
  uint8_t isReceiver() {

    return isEnabled() && config.getRs485Type() == Rs485Type::Rs485Receiver ? 1 : 0;
  }

  /**
   * Check if board is setup as RS485 sender.
   *
   * @return  1 if sender, else 0
   */
  uint8_t isSender() {

    return isEnabled() && config.getRs485Type() == Rs485Type::Rs485Sender ? 1 : 0;
  }

  /**
   * Check if RS port is not used.
   *
   * @return  1 if sleeping, else 0
   */
  uint8_t isSleeping() {

    return _sleep;
  }

  /**
   * Grab incoming message.
   *
   * @return  1 if a message arrives, else 0
   */
  uint8_t incoming() {
    if (isReceiver() && isSleeping()) {

      _sleep = 0;
      RS485.receive();
      if (RS485.available() > 0) {
        char r_message[40]; // Max message size is 40
        int r_index = 0;
        uint32_t wait = millis() + 5;
        while (1) {
          delay(50); // Wait for more incoming data
          int r_int = RS485.read();

          // Check if we goes faster than serial
          if (r_int != -1) {
            char r_char = (char) (byte) r_int;

            // Check end of message // TODO: end char choice
            if (r_char == '\n' || r_char == '\r') {
              r_message[r_index] = '\0';

              break;
            }

            r_message[r_index] = r_char;
            r_index++;
          } else {
            // force stop reading after 150 ms
            if (millis() - wait > 1000) {//watchdog.getTimeout()) {
              //monitor.setMessage("Receiving partial RS485 message", MonitorWarning);
              r_message[r_index] = '\0';

              break;
            }
          }
        }
        board.pingTimeout();
        RS485.noReceive();

        if (r_index) {
          _received = String(r_message);
          _sleep = 1;

          return 1;
        }
      }
      _sleep = 1;
    }

    return 0;
  }

  /**
   * Get received message.
   *
   * @return  The received message as String
   */
  String received() {
    String ret = "";
    if (_received.length()) {
      ret = _received;
      _received = "";
    }

    return ret;
  }

  /**
   * Send a message.
   *
   * @param   The message to send as string
   *
   * @return  1 if message send, else 0 (and you must retry)
   */
  uint8_t send(String msg) {
    if (isSender() && isSleeping()) {
      //monitor.setMessage("Sending RS485 message", MonitorInfo);

      _sleep = 0;

      RS485.beginTransmission();
      RS485.print(msg);
      RS485.endTransmission();

      _sleep = 1;

      return 1;
    }

    return 0;
  }

}; // class OptaLinkerRs485

} // namespace optalinker

#endif // #ifndef OPTALINKER_RS485_H