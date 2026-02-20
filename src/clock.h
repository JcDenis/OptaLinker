/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_TIME_H
#define OPTALINKER_TIME_H

#include <Ethernet.h>
#include <mbed_mktime.h>
#include <NTPClient.h>
#include <WiFi.h>

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerState;
class OptaLinkerMonitor;
class OptaLinkerBoard;
class OptaLinkerConfig;
class OptaLinkerNetwork;

class OptaLinkerClock : public OptaLinkerModule {

private:
  OptaLinkerState &state;
  OptaLinkerMonitor &monitor;
  OptaLinkerBoard &board;
  OptaLinkerConfig &config;
  OptaLinkerNetwork &network;

  /**
   * Memorized time update state.
   */
  uint8_t _isUpdated = 0;

  /**
   * Last time update attempt.
   */
  uint32_t _lastPoll = 0;

  /**
   * Update time using standard UDP object.
   *
   * @param   udp   The UDP object
   */
  void synchronizeFromUdp(UDP &udp) {
      board.setFreeze();
      WiFiUDP wifiUdpClient;
      NTPClient timeClient(udp, config.getTimeServer().c_str(), config.getTimeOffset() * 3600, 0);
      timeClient.begin();
      if (!timeClient.update() || !timeClient.isTimeSet()) {
        monitor.setMessage(LabelClockUpdateFail, MonitorFail);
      } else {
        const unsigned long epoch = timeClient.getEpochTime();
        set_time(epoch);
        monitor.setMessage(LabelClockUpdateSuccess + timeClient.getFormattedTime(), MonitorInfo);
        _isUpdated = 1;
      }
      board.unsetFreeze();
  }

public:
  OptaLinkerClock(OptaLinkerState &_state, OptaLinkerMonitor &_monitor, OptaLinkerBoard &_board, OptaLinkerConfig &_config, OptaLinkerNetwork &_network) : state(_state), monitor(_monitor), board(_board), config(_config), network(_network) {}

  uint8_t setup() {
    monitor.setMessage(LabelClockSetup, MonitorAction);

    synchronizeRtc();

    return 1;
  }

  uint8_t loop() {
    // If time update failed during setup, try every hour until success
    if (!_isUpdated && ((state.getTime() - 3600000) < _lastPoll)) {
      _lastPoll = state.getTime();
      synchronizeRtc();
    }

    return 1;
  }

  /**
   * Synchronize RTC using NTP.
   */
  void synchronizeRtc() {
    if (isEnabled() && network.isConnected() && !network.isAccessPoint()) {
      monitor.setMessage(LabelClockUpdate, MonitorAction);
      monitor.setMessage(LabelClockServer + config.getTimeServer(), MonitorInfo);

      if (network.isStandard()) {
        WiFiUDP wifiUdpClient;
        synchronizeFromUdp(wifiUdpClient);
      } else if (network.isEthernet()) {
        EthernetUDP ethernetUdpClient;
        synchronizeFromUdp(ethernetUdpClient);
      }
    }
  }

  /**
   * Get RCT time as Hour::Minute::Second.
   *
   * @return  String time
   */
  String toString() {
    char buffer[32];
    tm t;
    _rtc_localtime(time(NULL), &t, RTC_FULL_LEAP_YEAR_SUPPORT);
    strftime(buffer, 32, "%k:%M:%S", &t);

    return String(buffer);
  }

}; // class OptaLinkerTime

} // namespace optalinker

#endif // #ifndef OPTALINKER_TIME_H