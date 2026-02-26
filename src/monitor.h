/*
 * OptaLinker
 *
 * Author: Jean-Christian Paul Denis
 * Source: https://github.com/JcDenis/OptaLinker
 *
 * @see OptaLinker.h
 * @see README.md file
 */

#ifndef OPTALINKER_MONITOR_H
#define OPTALINKER_MONITOR_H

#include "OptaLinkerModule.h"

namespace optalinker {

class OptaLinkerState;

/**
 * OptaLinker Library serial monitor module.
 *
 * Manage serial monitor messages. (in and out)
 */
class OptaLinkerMonitor : public OptaLinkerModule {

private:
  OptaLinkerState &state;

  /**
   * Heartbeat timer.
   */
	uint32_t _heartbeat = 0;

	/**
	 * Progress percent.
	 */
  uint8_t _progress = 0;

  /**
   * Received message.
   */
  char _incoming[50]; // max length of 50

  /**
   * A message is available.
   */
  uint8_t _hasIncoming = 0;


public:
	OptaLinkerMonitor(OptaLinkerState &_state) : state(_state) {}

  uint8_t setup() {
    // Prepare serial monitor (USB)
    Serial.begin(115200);

    // Wait for Serial a maximum of 5 seconds
    for (size_t i = 0; i < 5000; i++) {
      delay(1);
      if (Serial) {
        break;
      }
    }

    // Display "welcome" message
    Serial.println(LabelMonitorSetup);

    return 1;
  }

  uint8_t loop() {
  	// reset incoming mesasge
  	_incoming[0] = '\0';
  	_hasIncoming = 0;

  	// Heartbeat
  	if (state.getTime() - _heartbeat > 10000) {
        setMessage(LabelMonitorHeartbeat, MonitorInfo);
        _heartbeat = state.getTime();
  	}

  	// receive message
	  uint8_t index = 0;
	  uint8_t ended = 0;

	  while (Serial.available() && !ended) {
	    int c = Serial.read();
	    if (c != -1) {
	      switch (c) {
	        case '\n':
	          _incoming[index] = '\0';
	          setMessage(String(LabelMonitorReceive) + _incoming, MonitorAction);
	          index = 0;
	          ended = 1;
	          break;
	        default:
	          if (index < 50) {  // max length of 50
	            _incoming[index++] = (char)c;
	            _hasIncoming = 1;
	          }
	          break;
	      }
	    }
	  }

    return 1;
  }

  /**
   * Print to serial monitor icons for test.
   */
  void testIcons() {
  	Serial.println("🔗➡️✅❌⚠️⚙️⛔🔒🔍🚀📝➕➖🔶🛑🌐⚪️📶🔄");
  }

  /**
   * Print a message.
   *
   * @param 	str 	The message.
   * @param 	type 	The MonitorType (this prepend an icon to the message)
   */
	void setMessage(String str, MonitorType type = MonitorNone) {
	  setMessage(str.c_str(), type);
	}
	void setMessage(const char *str, MonitorType type = MonitorNone) {
		lock();
	  Serial.println(MonitorTypeIcons[type] + " " + str);
	  unlock();
	}

	/**
	 * Print a progress percent.
	 *
   * @param 	offset			The current position
   * @param 	size 				The total size
   * @param 	threshold 	The thresold
   * @param 	reset 			Reset progress
   */
	void setProgress(uint32_t offset, uint32_t size, uint32_t threshold, uint8_t reset) {
	  if (reset == 1) {
	    _progress = 0;
	    setMessage(String(_progress) + "%", MonitorReceive);
	  } else {
	    uint8_t percent_done_new = offset * 100 / size;
	    if (percent_done_new >= _progress + threshold) {
	      _progress = percent_done_new;
	      setMessage(String(_progress) + "%", MonitorReceive);
	    }
	  }
	}

	/**
	 * Check if a message arrives.
	 *
	 * @return 	1 if there is a incoming mesage, else 0
	 */
	uint8_t hasIncoming() {

	  return _hasIncoming;
	}

	/**
	 * Get the last received message.
	 *
	 * A received message is available only for the current loop and is reseted on each loop.
	 *
	 * @return 	The message
	 */
	String getIncoming() {
	  String ret = String(_incoming);
	  ret.toLowerCase();

	  return ret;
	}

}; // class OptaLinkerConfig

} // namespace optalinker

#endif // #ifndef OPTALINKER_MONITOR_H