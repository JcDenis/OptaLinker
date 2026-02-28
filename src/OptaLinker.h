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
#include <mbed.h>
#include <Arduino_Portenta_OTA.h>

#ifdef CORE_CM7

#include "version.h"
#include "state.h"
#include "monitor.h"
#include "board.h"
#include "store.h"
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
  uint8_t setup() {

    // Setup OptaLinker library modules
    if (!version->setup()
      || !state->setup()
      || !monitor->setup()
      || !board->setup()
      || !store->setup()
      || !config->setup() // Even if disabled, we need to read #define
      || !io->setup()
      || !network->setup()
      || (clock->isEnabled() && !clock->setup())
      || (rs485->isEnabled() && !rs485->setup())
      || (modbus->isEnabled() && !modbus->setup())
      || (mqtt->isEnabled() && !mqtt->setup())
      || (web->isEnabled() && !web->setup())
    ) {

      // If an error occured, stop library execution
      return board->stop();
    }

    // Start OTA updater
    ota();

    // Switch to RUN state
    state->setType(StateType::StateRun);

    // Display end of setup
    monitor->setMessage(LabelOptaLinkerLoop, MonitorSuccess);

    // Return library state
    return state->getType();
  }

  /**
   * Execute the loop process.
   *
   * @return  StateType::StateRun on success
   */
  uint8_t loop() {

    // Loop through OptaLinker library modules
    if (!version->loop()
      || !state->loop()
      || !monitor->loop()
      || !board->loop()
      || !store->loop()
      || (config->isEnabled() && !config->loop())
      || !network->loop()
      || (clock->isEnabled() && !clock->loop())
      || !io->loop()
      || (rs485->isEnabled() && !rs485->loop())
      || (modbus->isEnabled() && !modbus->loop())
      || (mqtt->isEnabled() && !mqtt->loop())
      || (web->isEnabled() && !web->loop())
    ) {

      // If an error occured, stop library execution
      return board->stop();
    }

    // Move to main loop OTA state
    if (version->getOtaState() && (_otaLast == 0 || (state->getTime() - _otaLast > 10000))) { // 10s
      _otaLast = state->getTime();
      monitor->setMessage("Processing OTA update, this may take a while...", MonitorLock);
    }

    uint8_t loopBenchmarkStart = 0;

    // Check if a known serial message arrives
    if (monitor->hasIncoming()) {

      String message = monitor->getIncoming();
      if (message.equals("print version")) {
        monitor->setMessage(version->toString());
      }

      if (message.equals("print config")) {
        monitor->setMessage(config->writeToJson(false));
      }

      if (message.equals("print io")) {
        monitor->setMessage(io->writeToJson());
      }

      if (message.equals("print store")) {
        store->printKeys();
      }

      if (message.equals("print boot")) {
        monitor->setMessage(String(store->getBootCount()));
      }

      if (message.equals("print loop")) {
        loopBenchmarkStart = 1;
      }

      if (message.equals("print ip")) {
        monitor->setMessage(network->getLocalIp().toString());
      }

      if (message.equals("switch dhcp")) {
        config->setNetworkDhcp(config->getNetworkDhcp() ? false : true);
        config->writeToFile();
        monitor->setMessage(LabelOptaLinkerApply, MonitorWarning);
      }

      if (message.equals("switch wifi")) {
        config->setNetworkWifi(config->getNetworkWifi() ? false : true);
        config->writeToFile();
        monitor->setMessage(LabelOptaLinkerApply, MonitorWarning);
      }

      if (message.equals("print time")) {
        monitor->setMessage(clock->toString());
      }

      if (message.equals("update time")) {
        clock->synchronizeRtc();
      }

      if (message.equals("flash memory")) {
        if (store->formatMemory(1)) {
          board->reboot();
        }
      }

      if (message.equals("reset config")) {
        config->reset();
        board->reboot();
      }

      if (message.equals("reset io")) {
        io->initializeIo();
        if (io->writeToFile()) {
          board->reboot();
        }
      }

      if (message.equals("publish mqtt")) {
        mqtt->publishDevice();
        mqtt->publishInputs();
      }

      if (message.equals("reboot")) {
        board->reboot();
      }
    }

    // Start a loop benchmark
    if (loopBenchmarkStart && _benchmarkTime == 0) {
      monitor->setMessage(LabelOptaLinkerBenchmarkStart, MonitorAction);

      _benchmarkTime = state->getTime();
      _benchmarkCount = _benchmarkRepeat = _benchmarkSum = 0;

    // Count loops for benchmark
    } else if (_benchmarkTime > 0) {
      _benchmarkCount++;
      if (state->getTime() - _benchmarkTime > 1000) {
        monitor->setMessage(LabelOptaLinkerBenchmarkLine + String(_benchmarkCount));

        if (_benchmarkRepeat < 10) {
          _benchmarkTime = state->getTime();
          _benchmarkSum += _benchmarkCount;
          _benchmarkCount = 0;
          _benchmarkRepeat++;
        } else {
          _benchmarkTime = 0;

          monitor->setMessage(LabelOptaLinkerBenchmarkAverage + String(_benchmarkSum / 10));
        }
      }
    }

    return state->getType();
  }

  /**
   * Execute the OptaLinker library loop process in a dedicated thread.
   */
  void thread() {
    // Prevent start twice thread
    if (!_loopStarted) {
      monitor->setMessage(LabelOptaLinkerThread, MonitorAction);

      _loopStarted = 1;

      static rtos::Thread loopThread;
      loopThread.start([]() {
        // rtos::Thread.start() requires a static callback
        while(getInstance().loop()){
          // Because of thread, library loop and ino loop MUST use yield
          yield();
        }
      });
    }
  }

private:

  /**
   * Construct OptaLinker instance and construct library modules.
   */
  OptaLinker() {
    version  = new OptaLinkerVersion(1, 1, 0);
    state    = new OptaLinkerState();
    monitor  = new OptaLinkerMonitor(*state);
    board    = new OptaLinkerBoard(*state, *monitor);
    store    = new OptaLinkerStore(*monitor, *board);
    config   = new OptaLinkerConfig(*version, *monitor, *board, *store);
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
  void ota() {
    // Prevent start twice thread
    if (!_otaStarted) {
      _otaStarted = 1;

      static rtos::Thread otaThread;
      otaThread.start([]() {
        uint32_t otaLast = 0;
        uint32_t otaDelay = 3600000; // 1h
        auto ol = getInstance();

        // Infinite loop
        while(1) {
          // Check every hour
          if (ol.state->getTime() - otaLast > otaDelay) {
            otaLast = ol.state->getTime();
            String otaUrl = ol.config->getUpdateUrl();

            if (!ol.network->isConnected() || !otaUrl.length() || !ol.version->getOtaVersion()) {
              // Requirements missing
            } else if (ol.version->getOtaVersion() <= ol.version->toInt()) {
              ol.monitor->setMessage(LabelUpdateNone, MonitorInfo);
            } else {
              ol.monitor->setMessage(LabelUpdateCheck, MonitorAction);

              // Announce to main loop we are processing ota update
              ol.version->getOtaState(1);

              // Stop web server as OTA download fails if web server is running!
              ol.web->stopServer();

              // Prepare OTA
              Arduino_Portenta_OTA_QSPI aota(QSPI_FLASH_FATFS_MBR, 2);
              Arduino_Portenta_OTA::Error otaError = Arduino_Portenta_OTA::Error::None;

              // Check board
              if (!aota.isOtaCapable()) {
                ol.monitor->setMessage(LabelUpdateUpgrade, MonitorWarning);

              // Begin OTA
              } else if ((otaError = aota.begin()) != Arduino_Portenta_OTA::Error::None) {
                ol.monitor->setMessage(LabelUpdateBeginFail + String((int)otaError), MonitorWarning);

              } else {
                // Bad way to check if we use ssl
                bool ssl = strstr(otaUrl.c_str(), "https") != NULL;

                // Download
                ol.monitor->setMessage(LabelUpdateDownload + otaUrl, MonitorInfo);
                int const ota_download = aota.download(otaUrl.c_str(), ssl /* is_https */);
                if (ota_download <= 0) {
                  ol.monitor->setMessage(LabelUpdateDownloadFail + String(ota_download), MonitorWarning);

                } else {
                  ol.monitor->setMessage(String(ota_download) + " bytes stored.", MonitorInfo);

                  // Uncompress
                  ol.monitor->setMessage(LabelUpdateUncompress, MonitorInfo);
                  int const ota_decompress = aota.decompress();
                  if (ota_decompress < 0) {
                    ol.monitor->setMessage(LabelUpdateUncompressFail + String(ota_decompress), MonitorWarning);
                  } else {
                    ol.monitor->setMessage(String(ota_decompress) + " bytes decompressed.", MonitorInfo);

                    // Prepare bootloader
                    ol.monitor->setMessage(LabelUpdateBootloader, MonitorInfo);
                    if ((otaError = aota.update()) != Arduino_Portenta_OTA::Error::None) {
                      ol.monitor->setMessage(LabelUpdateBootloaderFail + String((int)otaError), MonitorWarning);
                    } else {
                      ol.monitor->setMessage(LabelUpdateSuccess, MonitorStop);

                      // Reboot
                      aota.reset();
                    }
                  }
                }
              }
              ol.web->startServer();
              ol.version->getOtaState(0);
            }
          }
          yield();
        }
      });
    }
  }

}; // class OptaLinker

} // namespace optalinker

#endif // #ifdef CORE_CM7

#endif // #ifndef OPTALINKER_H
