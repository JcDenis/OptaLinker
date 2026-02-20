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

  // Switch to RUN state
  state->setType(StateType::StateRun);

  // Display end of setup
  monitor->setMessage(LabelOptaLinkerLoop, MonitorAction);

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

  uint8_t loopBenchmarkStart = 0;

	// Check if a know serial message arrives
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

/**
 * Start library loop in a dedicated thread.
 */
void OptaLinker::thread() {
  // Prevent start twice thread
  if (!_threadStarted) {
    monitor->setMessage(LabelOptaLinkerThread, MonitorAction);

    _threadStarted = 1;

    static rtos::Thread thread;
    thread.start([]() {
      // rtos::Thread.start() requires a static callback
        while(getInstance().loop()){
  // Because of thread, library loop and ino loop MUST use yield
  yield();}
    });
  }
}

} // namesapce OptaLinker

#endif // #ifdef CORE_CM7
