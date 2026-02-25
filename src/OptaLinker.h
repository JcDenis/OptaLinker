/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * see README.md file
 */

#ifndef OPTALINKER_H
#define OPTALINKER_H

#include <Arduino.h>

#ifdef CORE_CM7

#include "version.h"
#include "state.h"
#include "monitor.h"
#include "board.h"
#include "store.h"
#include "flash.h"
#include "config.h"
#include "io.h"
#include "network.h"
#include "clock.h"
#include "rs485.h"
#include "modbus.h"
#include "mqtt.h"
#include "web.h"

namespace optalinker {

class OptaLinker {

public:

  /**
   * OptaLinker library modules.
   */

  OptaLinkerVersion *version;
  OptaLinkerState *state;
  OptaLinkerMonitor *monitor;
  OptaLinkerBoard *board;
  OptaLinkerStore *store;
  OptaLinkerFlash *flash;
  OptaLinkerConfig *config;
  OptaLinkerNetwork *network;
  OptaLinkerClock *clock;
  OptaLinkerIo *io;
  OptaLinkerRs485 *rs485;
  OptaLinkerModbus *modbus;
  OptaLinkerMqtt *mqtt;
  OptaLinkerWeb *web;

  /**
   * Singleton OptaLinker caller.
   *
   * @return  OptaLinker uniq instance
   */
  static OptaLinker &getInstance() {
    static OptaLinker instance;
    return instance;
  }

  /**
   * Execute the setup process.
   *
   * @return StateType::StateRun on success
   */
  uint8_t setup();

  /**
   * Execute the loop process.
   *
   * @return  StateType::StateRun on success
   */
  uint8_t loop();

  /**
   * Execute the OptaLinker library loop process in a dedicated thread.
   */
  void thread();

private:

  /**
   * Construct OptaLinker instance and construct library modules.
   */
  OptaLinker() {
    version  = new OptaLinkerVersion(1, 1, 0);
    state    = new OptaLinkerState();
    monitor  = new OptaLinkerMonitor(*state);
    board    = new OptaLinkerBoard(*state, *monitor);
    store    = new OptaLinkerStore(*monitor);
    flash    = new OptaLinkerFlash(*monitor, *board);
    config   = new OptaLinkerConfig(*version, *monitor, *board);
    network  = new OptaLinkerNetwork(*state, *monitor, *board, *config);
    clock    = new OptaLinkerClock(*state, *monitor, *board, *config, *network);
    io       = new OptaLinkerIo(*state, *monitor, *board, *store, *config);
    rs485    = new OptaLinkerRs485(*monitor, *board, *config);
    modbus   = new OptaLinkerModbus(*version, *state, *monitor, *board, *config, *network, *io, *rs485);
    mqtt     = new OptaLinkerMqtt(*version, *state, *monitor, *board, *config, *io, *network, *rs485);
    web      = new OptaLinkerWeb(*version, *state, *monitor, *board, *config, *io, *network, *clock, *mqtt);
  }
  //~OptaLinker();

  /**
   * Static instance of OptaLinker.
   *
   * Required by mbed::Thread::start()
   */
  static OptaLinker *instance;

  uint32_t _benchmarkTime  = 0;
  uint32_t _benchmarkCount = 0;
  uint32_t _benchmarkSum   = 0;
  uint8_t _benchmarkRepeat = 0;
  uint8_t _loopStarted     = 0;

  uint8_t _otaStarted = 0;
  uint32_t _otaLast   = 0;

  /**
   * Execute OTA updater process in a dedicated thread.
   * 
   * Download and uncompress large OTA file take lots of time.
   * Using OTA Watchdog callback has no effect on socket download.
   * So we use dedicated thread not to freeze main thread and loop.
   *
   * TODO: Best way to resolve this is to provide custom socket->download function with internal watchdog kicker...
   * 
   * TODO: Manage fatal error on ota class because we can not do ota.begin() twice
   * see https://github.com/arduino-libraries/Arduino_Portenta_OTA/issues/55
   */
  void ota();

}; // class OptaLinker

} // namespace optalinker

#endif // #ifdef CORE_CM7

#endif // #ifndef OPTALINKER_H
