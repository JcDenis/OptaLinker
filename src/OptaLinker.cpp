/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#include <mbed.h>
#include <Arduino_Portenta_OTA.h>

#ifdef CORE_CM7

#include "OptaLinker.h"

namespace optalinker {

uint8_t OptaLinker::setup() {

  // Setup OptaLinker library modules
  if (!version->setup()
    || !state->setup()
    || !monitor->setup()
    || !board->setup()
    || !store->setup()
    || (flash->isEnabled() && !flash->setup())
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

uint8_t OptaLinker::loop() {

  // Loop through OptaLinker library modules
  if (!version->loop()
    || !state->loop()
    || !monitor->loop()
    || !board->loop()
    || !store->loop()
    || (flash->isEnabled() && !flash->loop())
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
      store->print();
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
      if (flash->format(1)) {
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

void OptaLinker::thread() {
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

void OptaLinker::ota() {
  // Prevent start twice thread
  if (!_otaStarted) {
    _otaStarted = 1;

    static rtos::Thread otaThread;
    otaThread.start([]() {
      uint32_t otaLast = 0;
      uint32_t otaDelay = 60000;
      auto ol = getInstance();

      while(1) {
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
              bool ssl = strstr(otaUrl.c_str(), "ttps") > 0;

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

} // namesapce OptaLinker

#endif // #ifdef CORE_CM7
